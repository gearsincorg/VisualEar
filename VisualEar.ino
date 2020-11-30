// FFT Test
//
// Compute a 1024 point Fast Fourier Transform (spectrum analysis)
// on audio connected to the Left Line-In pin.  By changing code,
// a synthetic sine wave can be input instead.
//
// The first 40 (of 512) frequency analysis bins are printed to
// the Arduino Serial Monitor.  Viewing the raw data can help you
// understand how the FFT works and what results to expect when
// using the data to control LEDs, motors, or other fun things!
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "audioAnalyzer.h"

#define  FASTLED_INTERNAL
#include "FastLED.h"
CRGB     leds[NUM_LEDS];

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// -- LED Display Data
uint32_t  bandValues[NUM_BANDS];
//uint16_t  bandMaxBin[NUM_BANDS] = {6,7,8,9,10,11,12,13,15,16,18,20,22,24,26,29,32,35,39,43,48,53,58,64,71,78,86,95,105,116,128,142,157,173,191,211,233,257,284,313,346,382,421,465,514,567,626,691,763,843,930,1027,1134,1252,1383,1526,1685,1861,2047};
uint16_t  bandMaxBin[NUM_BANDS] = {11,12,13,14,16,18,19,21,24,26,29,32,35,39,43,47,52,58,64,71,78,86,95,105,116,128,141,156,172,190,210,231,256,282,312,344,380,419,463,511,564,623,688,760,839,926,1022,1129,1246,1376,1519,1677,1852,2044,2257,2492,2752,3038,3354,3703};

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S          audioInput;         // audio shield: mic or line-in
AudioSynthToneSweep    toneSweep;
AudioSynthWaveformSine sinewave;

AudioAnalyzeFFT        myFFT;

// Connect either the live input or synthesized sine wave
//AudioConnection patchCord1(audioInput, 0, myFFT, 0);
 AudioConnection patchCord1(toneSweep, 0, myFFT, 0);
// AudioConnection patchCord1(sinewave, 0, myFFT, 0);

void setup() {
  Serial.begin(2000000); 
  initDisplay();

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(NUM_BURSTS);

  // Create a synthetic frequency sweep
  toneSweep.play(1.0, 55, 19000, 20);

  // Create a synthetic sine wave
  sinewave.amplitude(1.0);
  sinewave.frequency(18950);

}

unsigned long startTime = millis();
unsigned long lastTime = millis();
unsigned long cycleTime = 0;

void loop() {
  float n;
  int i;

  if (myFFT.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    startTime = millis();
    cycleTime = startTime - lastTime;
    lastTime = startTime;

    updateDisplay();
    for (i=0; i < 500; i++) {
      n = myFFT.read(i);
      Serial.print(n);
      Serial.println(" 500");
    }
    Serial.print("-1 ");
    Serial.println(cycleTime);
  } else if (myFFT.missingBlocks()) {
    toneSweep.play(1.0, 55, 19000, 20);
  }
}

// Configure the LED string and preload the color values for each band.
void  initDisplay(void) {
  delay(1000);      // sanity check delay
  FastLED.addLeds<APA102, BGR>(leds, NUM_LEDS);

  // preload the hue into each LED and set the saturation to full and brightness to low.
  // This is a startup test to show that ALL LEDs are capable of displaying their base color.
  for (int i = 0; i < NUM_BANDS; i++) {
    leds[i] = CHSV(i * BAND_HUE_STEP , 255, 100);
  }
  FastLED.show();
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
  
    // Display LED Band in the correct Hue.
    leds[band].setHSV(band * BAND_HUE_STEP, 255, ledBrightness);
  }
  
  // Update LED display
  FastLED.show();
}

// Group Frequency Bins into Band Buckets based on the maximum nun number for each band
// Each band covers more bind because bins are linear and bands are logorithmic.
void  fillBands (void){
  uint16_t  minBin = 1; // skip over the DC level, and start with second freq.
  uint16_t  maxBin = 0; 
  uint32_t  noiseFloor;     

  //  zero out all the LED band magnitudes.
  memset(bandValues, 0, sizeof(bandValues));

  // Cycle through each of the LED bands.  Set initial noise threshold high and drop down.
  noiseFloor = START_NOISE_FLOOR;
  minBin = bandMaxBin[0] - 1;
  
  for (int band = 0; band < NUM_BANDS; band++){
    // get the new maximum freq for this band.
    maxBin = bandMaxBin[band];

    // Accumulate freq values from all bins that match this LED band,
    bandValues[band] = (uint32_t)myFFT.read(minBin, maxBin, noiseFloor);

    /*
    for (int bin = minBin; bin <= maxBin; bin++) {
      bandValue = (uint32_t)myFFT.read(bin);        
      if (bandValue > noiseFloor)  
       bandValues[band] += bandValue;  
    }
    */
    
    // slide the max of this band to the min of next band.
    minBin = maxBin + 1;

    // Adjust Noise Floor
    if (noiseFloor > BASE_NOISE_FLOOR) {
      noiseFloor = 95 * noiseFloor / 100;  // equiv 0.95 factor.
    }
  }
}
