#ifndef devconf_h /* Prevent loading library twice */
#define devconft_h

#define NUM_LO_BANDS        41                    // Number of LOW frequency bands  
#define NUM_HI_BANDS        63                    // Number of HIGH frequency bands
#define NUM_BANDS           (NUM_LO_BANDS + NUM_HI_BANDS)    // Total Number of frequency bands being displayed

#define NUM_LEDS            NUM_BANDS             // One LED per Band


#endif
