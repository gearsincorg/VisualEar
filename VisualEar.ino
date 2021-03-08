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

#define GAIN_HOLD_MS      1000
#define GAIN_STEP_MS       200
#define GAIN_BUTTON_PIN      3
#define GAIN_ADDRESS         1

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

const int MIN_GAIN_NUM   =   0;
const int MAX_GAIN_NUM   =  (NUM_LEDS - 1);
const int GAIN_RANGE     =  (MAX_GAIN_NUM - MIN_GAIN_NUM);
float     upGainAccumulator   = 0;
float     downGainAccumulator = 0;

const double MIN_GAIN_SCALE  = 0.00125;
const double MAX_GAIN_SCALE  = 0.1;  // was 2 
const double GAIN_SCALE      = pow((MAX_GAIN_SCALE / MIN_GAIN_SCALE), 1.0 / MAX_GAIN_NUM);   

// -- LED Display Data
uint32_t  bandValues[NUM_BANDS];
uint16_t  LO_bandBins[NUM_LO_BANDS + 1] = {11,12,13,14,15,16,17,18,19,20,21,22,24,25,27,28,30,32,33,35,38,40,42,45,47,50,53,56,60,63,67,71,75,80,84,89,95,100,106,112,119,126};
uint16_t  HI_bandBins[NUM_HI_BANDS + 1] = {21,22,24,25,27,28,30,32,33,35,37,40,42,45,47,50,53,56,60,63,67,71,75,79,84,89,94,100,106,112,119,126,134,142,150,159,168,178,189,200,212,225,238,252,267,283,300,318,337,357,378,400,424,449,476,504,534,566,600,636,673,713,756,801};

// Create the Audio components.  These should be created in the
AudioInputI2S          audioInput;     // audio shield: mic or line-in
AudioAnalyzeFFT        myFFT;

// Connect the live input
AudioConnection patchCord1(audioInput, 0, myFFT, 0);

unsigned long startTime = millis();
unsigned long lastTime = millis();
unsigned long cycleTime = 0;

unsigned long buttonStart = 0;
bool          adjusting   = false;
short         gainNumber  = 0;
bool          gainHolding = false;      
bool          gainButton  = false;      
bool          lastGainButton  = false;      
double        gainScale;

int           OLC = 0;

// ==================================================================================================

void setup() {
  Serial.begin(500000); 

  pinMode(GAIN_BUTTON_PIN, INPUT_PULLUP);

  // Audio connections require memory to work.  For more
  AudioMemory(NUM_BURSTS);
  
  initBallDisplay(NUM_BANDS);
//  initFFTDisplay(NUM_BANDS);
  setGain(EEPROM.read(GAIN_ADDRESS), true);
  delay(500);
}

void loop() {
  // debounce
  if (pressElapsed() > 100) {
    gainButton = !digitalRead(GAIN_BUTTON_PIN);

    //  Just Pressed, start the hold timer
    if (gainButton && !lastGainButton) {
      pressReset();
    }

    // Look for button release.  Bump gain up unless we were already going down (holding)
    if (!gainButton && lastGainButton) {
      if (!gainHolding) {
        bumpGain(1, true);
      }
    }

    if (gainButton) {
      adjusting = true;
    } else {
      gainHolding = false;  
    }

    if (adjusting) {
      if (pressElapsed() > GAIN_HOLD_MS) {
        adjusting = false;
      }
      else if (gainButton && (pressElapsed() > GAIN_STEP_MS)) {
        bumpGain(-1, true);
        gainHolding = true;
      }
    }
    lastGainButton = gainButton;
  }
  
  if (!adjusting && myFFT.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    startTime = millis();
    cycleTime = startTime - lastTime;
    lastTime = startTime;

    fillBands();
    // runAGC(updateFFTDisplay(bandValues));
    runAGC(updateBallDisplay(bandValues));
  }
}

// ==================================================================================================

long  pressElapsed() {
  return millis() - buttonStart;
}

void  pressReset() {
  buttonStart = millis() ;
}

void  runAGC(int unused){
  float OCR = (float)OLC / (float)NUM_BANDS;

  if (OCR < 0.04) {
    upGainAccumulator += 0.01;
  } else if (OCR > 0.45) {
    upGainAccumulator -= 0.04;
  }

  if (upGainAccumulator > 1.0) {
    bumpGain(1, false);
    upGainAccumulator = 0.0;
  } else if (upGainAccumulator < -1.0) {
    bumpGain(-1, false);
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

  Serial.print(upGainAccumulator);
  Serial.print(" 1 -1 ");
  Serial.println(gainScale * 10);
  }

void  bumpGain(int step, boolean displayGain ) {
  buttonStart = millis();
  setGain(gainNumber + step, displayGain);  
  pressReset();
}

// save the current selected gain value (which LED) and calculate Amplitude divide factor
int  setGain(int newgain, boolean displayGain) {

  if (newgain < MIN_GAIN_NUM)
    newgain = MIN_GAIN_NUM;
  else if (newgain > MAX_GAIN_NUM)
    newgain = MAX_GAIN_NUM;

  gainNumber = newgain;
  EEPROM.write(GAIN_ADDRESS, gainNumber);

  gainScale  = MIN_GAIN_SCALE * pow(GAIN_SCALE, gainNumber);
  myFFT.setInputScale(gainScale);

  if (displayGain) {
    FastLED.clear();
    setLEDBand(gainNumber, gainNumber, MAX_LED_BRIGHTNESS);
    FastLED.show();
  }
    
//  Serial.print("Gain =");
//  Serial.println(gainNumber);
  
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
  OLC = 0;
    
  for (int b = 0; b < NUM_LO_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(false, LO_bandBins[b], LO_bandBins[b+1], noiseFloor);
    if (bandValues[band] > 2)
      OLC++;

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 95 * noiseFloor / 100;  // equiv 0.95 factor.
    }
  }

  for (int b = 0; b < NUM_HI_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(true, HI_bandBins[b], HI_bandBins[b+1], noiseFloor);
    if (bandValues[band] > 2)
      OLC++;

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 95 * noiseFloor / 100;  // equiv 0.95 factor.
    }
  }

}
