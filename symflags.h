#ifndef _SYMFLAGS_H
#define _SYMFLAGS_H
/* symflags.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Symbol Flags Used in Symbol Table and TempResult                          */
/*                                                                           */
/*****************************************************************************/

typedef enum eSymbolFlags
{
  eSymbolFlag_None = 0,
  eSymbolFlag_NextLabelAfterBSR = 1 << 0,
  eSymbolFlag_StringSingleQuoted = 1 << 1,

  /* Hinweisflag: evtl. im ersten Pass unbe-
     kanntes Symbol, Ausdruck nicht ausgewertet */

  eSymbolFlag_FirstPassUnknown = 1 << 2,

  /* Hinweisflag:  Dadurch, dass Phasenfehler
     aufgetreten sind, ist dieser Symbolwert evtl.
     nicht mehr aktuell */

  eSymbolFlag_Questionable = 1 << 3,

  /* Hinweisflag: benutzt Vorwaertsdefinitionen */

  eSymbolFlag_UsesForwards = 1 << 4,

  eSymbolFlag_Label = 1 << 5,

  eSymbolFlags_Promotable = eSymbolFlag_FirstPassUnknown | eSymbolFlag_Questionable | eSymbolFlag_UsesForwards
} tSymbolFlags;

#ifdef __cplusplus
#include "cppops.h"
DefCPPOps_Mask(tSymbolFlags)
#endif

#define mFirstPassUnknown(Flags) (!!((Flags) & eSymbolFlag_FirstPassUnknown))

#define mSymbolQuestionable(Flags) (!!((Flags) & eSymbolFlag_Questionable))

#define mFirstPassUnknownOrQuestionable(Flags) (!!((Flags) & (eSymbolFlag_FirstPassUnknown | eSymbolFlag_Questionable)))

#define mUsesForwards(Flags) (!!((Flags) & eSymbolFlag_UsesForwards))

#endif /* _SYMFLAGS_H */
