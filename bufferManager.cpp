/*


*/

#include <Arduino.h>
#include "bufferManager.h"

// Constructor
BufferManager::BufferManager() {};

BufferManager::BufferManager(float *vReal, float *weight, short *vShort, unsigned short samples, unsigned short packingNum) {
  this->_vReal      = vReal;
  this->_weight     = weight;
	this->_vShort 		= vShort;
	this->_samples 		= samples;
	this->_packingNum 	= packingNum;
	this->_nextSample 	= 0;
	this->_sampleSum   	= 0;
	this->_DCBias     	= 0;
  this->_packCount    = 0;
  this->_packedValue  = 0;
};

// Add the new sample to the circular buffer
void	BufferManager::addSample(short value) {
  // Accumulate the new value
  _packedValue += value;
  _packCount++;

  //  look to see if we have a new packed value
  if (_packCount == _packingNum) {
    value = _packedValue / _packingNum;

    //Serial.print("P");

    // put new averaged value into the head of the list and update sum.
    _sampleSum -= _vShort[_nextSample];
    _vShort[_nextSample] = value;
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
    //Serial.print(".");
  }
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
