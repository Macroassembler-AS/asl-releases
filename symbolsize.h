#ifndef _SYMBOLSIZE_H
#define _SYMBOLSIZE_H
/* symbolsize.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Macro Assembler AS                                                        */
/*                                                                           */
/* Definition of a symbol's/instruction's operand type & size                */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef enum
{
  eSymbolSizeUnknown = -1,
  eSymbolSize8Bit = 0,
  eSymbolSize16Bit = 1,
  eSymbolSize32Bit = 2,
  eSymbolSize64Bit = 3,
  eSymbolSize80Bit = 4, /* Intel 80 Bit extended float */
  eSymbolSizeFloat32Bit = 5, /* IEEE Single Precision, DEC F, IBM Single Precision */
  eSymbolSizeFloat64Bit = 6, /* IEEE Double Precision, DEC D, IBM Double Precision */
  eSymbolSizeFloat96Bit = 7, /* Motorola Extended Precision */
  eSymbolSize24Bit = 8,
  eSymbolSizeFloatDec96Bit = 9,
  eSymbolSizeFloat16Bit = 10,
  eSymbolSize12Bit = 11,
  eSymbolSize48Bit = 12,
  eSymbolSizeFloat48Bit = 13,
  eSymbolSize128Bit = 14,
  eSymbolSizeFloat128Bit = 15, /* DEC H Floating Point */
  eSymbolSizeFloat64Bit_G = 16 /* DEC G Floating Point */
} tSymbolSize;

#ifdef __cplusplus
#include "cppops.h"
DefCPPOps_Enum(tSymbolSize)
#endif

extern const char *GetSymbolSizeName(tSymbolSize Size);

extern unsigned GetSymbolSizeBytes(tSymbolSize Size);

extern Boolean is_symbol_size_float(tSymbolSize size);

#endif /* _SYMBOLSIZE_H */
