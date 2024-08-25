#ifndef _TIFLOAT_H
#define _TIFLOAT_H
/* tifloat.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* IEEE -> TI DSP Floating Point Conversion on host                          */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern int as_float_2_ti2(as_float_t inp, Word *p_dest);

extern int as_float_2_ti4(as_float_t inp, LongWord *p_dest);

extern int as_float_2_ti5(as_float_t Inp, LongWord *p_dest_l, LongWord *p_dest_h);

#endif /* _TIFLOAT_H */
