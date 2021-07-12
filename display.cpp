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
short     displayMode; 
uint32_t  modeChangeRelease = 0;

double    gainSlope = 1.0;
double    minScale  = 0.0;
double    lowTrip   = 0;
double    highTrip  = 1;

int       orangeLED;
int       redLED;

double  peakFilter = 0;
double  levelFilter = 0;

// ======================================================================================================
// Generic Display Functions
// ======================================================================================================
void  setDisplayMode(short  mode) {
  displayMode = mode;    
}

short  getDisplayMode() {
  return(displayMode);    
}

short  switchDisplayMode() {
  displayMode = (short)((displayMode + 1 ) % NUM_MODES);
  modeChangeRelease = millis() + MODE_CHANGE_PAUSE;
  showMode();
  return (displayMode);
}

void  initDisplay(){

  delay(250);      // sanity check delay
  FastLED.addLeds<APA102, BGR>(leds, NUM_LEDS);
  FastLED.clear();
  
  switch (displayMode) {
    case 0:
      break;

    case 1:
      orangeLED   = (int)((ORANGE_DB - MIN_DB) / DB_PER_LED);
      redLED      = (int)((RED_DB    - MIN_DB) / DB_PER_LED);
      delay(250);
      break;
      
    case 2:
      initFFTDisplay(NUM_BANDS);
      break;

    case 3:
      initBallDisplay(NUM_BANDS);
      break;
  }
}

void  updateDisplay(uint32_t * bandValues) {
  // display the data in the selected way.
  if (millis() > modeChangeRelease) {
    switch (displayMode) {
      default:
      case 0:
        break;
      
      case 2:
        updateFFTDisplay(bandValues);
        break;
  
      case 3:
        updateBallDisplay(bandValues);
        break;
    }
  }
}

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

void  showMode () {
  FastLED.clearData();
  FastLED.show();
  for (int I = 0; I < displayMode; I++) {
    setLEDBand(I * 4, I * 8, MAX_LED_BRIGHTNESS); 
  }
  FastLED.show();
}

// ======================================================================================================
//  FFT display functions
// ======================================================================================================

// Configure the LED string and preload the color values for each band.
void  initFFTDisplay(int numberBands) {

  const double MIN_GAIN_SCALE  = 0.00125;
  const double MAX_GAIN_SCALE  = 0.10; 
  const double LOW_TRIP        = 0.05; 
  const double HIGH_TRIP       = 0.3; 
  
  minScale  = MIN_GAIN_SCALE;   
  gainSlope = pow((MAX_GAIN_SCALE / MIN_GAIN_SCALE), 1.0 / MAX_GAIN_NUM);   
  lowTrip   = LOW_TRIP;
  highTrip  = HIGH_TRIP;
  
  numBands = numberBands;
  hueStep = TOP_HUE_NUMBER / numBands;   // How much the LED Hue changes for each step.
}

// Update the LED string based on the intensities of all the Frequency bins.
void  updateFFTDisplay (uint32_t * bandValues){
  uint16_t ledBrightness;
  
  // Process the LED buckets into LED Intensities
  for (byte band = 0; band < numBands; band++) {
    
    // Scale the bars for the display
    ledBrightness = bandValues[band];

    if (ledBrightness > MAX_LED_BRIGHTNESS)  {
      ledBrightness = MAX_LED_BRIGHTNESS;
    }
  
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
  hueStep = TOP_HUE_NUMBER / numBands;   // How much the LED Hue changes for each step.

  const double MIN_GAIN_SCALE  = 0.001;
  const double MAX_GAIN_SCALE  = 0.05;
  const double LOW_TRIP        = 0.04; 
  const double HIGH_TRIP       = 0.35; 
  
  minScale  = MIN_GAIN_SCALE;   
  gainSlope = pow((MAX_GAIN_SCALE / MIN_GAIN_SCALE), 1.0 / MAX_GAIN_NUM);   
  lowTrip   = LOW_TRIP;
  highTrip  = HIGH_TRIP;

  // initialize ball data;
  memset(pos, 0, sizeof(pos));
  memset(vel, 0, sizeof(vel));
  memset(band, 0, sizeof(band));
  memset(numBalls, 0, sizeof(numBalls));
}

void updateBallDisplay (uint32_t * bandValues){
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
        addBall(velocity, b);
      }
    }
}

void  addBall(float avel, int  aband) {

    // add new ball to end of list of there is room
    if (nextBall < MAX_BALLS) {

      if (numBalls[aband] < MAX_BAND_BALLS) {
        pos[nextBall] = 0;
        vel[nextBall] = avel;
        band[nextBall] = aband;
  
        numBalls[aband]++;
        nextBall++;
      } else {
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

      // has the ball hit the ground?
      if (pos[ball] <= 0) {

        // remove this ball and move all others down.
        nextBall--;
        numBalls[band[ball]]--;
        
        for (int b = ball; b < nextBall; b++) {
          pos[b] = pos[b+1];
          vel[b] = vel[b+1];
          band[b] = band[b+1];
        }
      }
    }
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

// ======================================================================================================
//  VU Meter display
// ======================================================================================================

void  updateVuDisplay(double p2p) {
  double  db = (9.1024 * log(p2p)) + 115.82;

  levelFilter = spikeFilter(levelFilter, db, 0.2, 0.05);
  peakFilter  = spikeFilter(peakFilter,  db, 1.0, 0.001);

  int levelLED    = (int)((levelFilter - MIN_DB) / DB_PER_LED);
  int peakLED     = (int)((peakFilter  - MIN_DB) / DB_PER_LED);

  if (peakLED < levelLED) {
    peakLED = levelLED;
  }

  // run up from bottom of display to top.
  FastLED.clearData();
  short hue = 96;
  for (short l=0; l < NUM_LEDS; l++) {
    if (l >= redLED)
      hue = 0;
    else if (l >= orangeLED)    
      hue = 32;
      
    if (l == peakLED){
      setLED(l, hue, MAX_LED_BRIGHTNESS);
    } else if (l <= levelLED) {
      setLED(l, hue, MAX_LED_BRIGHTNESS / 4);
    } else {
      setLED(l, hue, DIM_LED_BRIGHTNESS);
    }
                
  }

  FastLED.show();

  Serial.print(peakFilter);
  Serial.print(" ");
  Serial.print(levelFilter);
  Serial.println(" 50 100");
  
}

double  spikeFilter(double filter, double live, double upTC, double downTC){
  if (live > filter) {
    filter += ((live - filter) * upTC); 
  } else {
    filter += ((live - filter) * downTC); 
  }

  return (filter);
}
