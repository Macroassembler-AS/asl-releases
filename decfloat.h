#ifndef _DECFLOAT_H
#define _DECFLOAT_H
/* decfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* DEC<->IEEE Floating Point Conversion on host                              */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern void DECF_2_Single(Byte *pDest, float inp);

extern void DECD_2_Double(Byte *pDest, as_float_t inp);

extern void DECD_2_LongDouble(Byte *pDest, as_float_t inp);

extern int as_float_2_dec_lit(as_float_t inp, Byte *p_dest);

extern int as_float_2_dec_f(as_float_t inp, Word *p_dest);

extern int as_float_2_dec_d(as_float_t inp, Word *p_dest);

extern int as_float_2_dec_g(as_float_t inp, Word *p_dest);

extern int as_float_2_dec_h(as_float_t inp, Word *p_dest);

#endif /* _DECFLOAT_H */
