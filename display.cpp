/*


*/

#include <Arduino.h>
#include "arm_math.h"
#include "devconf.h"
#include "display.h"

#define  FASTLED_INTERNAL
#include "FastLED.h"

// ======================================================================================================

CRGB      leds[NUM_LEDS];

float     hueStep;
int       numBands;
int       nextBall = 0;
uint32_t  lastMoveMs = 0;

float     pos[MAX_BALLS];
float     vel[MAX_BALLS];
uint8_t   band[MAX_BALLS];

uint8_t   numBalls[NUM_BANDS];

// ======================================================================================================
// trajectory functions
// ======================================================================================================

// ======================================================================================================
//  Basic display functions
// ======================================================================================================
void  setLEDBand(int pos,  int band,  int intensity) {
  setLED(band, (uint16_t)(band * hueStep), intensity);
}

int flipLEDs(int num) {
  return (NUM_LEDS - num - 1);
}

void  setLED(int pos, int hue, int intensity) {
  if (pos < NUM_LEDS) {
    if (FLIP_LED_ORDER){
      pos = flipLEDs(pos);
    } 
  
    leds[pos].setHSV(hue, 255, intensity);
  }
}


// ======================================================================================================
//  FFT display functions
// ======================================================================================================

// Configure the LED string and preload the color values for each band.
void  initFFTDisplay(int numberBands) {
  numBands = numberBands;
  hueStep = 220.0 / numBands;   // How much the LED Hue changes for each step.

  delay(250);      // sanity check delay
  FastLED.addLeds<APA102, BGR>(leds, NUM_LEDS);

  // preload the hue into each LED and set the saturation to full and brightness to low.
  // This is a startup test to show that ALL LEDs are capable of displaying their base color.
  for (int b = 100; b >= 0; b--) {
    for (int i = 0; i < NUM_LEDS; i++) {
      setLEDBand(i, i, b);
    }
    FastLED.show();
    delay(20);
  }
  delay(100);
}


// Update the LED string based on the intensities of all the Frequency bins.
void  updateFFTDisplay (uint32_t * bandValues){
  uint16_t ledBrightness;

  // Process the LED buckets into LED Intensities
  for (byte band = 0; band < numBands; band++) {
    
    // Scale the bars for the display
    ledBrightness = bandValues[band];

    if (ledBrightness > MAX_LED_BRIGHTNESS) 
      ledBrightness = MAX_LED_BRIGHTNESS;
  
    // Display LED Band in the correct Hue.
    setLEDBand(band, band, ledBrightness);
  }
  
  // Update LED display
  FastLED.show();
}

// ======================================================================================================
//  Air Balls display functions
// ======================================================================================================

// Configure the LED string and preload the color values for each band.
void  initBallDisplay(int numberBands) {
  numBands = numberBands;
  hueStep = 220.0 / numBands;   // How much the LED Hue changes for each step.

  delay(250);      // sanity check delay
  FastLED.addLeds<APA102, BGR>(leds, NUM_LEDS);

  // initialize ball data;
  memset(pos, 0, sizeof(pos));
  memset(vel, 0, sizeof(vel));
  memset(band, 0, sizeof(band));
  memset(numBalls, 0, sizeof(numBalls));
  
  // Bounce a white LED up and back
  // This is a startup test to show that ALL LEDs are capable of displaying their base color.
  for (int b = 100; b >= 0; b--) {
    for (int i = 0; i < NUM_LEDS; i++) {
      setLED(i, i, b);
    }
    delay(20);
    FastLED.show();
  }
  lastMoveMs = millis();
}

void  updateBallDisplay (uint32_t * bandValues){
    // see if we need to add some new balls.
    addBalls(bandValues);
  
    // Update LED display
    displayBalls();
    moveBalls();
}

void  addBalls(uint32_t * bandValues){
    uint32_t val;
    double   velocity;
    
    for (int b = 0; b < numBands; b++ ){
      val = bandValues[b];
      if (val > MAX_LED_BRIGHTNESS) {
          val = MAX_LED_BRIGHTNESS;
      }
      
      if (val > BALL_THRESHOLD) {
        velocity = (double)val / 100.0;
        Serial.print(b);
        Serial.print(": ");
        Serial.print(velocity);
        addBall(velocity, b);
      }
    }
    Serial.print(" ");
    Serial.print(nextBall);
    Serial.println("------------------");
}

void  addBall(float avel, int  aband) {

    // add new ball to end of list of there is room
    if (nextBall < MAX_BALLS) {

      if (numBalls[aband] < MAX_BAND_BALLS) {
        Serial.println(" Yes ");
        pos[nextBall] = 0;
        vel[nextBall] = avel;
        band[nextBall] = aband;
  
        numBalls[aband]++;
        nextBall++;
      } else {
        Serial.println(" No ");
      }
    }
}

void  moveBalls() {
    uint32_t tnow     = millis();  
    double elapsed = (double)(tnow - lastMoveMs) * 0.001;  

    double deltaV        = GRAVITY * elapsed ;
    double halfATSquared = deltaV  * elapsed * 0.5 ;

    // process each ball
    for (int ball = 0; ball < nextBall; ball++) {
      pos[ball] += (halfATSquared + (vel[ball] * elapsed)); 
      vel[ball] += deltaV  ;

      Serial.print(pos[ball]);
      Serial.print(" ");

      // has the ball hit the ground?
      if (pos[ball] <= 0) {

        Serial.print(" [Remove ");
        Serial.print(ball);
        Serial.print(" count ");
        Serial.print(numBalls[band[ball]]);
        Serial.print(" ->   ");
        
        // remove this ball and move all others down.
        nextBall--;
        numBalls[band[ball]]--;
        
        Serial.print(numBalls[band[ball]]);
        Serial.print("] ");

        for (int b = ball; b < nextBall; b++) {
          pos[b] = pos[b+1];
          vel[b] = vel[b+1];
          band[b] = band[b+1];
        }
      }
    }
    Serial.println("");

    lastMoveMs =tnow;
}

void  displayBalls() {

    // process each ball
    FastLED.clearData();
    for (int ball = 0; ball < nextBall; ball++) {
      setLED((int)(pos[ball] * LED_PER_METER), band[ball] * hueStep, MAX_LED_BRIGHTNESS); 
    }  
    
    FastLED.show();
}
