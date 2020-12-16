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

#ifndef audioAnalyzer_h /* Prevent loading library twice */
#define audioAnalyzer_h

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"
#include "arduinoFFT_float.h"

class AudioAnalyzeFFT : public AudioStream
{
public:
  AudioAnalyzeFFT(void);
  bool available(void);
  bool missingBlocks(void);
  float read(unsigned short binNumber);
  float read(unsigned short binNumber, float noiseThreshold);
  float read(unsigned short binFirst, unsigned short binLast, float noiseThreshold);
  void  setInputDivide(float divisor);
  virtual void update(void);

  ushort output[512] __attribute__ ((aligned (4)));

private:
  void init(void);
  void copy_to_fft_buffer(void *destination, const void *source);
  
  audio_block_t *blocklist[BURSTS_PER_AUDIO];
  short buffer[2048] __attribute__ ((aligned (4)));
  float inputDivisor = 256;
  byte state;
  volatile bool outputflag;
  volatile bool missedBlock;
  
  audio_block_t *inputQueueArray[1];

  float     vReal[FFT_SAMPLES];
  float     vImag[FFT_SAMPLES];
  float     weights[FFT_SAMPLES];
  arduinoFFT_float FFT;
};

#endif
