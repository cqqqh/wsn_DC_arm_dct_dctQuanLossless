#ifndef DCTQUANLOSSLESS_H
#define DCTQUANLOSSLESS_H

//#define WSN_NODE_COMPRESSION
#define DEBUG_OUTPUT
#define LOSSLESS_DECOMPRESSION

#ifndef WSN_NODE_COMPRESSION
	#include "integer.h"
#else
	#include "hal.h"
#endif

#ifdef LOSSLESS_DECOMPRESSION
	#include "UnBitArrayC.h"
#endif

#ifndef DEBUG_OUTPUT_FUN
	#ifdef DEBUG_OUTPUT
		#define DEBUG_OUTPUT_FUN(FUNCTION) { FUNCTION }
	#else
		#define DEBUG_OUTPUT_FUN(FUNCTION) {}
	#endif
#endif
			

#define DEFAULTBLOCKBITS 	(3)
//#define DCTBLOCKCOUNT 		(50u)  //对之前数据为50*2048   20141111
extern unsigned int DCTBLOCKCOUNT;      
#define DEFAULT_BITS_THRESHOLD		(6u)			//位数阈值，低于这个值的量化系数不编码直接保存
#define DCTBLOCKSIZE 		(2048u)
#define DQLC_DATABYTES		(sizeof(int16_t))
#define DQLC_BLOCKBYTES		(DCTBLOCKSIZE * sizeof(int16_t))
#define DQLC_DCTBLOCKBYTES	(DCTBLOCKSIZE * sizeof(float32_t))
#define DCTINIT_Nby2 		(DCTBLOCKSIZE / 2)
#define DCTINIT_NORM 		(0.03125)


void DQLC_ReadDCTData(uint8_t dctBlockIdx, float32_t * pfloatDCTData);
void DQLC_SaveDCTData(uint8_t dctBlockIdx, float32_t * pfloatDCTData);
void DQLC_ReadOriData2DCT(const char * originalFileName, uint32_t offset, float32_t * pfloatData, int16_t * p_int16TmpBuffer);
void DQLC_ReadOriData(const char * originalFileName, uint32_t offset, int16_t * p_int16Data);

int8_t DQLC_Init(void);
int DQLC_dctQuan(const char * originalFileName, const char * outputBitCFileName, float32_t meanBits, uint8_t isLossless);
void DQLC_QuanMAXBits(uint16_t* p_intMax, uint8_t * p_MaxBitsBuffer);
uint16_t DQLC_MaxValue(float32_t * pPowerBuffer);
void DQLC_QuanBitsAlloc(float32_t * pPowerBuffer, uint8_t * pBitsBuffer, uint8_t * p_MaxBitsBuffer, uint16_t bitsAmount);
void DQLC_Cal2NBits(uint32_t * p_2NBuffer, uint8_t * p_Bits);
void DQLC_QuanBitsAllocFast(float32_t * pPowerBuffer, int16_t * pIdxBuffer, uint8_t * pBitsBuffer, uint8_t * pMaxBitsBuffer, uint16_t bitsAmount);
//量化函数
void DQLC_QuanDCTCoeff(float32_t * p_floatDCTInline, uint16_t * p_uintMax, uint32_t * p_uint2N,
	uint8_t * p_uint8Bits, uint8_t * p_uint8MaxBits, uint8_t isLossless);

void DQLC_EncoderInit(void);
void DQLC_EncodingValue(int nEncode);
void DQLC_EncodingBits(int value, unsigned int nbits);

#ifdef LOSSLESS_DECOMPRESSION
void DQLC_DeQuanDCTCoeff(float32_t * p_floatDCTInline, uint16_t * p_uintMax, uint32_t * p_uint2N,
	uint8_t * p_uint8Bits, uint8_t * p_uint8MaxBits);
void DQLC_DeCompress(const char* inputFile, const char * outputFile, uint8_t isLossless);
#endif

#ifdef DEBUG_OUTPUT
void DQLC_PrintFloatArray(float32_t *toPrint);
void DQLC_PrintUint8Array(uint8_t *toPrint);
void DQLC_PrintUint16Array(uint16_t *toPrint);
void DQLC_PrintInt16Array(int16_t *toPrint);
void DQLC_PrintUintArray(uint32_t *toPrint);
void DQLC_PrintString(const char * str);
void DQLC_PrintIntData(int d);
#endif

#endif

