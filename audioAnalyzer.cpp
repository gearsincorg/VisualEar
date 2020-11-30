#include <Arduino.h>
#include <AudioStream.h>
#include "audioAnalyzer.h"

AudioAnalyzeFFT::AudioAnalyzeFFT(void) : AudioStream(1, inputQueueArray),
                      state(0),
                      outputflag(false) {
                        
  FFT = arduinoFFT_float(vReal, vImag, FFT_SAMPLES, SAMPLING_FREQ);    

  // Load up the windowing weights.  Generate an array of weights based on type.
  for (int i=0; i < FFT_SAMPLES; i++) {
    weights[i] = 1.0;
  }
  FFT.Windowing(weights, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
}

bool AudioAnalyzeFFT::available() {
  if (outputflag == true) {
    outputflag = false;
    return true;
  }
  return false;
}

bool AudioAnalyzeFFT::missingBlocks(){
  if (missedBlock) {
    missedBlock = false;
    return true;
  }
  return false;
}

float AudioAnalyzeFFT::read(unsigned short binNumber, float noiseThreshold) {
  float tempVal = vReal[binNumber];
  if (binNumber >= FREQ_BINS) return 0;
  if (tempVal   < noiseThreshold) return 0;
  return (tempVal);
}

float AudioAnalyzeFFT::read(unsigned short binNumber) {
  return (read(binNumber, 0.0));
}

float AudioAnalyzeFFT::read(unsigned short binFirst, unsigned short binLast, float noiseThreshold) {
  
  float sum = 0.0;
  do {
    sum += read(binFirst++, noiseThreshold);
  } while (binFirst <= binLast);
  return sum;
}

void AudioAnalyzeFFT::update(void)
{
  audio_block_t *block;
  short *src ;
  float *dest ;
  short DCLevel;

  block = receiveReadOnly();
  if (!block) {
    missedBlock = true;
    return;
  }

  blocklist[state] = block;

  // Do we have a full ausio buffer?
  if (state == (BURSTS_PER_AUDIO - 1)) {

    // First find average level to remove DC bias.
    long sum = 0;
    for (byte burst=0; burst < BURSTS_PER_AUDIO; burst++) {
        src = blocklist[burst]->data;
        for (short sample = 0; sample < BURST_SAMPLES; sample++) {
          sum += (long)*src++;
        }
    }
    DCLevel = sum >> SAMPLES_AVG_SHIFT; 

    // Now transfer samples to float while removeing DC level
    dest = vReal;
    short w = 0;
    for (byte burst=0; burst < BURSTS_PER_AUDIO; burst++, w++) {
        src = blocklist[burst]->data;
        for (short sample = 0; sample < BURST_SAMPLES; sample++) {
          *dest++ = ((float)(*src++ - DCLevel) * weights[w]) / 16384.0       ;
        }
    }

    // Process the FFT
    // Clear out imaginary values and run the FFT and then convert to magnitudes
    memset((void *)vImag, 0, sizeof(vImag));
    FFT.Compute(FFT_FORWARD);
    FFT.ComplexToMagnitude();

    outputflag = true;

    // Release bursts that are being discarded
    for (byte i = 0; i < BURSTS_PER_FFT_UPDATE; i++) {
      release(blocklist[i]);
    }
          
    // slide down remaining bursts
    for (byte i = 0; i < (BURSTS_PER_AUDIO - BURSTS_PER_FFT_UPDATE); i++) {
      blocklist[i] = blocklist[i + BURSTS_PER_FFT_UPDATE];
    }

    // Update the actve burst insert point.
    state = (BURSTS_PER_AUDIO) - BURSTS_PER_FFT_UPDATE;

  } else {
    state++;
  }
}
