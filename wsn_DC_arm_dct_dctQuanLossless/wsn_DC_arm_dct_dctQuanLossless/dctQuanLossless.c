#include "dctQuanLossless.h"
#include "BitArrayC.h"
#include "integer.h"
#include "arm_math.h"
#include <math.h>
extern float roundf(float);

unsigned int DCTBLOCKCOUNT = 50;

uint16_t    m_uint16Tmp;
extern FILE *F4Printf;

float32_t * p_inlineBufferFloatDCTPower;	//����ͬһ���ڴ�
uint32_t *	p_inlineBufferUint2N;			//����ͬһ���ڴ�
//ע�⣺2N��ʹ�õ��ڴ���DCTPower��һ���ģ�������float32��uint32�ڴ�ռ�ö���4�ֽڣ������������Ը��ã�Ϊ�˽�Լ�ڴ�

uint16_t *	p_uintDCTMaxValueBuffer;//ע�⣺�����õ�unsigned short������ʾΪ65536�����dctϵ�����������Χ��������⣬��һ������²��ᳬ��

uint8_t *	p_uint8BitsBuffer;		//br
uint8_t *	p_uint8MaxBitsBuffer;

float32_t * p_floatState;			//p_floatState ע�⣬�������������BlockSize�ڴ�ſ���
int16_t *	p_inlineMaxPowerIndex;	//�������power�������ݵ�index�����ڿ�������λ�������㷨��
int16_t * 	p_int16Buffer;			//ԭʼ���ݺͲ�ֵ����
//ע�⣺p_int16Buffer,p_inlineMaxPowerIndex��ʹ�õ��ڴ���p_floatState��һ���ģ�Ϊ�˽�Լ�ڴ�

float32_t * p_floatDCTBuffer;
unsigned int bitBuffer[BIT_ARRAY_ELEMENTS];
//�ļ���������
char m_DCT_block_fileName[10];
FIL fBitC;		//����Range������ʹ��
extern FIL fsrc;
extern FRESULT res;
extern UINT br;
//����
//extern float32_t floatBuffer1[2048*2];  	//p_floatState ע�⣬��������������ڴ�ſ���
//extern float32_t floatBuffer2[2048];   		//p_floatDCTBuffer
//extern float32_t floatBuffer3[2048];		//p_floatDCTPowerBuffer
extern BYTE buffer[2048];
//DCT
	
arm_status						dctStatus;
arm_dct4_instance_f32 			dctInstance;
arm_rfft_instance_f32 			rfftInstance;
arm_cfft_radix4_instance_f32 	radix4Instance;
  
int8_t DQLC_Init()
{	
	p_uint8BitsBuffer = buffer;
	//p_uint8BitsBuffer = (uint8_t *)malloc(DCTBLOCKSIZE * sizeof(uint8_t));
	p_floatState = (float32_t *)malloc(DCTBLOCKSIZE * 2 * sizeof(float32_t));
	p_int16Buffer = (int16_t *)p_floatState;				//����ͬһ���ڴ�
	p_inlineMaxPowerIndex = (int16_t *)p_floatState;		//����ͬһ���ڴ�

	p_floatDCTBuffer = (float32_t *)malloc(DCTBLOCKSIZE * sizeof(float32_t));

	p_inlineBufferFloatDCTPower = (float32_t *)malloc(DCTBLOCKSIZE * sizeof(float32_t));
	p_inlineBufferUint2N = (uint32_t *)p_inlineBufferFloatDCTPower;		//����ͬһ���ڴ�

	p_uint8MaxBitsBuffer = (uint8_t *)malloc(DCTBLOCKSIZE * sizeof(uint8_t));
	p_uintDCTMaxValueBuffer = (uint16_t *)malloc(DCTBLOCKSIZE * sizeof(uint16_t));

	dctStatus = arm_dct4_init_f32(&dctInstance, &rfftInstance, &radix4Instance, DCTBLOCKSIZE, DCTINIT_Nby2, DCTINIT_NORM);
	return 0;
}

//��ȡDCTϵ��
void DQLC_ReadDCTData(uint8_t dctBlockIdx, float32_t * pfloatDCTData)
{
	sprintf(m_DCT_block_fileName, "DCT_%d", dctBlockIdx);
	
	res = f_open(&fsrc, m_DCT_block_fileName, FA_READ | FA_OPEN_EXISTING);
	if(res == FR_OK)
	{
		f_read(&fsrc, pfloatDCTData, DQLC_DCTBLOCKBYTES, &br);
	}
	f_close(&fsrc);	
}

//����DCTϵ��
void DQLC_SaveDCTData(uint8_t dctBlockIdx, float32_t * pfloatDCTData)
{
	sprintf(m_DCT_block_fileName, "DCT_%d", dctBlockIdx);
	
	res = f_open(&fsrc, m_DCT_block_fileName, FA_WRITE | FA_CREATE_ALWAYS);
	if(res == FR_OK)
	{
		f_write(&fsrc, pfloatDCTData, DQLC_DCTBLOCKBYTES, &br);
	}
	f_close(&fsrc);	
}

//��ȡԭʼ���ݵ�float���飬�Խ���DCT�任
void DQLC_ReadOriData2DCT(const char * originalFileName, uint32_t offset, float32_t * pfloatData, int16_t * p_int16TmpBuffer)
{
	uint16_t i;
	res = f_open(&fsrc, originalFileName, FA_READ | FA_OPEN_EXISTING);
	if(res == FR_OK)
	{
		offset = offset * DQLC_DATABYTES;
		res = f_lseek(&fsrc, offset);
		res = f_read(&fsrc, p_int16TmpBuffer, DQLC_BLOCKBYTES, &br);
		for(i = 0; i < DCTBLOCKSIZE; ++i)
		{
			pfloatData[i] = p_int16TmpBuffer[i];
		}
	}
	f_close(&fsrc);		
}

//��ȡԭʼ����
void DQLC_ReadOriData(const char * originalFileName, uint32_t offset, int16_t * p_int16Data)
{
	res = f_open(&fsrc, originalFileName, FA_READ | FA_OPEN_EXISTING);
	if(res == FR_OK)
	{
		offset = offset * DQLC_DATABYTES;
		res = f_lseek(&fsrc, offset);
		res = f_read(&fsrc, p_int16Data, DQLC_BLOCKBYTES, &br);
	}
	f_close(&fsrc);		
}

int DQLC_dctQuan(const char * originalFileName, const char * outputBitCFileName, float32_t meanBits, uint8_t isLossless)
{
	uint8_t blockIdx;
	uint16_t blkCnt;
	uint32_t dataIdx = 0;
	uint32_t in;
	uint8_t *pUint8S;
	float32_t *pS1, *pS2;
	uint16_t *pMaxValue;
	int16_t *pint16S;
//	int32_t diffValue;
//	int32_t len;
//	int ori;
//	int16_t ori16;
	//��ʼ������
	//����ԭʼ���ݣ�����DCT�任����¼ÿ���Ӵ������ֵ��������
	memset(p_inlineBufferFloatDCTPower, 0, DCTBLOCKSIZE * sizeof(float32_t));
	memset(p_uintDCTMaxValueBuffer, 0, DCTBLOCKSIZE * sizeof(uint16_t));
	for(blockIdx = 0; blockIdx < DCTBLOCKCOUNT; ++blockIdx)
	{
		//DEBUG_OUTPUT_FUN(DQLC_PrintString("block----------:");)
		DQLC_ReadOriData2DCT(originalFileName, dataIdx, p_floatDCTBuffer, p_int16Buffer);
		//DEBUG_OUTPUT_FUN(DQLC_PrintFloatArray(p_floatDCTBuffer);)
		arm_dct4_f32(&dctInstance, p_floatState, p_floatDCTBuffer);
		//DEBUG_OUTPUT_FUN(DQLC_PrintFloatArray(p_floatDCTBuffer);)
		//ȡ��
		blkCnt = DCTBLOCKSIZE >> 2u;
		pS1 = p_floatDCTBuffer;
		while (blkCnt > 0u)
		{
			*pS1++ = roundf(*pS1);
			*pS1++ = roundf(*pS1);
			*pS1++ = roundf(*pS1);
			*pS1++ = roundf(*pS1);
			blkCnt--;
		}
		//��power
		blkCnt = DCTBLOCKSIZE >> 2u;
		pS1 = p_floatDCTBuffer;
		pS2 = p_inlineBufferFloatDCTPower;
		pMaxValue = p_uintDCTMaxValueBuffer;
		while(blkCnt > 0u)
		{
			in = (uint32_t)fabs(*pS1++);
			*pS2++ += in * in;
			if (in > *pMaxValue)
			{
				*pMaxValue = (uint16_t)in;
			}
			pMaxValue++;
			
			in = (uint32_t)fabs(*pS1++);
			*pS2++ += in * in;
			if (in > *pMaxValue)
			{
				*pMaxValue = (uint16_t)in;
			}
			pMaxValue++;
			
			in = (uint32_t)fabs(*pS1++);
			*pS2++ += in * in;
			if (in > *pMaxValue)
			{
				*pMaxValue = (uint16_t)in;
			}
			pMaxValue++;
			
			in = (uint32_t)fabs(*pS1++);
			*pS2++ += in * in;
			if (in > *pMaxValue)
			{
				*pMaxValue = (uint16_t)in;
			}
			pMaxValue++;

			blkCnt--;
		}
		//���ļ���ʽ����DCTϵ��
		DQLC_SaveDCTData(blockIdx, p_floatDCTBuffer);
		dataIdx += DCTBLOCKSIZE;
	}

	//DEBUG_OUTPUT_FUN(DQLC_PrintUintArray(p_uintDCTMaxValueBuffer);)
	//�������λ��
	memset(p_uint8MaxBitsBuffer, 0, DCTBLOCKSIZE*sizeof(uint8_t));
	DQLC_QuanMAXBits(p_uintDCTMaxValueBuffer, p_uint8MaxBitsBuffer);
	//DEBUG_OUTPUT_FUN(DQLC_PrintUint8Array(p_uint8MaxBitsBuffer);)
	//��������λ��
	memset(p_uint8BitsBuffer, 0, DCTBLOCKSIZE*sizeof(uint8_t));
	DQLC_QuanBitsAlloc(p_inlineBufferFloatDCTPower, p_uint8BitsBuffer, p_uint8MaxBitsBuffer, (uint16_t)(meanBits * DCTBLOCKSIZE));
	//DQLC_QuanBitsAllocFast(p_inlineBufferFloatDCTPower, p_inlineMaxPowerIndex, p_uint8BitsBuffer, p_uint8MaxBitsBuffer, (uint16_t)(meanBits * DCTBLOCKSIZE));
	/*DEBUG_OUTPUT_FUN(DQLC_PrintUint8Array(p_uint8BitsBuffer);)*/
	//DEBUG_OUTPUT_FUN(DQLC_PrintUint8Array(p_uint8BitsBuffer);)
	//����2N,2N��ʹ�õ��ڴ���Power��һ���ģ�������float32��uint32�ڴ�ռ�ö���4�ֽڣ������������Ը��ã�Ϊ�˽�Լ�ڴ�
	memset(p_inlineBufferUint2N, 0, DCTBLOCKSIZE*sizeof(int32_t));
	DQLC_Cal2NBits(p_inlineBufferUint2N, p_uint8BitsBuffer);
	//DEBUG_OUTPUT_FUN(DQLC_PrintUintArray(p_inlineBufferUint2N);)
	//����ߴ���Ϣ============
	if(BitArrayCEncodeValueStart(outputBitCFileName, &fBitC, 1))
	{
		return -1;
	}	
	//�������λ��nk
	blkCnt = DCTBLOCKSIZE >> 2u;
	pUint8S = p_uint8BitsBuffer;
	while (blkCnt > 0u)
	{
		EncodeValue((int32_t)*pUint8S++);
		EncodeValue((int32_t)*pUint8S++);
		EncodeValue((int32_t)*pUint8S++);
		EncodeValue((int32_t)*pUint8S++);
		blkCnt--;
	}
	//����DCTϵ��������ֵDCTMax
	blkCnt = DCTBLOCKSIZE >> 2u;
	pMaxValue = p_uintDCTMaxValueBuffer;
	while (blkCnt > 0u)
	{
		EncodeValue((int32_t)*pMaxValue++);
		EncodeValue((int32_t)*pMaxValue++);
		EncodeValue((int32_t)*pMaxValue++);
		EncodeValue((int32_t)*pMaxValue++);
		blkCnt--;
	}
	//����============
	if(isLossless)
	{
		////////====================����Ϊ�ߴ�DCTϵ�����ߴ��ֵ����
		///*
		//DEBUG_OUTPUT_FUN(DQLC_PrintString("-------to code diff--------");)
		dataIdx = 0;
		for(blockIdx = 0; blockIdx < DCTBLOCKCOUNT; ++blockIdx)
		{
			//DEBUG_OUTPUT_FUN(DQLC_PrintString("--block---");)
			//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(blockIdx);)
			//���ļ��ж���dctϵ��
			
			DQLC_ReadDCTData(blockIdx, p_floatDCTBuffer);
			DQLC_QuanDCTCoeff(p_floatDCTBuffer, p_uintDCTMaxValueBuffer, p_inlineBufferUint2N,
				p_uint8BitsBuffer, p_uint8MaxBitsBuffer, isLossless); 

			//idct
			arm_dct4_f32(&dctInstance, p_floatState, p_floatDCTBuffer);
			//DEBUG_OUTPUT_FUN(DQLC_PrintFloatArray(p_floatDCTBuffer);)
			DQLC_ReadOriData(originalFileName, dataIdx, p_int16Buffer);

			//�����ֵ������
			blkCnt = DCTBLOCKSIZE >> 2u;
			pS1 = p_floatDCTBuffer;
			pint16S = p_int16Buffer;
			//DEBUG_OUTPUT_FUN(DQLC_PrintString("--diff---");)
			
			while(blkCnt > 0u)
			{
				EncodeValue((int32_t)*pint16S++ - (int32_t)*pS1++);
				EncodeValue((int32_t)*pint16S++ - (int32_t)*pS1++);
				EncodeValue((int32_t)*pint16S++ - (int32_t)*pS1++);
				EncodeValue((int32_t)*pint16S++ - (int32_t)*pS1++);
				blkCnt--;
			}
			/*
			while(blkCnt > 0u)
			{
			if(blkCnt == 1)
				{
					len = len +1;
				}
				len  = (int32_t)*pS1++;
				diffValue = *pint16S++ - len;
				EncodeValue(diffValue);
				if(blockIdx == 0 || blockIdx == 1)
				{
					DEBUG_OUTPUT_FUN(DQLC_PrintIntData(len);)
				}
				//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(diffValue);)
				len  = (int32_t)*pS1++;
				diffValue = *pint16S++ - len;
				EncodeValue(diffValue);
				if(blockIdx == 0 || blockIdx == 1)
				{
					DEBUG_OUTPUT_FUN(DQLC_PrintIntData(len);)
				}
				//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(diffValue);)
				len  = (int32_t)*pS1++;
				diffValue = *pint16S++ - len;
				if(blockIdx == 0 || blockIdx == 1)
				{
					DEBUG_OUTPUT_FUN(DQLC_PrintIntData(len);)
				}
				//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(diffValue);)

				len  = (int32_t)*pS1++;
				ori16 = *pint16S;
				pint16S++;
				ori =ori16;
				diffValue = ori - len;
				if(blockIdx == 0 || blockIdx == 1)
				{
					DEBUG_OUTPUT_FUN(DQLC_PrintIntData(len);)
				}
				//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(diffValue);)
				blkCnt--;
			}
			//DEBUG_OUTPUT_FUN(DQLC_PrintString("\n\n");)
			*/
			dataIdx += DCTBLOCKSIZE;
		}	
		//*/
		////////============================================================

		/*
		////////====================����Ϊ�ȴ�DCTϵ�����ٴ��ֵ����
		dataIdx = 0;
		for (blockIdx = 0; blockIdx < DCTBLOCKCOUNT; ++blockIdx)
		{
			DQLC_ReadDCTData(blockIdx, p_floatDCTBuffer);
			DQLC_QuanDCTCoeff(p_floatDCTBuffer, p_floatDCTMaxValueBuffer, p_floatDCTPowerBuffer,	//p_floatDCTPowerBufferΪp_float2N
				p_uint8BitsBuffer, p_uint8MaxBitsBuffer, isLossless);
			DQLC_PrintFloatArray(p_floatDCTBuffer);
			//�ȱ���DCTϵ��
			DQLC_SaveDCTData(blockIdx, p_floatDCTBuffer);
			dataIdx += DCTBLOCKSIZE;
		}
		//����DCTϵ�����ټ����ֵ
		dataIdx = 0;
		for (blockIdx = 0; blockIdx < DCTBLOCKCOUNT; ++blockIdx)
		{
			DQLC_ReadDCTData(blockIdx, p_floatDCTBuffer);
			//idct
			arm_dct4_f32(&dctInstance, p_floatState, p_floatDCTBuffer);
			DQLC_PrintFloatArray(p_floatDCTBuffer);
			DQLC_ReadOriData(originalFileName, dataIdx, p_int16Buffer);
			//�����ֵ������
			blkCnt = DCTBLOCKSIZE >> 2u;
			pS1 = p_floatDCTBuffer;
			pint16S = p_int16Buffer;
			while (blkCnt > 0u)
			{
				diffValue = *pint16S++ - (int32_t)*pS1++;
				EncodeValue(diffValue);
				DQLC_PrintIntData(diffValue);
				diffValue = *pint16S++ - (int32_t)*pS1++;
				EncodeValue(diffValue);
				DQLC_PrintIntData(diffValue);
				diffValue = *pint16S++ - (int32_t)*pS1++;
				EncodeValue(diffValue);
				DQLC_PrintIntData(diffValue);
				diffValue = *pint16S++ - (int32_t)*pS1++;
				EncodeValue(diffValue);
				DQLC_PrintIntData(diffValue);
				blkCnt--;
			}
			DQLC_PrintString("\t\n\t\n");
			dataIdx += DCTBLOCKSIZE;
		}
		////////============================================================
		*/
	}else
	{
		//for(blockIdx = 0; blockIdx < DCTBLOCKCOUNT; ++blockIdx)
		//{
		//	DQLC_ReadDCTData(blockIdx, p_floatDCTBuffer);
		//	DQLC_QuanDCTCoeff(p_floatDCTBuffer, p_floatDCTMaxValueBuffer, p_floatDCTPowerBuffer,	//p_floatDCTPowerBufferΪp_float2N
		//						p_uint8BitsBuffer, p_uint8MaxBitsBuffer, isLossless); 
		//}		
	}

	//DEBUG_OUTPUT_FUN(DQLC_PrintString("-------to end--------");)

	//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(len);)
	
	
	
	return BitArrayCEncodeValueEnd();
}

//p_floatDCTInline����isLosslessֵ�任�����isLosslessΪ1����p_floatDCTInline������Ϊ������ԭ��DCTϵ��
//                                      ���isLosslessΪ0����p_floatDCTInline����ֵ����
void DQLC_QuanDCTCoeff(float32_t * p_floatDCTInline, uint16_t * p_uintMax, uint32_t * p_uint2N,
						 uint8_t * p_uint8Bits, uint8_t * p_uint8MaxBits, uint8_t isLossless)
{
	uint16_t blkCnt;
	uint8_t n;
	int32_t i2N, imax;
	int32_t dctC;
	float32_t fdctC;
	int32_t m;
	
	//������˳��������λ��������ֵm_bitThreshold����������
	blkCnt = DCTBLOCKSIZE;
	if(isLossless)
	{
		//DEBUG_OUTPUT_FUN(DQLC_PrintString("codem");)
		while(blkCnt > 0u)	
		{
			n = *p_uint8Bits++;
			dctC = (int32_t)*p_floatDCTInline;
			i2N = (int32_t)*p_uint2N++;
			imax = (int32_t)*p_uintMax++;
			//��������
			if(n == 0u)
			{
				m = 0;
			}else if(n < *p_uint8MaxBits)
			{
				m = dctC * i2N / imax;
				if (dctC < 0)
				{
					m -= 1;
					if (m  < -i2N)
					{
						m = -i2N;
					}
				}
				else if (m == i2N)
				{
					m -= 1;
				}
			}else//*p_uint8Bits == *p_uint8MaxBits
			{
				m = (int32_t)dctC;
			}
			//��������
			if (n >= DEFAULT_BITS_THRESHOLD)
			{
				EncodeValue(m);
			}
			else if (n > 0)
			{
				DQLC_EncodingBits(m, n);
			}
			//DEBUG_OUTPUT_FUN(DQLC_PrintIntData(m);)
			//������
			//��������
			if(n == 0u)
			{
				fdctC = 0.0f;
			}else if(n < *p_uint8MaxBits)
			{
				fdctC = (float_t)(((m << 1) + 1) * imax) / (float32_t)i2N / 2.0f;
			}else//*p_uint8Bits == *p_uint8MaxBits
			{
				fdctC = (float_t)m;
			}			
			*p_floatDCTInline = fdctC;
			
			p_floatDCTInline++;
			p_uint8MaxBits++;
			blkCnt--;
		}	
		//DEBUG_OUTPUT_FUN(DQLC_PrintString("\n");)
	}else
	{
		while (blkCnt > 0u)
		{
			n = *p_uint8Bits++;
			dctC = (int32_t)*p_floatDCTInline;
			i2N = (int32_t)*p_uint2N++;
			imax = (int32_t)*p_uintMax++;
			//��������
			if (n == 0u)
			{
				m = 0;
			}
			else if (n < *p_uint8MaxBits)
			{
				m = dctC * i2N / imax;
				if (dctC < 0)
				{
					m -= 1;
					if (m < -i2N)
					{
						m = -i2N;
					}
				}
				else if (m == i2N)
				{
					m -= 1;
				}
			}
			else//*p_uint8Bits == *p_uint8MaxBits
			{
				m = (int32_t)dctC;
			}
			//��������
			if (n >= DEFAULT_BITS_THRESHOLD)
			{
				EncodeValue(m);
			}
			else if (n > 0)
			{
				DQLC_EncodingBits(m, n);
			}

			p_floatDCTInline++;
			p_uint8MaxBits++;
			blkCnt--;
		}
	}
}

void DQLC_Cal2NBits(uint32_t * p_2NBuffer, uint8_t * p_Bits)
{
	uint16_t blkCnt;
	uint8_t n;
	blkCnt = DCTBLOCKSIZE >> 2u;
	while(blkCnt > 0u)
	{
		n = *p_Bits++;
		if(n > 0u)
		{
			*p_2NBuffer = 1u << (n - 1);
		}
		p_2NBuffer++;
		n = *p_Bits++;
		if (n > 0u)
		{
			*p_2NBuffer = 1u << (n - 1);
		}
		p_2NBuffer++;
		n = *p_Bits++;
		if (n > 0u)
		{
			*p_2NBuffer = 1u << (n - 1);
		}
		p_2NBuffer++;
		n = *p_Bits++;
		if (n > 0u)
		{
			*p_2NBuffer = 1u << (n - 1);
		}
		p_2NBuffer++;
		blkCnt--;	
	}
}

//���������λ��
uint8_t maxBits(uint16_t d)
{
	uint8_t mb = 0u;
	while(d > 0u)
	{
		++mb;
		d >>= 1u;
	}
	return mb + 1;
}

void DQLC_QuanMAXBits(uint16_t* p_intMax, uint8_t * p_MaxBitsBuffer)
{
	uint16_t blkCnt;
	blkCnt = DCTBLOCKSIZE >> 2u;
	while(blkCnt > 0u)	
	{
		*p_MaxBitsBuffer++ = maxBits(*p_intMax++);
		*p_MaxBitsBuffer++ = maxBits(*p_intMax++);
		*p_MaxBitsBuffer++ = maxBits(*p_intMax++);
		*p_MaxBitsBuffer++ = maxBits(*p_intMax++);
		blkCnt--;
	}		
}
//��һ�����ݿ������ֵ�Ͷ�Ӧ��index
uint16_t DQLC_MaxValue(float32_t * pPowerBuffer)
{
	uint16_t blkCnt, i, maxValueIdx;
	float32_t in, maxValue;

	blkCnt = DCTBLOCKSIZE >> 2u;
	maxValue = 0.0f;
	maxValueIdx = 0u;
	i = 0u;
	while (blkCnt > 0u)
	{
		in = *pPowerBuffer++;
		if (in > maxValue)
		{
			maxValue = in;
			maxValueIdx = i;
		}
		i++;

		in = *pPowerBuffer++;
		if (in > maxValue)
		{
			maxValue = in;
			maxValueIdx = i;
		}
		i++;

		in = *pPowerBuffer++;
		if (in > maxValue)
		{
			maxValue = in;
			maxValueIdx = i;
		}
		i++;

		in = *pPowerBuffer++;
		if (in > maxValue)
		{
			maxValue = in;
			maxValueIdx = i;
		}
		i++;

		blkCnt--;
	}

	return maxValueIdx;
}
//����λ������
void DQLC_QuanBitsAlloc(float32_t * pPowerBuffer, uint8_t * pBitsBuffer, uint8_t * p_MaxBitsBuffer, uint16_t bitsAmount)
{
	uint16_t maxValueIdx;
	while(bitsAmount > 0u)
	{
		maxValueIdx = DQLC_MaxValue(pPowerBuffer);
		pPowerBuffer[maxValueIdx] /= 4.0f;
		if(pBitsBuffer[maxValueIdx] <= p_MaxBitsBuffer[maxValueIdx])
		{
			pBitsBuffer[maxValueIdx]++;
		}
		bitsAmount--;
	}
}

void Qsort(int a[], int low, int high)
{
	int first = low;
	int last = high;
	int key = a[first];/*���ֱ�ĵ�һ����¼��Ϊ����*/
	if (low >= high)
	{
		return;
	}
	while (first<last)
	{
		while (first<last&&a[last] <= key)
			--last;
		a[first] = a[last];/*���ȵ�һ��С���Ƶ��Ͷ�*/
		while (first<last&&a[first] >= key)
			++first;
		a[last] = a[first];/*���ȵ�һ������Ƶ��߶�*/
	}
	a[first] = key;/*�����¼��λ*/
	Qsort(a, low, first - 1);
	Qsort(a, first + 1, high);
}
//float32_t * p_inlineBufferFloatDCTPower;
//int16_t *  p_inlineMaxPowerIndex;
//�ݹ鵼���ڴ�ռ��̫�ࡣ����
void QuickSortIdx(int16_t low, int16_t high)
{
	int16_t first, last;
	int16_t pivot;
	if (low >= high)
	{
		return;
	}
	first = low;
	last = high;
	pivot = p_inlineMaxPowerIndex[first];//���ֱ�ĵ�һ����¼��Ϊ����
	/* partition */
	while (first < last)
	{
		while (first < last && p_inlineBufferFloatDCTPower[p_inlineMaxPowerIndex[last]] <= p_inlineBufferFloatDCTPower[pivot])
			--last;
		p_inlineMaxPowerIndex[first] = p_inlineMaxPowerIndex[last];//���ȵ�һ������Ƶ��Ͷ�
		while (first < last && p_inlineBufferFloatDCTPower[p_inlineMaxPowerIndex[first]] >= p_inlineBufferFloatDCTPower[pivot])
			++first;
		p_inlineMaxPowerIndex[last] = p_inlineMaxPowerIndex[first];//���ȵ�һ��С���Ƶ��߶�
	}
	p_inlineMaxPowerIndex[first] = pivot;//�����¼��λ

	QuickSortIdx(low, first - 1);
	QuickSortIdx(first + 1, high);
}

void bubbleSort(int *x, uint16_t n)
{
	int16_t j, k, h, t;
	for (h = n - 1; h > 0; h = k) /*ѭ����û�бȽϷ�Χ*/
	{
		for (j = 0, k = 0; j < h; ++j) /*ÿ��Ԥ��k=0��ѭ��ɨ������k*/
		{
			if (*(x + j) < *(x + j + 1))/*��ķ���ǰ�棬С�ķŵ�����*/
			{
				t = *(x + j);
				*(x + j) = *(x + j + 1);
				*(x + j + 1) = t;/*��ɽ���*/
				k = j;/*��������³���λ�á�����k����Ķ��������ź��˵ġ�*/
			}
		}
	}
}

//float32_t * p_inlineBufferFloatDCTPower;
//int16_t *  p_inlineMaxPowerIndex;
void BubbleSortIdx(uint16_t n)
{
	int16_t j, k, h, t;
	for (h = n - 1; h > 0; h = k) /*ѭ����û�бȽϷ�Χ*/
	{
		for (j = 0, k = 0; j < h; ++j) /*ÿ��Ԥ��k=0��ѭ��ɨ������k*/
		{
			if (*(p_inlineBufferFloatDCTPower + *(p_inlineMaxPowerIndex + j)) < *(p_inlineBufferFloatDCTPower + *(p_inlineMaxPowerIndex + j + 1)))/*��ķ���ǰ�棬С�ķŵ�����*/
			{
				t = *(p_inlineMaxPowerIndex + j);
				*(p_inlineMaxPowerIndex + j) = *(p_inlineMaxPowerIndex + j + 1);
				*(p_inlineMaxPowerIndex + j + 1) = t;/*��ɽ���*/
				k = j;/*��������³���λ�á�����k����Ķ��������ź��˵ġ�*/
			}
		}
	}
}

void DQLC_QuanBitsAllocFast(float32_t * pPowerBuffer, int16_t * pIdxBuffer, uint8_t * pBitsBuffer, uint8_t * pMaxBitsBuffer, uint16_t bitsAmount)
{
	uint16_t i;
	float32_t currentValue;

	for (i = 0; i<DCTBLOCKSIZE; ++i)
	{
		pIdxBuffer[i] = i;
	}
	
	//int a[] = { 1, 3, 2, 6, 4, 2, 3, 9, 11, 20 };
	//Qsort(a, 0, 9);
	//for (i = 0; i<9; ++i)
	//{
	//	printf("%d ", a[i]);
	//}

//	QuickSortIdx(0, 50);
////	BubbleSortIdx(50);
//	for (i = 0; i<50; ++i)
//	{
//		printf("%d ", pIdxBuffer[i]);
//	}
//	printf("\n\n");
//	for (i = 0; i<50; ++i)
//	{
//		printf("%f ", pPowerBuffer[pIdxBuffer[i]]);
//	}
//	QuickSortIdx(0, DCTBLOCKSIZE - 1);
	BubbleSortIdx(DCTBLOCKSIZE);
	while (bitsAmount > 0u)
	{
		m_uint16Tmp = pIdxBuffer[0];
		currentValue = pPowerBuffer[m_uint16Tmp] / 4.0f;
		if (pBitsBuffer[m_uint16Tmp] < pMaxBitsBuffer[m_uint16Tmp])
		{
			pBitsBuffer[m_uint16Tmp]++;
		}
		for (i = 1; i < DCTBLOCKSIZE; ++i)
		{
			if (currentValue > pPowerBuffer[pIdxBuffer[i]])
			{
				pPowerBuffer[pIdxBuffer[i - 1]] = currentValue;
				break;
			}
			m_uint16Tmp = pIdxBuffer[i - 1];
			pIdxBuffer[i - 1] = pIdxBuffer[i];
			pIdxBuffer[i] = m_uint16Tmp;
		}
		bitsAmount--;
	}
}

void DQLC_EncoderInit()
{
	
}

void DQLC_EncodingValue(int nEncode)
{

}

void DQLC_EncodingBits(int nEncode, unsigned int nbits)
{
	if (nEncode < 0)
	{
		nEncode += (1 << nbits);
	}
	EncodeBits(nEncode, nbits);
}


///////////////////////////////��ѹ��
#ifdef LOSSLESS_DECOMPRESSION
int DQLC_DecodeValue()
{
	int deValue = UnBitArrayCDecodeValue(1);

	return deValue;
}

int DQLC_DecodeXBitsValue(unsigned int nBits)
{
	int nValue;
	int n = 1 << (nBits - 1);
	nValue = RangeDecodeFastWithUpdate(nBits);
	if (nValue >= n)
	{
		nValue -= (1 << nBits);
	}
	return nValue;
}

void DQLC_DeQuanDCTCoeff(float32_t * p_floatDCTInline, uint16_t * p_uintMax, uint32_t * p_uint2N,
							uint8_t * p_uint8Bits, uint8_t * p_uint8MaxBits)
{
	uint16_t blkCnt;
	uint8_t n;
	int32_t i2N, imax;
	float32_t fdctC;
	int32_t m;

	blkCnt = DCTBLOCKSIZE;
	DEBUG_OUTPUT_FUN(DQLC_PrintString("decm");)
	while (blkCnt > 0u)
	{
		n = *p_uint8Bits++;
		i2N = (int32_t)*p_uint2N++;
		imax = (int32_t)*p_uintMax++;
		//��������
		if (n >= DEFAULT_BITS_THRESHOLD)
		{
			m = DQLC_DecodeValue();
		}
		else if (n != 0u)
		{
			m = DQLC_DecodeXBitsValue(n);
		}
		else
		{
			m = 0;
		}
		DEBUG_OUTPUT_FUN(DQLC_PrintIntData(m);)
		//������
		if (n == 0u)
		{
			fdctC = 0.0f;
		}
		else if (n < *p_uint8MaxBits)
		{
			fdctC = (float_t)(((m << 1) + 1) * imax) / (float32_t)i2N / 2.0f;
		}
		else//*p_uint8Bits == *p_uint8MaxBits
		{
			fdctC = (float_t)m;
		}
		*p_floatDCTInline = fdctC;

		p_floatDCTInline++;
		p_uint8MaxBits++;
		blkCnt--;
	}
	DEBUG_OUTPUT_FUN(DQLC_PrintString("\n");)
}

void DQLC_DeCompress(const char* inputFile, const char * outputFile, uint8_t isLossless)
{
	uint8_t blockIdx, *pUint8S;
	uint16_t blkCnt;
	float32_t *pS1;
	int16_t *pint16S;
	int32_t diffValue;
	uint16_t *pMaxValue;

	FIL outFile;
	res = f_open(&outFile, outputFile, FA_WRITE | FA_CREATE_ALWAYS);
	UnBitArrayCInit(inputFile);
	//�������λ��nk
	blkCnt = DCTBLOCKSIZE >> 2u;
	//blkCnt = DCTBLOCKSIZE; //��Ϊ����Ľ�ѹ���ݿ���û��
	memset(p_uint8BitsBuffer, 0, DCTBLOCKSIZE*sizeof(uint8_t));
	pUint8S = p_uint8BitsBuffer;
	while (blkCnt > 0u)
	{
		*pUint8S++ = (uint8_t)DQLC_DecodeValue();
		*pUint8S++ = (uint8_t)DQLC_DecodeValue();
		*pUint8S++ = (uint8_t)DQLC_DecodeValue();
		*pUint8S++ = (uint8_t)DQLC_DecodeValue();
		blkCnt--;
	}
	DEBUG_OUTPUT_FUN(DQLC_PrintUint8Array(p_uint8BitsBuffer);)
	//����������ֵfloatMax
	blkCnt = DCTBLOCKSIZE >> 2u;
	memset(p_uintDCTMaxValueBuffer, 0, DCTBLOCKSIZE * sizeof(uint16_t));
	pMaxValue = p_uintDCTMaxValueBuffer;
	while (blkCnt > 0u)
	{
		*pMaxValue++ = (uint16_t)DQLC_DecodeValue();
		*pMaxValue++ = (uint16_t)DQLC_DecodeValue();
		*pMaxValue++ = (uint16_t)DQLC_DecodeValue();
		*pMaxValue++ = (uint16_t)DQLC_DecodeValue();
		blkCnt--;
	}
	DEBUG_OUTPUT_FUN(DQLC_PrintUint16Array(p_uintDCTMaxValueBuffer);)
	//�����λ��
	memset(p_uint8MaxBitsBuffer, 0, DCTBLOCKSIZE*sizeof(uint8_t));
	DQLC_QuanMAXBits(p_uintDCTMaxValueBuffer, p_uint8MaxBitsBuffer);
	DEBUG_OUTPUT_FUN(DQLC_PrintUint8Array(p_uint8MaxBitsBuffer);)
	//��λ����2N������p_floatDCTPowerBuffer��
	memset(p_inlineBufferUint2N, 0, DCTBLOCKSIZE*sizeof(int32_t));
	DQLC_Cal2NBits(p_inlineBufferUint2N, p_uint8BitsBuffer);
	DEBUG_OUTPUT_FUN(DQLC_PrintUintArray(p_inlineBufferUint2N);)

	if (isLossless)
	{
		////////====================����Ϊ�ߴ�DCTϵ�����ߴ��ֵ����
		for (blockIdx = 0; blockIdx < DCTBLOCKCOUNT; ++blockIdx)
		{
			DQLC_DeQuanDCTCoeff(p_floatDCTBuffer, p_uintDCTMaxValueBuffer, p_inlineBufferUint2N,
				p_uint8BitsBuffer, p_uint8MaxBitsBuffer);
			DEBUG_OUTPUT_FUN(DQLC_PrintFloatArray(p_floatDCTBuffer);)
				arm_dct4_f32(&dctInstance, p_floatState, p_floatDCTBuffer);
			DEBUG_OUTPUT_FUN(DQLC_PrintFloatArray(p_floatDCTBuffer);)
			blkCnt = DCTBLOCKSIZE >> 2u;
			memset(p_int16Buffer, 0, DCTBLOCKSIZE*sizeof(int16_t));
			pS1 = p_floatDCTBuffer;
			pint16S = p_int16Buffer;
			while (blkCnt > 0u)
			{
				diffValue = DQLC_DecodeValue();
				*pint16S++ = (int32_t)*pS1++ + diffValue;
				diffValue = DQLC_DecodeValue();
				*pint16S++ = (int32_t)*pS1++ + diffValue;
				diffValue = DQLC_DecodeValue();
				*pint16S++ = (int32_t)*pS1++ + diffValue;
				diffValue = DQLC_DecodeValue();
				*pint16S++ = (int32_t)*pS1++ + diffValue;

				blkCnt--;
			}
			DEBUG_OUTPUT_FUN(DQLC_PrintString("--original:\n");)
			DEBUG_OUTPUT_FUN(DQLC_PrintInt16Array(p_int16Buffer);)
			f_write(&outFile, p_int16Buffer, DCTBLOCKSIZE*sizeof(int16_t), &br);
		}
	}

	f_close(&outFile);
	UnBitArrayCClose();
}

#endif

#ifdef DEBUG_OUTPUT

void DQLC_PrintFloatArray(float32_t *toPrint)
{
	uint16_t i;

	for(i = 0; i < DCTBLOCKSIZE - 1; ++i)
	{
		fprintf(F4Printf, "%f, ", toPrint[i]);
	}
	fprintf(F4Printf, "%f\n", toPrint[i]);
	fprintf(F4Printf, "------------------------------------------\n");
}

void DQLC_PrintUint8Array(uint8_t *toPrint)
{
	uint16_t i;

	for(i = 0; i < DCTBLOCKSIZE - 1; ++i)
	{
		fprintf(F4Printf, "%d, ", toPrint[i]);
	}
	fprintf(F4Printf, "%d\n", toPrint[i]);
	fprintf(F4Printf, "------------------------------------------\n");
}

void DQLC_PrintUint16Array(uint16_t *toPrint)
{
	uint16_t i;

	for (i = 0; i < DCTBLOCKSIZE - 1; ++i)
	{
		fprintf(F4Printf, "%d, ", toPrint[i]);
	}
	fprintf(F4Printf, "%d\n", toPrint[i]);
	fprintf(F4Printf, "------------------------------------------\n");
}
void DQLC_PrintInt16Array(int16_t *toPrint)
{
	uint16_t i;

	for (i = 0; i < DCTBLOCKSIZE - 1; ++i)
	{
		fprintf(F4Printf, "%d, ", toPrint[i]);
	}
	fprintf(F4Printf, "%d\n", toPrint[i]);
	fprintf(F4Printf, "------------------------------------------\n");
}

void DQLC_PrintUintArray(uint32_t *toPrint)
{
	uint16_t i;

	for (i = 0; i < DCTBLOCKSIZE - 1; ++i)
	{
		fprintf(F4Printf, "%d, ", toPrint[i]);
	}
	fprintf(F4Printf, "%d\n", toPrint[i]);
	fprintf(F4Printf, "------------------------------------------\n");
}

void DQLC_PrintIntData(int d)
{
	fprintf(F4Printf, "%d,  ", d);
}

void DQLC_PrintString(const char * str)
{
	fprintf(F4Printf, "%s\n", str);
}

#endif
