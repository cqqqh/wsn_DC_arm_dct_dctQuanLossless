#include "BitArrayC.h"

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
// #define BIT_ARRAY_ELEMENTS            (4096)                        // the number of elements in the bit array (4 MB)
//#define BIT_ARRAY_ELEMENTS            (768)                        // the number of elements in the bit array (4 MB)
#define BIT_ARRAY_BYTES                (BIT_ARRAY_ELEMENTS * 4)    // the number of bytes in the bit array
#define BIT_ARRAY_BITS                (BIT_ARRAY_BYTES    * 8)    // the number of bits in the bit array

#define MAX_ELEMENT_BITS            128
#define REFILL_BIT_THRESHOLD        (BIT_ARRAY_BITS - MAX_ELEMENT_BITS)

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int) 1 << (CODE_BITS - 1))
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS - 2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

/************************************************************************************
Lookup tables
************************************************************************************/
//const unsigned int K_SUM_MIN_BOUNDARY[32] = {0,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0};

#define MODEL_ELEMENTS                    64
#define RANGE_OVERFLOW_TOTAL_WIDTH        65536
#define RANGE_OVERFLOW_SHIFT            16

const unsigned int RANGE_TOTAL[64] = {0,19578,36160,48417,56323,60899,63265,64435,64971,65232,65351,65416,65447,65466,65476,65482,65485,65488,65490,65491,65492,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,};
const unsigned int RANGE_WIDTH[64] = {19578,16582,12257,7906,4576,2366,1170,536,261,119,65,31,19,10,6,3,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

// data members
extern unsigned int bitBuffer[BIT_ARRAY_ELEMENTS];
unsigned int *					m_pBitArray;
//文件操作变量
FIL    *						m_pIO;
extern FRESULT 						res;

unsigned int					m_nCurrentBitIndex;
struct RANGE_CODER_STRUCT_COMPRESS		m_RangeCoderInfo;
int								m_bSigned;
struct BIT_ARRAY_STATE					m_BitArrayState;


int BitArrayCEncodeValueStart(const char* pOutputFilename, FIL* pFileInstance, int bIsSigned)
{
	m_bSigned = bIsSigned;
	m_pIO = pFileInstance;
	// 分配空间
//	m_pBitArray = (unsigned int *)malloc(1 * sizeof(unsigned int));
//	m_pBitArray = (unsigned int *)malloc(BIT_ARRAY_ELEMENTS * sizeof(unsigned int));
//	if(m_pBitArray == NULL)
//	{
//		return -1;
//	}
	m_pBitArray = bitBuffer;
	memset(m_pBitArray, 0, BIT_ARRAY_BYTES);
	// open file
	res = f_open(m_pIO, pOutputFilename, FA_WRITE | FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		return -1;
	}
	// 初始化
	m_nCurrentBitIndex = 0;
	FlushBitArray();
	FlushState();	
	
	return 0;
}

int BitArrayCEncodeValueEnd()
{
	int len;
	Finalize();
	OutputBitArray(1);
	
//	free(m_pBitArray);
//	m_pBitArray = NULL;
	
	//读出大小
	len = ftell(m_pIO->pfile);

	f_close(m_pIO);
	return len;
}

int BitArrayC(int* oriData, int dataLength,
			  int bIsSigned, const char* pOutputFilename, FIL * pFileInstance)
{
	int nFileLength = 0;
	int i;
	m_bSigned = bIsSigned;
	m_pIO = pFileInstance;
	// allocate memory for the bit array
	// 分配空间
	m_pBitArray = (unsigned int *)malloc(BIT_ARRAY_ELEMENTS * sizeof(unsigned int));
    memset(m_pBitArray, 0, BIT_ARRAY_BYTES);
	// open file
	res = f_open(m_pIO, pOutputFilename, FA_WRITE | FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		return -1;
	}

	// 初始化
	m_nCurrentBitIndex = 0;
	FlushBitArray();
	FlushState();
	EncodeValue(dataLength);
	for(i = 0; i < dataLength; ++i)
	{
		EncodeValue(oriData[i]);
	}

	//结尾工作
	Finalize();
	OutputBitArray(1);

	nFileLength = m_pIO -> fsize;

	free(m_pBitArray);
	m_pBitArray = NULL;
	
	res = f_close(m_pIO);
	
	return nFileLength;
}

int OutputBitArray(int bFinalize)
{
    // write the entire file to disk
    unsigned int nBytesWritten = 0;
    unsigned int nBytesToWrite = 0;

    if (bFinalize)
    {
        nBytesToWrite = ((m_nCurrentBitIndex >> 5) * 4) + 4;

        //m_MD5.AddData(m_pBitArray, nBytesToWrite);

        //RETURN_ON_ERROR(m_pIO->Write(m_pBitArray, nBytesToWrite, &nBytesWritten))
		//fwrite(m_pBitArray, 1, nBytesToWrite, m_pIO);
		res = f_write(m_pIO, m_pBitArray, nBytesToWrite, &nBytesWritten);

        // reset the bit pointer
        m_nCurrentBitIndex = 0;    
    }
    else
    {
        nBytesToWrite = (m_nCurrentBitIndex >> 5) * 4;

        //m_MD5.AddData(m_pBitArray, nBytesToWrite);

        //RETURN_ON_ERROR(m_pIO->Write(m_pBitArray, nBytesToWrite, &nBytesWritten))
        //fwrite(m_pBitArray, 1, nBytesToWrite, m_pIO);
		    res = f_write(m_pIO, m_pBitArray, nBytesToWrite, &nBytesWritten);
        // move the last value to the front of the bit array
        m_pBitArray[0] = m_pBitArray[m_nCurrentBitIndex >> 5];
        m_nCurrentBitIndex = (m_nCurrentBitIndex & 31);
        
        // zero the rest of the memory (may not need the +1 because of frame byte alignment)
        memset(&m_pBitArray[1], 0, min(nBytesToWrite + 1, BIT_ARRAY_BYTES - 1));
    }
    
    // return a success
    return 0;
}

/************************************************************************************
Range coding macros -- ugly, but outperform inline's (every cycle counts here)
************************************************************************************/
#define PUTC(VALUE) m_pBitArray[m_nCurrentBitIndex >> 5] |= ((VALUE) & 0xFF) << (24 - (m_nCurrentBitIndex & 31)); m_nCurrentBitIndex += 8;
#define PUTC_NOCAP(VALUE) m_pBitArray[m_nCurrentBitIndex >> 5] |= (VALUE) << (24 - (m_nCurrentBitIndex & 31)); m_nCurrentBitIndex += 8;

#define NORMALIZE_RANGE_CODER                                                                    \
    while (m_RangeCoderInfo.range <= BOTTOM_VALUE)                                                \
    {                                                                                            \
        if (m_RangeCoderInfo.low < (0xFF << SHIFT_BITS))                                        \
        {                                                                                        \
            PUTC(m_RangeCoderInfo.buffer);                                                        \
            for ( ; m_RangeCoderInfo.help; m_RangeCoderInfo.help--) { PUTC_NOCAP(0xFF); }        \
            m_RangeCoderInfo.buffer = (m_RangeCoderInfo.low >> SHIFT_BITS);                        \
        }                                                                                        \
        else if (m_RangeCoderInfo.low & TOP_VALUE)                                                \
        {                                                                                        \
            PUTC(m_RangeCoderInfo.buffer + 1);                                                    \
            m_nCurrentBitIndex += (m_RangeCoderInfo.help * 8);                                    \
            m_RangeCoderInfo.help = 0;                                                            \
            m_RangeCoderInfo.buffer = (m_RangeCoderInfo.low >> SHIFT_BITS);                        \
        }                                                                                        \
        else                                                                                    \
        {                                                                                        \
            m_RangeCoderInfo.help++;                                                            \
        }                                                                                        \
                                                                                                \
        m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) & (TOP_VALUE - 1);                    \
        m_RangeCoderInfo.range <<= 8;                                                            \
    }            

	
//    const int nTemp = m_RangeCoderInfo.range >> (SHIFT);   去掉const
#define ENCODE_FAST(RANGE_WIDTH, RANGE_TOTAL, SHIFT)                                            \
    NORMALIZE_RANGE_CODER                                                                        \
    nTemp = m_RangeCoderInfo.range >> (SHIFT);                                        \
    m_RangeCoderInfo.range = nTemp * (RANGE_WIDTH);                                                \
    m_RangeCoderInfo.low += nTemp * (RANGE_TOTAL);    

#define ENCODE_DIRECT(VALUE, SHIFT)                                                                \
    NORMALIZE_RANGE_CODER                                                                        \
    m_RangeCoderInfo.range = m_RangeCoderInfo.range >> (SHIFT);                                    \
    m_RangeCoderInfo.low += m_RangeCoderInfo.range * (VALUE);


/************************************************************************************
Directly encode bits to the bitstream
************************************************************************************/
int EncodeBits(unsigned int nValue, int nBits)
{
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (m_nCurrentBitIndex > REFILL_BIT_THRESHOLD)
    {
        RETURN_ON_ERROR(OutputBitArray(0))
    }
    
    ENCODE_DIRECT(nValue, nBits);
    return 0;
}

/************************************************************************************
Encodes an unsigned int to the bit array (no rice coding)
************************************************************************************/
int EncodeUnsignedLong(unsigned int n) 
{
	// encode the value
    unsigned int nBitArrayIndex = m_nCurrentBitIndex >> 5;
    int nBitIndex = m_nCurrentBitIndex & 31;
	
    // make sure there are at least 8 bytes in the buffer
    if (m_nCurrentBitIndex > (BIT_ARRAY_BYTES - 8))
    {
        RETURN_ON_ERROR(OutputBitArray(0))
    }
    
    if (nBitIndex == 0)
    {
        m_pBitArray[nBitArrayIndex] = n;
    }
    else 
    {
        m_pBitArray[nBitArrayIndex] |= n >> nBitIndex;
        m_pBitArray[nBitArrayIndex + 1] = n << (32 - nBitIndex);
    }    

    m_nCurrentBitIndex += 32;

    return 0;
}

/************************************************************************************
Encode a value
************************************************************************************/
int EncodeValue(int nEncode)
{
	int nPivotValue;
	int nOverflow;
	int nBase;
	int nPivotValueBits;
	int nSplitFactor;
	int nPivotValueA;
    int nPivotValueB;
	int nBaseA;
	int nBaseB;
	//const int nTemp;  这里去掉了const，因为坑爹的编译不通过，必须在前面定义
	int nTemp;
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (m_nCurrentBitIndex > REFILL_BIT_THRESHOLD)
    {
        RETURN_ON_ERROR(OutputBitArray(0))
    }
    
	if (m_bSigned)
	{
		// convert to unsigned
		nEncode = (nEncode > 0) ? nEncode * 2 - 1 : -nEncode * 2;
	}

    // figure the pivot value
    nPivotValue = max(m_BitArrayState.nKSum / 32, 1);
    nOverflow = nEncode / nPivotValue;
    nBase = nEncode - (nOverflow * nPivotValue);

    // update nKSum
    m_BitArrayState.nKSum += ((nEncode + 1) / 2) - ((m_BitArrayState.nKSum + 16) >> 5);

    // store the overflow
    if (nOverflow < (MODEL_ELEMENTS - 1))
    {
        ENCODE_FAST(RANGE_WIDTH[nOverflow], RANGE_TOTAL[nOverflow], RANGE_OVERFLOW_SHIFT);

        #ifdef BUILD_RANGE_TABLE
            g_aryOverflows[nOverflow]++;
            g_nTotalOverflow++;
        #endif
    }
    else
    {
        // store the "special" overflow (tells that perfect k is encoded next)
        ENCODE_FAST(RANGE_WIDTH[MODEL_ELEMENTS - 1], RANGE_TOTAL[MODEL_ELEMENTS - 1], RANGE_OVERFLOW_SHIFT);

        #ifdef BUILD_RANGE_TABLE
            g_aryOverflows[MODEL_ELEMENTS - 1]++;
            g_nTotalOverflow++;
        #endif

        // code the overflow using straight bits
        ENCODE_DIRECT((nOverflow >> 16) & 0xFFFF, 16);
        ENCODE_DIRECT(nOverflow & 0xFFFF, 16);
    }

    // code the base
    {
        if (nPivotValue >= (1 << 16))
        {
            nPivotValueBits = 0;
            while ((nPivotValue >> nPivotValueBits) > 0) { nPivotValueBits++; }
            nSplitFactor = 1 << (nPivotValueBits - 16);

            // we know that base is smaller than pivot coming into this
            // however, after we divide both by an integer, they could be the same
            // we account by adding one to the pivot, but this hurts compression
            // by (1 / nSplitFactor) -- therefore we maximize the split factor
            // that gets one added to it

            // encode the pivot as two pieces
            nPivotValueA = (nPivotValue / nSplitFactor) + 1;
            nPivotValueB = nSplitFactor;

            nBaseA = nBase / nSplitFactor;
            nBaseB = nBase % nSplitFactor;

            {
                NORMALIZE_RANGE_CODER
                nTemp = m_RangeCoderInfo.range / nPivotValueA;
                m_RangeCoderInfo.range = nTemp;
                m_RangeCoderInfo.low += nTemp * nBaseA;
            }

            {
                NORMALIZE_RANGE_CODER
                nTemp = m_RangeCoderInfo.range / nPivotValueB;
                m_RangeCoderInfo.range = nTemp;
                m_RangeCoderInfo.low += nTemp * nBaseB;
            }
        }
        else
        {
            NORMALIZE_RANGE_CODER
            nTemp = m_RangeCoderInfo.range / nPivotValue;
            m_RangeCoderInfo.range = nTemp;
            m_RangeCoderInfo.low += nTemp * nBase;
        }
    }

    return 0;
}

/************************************************************************************
Advance to a byte boundary (for frame alignment)
************************************************************************************/
void AdvanceToByteBoundary() 
{
    while (m_nCurrentBitIndex % 8)
        m_nCurrentBitIndex++;
}

/************************************************************************************
Flush
************************************************************************************/
void FlushBitArray()
{
    // advance to a byte boundary (for alignment)
    AdvanceToByteBoundary();

    // the range coder
    m_RangeCoderInfo.low = 0;  // full code range
    m_RangeCoderInfo.range = TOP_VALUE;
    m_RangeCoderInfo.buffer = 0;
    m_RangeCoderInfo.help = 0;  // no bytes to follow
}

void FlushState() 
{
    // ksum
    m_BitArrayState.nKSum = (1 << 10) * 16;
}

/************************************************************************************
Finalize
************************************************************************************/
void Finalize()
{
	unsigned int nTemp;
    NORMALIZE_RANGE_CODER

    nTemp = (m_RangeCoderInfo.low >> SHIFT_BITS) + 1;

    if (nTemp > 0xFF) // we have a carry
    {
        PUTC(m_RangeCoderInfo.buffer + 1);
        for ( ; m_RangeCoderInfo.help; m_RangeCoderInfo.help--)
        {
            PUTC(0);
        }
    } 
    else  // no carry
    {
        PUTC(m_RangeCoderInfo.buffer);
        for ( ; m_RangeCoderInfo.help; m_RangeCoderInfo.help--)
        {
            PUTC(((unsigned char) 0xFF));
        }
    }

    // we must output these bytes so the decoder can properly work at the end of the stream
    PUTC(nTemp & 0xFF);
    PUTC(0);
    PUTC(0);
    PUTC(0);
}

