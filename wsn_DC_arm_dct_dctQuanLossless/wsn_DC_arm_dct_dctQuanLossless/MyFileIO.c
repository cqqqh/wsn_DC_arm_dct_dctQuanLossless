#include "MyFileIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>

int myFileGetFileLength(const char *fileName)
{
	int len;
	FILE *fp;
	fp = fopen(fileName,"r");
	if(fp==NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	fclose(fp);

	return len;
}

//假定每行不超过256字符，最后有一空行
int myFileGetFileLines(const char *fileName)
{
	char c; 
	char line[256]="";
	int lines = 0;
	FILE *fp;
	fp = fopen(fileName,"r");
	if(fp==NULL)
		return -1;

	while(!feof(fp))
	{ 
		fgets(line, 256, fp); 
		lines++;
	}
	fclose(fp);

	return lines - 1;
}

int myFileWriteFloatArray(const float *data, const unsigned int len, const char *fileName)
{
	FILE *fp;
	unsigned int i;
	if((fp = fopen(fileName, "w")) == NULL)
	{
		printf("cannot open file...\n");
		return -1;
	}
	for(i = 0; i < len; ++i)
	{
		fprintf(fp, "%f\n", data[i]);
	}
	fclose(fp);
	return 0;
}

int myFileReadRawFloatArray(const char *fileName, float **data, int *dataLen)
{
	int i;
	int lines;
	float *d;
	FILE *fp;
	lines = myFileGetFileLength(fileName);
	lines = lines / sizeof(float);

	fp = fopen(fileName,"rb");
	if(fp == NULL)
		return -1;

	d = (float *)malloc(sizeof(float) * lines);
	memset(d, 0, sizeof(float) * lines);

	for(i = 0; i < lines; ++i)
	{
		fread(&(d[i]), sizeof(float), 1, fp);
	}

	fclose(fp);
	*data = d;
	*dataLen = lines;
	return 0;
}


int myFileWriteIntMatrix(const short **data, int nrows, int ncols, const char *fileName)
{
	FILE *fp;
	int i, j;
	if((fp = fopen(fileName, "w")) == NULL)
	{
		printf("cannot open file...\n");
		return -1;
	}
	for(i = 0; i < nrows; i++)
	{
		for(j = 0; j < ncols; j++)
		{
			fprintf(fp, "%d\t", data[i][j]);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
	return 0;	
}

int myFileWriteIntArray(const int *data, const unsigned int len, const char *fileName)
{
	FILE *fp;
	unsigned int i;
	if((fp = fopen(fileName, "w")) == NULL)
	{
		printf("cannot open file...\n");
		return -1;
	}
	for(i = 0; i < len; ++i)
	{
		fprintf(fp, "%d\n", data[i]);
	}
	fclose(fp);
	return 0;
}

int myFileReadIntArray(const char *fileName, int **data, int *dataLen)
{
	int i;
	int lines;
	int *d;
	FILE *fp;
	lines = myFileGetFileLines(fileName);
	fp = fopen(fileName,"r");
	if(fp == NULL)
		return -1;
	d = (int *)malloc(sizeof(int) * lines);
	memset(d, 0, sizeof(int) * lines);
	for(i = 0; i < lines; ++i)
	{
		fscanf(fp, "%d", &(d[i]));
	}

	fclose(fp);
	*data = d;
	*dataLen = lines;
	return 0;
}

int myFileReadNodeRawData(const char *fileName, int **data, int *dataLen)
{
	int i;
	int fileLen;
	short tmp = 0;
	int *d;
	FILE *fp;

	fileLen = myFileGetFileLength(fileName);
	fileLen = fileLen / sizeof(short);
	fp = fopen(fileName, "rb");
	if(fp == NULL)
		return -1;
	d = (int *)malloc(sizeof(int) * fileLen);
	memset(d, 0, sizeof(int) * fileLen);
	for(i = 0; i < fileLen; ++i)
	{
		fread(&tmp, sizeof(short), 1, fp);
		d[i] = tmp;
	}

	*data = d;
	*dataLen = fileLen;
	fclose(fp);

	return 0;
}

//myFileGetFileListByType("_*.txt", fileList, &fileListCount);
//char fileList[200][260];//200个文件，每个最长名称260，这里必须设置为260，不然会出现问题
int myFileGetFileListByType(const char *type, char fileList[][260], unsigned int *fileListCount)
{
	struct _finddata_t c_file;
	char (*p)[260];
	intptr_t hFile;
	unsigned int count = 0;
	p = fileList;
	// Find first .c file in current directory 
	if((hFile = _findfirst(type, &c_file)) == -1L)
	{
		printf( "No %s files in current directory!\n", type);
	}
	else
	{
		//printf( "Listing of .c files\n\n" );
		//printf( "RDO HID SYS ARC  FILE         DATE %25c SIZE\n", ' ' );
		//printf( "--- --- --- ---  ----         ---- %25c ----\n", ' ' );
		do {
			//char buffer[30];
			//printf( ( c_file.attrib & _A_RDONLY ) ? " Y  " : " N  " );
			//printf( ( c_file.attrib & _A_HIDDEN ) ? " Y  " : " N  " );
			//printf( ( c_file.attrib & _A_SYSTEM ) ? " Y  " : " N  " );
			//printf( ( c_file.attrib & _A_ARCH )   ? " Y  " : " N  " );
			//ctime_s( buffer, _countof(buffer), &c_file.time_write );
			//printf( " %-12s %.24s  %9ld\n",
			//   c_file.name, buffer, c_file.size );
			strcpy(*p++, c_file.name);
			++count;
		} while( _findnext( hFile, &c_file ) == 0 );
		_findclose( hFile );

		*fileListCount = count;
	}
}

int myFileCompare(const char * fname1, const char * fname2)
{
	char c1, c2;
	FILE * f1 = fopen(fname1, "r");
	FILE * f2 = fopen(fname2, "r");

	c1 = fgetc(f1);
	c2 = fgetc(f2);

	while (!feof(f1) && !feof(f2)) {
		if (c1 != c2) 
		{ 
			break;
		}
		c1 = fgetc(f1);
		c2 = fgetc(f2);
	}

	fclose(f1);
	fclose(f2);

	if (c1 == EOF&&c2 == EOF)  /* 判断两个文件是否都到结尾 */
	{
		printf("File is same \n");
		return 0;
	}
	else
	{
		printf("File is not same!! \n");
		return -1;
	}
}

