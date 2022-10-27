/*


*/
#ifndef display_H /* Prevent loading library twice */
#define display_H

#define MODE_CHANGE_PAUSE   1000

#define BALL_THRESHOLD      50
#define MAX_BALLS           40                  // maximum number of balls in the air
#define MAX_BAND_BALLS       1
#define MAX_LED_BRIGHTNESS 255
#define DIM_LED_BRIGHTNESS  15

#define TOP_HUE_NUMBER      240.0

#define DISPLAY_MODE_OFF   0
#define DISPLAY_MODE_VU    1
#define DISPLAY_MODE_FFT   2
#define DISPLAY_MODE_TONE  3
#define DISPLAY_MODE_BALLS 4


//#define GRAVITY          -2.0                //  
#define GRAVITY            -1.2                //  was 1.5

#define LED_PER_METER       60

extern double minScale;
extern double gainSlope;
extern double lowTrip;
extern double highTrip;

short switchDisplayMode();
short getDisplayMode();
void  setDisplayMode(short mode) ;
void  showMode();

void  initDisplay() ;
void  updateDisplay (uint32_t * bandValues);

void  initFFTDisplay(int numBands) ;
void  initBallDisplay(int numBands) ;

void  updateFFTDisplay (uint32_t * bandValues);
void  updateToneDisplay (uint32_t * bandValues);
void  updateBallDisplay (uint32_t * bandValues);
void  updateVuDisplay(double  peakToPeak);

int   flipLEDs(int num);

void  addBalls(uint32_t * bandValues);
void  addBall(float vel, int  band);
void  moveBalls();
void  displayBalls();

double  spikeFilter(double filter, double live, double upTC, double downTC);

void  setLED(int pos, int hue, int intensity);
void  setLEDBand(int pos,  int band,  int intensity);

#endif
