#ifndef devconf_h /* Prevent loading library twice */
#define devconf_h

#define FLIP_LED_ORDER      true
#define NUM_MODES 3

#define NUM_LO_BANDS        41                    // Number of LOW frequency bands  
#define NUM_HI_BANDS        63                    // Number of HIGH frequency bands
#define NUM_BANDS           (NUM_LO_BANDS + NUM_HI_BANDS)    // Total Number of frequency bands being displayed

#define NUM_LEDS            NUM_BANDS             // One LED per Band

#define LED_DATA_PIN        12
#define LED_CLOCK_PIN       14

// NV RAM locations
#define MODE_ADDRESS         1
#define GAIN_ADDRESS         2

#define MIN_GAIN_NUM         0
#define MAX_GAIN_NUM        (NUM_LEDS - 1)
#define GAIN_RANGE          (MAX_GAIN_NUM - MIN_GAIN_NUM)

#endif
