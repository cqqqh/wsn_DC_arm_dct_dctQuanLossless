#ifndef BITARRAYC_H
#define BITARRAYC_H

#include <stdio.h>
#include "FatFsSimulation.h"
#include "dctQuanLossless.h"
#define BIT_ARRAY_ELEMENTS            (768)                        // the number of elements in the bit array (4 MB)
struct RANGE_CODER_STRUCT_COMPRESS
{
	unsigned int low;        // low end of interval
	unsigned int range;        // length of interval
	unsigned int help;        // bytes_to_follow resp. intermediate value
	unsigned char buffer;    // buffer for input / output
};

struct BIT_ARRAY_STATE
{
	unsigned int   nKSum;
};

// 编码一组数据
int BitArrayC(int* oriData, int dataLength,
			  int bIsSigned, const char* pOutputFilename, FIL * pFileInstance);


// output (saving)
int OutputBitArray(int bFinalize);
//static inline unsigned int GetCurrentBitIndex() { return m_nCurrentBitIndex; }
// encoding
int EncodeBits(unsigned int nValue, int nBits);
int EncodeUnsignedLong(unsigned int n);
int EncodeValue(int nEncode);
// other functions
void AdvanceToByteBoundary(void);
void FlushBitArray(void);
void FlushState(void);
void Finalize(void);

int BitArrayCEncodeValueStart(const char* pOutputFilename, FIL* pFileInstance, int bIsSigned);
int BitArrayCEncodeValueEnd(void);


#endif
