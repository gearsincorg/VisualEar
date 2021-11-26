#ifndef devconf_h /* Prevent loading library twice */
#define devconf_h

#define FLIP_LED_ORDER      true
#define NUM_MODES 4

#define NUM_LO_BANDS        25                    // Number of LOW frequency bands  
#define NUM_MD_BANDS        31                    // Number of MID frequency bands  
#define NUM_HI_BANDS        48                    // Number of HIGH frequency bands
#define NUM_BANDS           (NUM_LO_BANDS + NUM_MD_BANDS + NUM_HI_BANDS)    // Total Number of frequency bands being displayed

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
