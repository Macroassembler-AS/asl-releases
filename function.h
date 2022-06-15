#ifndef _FUNCTION_H
#define _FUNCTION_H
/* function.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* built-in (non-user-defined) functions                                     */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>
#include "datatypes.h"
#include "tempresult.h"

#define AS_FARG_MInt (1 << TempInt)
#define AS_FARG_MFloat (1 << TempFloat)
#define AS_FARG_MString (1 << TempString)
#define AS_FARG_MAll (AS_FARG_MInt | AS_FARG_MFloat | AS_FARG_MString)

typedef struct
{
  const char *pName;
  Byte MinNumArgs, MaxNumArgs;
  Byte ArgTypes[3];
  Boolean (*pFunc)(TempResult *pErg, const TempResult *pArgs, unsigned ArgCnt);
} tFunction;

extern const tFunction *function_find(const char *p_name);

#endif /* _FUNCTION_H */
