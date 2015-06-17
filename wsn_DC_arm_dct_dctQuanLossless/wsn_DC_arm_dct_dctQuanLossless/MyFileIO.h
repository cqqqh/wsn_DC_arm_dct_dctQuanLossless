#ifndef MYFILEIO_H
#define MYFILEIO_H

int myFileWriteFloatArray(const float *data, const unsigned int len, const char *fileName);
int myFileReadRawFloatArray(const char *fileName, float **data, int *dataLen);
int myFileWriteIntMatrix(const short **data, int nrows, int ncols, const char *fileName);
int myFileWriteIntArray(const int *data, const unsigned int len, const char *fileName);
int myFileReadIntArray(const char *fileName, int **data, int *dataLen);
int myFileReadNodeRawData(const char *fileName, int **data, int *dataLen);

int myFileGetFileLines(const char *fileName);
int myFileGetFileLength(const char *fileName);

int myFileGetFileListByType(const char *type, char fileList[][260], unsigned int *fileListCount);


int myFileCompare(const char * fname1, const char * fname2);
#endif
