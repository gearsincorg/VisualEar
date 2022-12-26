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

#ifndef audioAnalyzer_h /* Prevent loading library twice */
#define audioAnalyzer_h

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"
#include "arduinoFFT_float.h"
#include "bufferManager.h"
#include "devconf.h"

//  =================  Multi-Task Shared Data =================
// -- Audio Constants
#define MIC_CLOCK_PIN       26                    // Serial Clock (SCK)
#define MIC_DATA_PIN        25                    // Serial Data (SD)
#define MIC_SEL_PIN         33                    // unsigned short Select (WS)
#define UNUSED_AUDIO_BITS   16                    // Bits do discard from the 32 bit audio sample.

#define CUTOFF_SCALE  0.75

// Low Range Constants
const unsigned short SAMPLE_SKIP[NUM_FFTS]    = {64, 16, 4, 1};           // How many samples to combine
const float          CUTOFF_FREQ[NUM_FFTS]    = {220.0 * CUTOFF_SCALE, 880.0 * CUTOFF_SCALE, 3520.0 * CUTOFF_SCALE, 14080.0 * CUTOFF_SCALE}; // High Cutoff Freq
const unsigned short FFT_SAMPLES       =  512;        // Number of samples used to do FFT.
const unsigned short FREQ_BINS         =  FFT_SAMPLES >> 1; // Number of results

// Audio Sample constants
const unsigned short BURST_SAMPLES     =   128;         // Number of audio samples taken in one "Burst"
const unsigned short BURSTS_PER_FFT_UPDATE = 4;         // Number of Burst received before doing an FFT update
const unsigned short NUM_BURSTS        = 8;
const unsigned short SIZEOF_BURST      = (BURST_SAMPLES << 2);      // Number of bytes in a Burst Buffer

// ---------------------------------------------

class AudioAnalyzeFFT : public AudioStream
{
public:
  AudioAnalyzeFFT(void);
  bool available(void);
  bool missingBlocks(void);
  float read(int range, unsigned short binNumber, float noiseThreshold);
  float read(int range, unsigned short binFirst, unsigned short binLast, float noiseThreshold);
  void  setInputScale(float scale);
  virtual void update(void);

  ushort output[512] __attribute__ ((aligned (4)));

private:
  void init(void);
  void copy_to_fft_buffer(void *destination, const void *source);
  
  //audio_block_t *blocklist[BURSTS_PER_AUDIO];
  short buffer[2048] __attribute__ ((aligned (4)));
  float inputScale;
  volatile bool outputflag;
  volatile bool missedBlock;
  unsigned short state;
  
  audio_block_t *inputQueueArray[1];

  short     shortIn[NUM_FFTS][FFT_SAMPLES];
  float     vReal[NUM_FFTS][FFT_SAMPLES];
  float     vImag[NUM_FFTS][FFT_SAMPLES];
  float     weights[NUM_FFTS][FFT_SAMPLES];

  arduinoFFT_float FFT[NUM_FFTS];
  BufferManager    Buffer[NUM_FFTS];
};

#endif
