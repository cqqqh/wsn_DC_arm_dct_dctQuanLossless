//#include "hal.h"
#include "mytest.h"
#include "BitArrayC.h"
#include "dctQuanLossless.h"
#include "myFileIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>
FIL fsrc;
FRESULT res;
UINT br;

float32_t floatBuffer1[2048 * 2];  	//p_floatState
float32_t floatBuffer2[2048];   	//p_floatDCTBuffer
float32_t floatBuffer3[2048];			//p_floatDCTPowerBuffer
BYTE buffer[2048];


FILE *F4Printf;
const char oridata[8][20] = { "k48OR007_61.dat", "k48OR007_60.dat", "k48IR007_0.dat", "k48OR014_60.dat", "k12OR007_121.dat", "k12OR007_120.dat", "k12IR007_01.dat", "k12OR107_60.dat" };
const char bitdata[8][20] = { "k48OR007_61.bit", "k48OR007_60.bit", "k48IR007_0.bit", "k48OR014_60.bit", "k12OR007_121.bit", "k12OR007_120.bit", "k12IR007_01.bit", "k12OR107_60.bit" };
const char decdata[8][20] = { "k48OR007_61.dec", "k48OR007_60.dec", "k48IR007_0.dec", "k48OR014_60.dec", "k12OR007_121.dec", "k12OR007_120.dec", "k12IR007_01.dec", "k12OR107_60.dec" };
//const char oridata[8][10] = { "o41.dat", "o42.dat", "o43.dat", "o44.dat", "o11.dat", "o12.dat", "o13.dat", "o14.dat" };
//const char bitdata[8][10] = { "o41.bit", "o42.bit", "o43.bit", "o44.bit", "o11.bit", "o12.bit", "o13.bit", "o14.bit" };
//const char decdata[8][10] = { "o41.dec", "o42.dec", "o43.dec", "o44.dec", "o11.dec", "o12.dec", "o13.dec", "o14.dec" };

void mytest20150607yiqiyibiao()
{
	char fileList[200][260];//200个文件，每个最长名称260，这里必须设置为260，不然会出现问题
	char outputFilename[260];
	char inputFilename[260];
	unsigned int fileListCount = 0;
	unsigned int fileidx;

	float initBits = 2.0;
	int dlen = 0;
	int oriFileLen;

	_chdir("TestData\\节点采集_16位数据\\Node9");
	myFileGetFileListByType("*.DAT", fileList, &fileListCount);

	//初始化
	DCTBLOCKCOUNT = 40; //对DDS采集的数据，为40*2048 20150607
	DQLC_Init();

	F4Printf = fopen("log.txt", "w");

	for (fileidx = 0; fileidx < fileListCount; fileidx++)
	{
		strcpy(outputFilename, fileList[fileidx]);
		strcat(outputFilename, ".bit");
		dlen = DQLC_dctQuan(fileList[fileidx], outputFilename, initBits, 1);

		//解压数据
		strcpy(inputFilename, outputFilename);
		strcpy(outputFilename, fileList[fileidx]);
		strcat(outputFilename, ".dec");
		DQLC_DeCompress(inputFilename, outputFilename, 1);
		
		oriFileLen = myFileGetFileLength(fileList[fileidx]);
		printf("%s\t\t%f\r\n", fileList[fileidx], dlen / (float)oriFileLen);
	}

	//测试数据是否无损
	for (fileidx = 0; fileidx < fileListCount; fileidx++)
	{
		strcpy(outputFilename, fileList[fileidx]);
		strcat(outputFilename, ".dec");

		printf("%s\t", fileList[fileidx]);
		myFileCompare(fileList[fileidx], outputFilename);
	}

	fclose(F4Printf);
	getchar();
	return;

}

void mytest()
{
	int i;
	int dlen;

	_chdir("TestData\\data1");

	F4Printf = fopen("log.txt", "w");
	//初始化
	DCTBLOCKCOUNT = 25; //改组数据长度为25*2048
	DQLC_Init();

	dlen = 0;
	for (i = 0; i < 8; ++i)
	{
		dlen = DQLC_dctQuan(oridata[i], bitdata[i], 1.5f, 1);
		DQLC_DeCompress(bitdata[i], decdata[i], 1);
		printf("%s\t\t%f \t----- \r\n", oridata[i], dlen / 102400.0f);
		myFileCompare(oridata[i], decdata[i]);
	}
	fclose(F4Printf);
	getchar();
	return;
}

void mytest2()
{
	int dlen;

	_chdir("TestData\\data2");

	F4Printf = fopen("log.txt", "w");
	//初始化
	DCTBLOCKCOUNT = 25; //改组数据长度为25*2048
	DQLC_Init();

	dlen = DQLC_dctQuan("OR014_60.DAT", "OR014_60.bit", 2.0f, 1);
	DQLC_DeCompress("OR014_60.bit", "OR014_60.bit.dec", 1);
	myFileCompare("OR014_60.DAT", "OR014_60.bit.dec");
	printf("%s  \t%d \t----- \r\n", "OR014_60.dat", dlen);

	dlen = DQLC_dctQuan("OR007_61.DAT", "OR007_61.bit", 2.0f, 1);
	DQLC_DeCompress("OR007_61.bit", "OR007_61.bit.dec", 1);
	myFileCompare("OR007_61.DAT", "OR007_61.bit.dec");
	printf("%s  \t%d \t----- \r\n", "OR007_61.dat", dlen);

	dlen = DQLC_dctQuan("OR007_60.DAT", "OR007_60.bit", 2.0f, 1);
	DQLC_DeCompress("OR007_60.bit", "OR007_60.bit.dec", 1);
	myFileCompare("OR007_60.DAT", "OR007_60.bit.dec");
	printf("%s  \t%d \t----- \r\n", "OR007_60.dat", dlen);

	dlen = DQLC_dctQuan("IR007_0.DAT", "IR007_0.bit", 2.0f, 1);
	DQLC_DeCompress("IR007_0.bit", "IR007_0.bit.dec", 1);
	myFileCompare("IR007_0.DAT", "IR007_0.bit.dec");
	printf("%s  \t%d \t----- \r\n", "IR007_0.dat", dlen);
	DQLC_PrintString("---------------------\n\n\n\n\n\n--------------------\nDecompress!\n");

	fclose(F4Printf);
	getchar();

	return;
}
