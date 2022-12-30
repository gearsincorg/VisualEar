/*
  Visual Ear
  Audio spectrum analyser
  LED strip display.
  Copyright (C) 2022 Philip Malone

  DATE         Comment
  12/20/2022   Multi Band processing.

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

// Set project identification here
const char  Version[] = "Visyual Ear. V1.0";
const char  Branch[]  = "MultiBand";
const char  Description[]  = "4x24 Bands.  55Hz to 14KHz";

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
uint16_t  bandBins[SUB_BANDS + 1] = {40,42,45,47,50,53,56,60,63,67,71,75,79,84,89,94,100,106,112,119,126,134,142,150,159};

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

  Serial.println(Version);
  Serial.println(Branch);
  Serial.println(Description);
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

  if (SHOW_GAINS) {
    Serial.print("AB ");
    Serial.print(activeBands);
    Serial.print(", GN ");
    Serial.println(gainNumber);
  }
  

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
  uint32_t  bandStrength;  
  uint8_t   band;   

  //  zero out all the LED band magnitudes.
  memset(bandValues, 0, sizeof(bandValues));

  // Cycle through each of the LED bands.  Set initial noise threshold high and drop down.
  band = 0;
  activeBands = 0;
    
  for (int subBands = 0; subBands < 4; subBands++){
    if (SHOW_BANKS) Serial.printf(" %d --- ", subBands);
    for (int sb = 0; sb < SUB_BANDS; sb++, band++){
      // Accumulate freq values from all bins that match this LED band,
      bandStrength = (uint32_t)myFFT.read(subBands, bandBins[sb], bandBins[sb+1], 30);
      bandValues[band] = bandStrength;
      if (SHOW_BANKS)Serial.printf("%d ", bandStrength);
      
      if (bandValues[band] > 2)
        activeBands++;
    }
    if (SHOW_BANKS)  Serial.println("");
  }
  if (SHOW_BANKS)  Serial.println("");
}
