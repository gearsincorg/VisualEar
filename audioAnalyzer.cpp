/*

  FFT libray
  Copyright (C) 2021 Phil Malone
  Copyright (C) 2010 Didier Longueville
  Copyright (C) 2014 Enrique Condes
  Copyright (C) 2021 Philip Malone

*/

#include <Arduino.h>
#include <AudioStream.h>
#include "audioAnalyzer.h"
#include "bufferManager.h"

AudioAnalyzeFFT::AudioAnalyzeFFT(void) : AudioStream(1, inputQueueArray) 
{
  for (int f=0; f < NUM_FFTS; f++){
    FFT[f]    = arduinoFFT_float(vReal[f], vImag[f], weights[f], FFT_SAMPLES, FFT_WIN_TYP_HAMMING);    
    Buffer[f] = BufferManager(vReal[f], weights[f], shortIn[f],   FFT_SAMPLES, SAMPLE_SKIP[f], CUTOFF_FREQ[f]);
    memset(shortIn[f], 0, sizeof(shortIn[f]));
    
  }
  state = 0;
  outputflag = false;
}

// Return and then clear the "available" flag.
bool AudioAnalyzeFFT::available() {
  bool temp = outputflag;
  outputflag = false;
  return temp;
}

// Return and then clear the "missedBlock" flag.
bool AudioAnalyzeFFT::missingBlocks(){
  bool temp = missedBlock;
  missedBlock = false;
  return temp;
}

// Return the current Scale ratio
void  AudioAnalyzeFFT::setInputScale(float scale){
  inputScale = scale;
}

float AudioAnalyzeFFT::read(int  range, unsigned short binNumber, float noiseThreshold) {
  float tempVal;
  
  if ((range < NUM_FFTS) && (binNumber < FREQ_BINS)) {
    tempVal = vReal[range][binNumber];
  } else {
    tempVal = 0;
  }

  if (tempVal < noiseThreshold) 
    tempVal = 0;

  return (tempVal);
}

float AudioAnalyzeFFT::read(int  range, unsigned short binFirst, unsigned short binLast, float noiseThreshold) {
  
  float sum = 0.0;
  do {
    sum += read(range, binFirst++, noiseThreshold);
  } while (binFirst <= binLast);
  return sum;
}

void AudioAnalyzeFFT::update(void)
{
  audio_block_t *block;
  short *src ;
  
  // unsigned long startUpdate = micros();

  block = receiveReadOnly();
  if (!block) {
    missedBlock = true;
    return;
  }

  // Save a pointer to the latest audio block
  src = block->data;

  // add the latest block to all the buffers
  for (short sample = 0; sample < BURST_SAMPLES; sample++) {
    for (int f=0; f < NUM_FFTS; f++){
      Buffer[f].addSample(*src);
    }
    src++;
  }

  // Release audio block back into the pool
  release(block);
  state++;

  // Do we have a full audio buffer?
  if (state == BURSTS_PER_FFT_UPDATE) {

    // transfer the accumulated buffers to all the FFTs.  Remove bias and apply weights along the way
    for (int f=0; f < NUM_FFTS; f++){
      Buffer[f].transfer(inputScale);
      FFT[f].RunFFT();
    }
    
    outputflag = true;
    state = 0;

    // Serial.print("Update= ");
    // Serial.print((float)(micros() - startUpdate) / 1000.0);
    // Serial.println(" mSec");
  }
}
