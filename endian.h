#ifndef _MYENDIAN_H
#define _MYENDIAN_H
/* endian.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Little/Big-Endian-Routinen                                                */
/*                                                                           */
/* Historie: 30. 5.1996 Grundsteinlegung                                     */
/*            6. 7.1997 Dec32BlankString dazu                                */
/*            1. 6.2000 added LargeHIntFormat                                */
/*            7. 7.2000 added memory read/write functions                    */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern Boolean HostBigEndian;

extern const char *Integ16Format, *Integ32Format, *Integ64Format;
extern const char *IntegerFormat, *LongIntFormat, *QuadIntFormat;
extern const char *LargeIntFormat, *LargeHIntFormat;


extern void WSwap(void *Field, int Cnt);

extern void DSwap(void *Field, int Cnt);

extern void QSwap(void *Field, int Cnt);

extern void TSwap(void *Field, int Cnt);

extern void DWSwap(void *Field, int Cnt);

extern void QWSwap(void *Field, int Cnt);

extern void TWSwap(void *Field, int Cnt);


extern Boolean Read2(FILE *file, void *Ptr);

extern Boolean Read4(FILE *file, void *Ptr);

extern Boolean Read8(FILE *file, void *Ptr);


extern Boolean Write2(FILE *file, void *Ptr);

extern Boolean Write4(FILE *file, void *Ptr);

extern Boolean Write8(FILE *file, void *Ptr);

#define MRead1L(Buffer) (*((Byte *)(Buffer)))

#define MRead1B(Buffer) (*((Byte *)(Buffer)))

extern Word MRead2L(Byte *Buffer);

extern Word MRead2B(Byte *Buffer);

#define MWrite1L(Buffer, Value) (*((Byte*) (Buffer))) = Value;

#define MWrite1B(Buffer, Value) (*((Byte*) (Buffer))) = Value;

extern void MWrite2L(Byte *Buffer, Word Value);

extern void MWrite2B(Byte *Buffer, Word Value);

extern LongWord MRead4L(Byte *Buffer);

extern LongWord MRead4B(Byte *Buffer);

extern void MWrite4L(Byte *Buffer, LongWord Value);

extern void MWrite4B(Byte *Buffer, LongWord Value);

#ifdef HAS64
extern QuadWord MRead8L(Byte *Buffer);

extern QuadWord MRead8B(Byte *Buffer);

extern void MWrite8L(Byte *Buffer, QuadWord Value);

extern void MWrite8B(Byte *Buffer, QuadWord Value);
#endif

extern void endian_init(void);
#endif /* _MYENDIAN_H */
