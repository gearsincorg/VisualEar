/*
  Visual Ear
  Audio spectrum analyser
  LED strip display.
  For details see:  https://hackaday.io/project/175944-the-visual-ear
  Copyright (C) 2021 Philip Malone

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include  <Audio.h>
#include  <Wire.h>
#include  <SPI.h>
#include  <SerialFlash.h>
#include  <EEPROM.h>
#include  "audioAnalyzer.h"
#include  "devconf.h"
#include  "display.h"

#define  FASTLED_INTERNAL
#include "FastLED.h"

// -- LED Display Constants
#define START_NOISE_FLOOR   80                    // Frequency Bin Magnitudes below this value will not get summed into Bands. (Initial high value)
#define BASE_NOISE_FLOOR    40                    // Frequency Bin Magnitudes below this value will not get summed into Bands. (Final minimumm value)

#define UI_HOLD_MS      3000
#define UI_STEP_MS       200
#define UI_BUTTON_PIN      3

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

float     upGainAccumulator   = 0;
float     downGainAccumulator = 0;


// -- LED Display Data
uint32_t  bandValues[NUM_BANDS];
uint16_t  LO_bandBins[NUM_LO_BANDS + 1] = {17,18,19,20,21,22,23,24,25,26,27,28,30,31,32,34,35,37,38,40,42,44,46,48,50,52,54,57,59,62,64,67,70,73,77,80,84};
uint16_t  MD_bandBins[NUM_MD_BANDS + 1] = {42,44,46,48,50,52,54,57,59,62,64,67,70,73,77,80,84,87,91,95,99,104,108,113,118,123,129,135,141,147,153,160,167,174,182,190,199,207,217,226,236,247,258,269,281,293,306,320,334};
uint16_t  HI_bandBins[NUM_HI_BANDS + 1] = {42,44,46,48,50,52,54,57,59,62,64,67,70,73,77,80,84,87,91,95,99,104,108,113,118,123,129,135,141,147,153,160,167,174,182,190,199,207,217,226,236,247,258,269,281,293,306,320,334,349,364,381,397};

// Create the Audio components.  These should be created in the
AudioInputI2S          audioInput;     // audio shield: mic or line-in
AudioAnalyzeFFT        myFFT;
AudioAnalyzePeak       peak;          //xy=393,385

// Connect the live input
AudioConnection patchCord1(audioInput, 0, myFFT, 0);
AudioConnection patchCord2(audioInput, 0, peak, 0);

unsigned long startTime = millis();
unsigned long lastTime = millis();
unsigned long cycleTime = 0;

unsigned long buttonStart = 0;
bool          adjusting   = false;
bool          UIHolding = false;      
bool          UIButton  = false;      
bool          lastUIButton  = false;      

int           activeBands = 0;

// Non Volatile values
short         gainNumber  = 0;

// ==================================================================================================

void setup() {
  Serial.begin(500000); 

  pinMode(UI_BUTTON_PIN, INPUT_PULLUP);

  // Audio connections require memory to work.  For more
  AudioMemory(NUM_BURSTS);

  // read NV Ram
  setDisplayMode( EEPROM.read(MODE_ADDRESS));
  setGain(EEPROM.read(GAIN_ADDRESS));
  delay(500);

  initDisplay();
}

void loop() {
  // check the button and see if we have a change
  runUI();

  if (getDisplayMode() == 1) {       
    if (peak.available()){
      updateVuDisplay(peak.readPeakToPeak());
    }
  } else {       
    if (myFFT.available()) {
      // each time new FFT data is available update the diplay
      startTime = millis();
      cycleTime = startTime - lastTime;
      lastTime = startTime;
  
      fillBands();
      updateDisplay(bandValues);
      runAGC();
    }
  }
}

// ==================================================================================================

void  runUI() {
  
  // debounce
  if (pressElapsed() > 100) {
    UIButton = !digitalRead(UI_BUTTON_PIN);

    //  Just Pressed, start the hold timer
    if (UIButton && !lastUIButton) {
      pressReset();
    }

    // Look for button release.  Bump gain up unless we were already going down (holding)
    if (!UIButton && lastUIButton) {
      if (!UIHolding) {
        bumpMode();
      }
    }

    lastUIButton = UIButton;
  }
}

long  pressElapsed() {
  return millis() - buttonStart;
}

void  pressReset() {
  buttonStart = millis() ;
}

void  runAGC(){
  float OCR = (float)activeBands / (float)NUM_BANDS;

  
  Serial.print("AB ");
  Serial.print(activeBands);
  Serial.print(", GN ");
  Serial.println(gainNumber);
  

  if (OCR < lowTrip) {
    upGainAccumulator += 0.01;
  } else if (OCR > highTrip) {
    upGainAccumulator -= 0.04;
  }

  if (upGainAccumulator > 1.0) {
    bumpGain(1);
    upGainAccumulator = 0.0;
  } else if (upGainAccumulator < -1.0) {
    bumpGain(-1);
    upGainAccumulator = 0.0;
  }

  /*  
  Serial.print("AGC OLC ");
  Serial.print(OLC);
  Serial.print(", OCR ");
  Serial.print(OCR);
  Serial.print(", UA ");
  Serial.print(upGainAccumulator);
  Serial.print(", Gain Number ");
  Serial.println(gainNumber);
  */

  }

void  bumpGain(int step ) {
  buttonStart = millis();
  setGain(gainNumber + step);  
  pressReset();
}

void  bumpMode() {
  buttonStart = millis();
  EEPROM.write(MODE_ADDRESS, switchDisplayMode());
  initDisplay();  
}

// save the current selected gain value (which LED) and calculate Amplitude divide factor
int  setGain(short newgain) {
double        gainScale;

  if (newgain < MIN_GAIN_NUM)
    newgain = MIN_GAIN_NUM;
  else if (newgain > MAX_GAIN_NUM)
    newgain = MAX_GAIN_NUM;

  gainNumber = newgain;
  EEPROM.write(GAIN_ADDRESS, gainNumber);

  gainScale  = minScale * pow(gainSlope, gainNumber);
  myFFT.setInputScale(gainScale);

  /*
  Serial.print("Gain # ");
  Serial.print(gainNumber);
  Serial.print(" Gain ");
  Serial.println(gainScale * 100);
  */

  return gainNumber;
}

// ==================================================================================================

// Group Frequency Bins into Band Buckets based on the maximum nun number for each band
// Each band covers more bind because bins are linear and bands are logorithmic.
void  fillBands (void){
  uint32_t  noiseFloor;  
  uint8_t   band;   

  //  zero out all the LED band magnitudes.
  memset(bandValues, 0, sizeof(bandValues));

  // Cycle through each of the LED bands.  Set initial noise threshold high and drop down.
  noiseFloor = START_NOISE_FLOOR;
  band = 0;
  activeBands = 0;
    
  for (int b = 0; b < NUM_LO_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(0, LO_bandBins[b], LO_bandBins[b+1], noiseFloor);
    if (bandValues[band] > 2)
      activeBands++;

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 97 * noiseFloor / 100;  // equiv 0.97 factor.
    }
  }

  for (int b = 0; b < NUM_MD_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(1, MD_bandBins[b], MD_bandBins[b+1], noiseFloor);
    if (bandValues[band] > 2)
      activeBands++;

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 97 * noiseFloor / 100;  // equiv 0.97 factor.
    }
  }

  for (int b = 0; b < NUM_HI_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(2, HI_bandBins[b], HI_bandBins[b+1], noiseFloor);
    if (bandValues[band] > 2)
      activeBands++;

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 95 * noiseFloor / 100;  // equiv 0.95 factor.
    }
  }

}
