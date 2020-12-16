/*

	FFT libray
	Copyright (C) 2010 Didier Longueville
	Copyright (C) 2014 Enrique Condes

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

#ifndef arduinoFFT_float_h /* Prevent loading library twice */
#define arduinoFFT_float_h

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

#define FFT_LIB_REV 0x14
/* Custom constants */
#define FFT_FORWARD 0x01
#define FFT_REVERSE 0x00

/* Windowing type */
#define FFT_WIN_TYP_RECTANGLE 0x00 /* rectangle (Box car) */
#define FFT_WIN_TYP_HAMMING 0x01 /* hamming */
#define FFT_WIN_TYP_HANN 0x02 /* hann */
#define FFT_WIN_TYP_TRIANGLE 0x03 /* triangle (Bartlett) */
#define FFT_WIN_TYP_NUTTALL 0x04 /* nuttall */
#define FFT_WIN_TYP_BLACKMAN 0x05 /* blackman */
#define FFT_WIN_TYP_BLACKMAN_NUTTALL 0x06 /* blackman nuttall */
#define FFT_WIN_TYP_BLACKMAN_HARRIS 0x07 /* blackman harris*/
#define FFT_WIN_TYP_FLT_TOP 0x08 /* flat top */
#define FFT_WIN_TYP_WELCH 0x09 /* welch */
/*Mathematial constants*/
#define twoPi 6.28318531
#define fourPi 12.56637061
#define sixPi 18.84955593

//  =================  Multi-Task Shared Data =================
// -- Audio Constants
#define MIC_CLOCK_PIN       26                    // Serial Clock (SCK)
#define MIC_DATA_PIN        25                    // Serial Data (SD)
#define MIC_SEL_PIN         33                    // unsigned short Select (WS)
#define UNUSED_AUDIO_BITS   16                    // Bits do discard from the 32 bit audio sample.

const unsigned short SAMPLING_FREQ     = 44100;         // Frequency at which microphone is sampled
const unsigned short BURST_SAMPLES     =   128;         // Number of audio samples taken in one "Burst"
const unsigned short BURSTS_PER_AUDIO  =    64;         // Number of Burst Buffers used to create a single Audio Packet
const unsigned short BURSTS_PER_FFT_UPDATE = 4;         // Number of Burst received before doing an FFT update
const unsigned short SAMPLES_AVG_SHIFT =    13;         // Bit shift required to average one full Sample
const unsigned short EXTRA_BURSTS      =     8;         // Extra Burst packets to avoid overlap
const unsigned short NUM_BURSTS        = (BURSTS_PER_AUDIO + EXTRA_BURSTS);
const unsigned short SIZEOF_BURST      = (BURST_SAMPLES << 2);      // Number of bytes in a Burst Buffer

// -- FFT Constants
const unsigned short FFT_SAMPLES = BURST_SAMPLES * BURSTS_PER_AUDIO; // Number of samples used to do FFT.  (BURST_SAMPLES * BURSTS_PER_AUDIO)
const unsigned short FREQ_BINS   = (FFT_SAMPLES >> 1);              // Number or resulting Frequency Bins after FFT is done

// -- LED Display Constants
#define NUM_BANDS           60                    // Number of frequency bands being displayed as LEDs = Number of LEDs
#define NUM_LEDS            NUM_BANDS * 2         // Two LEDS per Band
#define LED_DATA_PIN        12
#define LED_CLOCK_PIN       14
#define GAIN_DIVIDE       1400                    // Brighness control used to reduce frequency band magnitude to get LED brightness
#define START_NOISE_FLOOR   80                    // Frequency Bin Magnitudes below this value will not get summed into Bands. (Initial high value)
#define BASE_NOISE_FLOOR    40                    // Frequency Bin Magnitudes below this value will not get summed into Bands. (Final minimumm value)
#define BAND_HUE_STEP      220 / NUM_BANDS        // How much the LED Hue changes for each band.

// ---------------------------------------------

class arduinoFFT_float {
  
public:
	/* Constructor */
	arduinoFFT_float(void);
	arduinoFFT_float(float *vReal, float *vImag, ushort samples, float samplingFrequency);
	/* Destructor */
	~arduinoFFT_float(void);
	/* Functions */
	byte Revision(void);
	byte Exponent(ushort value);

	void ComplexToMagnitude(float *vReal, float *vImag, ushort samples);
	void Compute(float *vReal, float *vImag, ushort samples, byte dir);
	void Compute(float *vReal, float *vImag, ushort samples, byte power, byte dir);
	void DCRemoval(float *vData, ushort samples);
	float MajorPeak(float *vD, ushort samples, float samplingFrequency);
	void MajorPeak(float *vD, ushort samples, float samplingFrequency, float *f, float *v);
	void Windowing(float *vData, ushort samples, byte windowType, byte dir);

	void ComplexToMagnitude();
	void Compute(byte dir);
	void DCRemoval();
	float MajorPeak();
	void MajorPeak(float *f, float *v);

private:
	/* Variables */
	ushort _samples;
	float _samplingFrequency;
	float *_vReal;
	float *_vImag;
	byte _power;
	/* Functions */
	void Swap(float *x, float *y);
};

#endif
