#ifndef _INTEGER_H
#define _INTEGER_H

typedef float			float32_t;
typedef float			float_t;

typedef int				int32_t;
typedef unsigned int	uint32_t;

typedef short			int16_t;
typedef unsigned short	uint16_t;

typedef char			int8_t;
typedef unsigned char   uint8_t;


typedef int q31_t;
typedef short q15_t;


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

/* Boolean type */
typedef enum { FALSE = 0, TRUE } BOOL;


#endif


