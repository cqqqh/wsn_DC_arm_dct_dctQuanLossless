#include "UnBitArrayC.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*****************************************************************************************
Macros
*****************************************************************************************/
#ifndef RETURN_ON_ERROR
#define RETURN_ON_ERROR(FUNCTION) { int nRetVal = FUNCTION; if (nRetVal != 0) { return nRetVal; } }
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
/************************************************************************************
Declares
************************************************************************************/
#define BIT_ARRAY_ELEMENTS            (4096)                        // the number of elements in the bit array (4 MB)
#define BIT_ARRAY_BYTES               (BIT_ARRAY_ELEMENTS * 4)    // the number of bytes in the bit array
#define BIT_ARRAY_BITS                (BIT_ARRAY_BYTES    * 8)    // the number of bits in the bit array

#define REFILL_BIT_THRESHOLD          (BIT_ARRAY_BITS - 512)

/************************************************************************************
Lookup tables
************************************************************************************/
const unsigned int POWERS_OF_TWO_MINUS_ONE[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295};

const unsigned int K_SUM_MIN_BOUNDARY[32] = {0,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0};

const unsigned int RANGE_TOTAL_2[65] = {0,19578,36160,48417,56323,60899,63265,64435,64971,65232,65351,65416,65447,65466,65476,65482,65485,65488,65490,65491,65492,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,65536};
const unsigned int RANGE_WIDTH_2[64] = {19578,16582,12257,7906,4576,2366,1170,536,261,119,65,31,19,10,6,3,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

#define RANGE_OVERFLOW_SHIFT 16

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int ) 1 << (CODE_BITS - 1))

#define EXTRA_BITS ((CODE_BITS - 2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

#define MODEL_ELEMENTS 64

// data members
unsigned int *					m_pBitArray;
FILE    *						m_pIO;
int                             m_FurthestReadByte;
unsigned int					m_nCurrentBitIndex;
int								m_bSigned;
struct UNBIT_ARRAY_STATE               m_UnBitArrayState;
struct RANGE_CODER_STRUCT_DECOMPRESS   m_RangeCoderInfo;
unsigned int m_nGoodBytes;
char									m_UnBitArrayCIsInit = 0;

static int FillBitArray()
{
	// get the bit array index
	unsigned int nBitArrayIndex = m_nCurrentBitIndex >> 5;
	int nBytesToRead;
	unsigned int nBytesRead;
	// move the remaining data to the front
	memmove((void *) (m_pBitArray), (const void *) (m_pBitArray + nBitArrayIndex), BIT_ARRAY_BYTES - (nBitArrayIndex * 4));

	// get the number of bytes to read
	nBytesToRead = nBitArrayIndex * 4;
	if (m_FurthestReadByte > 0)
	{
		int nFurthestReadBytes = m_FurthestReadByte - ftell(m_pIO);
		if (nBytesToRead > nFurthestReadBytes)
			nBytesToRead = nFurthestReadBytes;
	}

	// read the new data
	nBytesRead = 0;
	nBytesRead = fread((unsigned char *) (m_pBitArray + BIT_ARRAY_ELEMENTS - nBitArrayIndex), 1, nBytesToRead, m_pIO);

	// zero anything at the tail we didn't fill
	m_nGoodBytes = ((BIT_ARRAY_ELEMENTS - nBitArrayIndex) * 4) + nBytesRead;
	if (m_nGoodBytes < BIT_ARRAY_BYTES)
		memset(&((unsigned char *) m_pBitArray)[m_nGoodBytes], 0, BIT_ARRAY_BYTES - m_nGoodBytes);

	// adjust the m_Bit pointer
	m_nCurrentBitIndex = m_nCurrentBitIndex & 31;

	// return
	return (nBytesRead != -1) ? 0 : -1;
}

static int FillAndResetBitArray(int nFileLocation, int nNewBitIndex) 
{
	int nRetVal;
	// seek if necessary
	if (nFileLocation != -1)
	{
		if (fseek(m_pIO,nFileLocation, SEEK_SET) != 0)
			return -1;
	}

	// fill
	m_nCurrentBitIndex = BIT_ARRAY_BITS; // position at the end of the buffer
	nRetVal = FillBitArray();

	// set bit index
	m_nCurrentBitIndex = nNewBitIndex;

	return nRetVal;
}

static void FlushState(struct UNBIT_ARRAY_STATE * BitArrayState)
{
	(*BitArrayState).k = 10;
	(*BitArrayState).nKSum = (1 << (*BitArrayState).k) * 16;
}

static void AdvanceToByteBoundary() 
{
	int nMod = m_nCurrentBitIndex % 8;
	if (nMod != 0) { m_nCurrentBitIndex += 8 - nMod; }
}

unsigned int DecodeValueXBits(unsigned int nBits) 
{
	unsigned int nLeftBits;
	unsigned int nBitArrayIndex;
	int nRightBits;
	unsigned int nLeftValue;
	unsigned int nRightValue;
	// get more data if necessary
	if ((m_nCurrentBitIndex + nBits) >= BIT_ARRAY_BYTES)
		FillBitArray();

	// variable declares
	nLeftBits = 32 - (m_nCurrentBitIndex & 31);
	nBitArrayIndex = m_nCurrentBitIndex >> 5;
	m_nCurrentBitIndex += nBits;

	// if their isn't an overflow to the right value, get the value and exit
	if (nLeftBits >= nBits)
		return (m_pBitArray[nBitArrayIndex] & (POWERS_OF_TWO_MINUS_ONE[nLeftBits])) >> (nLeftBits - nBits);

	// must get the "split" value from left and right
	nRightBits = nBits - nLeftBits;

	nLeftValue = ((m_pBitArray[nBitArrayIndex] & POWERS_OF_TWO_MINUS_ONE[nLeftBits]) << nRightBits);
	nRightValue = (m_pBitArray[nBitArrayIndex + 1] >> (32 - nRightBits));
	return (nLeftValue | nRightValue);
}

static void FlushBitArray()
{
	AdvanceToByteBoundary();
	DecodeValueXBits(8); // ignore the first byte... (slows compression too much to not output this dummy byte)
	m_RangeCoderInfo.buffer = DecodeValueXBits(8);
	m_RangeCoderInfo.low = m_RangeCoderInfo.buffer >> (8 - EXTRA_BITS);
	m_RangeCoderInfo.range = (unsigned int) 1 << EXTRA_BITS;
}

//inline
static unsigned int DecodeByte()
{
	// read byte
	unsigned int nByte = ((m_pBitArray[m_nCurrentBitIndex >> 5] >> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
	m_nCurrentBitIndex += 8;
	return nByte;
}

//inline
int RangeDecodeFast(int nShift)
{
	while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
	{   
		m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | DecodeByte();
		m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
		m_RangeCoderInfo.range <<= 8;
	}

	// decode
	m_RangeCoderInfo.range = m_RangeCoderInfo.range >> nShift;

	return m_RangeCoderInfo.low / m_RangeCoderInfo.range;
}

//inline
int RangeDecodeFastWithUpdate(int nShift)
{
	int nRetVal;

	//add by hqq 2015.6.8, 当连续使用这个函数时，并不会自动载入新数据，m_nCurrentBitIndex会超过BIT_ARRAY_BITS
	if (m_nCurrentBitIndex > REFILL_BIT_THRESHOLD)
		FillBitArray();

	while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
	{   
		m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | DecodeByte();
		m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
		m_RangeCoderInfo.range <<= 8;
	}

	// decode
	m_RangeCoderInfo.range = m_RangeCoderInfo.range >> nShift;
	nRetVal = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
	m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nRetVal;
	return nRetVal;
}

static int DecodeValueRange(struct UNBIT_ARRAY_STATE * BitArrayState)
{
	int nValue;
	int nPivotValue;
	int nOverflow;
	int nRangeTotal;
	int nBase;
	int nShift;
	int nPivotValueBits;
	int nSplitFactor;
	int nPivotValueA;
	int nPivotValueB;
	int nBaseA;
	int nBaseB;
	int nBaseLower;
	// make sure there is room for the data
	// this is a little slower than ensuring a huge block to start with, but it's safer
	if (m_nCurrentBitIndex > REFILL_BIT_THRESHOLD)
		FillBitArray();

	nValue = 0;


	// figure the pivot value
	nPivotValue = max((*BitArrayState).nKSum / 32, 1);

	// get the overflow
	nOverflow = 0;
	{
		// decode
		nRangeTotal = RangeDecodeFast(RANGE_OVERFLOW_SHIFT);

		// lookup the symbol (must be a faster way than this)
		while (nRangeTotal >= (int)(RANGE_TOTAL_2[nOverflow + 1])) { nOverflow++; }

		// update
		m_RangeCoderInfo.low -= m_RangeCoderInfo.range * RANGE_TOTAL_2[nOverflow];
		m_RangeCoderInfo.range = m_RangeCoderInfo.range * RANGE_WIDTH_2[nOverflow];

		// get the working k
		if (nOverflow == (MODEL_ELEMENTS - 1))
		{
			nOverflow = RangeDecodeFastWithUpdate(16);
			nOverflow <<= 16;
			nOverflow |= RangeDecodeFastWithUpdate(16);
		}
	}

	// get the value
	nBase = 0;
	{
		nShift = 0;
		if (nPivotValue >= (1 << 16))
		{
			nPivotValueBits = 0;
			while ((nPivotValue >> nPivotValueBits) > 0) { nPivotValueBits++; }
			nSplitFactor = 1 << (nPivotValueBits - 16);

			nPivotValueA = (nPivotValue / nSplitFactor) + 1;
			nPivotValueB = nSplitFactor;

			while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
			{   
				m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | DecodeByte();
				m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
				m_RangeCoderInfo.range <<= 8;
			}
			m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValueA;
			nBaseA = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
			m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseA;

			while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
			{   
				m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | DecodeByte();
				m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
				m_RangeCoderInfo.range <<= 8;
			}
			m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValueB;
			nBaseB = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
			m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseB;

			nBase = nBaseA * nSplitFactor + nBaseB;
		}
		else
		{
			while (m_RangeCoderInfo.range <= BOTTOM_VALUE)
			{   
				m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8) | DecodeByte();
				m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) | ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
				m_RangeCoderInfo.range <<= 8;
			}

			// decode
			m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValue;
			nBaseLower = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
			m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseLower;

			nBase = nBaseLower;
		}


		// build the value
		nValue = nBase + (nOverflow * nPivotValue);
	}

	// update nKSum
	(*BitArrayState).nKSum += ((nValue + 1) / 2) - (((*BitArrayState).nKSum + 16) >> 5);

	// update k
	if ((*BitArrayState).nKSum < K_SUM_MIN_BOUNDARY[(*BitArrayState).k]) 
		(*BitArrayState).k--;
	else if ((*BitArrayState).nKSum >= K_SUM_MIN_BOUNDARY[(*BitArrayState).k + 1]) 
		(*BitArrayState).k++;

	// output the value (converted to signed)
	if(m_bSigned)
	{
		return (nValue & 1) ? (nValue >> 1) + 1 : -(nValue >> 1);
	}else
	{
		return nValue;
	}
}

int UnBitArrayCInit(const char* pInputFilename)
{
	if(m_UnBitArrayCIsInit)
	{
		printf("已经初始化UnBitArrayC\n");
		return -1;
	}

	// 分配空间
	m_pBitArray = (unsigned int *)malloc((BIT_ARRAY_ELEMENTS + 64) * sizeof(unsigned int));
	memset(m_pBitArray, 0, (BIT_ARRAY_ELEMENTS + 64) * sizeof(unsigned int));

	// open file
	if((m_pIO = fopen(pInputFilename, "rb")) == NULL)
	{
		if(m_pBitArray != NULL)
		{
			free(m_pBitArray);
			m_pBitArray = NULL;
		}
		return -1;
	}

	fseek(m_pIO, 0, SEEK_END);
	m_FurthestReadByte = ftell(m_pIO);
	fseek(m_pIO, 0, SEEK_SET);

	// 初始化
	m_nCurrentBitIndex = 0;
	m_nGoodBytes = 0;

	FillAndResetBitArray(-1, 0);
	FlushState(&m_UnBitArrayState);
	FlushBitArray();

	m_UnBitArrayCIsInit = 1;

	return 0;
}

int UnBitArrayCClose()
{
	if(!m_UnBitArrayCIsInit)
	{
		printf("没有初始化UnBitArrayC\n");
		return -1;
	}

	//结尾工作
	free(m_pBitArray);
	m_pBitArray = NULL;
	fclose(m_pIO);
	m_UnBitArrayCIsInit = 0;
	return 0;
}

int UnBitArrayCDecodeValue(int bIsSigned)
{
	if(!m_UnBitArrayCIsInit)
	{
		printf("没有初始化UnBitArrayC\n");
		return -1;
	}

	m_bSigned = bIsSigned;
	return DecodeValueRange(&m_UnBitArrayState);
}

int UnBitArrayCDecodeArray(int *decData, unsigned int dataLength, int bIsSigned)
{
	unsigned int i;
	int *pDec = decData;
	if(!m_UnBitArrayCIsInit)
	{
		printf("没有初始化UnBitArrayC\n");
		return -1;
	}

	m_bSigned = bIsSigned;
	for(i = 0; i < dataLength; ++i)
	{
		pDec[i] = DecodeValueRange(&m_UnBitArrayState);
	}

	return 0;
}

int UnBitArrayC(const char *pInputFilename, const char *pOutputFilename, int bIsSigned,
				int **decData, unsigned int *dataLength)
{
	int nFileLength = 0;
	int i;
	int *oriData = NULL;
	int dataLen = 0;
	FILE *pIO_output;
	m_bSigned = bIsSigned;

	// allocate memory for the bit array
	// 分配空间
	m_pBitArray = (unsigned int *)malloc((BIT_ARRAY_ELEMENTS + 64) * sizeof(unsigned int));
	memset(m_pBitArray, 0, (BIT_ARRAY_ELEMENTS + 64) * sizeof(unsigned int));

	// open file
	if((m_pIO = fopen(pInputFilename, "rb")) == NULL)
	{
		return -1;
	}

	// open output file
	if((pIO_output = fopen(pOutputFilename, "w")) == NULL)
	{
		return -1;
	}

	fseek(m_pIO, 0, SEEK_END);
	m_FurthestReadByte = ftell(m_pIO);
	fseek(m_pIO, 0, SEEK_SET);

	// 初始化
	m_nCurrentBitIndex = 0;
	m_nGoodBytes = 0;

	FillAndResetBitArray(-1, 0);
	FlushState(&m_UnBitArrayState);
	FlushBitArray();

	dataLen = DecodeValueRange(&m_UnBitArrayState);
	
	oriData = (int *)malloc(dataLen * sizeof(int));
	memset(oriData, 0, dataLen * sizeof(int));
	fprintf(pIO_output, "%d\n", dataLen);

	for(i = 0; i < dataLen; ++i)
	{
		oriData[i] = DecodeValueRange(&m_UnBitArrayState);
		fprintf(pIO_output, "%d\n", oriData[i]);	
	}

	*decData = oriData;
	*dataLength = (unsigned int)dataLen;

	//结尾工作
	free(m_pBitArray);
	m_pBitArray = NULL;
	fclose(m_pIO);
	fclose(pIO_output);

	return 0;
}





