#include "FatFsSimulation.h"


FRESULT f_open(
	FIL *fp,			/* Pointer to the blank file object */
	const char *path,	/* Pointer to the file name */
	BYTE mode			/* Access mode and file open mode flags */
	)
{
	FRESULT res = FR_OK;
	if (mode == (FA_WRITE | FA_CREATE_ALWAYS))
	{
		fp->pfile = fopen(path, "wb");
	}
	else if (mode == (FA_READ | FA_OPEN_EXISTING))
	{
		fp->pfile = fopen(path, "rb");
	}
	else
	{
		return FR_TIMEOUT;
	}

	if (fp->pfile == NULL)
	{
		res = FR_INT_ERR;
	}

	return res;
}

FRESULT f_read_m(
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	char datatype,
	UINT *br		/* Pointer to number of bytes read */
	)
{
	short shorttmp;
	float floattmp;
	int len;
	int i;
	if (datatype == 's')//short
	{
		len = btr / sizeof(short);
		for (i = 0; i < len; ++i)
		{
			fread(&shorttmp, sizeof(short), 1, fp->pfile);
			((short *)buff)[i] = shorttmp;
		}
	}
	else if (datatype == 'f')
	{
		len = btr / sizeof(float);
		for (i = 0; i < len; ++i)
		{
			fread(&floattmp, sizeof(float), 1, fp->pfile);
			((float *)buff)[i] = floattmp;
		}
	}
	
	return FR_OK;
}

FRESULT f_read(
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	UINT *br		/* Pointer to number of bytes read */
	)
{
	*br = fread(buff, btr, 1, fp->pfile);
	return FR_OK;
}

FRESULT f_lseek(
	FIL *fp,		/* Pointer to the file object */
	DWORD ofs		/* File pointer from top of file */
	)
{
	fseek(fp->pfile, ofs, SEEK_SET);
	return FR_OK;
}

FRESULT f_write(
	FIL *fp,			/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	UINT btw,			/* Number of bytes to write */
	UINT *bw			/* Pointer to number of bytes written */
	)
{
	*bw = fwrite(buff, btw, 1, fp->pfile);
	return FR_OK;
}

FRESULT f_close(
	FIL *fp		/* Pointer to the file object to be closed */
	)
{
	fclose(fp->pfile);
	return FR_OK;
}







