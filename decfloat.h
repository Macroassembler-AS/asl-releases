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

extern void DECD_2_Double(Byte *pDest, Double inp);

extern void DECD_2_LongDouble(Byte *pDest, Double inp);

extern int Double_2_dec4(Double inp, Word *p_dest);

extern int Double_2_dec8(Double inp, Word *p_dest);

struct sStrComp;
extern Boolean check_dec_fp_dispose_result(int ret, const struct sStrComp *p_arg);

#endif /* _DECFLOAT_H */
