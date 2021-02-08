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

//  =================  Multi-Task Shared Data =================
// -- Audio Constants
#define MIC_CLOCK_PIN       26                    // Serial Clock (SCK)
#define MIC_DATA_PIN        25                    // Serial Data (SD)
#define MIC_SEL_PIN         33                    // unsigned short Select (WS)
#define UNUSED_AUDIO_BITS   16                    // Bits do discard from the 32 bit audio sample.

// Low Range Constants
const unsigned short LO_SAMPLE_SKIP       =     4;         // How many samples to combine
const unsigned short LO_SAMPLING_FREQ     = 44100 / LO_SAMPLE_SKIP; // Frequency at which microphone is sampled
const unsigned short LO_FFT_SAMPLES       =  2048;        // Number of samples used to do FFT.
const unsigned short LO_FREQ_BINS         =  LO_FFT_SAMPLES >> 1; // Number of results
const unsigned short LO_FIRST_BURST       =    0;          // First burst in FFT source
const unsigned short LO_BURSTS_PER_AUDIO  =    16 * LO_SAMPLE_SKIP ;

// High Range Constants
const unsigned short HI_SAMPLE_SKIP       =     1;         // How many samples to combine
const unsigned short HI_SAMPLING_FREQ     = 44100 / HI_SAMPLE_SKIP; // Frequency at which microphone is sampled
const unsigned short HI_FFT_SAMPLES       =  2048;        // Number of samples used to do FFT. 
const unsigned short HI_FREQ_BINS         =  HI_FFT_SAMPLES >> 1; // Number of results
const unsigned short HI_FIRST_BURST       =    48;        // First burst in FFT source
const unsigned short HI_BURSTS_PER_AUDIO  =    16 * HI_SAMPLE_SKIP ; 

// Audio Sample constants
const unsigned short BURST_SAMPLES     =   128;         // Number of audio samples taken in one "Burst"
const unsigned short BURSTS_PER_AUDIO  =    LO_BURSTS_PER_AUDIO;         // Number of Burst Buffers used to create a single Audio Packet
const unsigned short BURSTS_PER_FFT_UPDATE = 4;         // Number of Burst received before doing an FFT update
const unsigned short SAMPLES_AVG_SHIFT =    13;         // Bit shift required to average one full Sample
const unsigned short EXTRA_BURSTS      =    16;         // Extra Burst packets to avoid overlap
const unsigned short NUM_BURSTS        = (BURSTS_PER_AUDIO + EXTRA_BURSTS);
const unsigned short SIZEOF_BURST      = (BURST_SAMPLES << 2);      // Number of bytes in a Burst Buffer

// ---------------------------------------------

class AudioAnalyzeFFT : public AudioStream
{
public:
  AudioAnalyzeFFT(void);
  bool available(void);
  bool missingBlocks(void);
  float read(bool  hiRange, unsigned short binNumber);
  float read(bool  hiRange, unsigned short binNumber, float noiseThreshold);
  float read(bool  hiRange, unsigned short binFirst, unsigned short binLast, float noiseThreshold);
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

  float     LO_vReal[LO_FFT_SAMPLES];
  float     LO_vImag[LO_FFT_SAMPLES];
  float     LO_weights[LO_FFT_SAMPLES];
  arduinoFFT_float LO_FFT;

  float     HI_vReal[HI_FFT_SAMPLES];
  float     HI_vImag[HI_FFT_SAMPLES];
  float     HI_weights[HI_FFT_SAMPLES];
  arduinoFFT_float HI_FFT;
};

#endif
