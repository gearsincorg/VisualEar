/*


*/

#ifndef bufferManager_h /* Prevent loading library twice */
#define bufferManager_h

//  =================  Multi-Task Shared Data =================
class BufferManager
{
public:
  BufferManager();
  BufferManager(float *vReal, float *weight, short *vShort, unsigned short samples, unsigned short packingNum, float cutoffFreq);
	void	addSample(short value);
	void	transfer(float inputScale);
  float filt(float xn);


private:
	float   *_vReal;
  float   *_weight;
  short 	*_vShort;
	unsigned short _samples;
	unsigned short _packingNum;
	unsigned short _nextSample;
	unsigned short _packCount;
  int   _DCBias;
  int   _packedSum;
  int   _packedValue;
  long  _sampleSum;
  int  _doDebug;

  //  Filter parameters
  float _a[2];
  float _b[3];
  float _omega0;
  float _dt;
  float _tn1 = 0;
  float _x[3]; // Raw values
  float _y[3]; // Filtered values

  float _lastFilt;
  
  
};

#endif
