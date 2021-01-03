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

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <EEPROM.h>
#include "audioAnalyzer.h"

#define  FASTLED_INTERNAL
#include "FastLED.h"

// -- LED Display Constants
#define NUM_LO_BANDS        29                    // Number of LOW frequency bands  
#define NUM_HI_BANDS        30                    // Number of HIGH frequency bands
#define NUM_BANDS           59                    // Total Number of frequency bands being displayed
#define NUM_LEDS            NUM_BANDS * 2         // Two LEDS per Band
#define START_NOISE_FLOOR   80                    // Frequency Bin Magnitudes below this value will not get summed into Bands. (Initial high value)
#define BASE_NOISE_FLOOR    40                    // Frequency Bin Magnitudes below this value will not get summed into Bands. (Final minimumm value)
#define BAND_HUE_STEP      220 / NUM_BANDS        // How much the LED Hue changes for each band.

#define LED_DATA_PIN        12
#define LED_CLOCK_PIN       14

#define       MIN_GAIN         0
#define       MAX_GAIN        (NUM_LEDS - 1)
#define       GAIN_HOLD_MS  1000
#define       GAIN_STEP_MS   200
#define       GAIN_UP_PIN      3
#define       GAIN_DN_PIN      2

#define       MIN_DIVIDE        10   
#define       MAX_DIVIDE        400  

#define       BRIGHTNESS       510                    // Max overall Brightness
#define       MAX_LED_BRIGHTNESS 255                  // Max LED Brightness

#define       GAIN_ADDRESS    1

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// -- LED Display Data
CRGB      leds[NUM_LEDS];
uint32_t  bandValues[NUM_BANDS];
uint16_t  LO_bandBins[NUM_LO_BANDS + 1] = {10,11,12,13,14,16,18,19,21,24,26,29,32,35,39,43,47,52,58,64,71,78,86,95,105,116,128,141,156,172};
uint16_t  HI_bandBins[NUM_HI_BANDS + 1] = {43,47,52,58,64,71,78,86,95,105,116,128,141,156,172,190,210,231,256,282,312,344,380,419,463,511,564,623,688,760,839};

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
AudioInputI2S          audioInput;     // audio shield: mic or line-in
AudioSynthToneSweep    toneSweep;      // audio input to sweep frequency
AudioSynthWaveformSine sinewave;       // Standard Sine wave output

AudioAnalyzeFFT        myFFT;

// Connect either the live input or synthesized sine wave
AudioConnection patchCord1(audioInput, 0, myFFT, 0);
// AudioConnection patchCord1(sinewave, 0, myFFT, 0);
// AudioConnection patchCord1(toneSweepLog, 0, myFFT, 0);

unsigned long startTime = millis();
unsigned long lastTime = millis();
unsigned long cycleTime = 0;


unsigned long buttonStart = 0;
bool          adjusting   = false;
short         gain        = 0;
bool          gainUp      = 0;      
bool          lastgainUp  = 0;      
bool          gainDn      = 0;      
bool          lastgainDn  = 0;      
short         gainStep    = 0;      

void setup() {
  Serial.begin(500000); 

  pinMode(GAIN_UP_PIN, INPUT_PULLUP);
  pinMode(GAIN_DN_PIN, INPUT_PULLUP);
  
  initDisplay();
  setGain(EEPROM.read(GAIN_ADDRESS));

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(NUM_BURSTS);

  // Create a synthetic frequency sweep
  // toneSweep.play(0.01, 55, 19000, 100);
  // toneSweepLog.play(0.01, 55, 19000, 6);

  // Create a synthetic sine wave
  sinewave.amplitude(1.0);
  sinewave.frequency(16000);
}

void  bumpGain(int step) {
  buttonStart = millis();
  adjusting = true;
  gainStep = step;
  setGain(gain + step);  
}

// save the current selected gain value (which LED) and calculate Amplitude divide factor
int  setGain(int newgain) {
  double  divide;

  if (newgain < MIN_GAIN)
    newgain = MIN_GAIN;
  else if (newgain > MAX_GAIN)
    newgain = MAX_GAIN;

  gain = newgain;
  divide = MIN_DIVIDE + ((MAX_DIVIDE - MIN_DIVIDE) * (MAX_GAIN - gain) / MAX_GAIN) ;
  myFFT.setInputDivide(divide);

  FastLED.clear();
  leds[gain].setHSV((gain>>1) * BAND_HUE_STEP, 255, 255);
  FastLED.show();

  EEPROM.write(GAIN_ADDRESS, gain);
  return gain;
}

long  pressElapsed() {
  return millis() - buttonStart;
}

void loop() {

  // debounce
  if (pressElapsed() > 100) {
    gainUp = !digitalRead(GAIN_UP_PIN);
    gainDn = !digitalRead(GAIN_DN_PIN);
    
    if (gainUp && !lastgainUp) {
      bumpGain(1);
    }
  
    if (gainDn && !lastgainDn) {
      bumpGain(-1);
    }
  
    lastgainUp = gainUp;
    lastgainDn = gainDn;
  
    if (adjusting) {
      if (pressElapsed() > GAIN_HOLD_MS) {
        adjusting = false;
      }
      else if ((gainUp || gainDn) && (pressElapsed() > GAIN_STEP_MS)) {
        bumpGain(gainStep);
      }
    }
  }
  
  if (!adjusting && myFFT.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    startTime = millis();
    cycleTime = startTime - lastTime;
    lastTime = startTime;

    updateDisplay();
    
  } else if (myFFT.missingBlocks()) {
    toneSweep.play(1.0, 55, 19000, 6);
  }
}

// Configure the LED string and preload the color values for each band.
void  initDisplay(void) {
  delay(250);      // sanity check delay
  FastLED.addLeds<APA102, BGR>(leds, NUM_LEDS);

  // preload the hue into each LED and set the saturation to full and brightness to low.
  // This is a startup test to show that ALL LEDs are capable of displaying their base color.
  for (int b = 100; b >= 0; b--) {
    for (int i = 0; i < NUM_BANDS; i++) {
      leds[i<<1]     = CHSV(i * BAND_HUE_STEP , 255, b);
      leds[(i<<1) + 1] = CHSV(i * BAND_HUE_STEP , 255, b);
    }
    delay(20);
    FastLED.show();
  }
  delay(500);
}

// Update the LED string based on the intensities of all the Frequency bins.
void  updateDisplay (void){
  uint16_t ledBrightness;

  // Allocate FFT results into LED Bands (
  fillBands ();
  
  // Process the LED buckets into LED Intensities
  for (byte band = 0; band < NUM_BANDS; band++) {
    
    // Scale the bars for the display
    ledBrightness = bandValues[band];

    if (ledBrightness > BRIGHTNESS) 
      ledBrightness = BRIGHTNESS;
  
    // Display LED Band in the correct Hue.  Spread intensity over TWO LEDs to get 511 counts
    if (ledBrightness <= MAX_LED_BRIGHTNESS){
      leds[(band<<1)   ].setHSV(band * BAND_HUE_STEP, 255, ledBrightness);
      leds[(band<<1) +1].setHSV(band * BAND_HUE_STEP, 255, 0);
    } else {
      leds[(band<<1)   ].setHSV(band * BAND_HUE_STEP, 255, MAX_LED_BRIGHTNESS);
      leds[(band<<1) +1].setHSV(band * BAND_HUE_STEP, 255, ledBrightness - MAX_LED_BRIGHTNESS);
    }
  }
  
  // Update LED display
  FastLED.show();
}

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
    
  for (int b = 0; b < NUM_LO_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(false, LO_bandBins[b], LO_bandBins[b+1], noiseFloor);

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 95 * noiseFloor / 100;  // equiv 0.95 factor.
    }
  }

  for (int b = 0; b < NUM_HI_BANDS; b++, band++){
    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(true, HI_bandBins[b], HI_bandBins[b+1], noiseFloor);

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 95 * noiseFloor / 100;  // equiv 0.95 factor.
    }
  }

}
