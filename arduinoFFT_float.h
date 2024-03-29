/*

	FFT libray
	Copyright (C) 2010 Didier Longueville
	Copyright (C) 2014 Enrique Condes
  Copyright (C) 2021 Philip Malone

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

class arduinoFFT_float {
  
public:
	/* Constructor */
  arduinoFFT_float(void); 
  arduinoFFT_float(float *vReal, float *vImag, float *weights, unsigned short samples, float samplingFrequency, uint8_t windowType); 
  
	/* Destructor */
	~arduinoFFT_float(void);
  
	/* Functions */
  void RunFFT(void);
	byte Revision(void);
	byte Exponent(ushort value);
	void ComplexToMagnitude(float *vReal, float *vImag, ushort samples);
	void Compute(float *vReal, float *vImag, ushort samples, byte dir);
	void Compute(float *vReal, float *vImag, ushort samples, byte power, byte dir);
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
  float *_weights;
	byte _power;
	/* Functions */
	void Swap(float *x, float *y);
};

#endif
