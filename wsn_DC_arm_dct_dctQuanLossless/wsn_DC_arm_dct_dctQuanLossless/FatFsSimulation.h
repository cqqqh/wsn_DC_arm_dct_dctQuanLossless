#ifndef FATFSSIMULATION_H
#define FATFSSIMULATION_H

#include <stdio.h>
/* File object structure */

/* These types must be 16-bit, 32-bit or larger integer */
typedef int				INT;
typedef unsigned int	UINT;

/* These types must be 8-bit integer */
typedef signed char		CHAR;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* These types must be 16-bit integer */
typedef short			SHORT;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types must be 32-bit integer */
typedef long			LONG;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;

typedef struct _FIL_ {
	FILE * pfile;
	int fsize;
} FIL;


#define	FA_WRITE			0x02
#define	FA_CREATE_ALWAYS	0x08
#define	FA_READ				0x01
#define	FA_OPEN_EXISTING	0x00


typedef enum {
	FR_OK = 0,			/* 0 */
	FR_DISK_ERR,		/* 1 */
	FR_INT_ERR,			/* 2 */
	FR_NOT_READY,		/* 3 */
	FR_NO_FILE,			/* 4 */
	FR_NO_PATH,			/* 5 */
	FR_INVALID_NAME,	/* 6 */
	FR_DENIED,			/* 7 */
	FR_EXIST,			/* 8 */
	FR_INVALID_OBJECT,	/* 9 */
	FR_WRITE_PROTECTED,	/* 10 */
	FR_INVALID_DRIVE,	/* 11 */
	FR_NOT_ENABLED,		/* 12 */
	FR_NO_FILESYSTEM,	/* 13 */
	FR_MKFS_ABORTED,	/* 14 */
	FR_TIMEOUT			/* 15 */
} FRESULT;

FRESULT f_open(FIL*, const char*, BYTE);			/* Open or create a file */
FRESULT f_read(FIL*, void*, UINT, UINT*);			/* Read data from a file */
FRESULT f_write(FIL*, const void*, UINT, UINT*);	/* Write data to a file */
FRESULT f_lseek(FIL*, DWORD);						/* Move file pointer of a file object */
FRESULT f_close(FIL*);								/* Close an open file object */
FRESULT f_read_m(
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	char datatype,
	UINT *br		/* Pointer to number of bytes read */
	);
#endif
