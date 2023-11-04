#ifndef _MATH64_H
#define _MATH64_H
/* math64.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* 64 bit arithmetic for platforms not having a 64 bit integer               */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef struct
{
  LongWord low;
  LongWord high;
} t64;

extern void add64(t64 *pRes, const t64 *pA, const t64 *pB);

extern void sub64(t64 *pRes, const t64 *pA, const t64 *pB);

extern void mul64(t64 *pRes, const t64 *pA, const t64 *pB);

extern void div64(t64 *pRes, const t64 *pA, const t64 *pB);
extern void mod64(t64 *pRes, const t64 *pA, const t64 *pB);
extern void mod_div64(t64 *pQuot, t64 *pRem, const t64 *pA, const t64 *pB);

#endif /* _MATH64_H */
