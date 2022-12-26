#include <Arduino.h>
#include "bufferManager.h"
#include "devconf.h"

#define SAMPLE_FREQ 44100.0
#define SAMPLE_PERIOD (1.0 / SAMPLE_FREQ);

// Constructor
BufferManager::BufferManager() {};

BufferManager::BufferManager(float *vReal, float *weight, short *vShort, unsigned short samples, unsigned short packingNum, float f0) {
  _vReal      = vReal;
  _weight     = weight;
	_vShort 		= vShort;
	_samples 		= samples;
	_packingNum 	= packingNum;
	_nextSample 	= 0;
	_sampleSum   	= 0;
	_DCBias     	= 0;
  _packCount    = 0;
  _packedValue  = 0;
  _lastFilt     = 0;

  _omega0 = 6.28318530718*f0;
  _dt = SAMPLE_PERIOD;
  _tn1 = -_dt;
  for(int k = 0; k < 3; k++){
    _x[k] = 0;
    _y[k] = 0;        
  }

  // Calculate coofs
  float alpha = _omega0*_dt;
  float alphaSq = alpha*alpha;
  float beta[] = {1, 1.41421356237, 1};
  float D = alphaSq*beta[0] + 2*alpha*beta[1] + 4*beta[2];
  _b[0] = alphaSq/D;
  _b[1] = 2*_b[0];
  _b[2] = _b[0];
  _a[0] = -(2*alphaSq*beta[0] - 8*beta[2])/D;
  _a[1] = -(beta[0]*alphaSq - 2*beta[1]*alpha + 4*beta[2])/D;      

};

// Add the new sample to the circular buffer
void	BufferManager::addSample(short value) {
  // Accumulate the new value
  float filteredValue;
    
  _packedValue += value;
  _packCount++;

  filteredValue = filt((float)value);

  if (SHOW_RAW) Serial.print(filteredValue);

  //  look to see if we have a new packed value
  if (_packCount == _packingNum) {
    //value = _packedValue / _packingNum;  
    _lastFilt = filteredValue;

    if (SHOW_FILT) Serial.print(filteredValue);

    // put new averaged value into the head of the list and update sum.
    _sampleSum -= _vShort[_nextSample];
    _vShort[_nextSample] = (short)filteredValue;
    _sampleSum += _vShort[_nextSample];

    _nextSample++ ;

    // wrap the pointer at the end of the buffer.
    if (_nextSample == _samples) {
      _nextSample = 0;
    }

    // reset the packing counter.  
    _packedValue = 0;
    _packCount   = 0;
  } else {
    if (SHOW_FILT) Serial.print(_lastFilt);
  }

  //---------------------------------------
  if (SHOW_FILT || SHOW_RAW) {
    if (_packingNum == 1)
      Serial.println(" ");
    else
      Serial.print(", ");
  }
  //---------------------------------------

}

// Transfer from the circular buffer.  Remove the DC Bias and apply weight
void	BufferManager::transfer(float inputScale){
  unsigned short  fromIndex = _nextSample;
  unsigned short  toIndex   = 0;
  float           audioVal;
  
  _DCBias =  _sampleSum / _samples;

  while (toIndex < _samples) {
    audioVal = ((float)(_vShort[fromIndex++] - _DCBias) * _weight[toIndex]) * inputScale;
    _vReal[toIndex++] = audioVal;

    if (fromIndex >= _samples) {
      fromIndex = 0;
    }
  }
}


float BufferManager::filt(float xn){
  // Provide me with the current raw value: x
  // I will give you the current filtered value: y
  _y[0] = 0;
  _x[0] = xn;
  // Compute the filtered values
  for(int k = 0; k < 2; k++){
    _y[0] += _a[k]*_y[k+1] + _b[k]*_x[k];
  }
  _y[0] += _b[2]*_x[2];

  // Save the historical values
  for(int k = 2; k > 0; k--){
    _y[k] = _y[k-1];
    _x[k] = _x[k-1];
  }

  // Return the filtered value    
  return _y[0];
}
