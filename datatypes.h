#ifndef _DATATYPES_H
#define _DATATYPES_H
/* datatypes.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* define some handy types & constants                                       */
/*                                                                           */
/* History:  2001-10-13 /AArnold - created this comment                      */
/*                                                                           */
/*****************************************************************************/

#include "sysdefs.h"

typedef as_uint8_t Byte;       /* Integertypen */
typedef as_int8_t ShortInt;

#ifdef HAS16
typedef as_uint16_t Word;
typedef as_int16_t Integer;
#endif

typedef as_uint32_t LongWord;
typedef as_int32_t LongInt;
#define PRILongInt PRIas_int32_t
#define MaxLongInt 2147483647

#ifdef HAS64
typedef as_uint64_t QuadWord;
typedef as_int64_t QuadInt;
#endif

#ifdef HAS64
typedef QuadInt LargeInt;
typedef QuadWord LargeWord;
#define LARGEBITS 64
#else
typedef LongInt LargeInt;
typedef LongWord LargeWord;
#define LARGEBITS 32
#endif

typedef signed int sint;
typedef unsigned int usint;

typedef Byte Boolean;

#ifndef STRINGSIZE
# define STRINGSIZE 256
#endif
#define SHORTSTRINGSIZE 65

typedef char String[STRINGSIZE];
typedef char ShortString[SHORTSTRINGSIZE];

#ifndef TRUE
#define TRUE 1
#endif
#ifndef True
#define True 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef False
#define False 0
#endif

#endif /* _DATATYPES_H */
