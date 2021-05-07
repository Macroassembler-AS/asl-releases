/* endian.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Little/Big-Endian-Routinen                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "endian.h"

/*****************************************************************************/

Boolean HostBigEndian;

const char *Integ16Format, *Integ32Format, *Integ64Format;
const char *IntegerFormat, *LongIntFormat, *QuadIntFormat;
const char *LargeIntFormat, *LargeHIntFormat;

/*****************************************************************************/

#define SWAP(x,y) \
do \
{ \
  Swap = x; x = y; y = Swap; \
} \
while (0)

void WSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 2; z++, Run += 2)
  {
    SWAP(Run[0], Run[1]);
  }
}

void DSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 4; z++, Run += 4)
  {
    SWAP(Run[0], Run[3]);
    SWAP(Run[1], Run[2]);
  }
}

void QSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 8; z++, Run += 8)
  {
    SWAP(Run[0], Run[7]);
    SWAP(Run[1], Run[6]);
    SWAP(Run[2], Run[5]);
    SWAP(Run[3], Run[4]);
  }
}

void TSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 10; z++, Run += 10)
  {
    SWAP(Run[0], Run[9]);
    SWAP(Run[1], Run[8]);
    SWAP(Run[2], Run[7]);
    SWAP(Run[3], Run[6]);
    SWAP(Run[4], Run[5]);
  }
}

void DWSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 4; z++, Run += 4)
  {
    SWAP(Run[0], Run[2]);
    SWAP(Run[1], Run[3]);
  }
}

void QWSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 8; z++, Run += 8)
  {
    SWAP(Run[0], Run[6]);
    SWAP(Run[1], Run[7]);
    SWAP(Run[2], Run[4]);
    SWAP(Run[3], Run[5]);
  }
}

void TWSwap(void *Field, int Cnt)
{
  register unsigned char *Run = (unsigned char *) Field, Swap;
  register int z;

  for (z = 0; z < Cnt / 10; z++, Run += 10)
  {
    SWAP(Run[0], Run[8]);
    SWAP(Run[1], Run[9]);
    SWAP(Run[2], Run[6]);
    SWAP(Run[3], Run[7]);
    /* center word needs not be swapped with itself */
  }
}

Boolean Read2(FILE *file, void *Ptr)
{
  if (fread(Ptr, 1, 2, file) != 2)
    return False;
  if (HostBigEndian)
    WSwap(Ptr, 2);
  return True;
}

Boolean Read4(FILE *file, void *Ptr)
{
  if (fread(Ptr, 1, 4, file) != 4)
    return False;
  if (HostBigEndian)
    DSwap(Ptr, 4);
  return True;
}

Boolean Read8(FILE *file, void *Ptr)
{
  if (fread(Ptr, 1, 8, file) != 8)
    return False;
  if (HostBigEndian)
    QSwap(Ptr, 8);
  return True;
}


Boolean Write2(FILE *file, void *Ptr)
{
  Boolean OK;

  if (HostBigEndian)
    WSwap(Ptr, 2);
  OK = (fwrite(Ptr, 1, 2, file) == 2);
  if (HostBigEndian)
    WSwap(Ptr, 2);
  return OK;
}

Boolean Write4(FILE *file, void *Ptr)
{
  Boolean OK;

  if (HostBigEndian)
    DSwap(Ptr, 4);
  OK = (fwrite(Ptr, 1, 4, file) == 4);
  if (HostBigEndian)
    DSwap(Ptr, 4);
  return OK;
}

Boolean Write8(FILE *file, void *Ptr)
{
  Boolean OK;

  if (HostBigEndian)
    QSwap(Ptr, 8);
  OK = (fwrite(Ptr, 1, 8, file) == 8);
  if (HostBigEndian)
    QSwap(Ptr, 8);
  return OK;
}


Word MRead2L(Byte *Buffer)
{
  return (((Word) Buffer[1]) << 8) | Buffer[0];
}

Word MRead2B(Byte *Buffer)
{
  return (((Word) Buffer[0]) << 8) | Buffer[1];
}

void MWrite2L(Byte *Buffer, Word Value)
{
  Buffer[0] = Value & 0xff;
  Buffer[1] = (Value >> 8) & 0xff;
}

void MWrite2B(Byte *Buffer, Word Value)
{
  Buffer[1] = Value & 0xff;
  Buffer[0] = (Value >> 8) & 0xff;
}

LongWord MRead4L(Byte *Buffer)
{
  return (((LongWord) Buffer[3]) << 24) |
         (((LongWord) Buffer[2]) << 16) |
         (((LongWord) Buffer[1]) << 8)  | Buffer[0];
}

LongWord MRead4B(Byte *Buffer)
{
  return (((LongWord) Buffer[0]) << 24) |
         (((LongWord) Buffer[1]) << 16) |
         (((LongWord) Buffer[2]) << 8) | Buffer[3];
}

void MWrite4L(Byte *Buffer, LongWord Value)
{
  Buffer[0] = Value & 0xff;
  Buffer[1] = (Value >> 8) & 0xff;
  Buffer[2] = (Value >> 16) & 0xff;
  Buffer[3] = (Value >> 24) & 0xff;
}

void MWrite4B(Byte *Buffer, LongWord Value)
{
  Buffer[3] = Value & 0xff;
  Buffer[2] = (Value >> 8) & 0xff;
  Buffer[1] = (Value >> 16) & 0xff;
  Buffer[0] = (Value >> 24) & 0xff;
}

#ifdef HAS64
QuadWord MRead8L(Byte *Buffer)
{
  return (((LargeWord) Buffer[7]) << 56) |
         (((LargeWord) Buffer[6]) << 48) |
         (((LargeWord) Buffer[5]) << 40) |
         (((LargeWord) Buffer[4]) << 32) |
         (((LargeWord) Buffer[3]) << 24) |
         (((LargeWord) Buffer[2]) << 16) |
         (((LargeWord) Buffer[1]) << 8)  |
                       Buffer[0];
}

QuadWord MRead8B(Byte *Buffer)
{
  return (((LargeWord) Buffer[0]) << 56) |
         (((LargeWord) Buffer[1]) << 48) |
         (((LargeWord) Buffer[2]) << 40) |
         (((LargeWord) Buffer[3]) << 32) |
         (((LargeWord) Buffer[4]) << 24) |
         (((LargeWord) Buffer[7]) << 16) |
         (((LargeWord) Buffer[6]) << 8)  |
                       Buffer[7];
}

void MWrite8L(Byte *Buffer, QuadWord Value)
{
  Buffer[0] = Value & 0xff;
  Buffer[1] = (Value >> 8) & 0xff;
  Buffer[2] = (Value >> 16) & 0xff;
  Buffer[3] = (Value >> 24) & 0xff;
  Buffer[4] = (Value >> 32) & 0xff;
  Buffer[5] = (Value >> 40) & 0xff;
  Buffer[6] = (Value >> 48) & 0xff;
  Buffer[7] = (Value >> 56) & 0xff;
}

void MWrite8B(Byte *Buffer, QuadWord Value)
{
  Buffer[7] = Value & 0xff;
  Buffer[6] = (Value >> 8) & 0xff;
  Buffer[5] = (Value >> 16) & 0xff;
  Buffer[4] = (Value >> 24) & 0xff;
  Buffer[3] = (Value >> 32) & 0xff;
  Buffer[2] = (Value >> 40) & 0xff;
  Buffer[1] = (Value >> 48) & 0xff;
  Buffer[0] = (Value >> 56) & 0xff;
}
#endif


static void CheckSingle(int Is, int Should, const char *Name)
{
  if (Is != Should)
  {
    fprintf(stderr, "Configuration error: Sizeof(%s) is %d, should be %d\n",
            Name, Is, Should);
    exit(255);
  }
}

static void CheckDataTypes(void)
{
  int intsize = sizeof(int);

  if (intsize < 2)
  {
    fprintf(stderr, "Configuration error: Sizeof(int) is %d, should be >=2\n",
            (int) sizeof(int));
    exit(255);
  }
  CheckSingle(sizeof(Byte),    1, "Byte");
  CheckSingle(sizeof(ShortInt), 1, "ShortInt");
#ifdef HAS16
  CheckSingle(sizeof(Word),    2, "Word");
  CheckSingle(sizeof(Integer), 2, "Integer");
#endif
  CheckSingle(sizeof(LongInt), 4, "LongInt");
  CheckSingle(sizeof(LongWord), 4, "LongWord");
#ifdef HAS64
  CheckSingle(sizeof(QuadInt), 8, "QuadInt");
  CheckSingle(sizeof(QuadWord), 8, "QuadWord");
#endif
  CheckSingle(sizeof(Single),  4, "Single");
  CheckSingle(sizeof(Double),  8, "Double");
}


static const char *AssignSingle(int size)
{
  if (size == sizeof(short))
    return "%d";
  else if (size == sizeof(int))
    return "%d";
  else if (size == sizeof(long))
    return "%ld";
#ifndef NOLONGLONG
  else if (size == sizeof(long long))
    return "%lld";
#endif
  else
  {
    fprintf(stderr,
            "Configuration error: cannot assign format string for integer of size %d\n", size);
    exit(255);
    return "";
  }
}

static const char *AssignHSingle(int size)
{
  if (size == sizeof(short))
    return "%x";
  else if (size == sizeof(int))
    return "%x";
  else if (size == sizeof(long))
    return "%lx";
#ifndef NOLONGLONG
  else if (size == sizeof(long long))
    return "%llx";
#endif
  else
  {
    fprintf(stderr,
            "Configuration error: cannot assign format string for integer of size %d\n", size);
    exit(255);
    return "";
  }
}

static void AssignFormats(void)
{
#ifdef HAS16
  IntegerFormat = Integ16Format = AssignSingle(2);
#endif
  LongIntFormat = Integ32Format = AssignSingle(4);
#ifdef HAS64
  QuadIntFormat = Integ64Format = AssignSingle(8);
#endif
  LargeIntFormat = AssignSingle(sizeof(LargeInt));
  LargeHIntFormat = AssignHSingle(sizeof(LargeInt));
}

void endian_init(void)
{
  union
  {
    unsigned char field[sizeof(int)];
    int test;
  } TwoFace;

  CheckDataTypes();
  AssignFormats();

  memset(TwoFace.field, 0, sizeof(int));
  TwoFace.field[0] = 1;
  HostBigEndian = ((TwoFace.test) != 1);
}
