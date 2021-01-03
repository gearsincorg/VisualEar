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

#include <Arduino.h>
#include <AudioStream.h>
#include "arduinoFFT_float.h"

//  =================  Multi-Task Shared Data =================

// -- Object Constructors
arduinoFFT_float::arduinoFFT_float(){} ;

// Constructor
arduinoFFT_float::arduinoFFT_float(float *vReal, float *vImag, float *weights, unsigned short samples, float samplingFrequency, uint8_t windowType) {
	this->_vReal = vReal;
	this->_vImag = vImag;
  this->_weights = weights;
	this->_samples = samples;
	this->_samplingFrequency = samplingFrequency;
	this->_power = Exponent(samples);

  // load up weights array 
  for (int i=0; i < samples; i++) {
    weights[i] = 1.0;
  }
  Windowing(weights, samples, windowType, FFT_FORWARD);
}

// Destructor
arduinoFFT_float::~arduinoFFT_float(void) {
}

byte arduinoFFT_float::Revision(void) {
	return(FFT_LIB_REV);
}

void arduinoFFT_float::RunFFT(void) {
    // Clear out imaginary values and run the FFT and then convert to magnitudes
    
    memset((void *)this->_vImag, 0, (this->_samples * sizeof(float)));
    Compute(FFT_FORWARD);
    ComplexToMagnitude();
}

void arduinoFFT_float::Compute(byte dir)
{// Computes in-place complex-to-complex FFT /

	// Reverse bits /
	unsigned short j = 0;
	for (unsigned short i = 0; i < (this->_samples - 1); i++) {
		if (i < j) {
			Swap(&this->_vReal[i], &this->_vReal[j]);
			if(dir==FFT_REVERSE)
				Swap(&this->_vImag[i], &this->_vImag[j]);
		}
		unsigned short k = (this->_samples >> 1);
		while (k <= j) {
			j -= k;
			k >>= 1;
		}
		j += k;
	}
	// Compute the FFT  /
#ifdef __AVR__
	byte index = 0;
#endif
	float c1 = -1.0;
	float c2 = 0.0;
	unsigned short l2 = 1;
	for (byte l = 0; (l < this->_power); l++) {
		unsigned short l1 = l2;
		l2 <<= 1;
		float u1 = 1.0;
		float u2 = 0.0;
		for (j = 0; j < l1; j++) {
			 for (unsigned short i = j; i < this->_samples; i += l2) {
					unsigned short i1 = i + l1;
					float t1 = u1 * this->_vReal[i1] - u2 * this->_vImag[i1];
					float t2 = u1 * this->_vImag[i1] + u2 * this->_vReal[i1];
					this->_vReal[i1] = this->_vReal[i] - t1;
					this->_vImag[i1] = this->_vImag[i] - t2;
					this->_vReal[i] += t1;
					this->_vImag[i] += t2;
			 }
			 float z = ((u1 * c1) - (u2 * c2));
			 u2 = ((u1 * c2) + (u2 * c1));
			 u1 = z;
		}
		c2 = sqrt((1.0 - c1) / 2.0);
		c1 = sqrt((1.0 + c1) / 2.0);
		if (dir == FFT_FORWARD) {
			c2 = -c2;
		}
	}
	// Scaling for reverse transform /
	if (dir != FFT_FORWARD) {
		for (unsigned short i = 0; i < this->_samples; i++) {
			 this->_vReal[i] /= this->_samples;
			 this->_vImag[i] /= this->_samples;
		}
	}
}

void arduinoFFT_float::ComplexToMagnitude()
{ // vM is half the size of vReal and vImag
	for (unsigned short i = 0; i < this->_samples; i++) {
		this->_vReal[i] = sqrt(sq(this->_vReal[i]) + sq(this->_vImag[i]));
	}
}

void arduinoFFT_float::Windowing(float *vData, uint16_t samples, uint8_t windowType, uint8_t dir)
{ // Weighing factors are computed once before multiple use of FFT
  float samplesMinusOne = (float(samples) - 1.0);
  for (uint16_t i = 0; i < (samples >> 1); i++) {
    float indexMinusOne = float(i);
    float ratio = (indexMinusOne / samplesMinusOne);
    float weighingFactor = 1.0;
    // Compute and record weighting factor
    switch (windowType) {
    case FFT_WIN_TYP_RECTANGLE: // rectangle (box car)
      weighingFactor = 1.0;
      break;
    case FFT_WIN_TYP_HAMMING: // hamming
      weighingFactor = 0.54 - (0.46 * cos(twoPi * ratio));
      break;
    case FFT_WIN_TYP_HANN: // hann
      weighingFactor = 0.54 * (1.0 - cos(twoPi * ratio));
      break;
    case FFT_WIN_TYP_TRIANGLE: // triangle (Bartlett)
      #if defined(ESP8266) || defined(ESP32)
      weighingFactor = 1.0 - ((2.0 * fabs(indexMinusOne - (samplesMinusOne / 2.0))) / samplesMinusOne);
      #else
      weighingFactor = 1.0 - ((2.0 * abs(indexMinusOne - (samplesMinusOne / 2.0))) / samplesMinusOne);
      #endif
      break;
    case FFT_WIN_TYP_NUTTALL: // nuttall
      weighingFactor = 0.355768 - (0.487396 * (cos(twoPi * ratio))) + (0.144232 * (cos(fourPi * ratio))) - (0.012604 * (cos(sixPi * ratio)));
      break;
    case FFT_WIN_TYP_BLACKMAN: // blackman
      weighingFactor = 0.42323 - (0.49755 * (cos(twoPi * ratio))) + (0.07922 * (cos(fourPi * ratio)));
      break;
    case FFT_WIN_TYP_BLACKMAN_NUTTALL: // blackman nuttall
      weighingFactor = 0.3635819 - (0.4891775 * (cos(twoPi * ratio))) + (0.1365995 * (cos(fourPi * ratio))) - (0.0106411 * (cos(sixPi * ratio)));
      break;
    case FFT_WIN_TYP_BLACKMAN_HARRIS: // blackman harris
      weighingFactor = 0.35875 - (0.48829 * (cos(twoPi * ratio))) + (0.14128 * (cos(fourPi * ratio))) - (0.01168 * (cos(sixPi * ratio)));
      break;
    case FFT_WIN_TYP_FLT_TOP: // flat top
      weighingFactor = 0.2810639 - (0.5208972 * cos(twoPi * ratio)) + (0.1980399 * cos(fourPi * ratio));
      break;
    case FFT_WIN_TYP_WELCH: // welch
      weighingFactor = 1.0 - sq((indexMinusOne - samplesMinusOne / 2.0) / (samplesMinusOne / 2.0));
      break;
    }
    if (dir == FFT_FORWARD) {
      vData[i] *= weighingFactor;
      vData[samples - (i + 1)] *= weighingFactor;
    }
    else {
      vData[i] /= weighingFactor;
      vData[samples - (i + 1)] /= weighingFactor;
    }
  }
}

byte arduinoFFT_float::Exponent(unsigned short value)
{
  // Calculates the base 2 logarithm of a value
  byte result = 0;
  while (((value >> result) & 1) != 1) result++;
  return(result);
}

// Private functions

void arduinoFFT_float::Swap(float *x, float *y)
{
	float temp = *x;
	*x = *y;
	*y = temp;
}
