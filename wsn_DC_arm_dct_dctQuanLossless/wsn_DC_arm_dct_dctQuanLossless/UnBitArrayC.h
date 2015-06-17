#ifndef UNBITARRAYC_H
#define UNBITARRAYC_H

struct RANGE_CODER_STRUCT_DECOMPRESS
{
    unsigned int low;       // low end of interval
    unsigned int range;     // length of interval
    unsigned int buffer;    // buffer for input/output
};

struct UNBIT_ARRAY_STATE
{
    unsigned int k;
    unsigned int nKSum;
};

// 一次性解码一组数据
int UnBitArrayC(const char *pInputFilename, const char *pOutputFilename, int bIsSigned,
	int **decData, unsigned int *dataLength);
//int UnBitArrayC(int **oriData, unsigned int *dataLength,
//		int bIsSigned, const char *pInputFilename, const char *pOutputFilename);
// 分阶段解码
int UnBitArrayCInit(const char* pInputFilename);
int UnBitArrayCClose();
int UnBitArrayCDecodeValue(int bIsSigned);
int UnBitArrayCDecodeArray(int *decData, unsigned int dataLength, int bIsSigned);
unsigned int DecodeValueXBits(unsigned int nBits);
int RangeDecodeFast(int nShift);
int RangeDecodeFastWithUpdate(int nShift);
#endif
