/* symbolsize.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Macro Assembler AS                                                        */
/*                                                                           */
/* Definition of a symbol's/instruction's operand type & size                */
/*                                                                           */
/*****************************************************************************/

#include <string.h>

#include "symbolsize.h"

typedef struct
{
  tSymbolSize Size;
  const char Name[5];
  unsigned Bytes;
} tSizeDescr;

static const tSizeDescr Descrs[] =
{
  { eSymbolSizeUnknown       , "?"   ,  0 },
  { eSymbolSize8Bit          , "I8"  ,  1 },
  { eSymbolSize16Bit         , "I16" ,  2 },
  { eSymbolSize32Bit         , "I32" ,  4 },
  { eSymbolSize64Bit         , "I64" ,  8 },
  { eSymbolSize80Bit         , "F80" , 10 },
  { eSymbolSizeFloat32Bit    , "F32" ,  4 },
  { eSymbolSizeFloat64Bit    , "F64" ,  8 },
  { eSymbolSizeFloat96Bit    , "F96" , 12 },
  { eSymbolSize24Bit         , "I24" ,  3 },
  { eSymbolSizeFloatDec96Bit , "D96" , 12 },
  { eSymbolSize48Bit         , "I48" ,  6 },
  { eSymbolSizeFloat48Bit    , "F48" ,  6 },
  { eSymbolSize128Bit        , "I128", 16 },
  { eSymbolSizeUnknown       , ""    ,  0 },
};

/*!------------------------------------------------------------------------
 * \fn     GetSymbolSizeName(tSymbolSize Size)
 * \brief  retrieve human-readable name of symbol size
 * \param  Size size to query
 * \return name
 * ------------------------------------------------------------------------ */

const char *GetSymbolSizeName(tSymbolSize Size)
{
  const tSizeDescr *pDescr;

  for (pDescr = Descrs; pDescr->Name[0]; pDescr++)
    if (pDescr->Size == Size)
      return pDescr->Name;
  return "?";
}

/*!------------------------------------------------------------------------
 * \fn     GetSymbolSizeBytes(tSymbolSize Size)
 * \brief  retrieve # of bytes of symbol size
 * \param  Size size to query
 * \return # of bytes
 * ------------------------------------------------------------------------ */

unsigned GetSymbolSizeBytes(tSymbolSize Size)
{
  const tSizeDescr *pDescr;

  for (pDescr = Descrs; pDescr->Name[0]; pDescr++)
    if (pDescr->Size == Size)
      return pDescr->Bytes;
  return 0;
}
