/*


*/
#ifndef display_H /* Prevent loading library twice */
#define display_H

#define BALL_THRESHOLD      50
#define MAX_BALLS           40                  // maximum number of balls in the air
#define MAX_BAND_BALLS       2
#define MAX_LED_BRIGHTNESS 255

//#define GRAVITY          -9.8                //  
#define GRAVITY            -2.0                //  
#define LED_PER_METER       60

void  initFFTDisplay(int numBands) ;
int   updateFFTDisplay (uint32_t * bandValues);

void  initBallDisplay(int numBands) ;
int   updateBallDisplay (uint32_t * bandValues);

int   flipLEDs(int num);

int   addBalls(uint32_t * bandValues);
void  addBall(float vel, int  band);
void  moveBalls();
void  displayBalls();

void  setLED(int pos, int hue, int intensity);
void  setLEDBand(int pos,  int band,  int intensity);

#endif
