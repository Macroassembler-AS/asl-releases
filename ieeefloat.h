#ifndef _IEEEFLOAT_H
#define _IEEEFLOAT_H
/* ieeefloat.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* IEEE Floating Point Handling                                              */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern int as_float_2_ieee2(as_float_t inp, Byte *pDest, Boolean NeedsBig);

extern int as_float_2_ieee4(as_float_t inp, Byte *pDest, Boolean NeedsBig);

extern int as_float_2_ieee8(as_float_t inp, Byte *pDest, Boolean NeedsBig);

extern int as_float_2_ieee10(as_float_t inp, Byte *pDest, Boolean NeedsBig);

extern int as_float_2_ieee16(as_float_t inp, Byte *pDest, Boolean NeedsBig);

#endif /* _IEEEFLOAT_H */
