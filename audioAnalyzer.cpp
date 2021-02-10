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
  LO_FFT = arduinoFFT_float(LO_vReal, LO_vImag,   LO_weights, LO_FFT_SAMPLES, LO_SAMPLING_FREQ, FFT_WIN_TYP_HAMMING);    
  LO_Buffer = BufferManager(LO_vReal, LO_weights, LO_short,   LO_FFT_SAMPLES, LO_SAMPLE_SKIP);
  
  HI_FFT = arduinoFFT_float(HI_vReal, HI_vImag,   HI_weights, HI_FFT_SAMPLES, HI_SAMPLING_FREQ, FFT_WIN_TYP_HAMMING);    
  HI_Buffer = BufferManager(HI_vReal, HI_weights, HI_short,   HI_FFT_SAMPLES, HI_SAMPLE_SKIP);

  state = 0;
  outputflag = false;

//  memset(LO_vReal, 0, sizeof(LO_vReal));
//  memset(Hi_vReal, 0, sizeof(Hi_vReal));
  memset(LO_short, 0, sizeof(LO_short));
  memset(HI_short, 0, sizeof(HI_short));
  
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

float AudioAnalyzeFFT::read(bool  hiRange, unsigned short binNumber, float noiseThreshold) {
  float tempVal;

  if (!hiRange && (binNumber < LO_FREQ_BINS)) {
    tempVal = LO_vReal[binNumber];
  } else if (hiRange && (binNumber < HI_FREQ_BINS)) {
    tempVal = HI_vReal[binNumber];
  } else {
    tempVal = 0;
  }
    
  if (tempVal < noiseThreshold) 
    tempVal = 0;
    
  return (tempVal);
}

float AudioAnalyzeFFT::read(bool  hiRange, unsigned short binNumber) {
  return (read(hiRange, binNumber, 0.0));
}

float AudioAnalyzeFFT::read(bool  hiRange, unsigned short binFirst, unsigned short binLast, float noiseThreshold) {
  
  float sum = 0.0;
  do {
    sum += read(hiRange, binFirst++, noiseThreshold);
  } while (binFirst <= binLast);
  return sum;
}

void AudioAnalyzeFFT::update(void)
{
  audio_block_t *block;
  short *src ;
  
  unsigned long startUpdate = micros();

  block = receiveReadOnly();
  if (!block) {
    missedBlock = true;
    return;
  }

  // Save a pointer to the latest audio block
  src = block->data;

  // add the latest block to the hi and low buffers
  for (short sample = 0; sample < BURST_SAMPLES; sample++) {
    LO_Buffer.addSample(*src);
    HI_Buffer.addSample(*src);
    src++;
  }

  // Release audio block back into the pool
  release(block);
  state++;

  // Do we have a full audio buffer?
  if (state == BURSTS_PER_FFT_UPDATE) {

    // transfer the accumulated buffers to the FFT.  Remove bias and apply weights along the way
    LO_Buffer.transfer(inputScale);
    HI_Buffer.transfer(inputScale);

    // Process the FFT
    LO_FFT.RunFFT();
    HI_FFT.RunFFT();

    outputflag = true;
    state = 0;

    // Serial.print("Update= ");
    // Serial.print((float)(micros() - startUpdate) / 1000.0);
    // Serial.println(" mSec");
  }
}
