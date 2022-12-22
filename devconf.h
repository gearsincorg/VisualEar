#ifndef devconf_h /* Prevent loading library twice */
#define devconf_h

#define FLIP_LED_ORDER      true
#define NUM_MODES 4

#define SUB_BANDS           24                     // Number of sub
#define NUM_FFTS             4                     // Number of FFTs
#define NUM_BANDS           (SUB_BANDS * NUM_FFTS) // Total Number of frequency bands being displayed

#define NUM_LEDS            NUM_BANDS             // One LED per Band
#define MIN_DB              30.0
#define ORANGE_DB           70.0
#define RED_DB              85.0
#define MAX_DB             100.0
#define DB_RANGE           (MAX_DB - MIN_DB)
#define DB_PER_LED         (DB_RANGE / NUM_LEDS)

#define LED_DATA_PIN        12
#define LED_CLOCK_PIN       14

// NV RAM locations
#define MODE_ADDRESS         1
#define GAIN_ADDRESS         2

#define MIN_GAIN_NUM         0
#define MAX_GAIN_NUM        (NUM_LEDS - 1)
#define GAIN_RANGE          (MAX_GAIN_NUM - MIN_GAIN_NUM)

#endif
