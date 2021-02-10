/*


*/

#ifndef bufferManager_h /* Prevent loading library twice */
#define bufferManager_h

//  =================  Multi-Task Shared Data =================
class BufferManager
{
public:
  BufferManager();
  BufferManager(float *vReal, float *weight, short *vShort, unsigned short samples, unsigned short packingNum);
	void	addSample(short value);
	void	transfer(float inputScale);

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
};

#endif
