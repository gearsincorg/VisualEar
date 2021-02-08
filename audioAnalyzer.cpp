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
#include "audioAnalyzer.h"

AudioAnalyzeFFT::AudioAnalyzeFFT(void) : AudioStream(1, inputQueueArray),
                      state(0),
                      outputflag(false) {
                        
  LO_FFT = arduinoFFT_float(LO_vReal, LO_vImag, LO_weights, LO_FFT_SAMPLES, LO_SAMPLING_FREQ, FFT_WIN_TYP_HAMMING);    
  HI_FFT = arduinoFFT_float(HI_vReal, HI_vImag, HI_weights, HI_FFT_SAMPLES, HI_SAMPLING_FREQ, FFT_WIN_TYP_HAMMING);    
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

// Return the current Divide ratio
void  AudioAnalyzeFFT::setInputDivide(float divisor){
  inputDivisor = divisor;
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
  float *dest ;
  short DCLevel;
  short w;
  float val;
  
  // unsigned long startUpdate = micros();

  block = receiveReadOnly();
  if (!block) {
    missedBlock = true;
    return;
  }

  // Save the latest audio block
  blocklist[state] = block;

  // Do we have a full audio buffer?
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

    // =============================================================
    // Now transfer samples to both FFT reals while removing DC level

    // Start with Lo range.  Take every N'th sample (effectively reducing the sample frequency)
    dest = LO_vReal;
    w = 0;
    
    for (byte burst=LO_FIRST_BURST; burst < LO_BURSTS_PER_AUDIO; burst++) {
        src = blocklist[burst]->data;

        // Create values to process by averaging groups of samples.
        for (short sample = 0; sample < BURST_SAMPLES; sample += LO_SAMPLE_SKIP,  w++) {
          val = 0.0;
          for (short s = 0; s < LO_SAMPLE_SKIP; s++) {
            val += ((float)(*src++ - DCLevel) * LO_weights[w]) / inputDivisor;
          }
          *dest++ = val / LO_SAMPLE_SKIP;
        }
    }

    // Now do the Hight range.  Take every sample of the most recent bursts.
    dest = HI_vReal;
    w = 0;
    
    for (byte burst=HI_FIRST_BURST; burst < (HI_FIRST_BURST + HI_BURSTS_PER_AUDIO); burst++) {
        src = blocklist[burst]->data;

        for (short sample = 0; sample < BURST_SAMPLES; sample += 1,  w++) {
          val = ((float)(*src - DCLevel) * HI_weights[w]) / inputDivisor;
          *dest++ = val;
          src += 1;  // Next sample
        }
    }

    // Process the FFT
    LO_FFT.RunFFT();
    HI_FFT.RunFFT();
    outputflag = true;

    // Release bursts that are being discarded
    for (byte i = 0; i < BURSTS_PER_FFT_UPDATE; i++) {
      release(blocklist[i]);
    }
          
    // slide down the remaining bursts
    for (byte i = 0; i < (BURSTS_PER_AUDIO - BURSTS_PER_FFT_UPDATE); i++) {
      blocklist[i] = blocklist[i + BURSTS_PER_FFT_UPDATE];
    }

    // Update the actve burst insert point.
    state = (BURSTS_PER_AUDIO) - BURSTS_PER_FFT_UPDATE;

    // Serial.print("Update Time = ");
    // Serial.print((float)(micros() - startUpdate) / 1000.0);
    // Serial.println(" mSec");

  } else {
    state++;
  }
}
