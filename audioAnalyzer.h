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

//  =================  Multi-Task Shared Data =================
// -- Audio Constants
#define MIC_CLOCK_PIN       26                    // Serial Clock (SCK)
#define MIC_DATA_PIN        25                    // Serial Data (SD)
#define MIC_SEL_PIN         33                    // unsigned short Select (WS)
#define UNUSED_AUDIO_BITS   16                    // Bits do discard from the 32 bit audio sample.

// Low Range Constants
const unsigned short LO_SAMPLE_SKIP       =    16;         // How many samples to combine
const unsigned short LO_SAMPLING_FREQ     = 44100 / LO_SAMPLE_SKIP; // Frequency at which microphone is sampled
const unsigned short LO_FFT_SAMPLES       =  1024;        // Number of samples used to do FFT.
const unsigned short LO_FREQ_BINS         =  LO_FFT_SAMPLES >> 1; // Number of results

// Low Range Constants
const unsigned short MD_SAMPLE_SKIP       =     8;         // How many samples to combine
const unsigned short MD_SAMPLING_FREQ     = 44100 / MD_SAMPLE_SKIP; // Frequency at which microphone is sampled
const unsigned short MD_FFT_SAMPLES       =  1024;        // Number of samples used to do FFT.
const unsigned short MD_FREQ_BINS         =  MD_FFT_SAMPLES >> 1; // Number of results

// High Range Constants
const unsigned short HI_SAMPLE_SKIP       =     1;         // How many samples to combine
const unsigned short HI_SAMPLING_FREQ     = 44100 / HI_SAMPLE_SKIP; // Frequency at which microphone is sampled
const unsigned short HI_FFT_SAMPLES       =  1024;        // Number of samples used to do FFT. 
const unsigned short HI_FREQ_BINS         =  HI_FFT_SAMPLES >> 1; // Number of results

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
  float read(int range, unsigned short binNumber);
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

  short     LO_short[LO_FFT_SAMPLES];
  float     LO_vReal[LO_FFT_SAMPLES];
  float     LO_vImag[LO_FFT_SAMPLES];
  float     LO_weights[LO_FFT_SAMPLES];

  arduinoFFT_float LO_FFT;
  BufferManager    LO_Buffer;

  short     MD_short[MD_FFT_SAMPLES];
  float     MD_vReal[MD_FFT_SAMPLES];
  float     MD_vImag[MD_FFT_SAMPLES];
  float     MD_weights[MD_FFT_SAMPLES];

  arduinoFFT_float MD_FFT;
  BufferManager    MD_Buffer;

  short     HI_short[LO_FFT_SAMPLES];
  float     HI_vReal[HI_FFT_SAMPLES];
  float     HI_vImag[HI_FFT_SAMPLES];
  float     HI_weights[HI_FFT_SAMPLES];
  
  arduinoFFT_float HI_FFT;
  BufferManager    HI_Buffer;

};

#endif
