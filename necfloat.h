#ifndef _NECFLOAT_H
#define _NECFLOAT_H
/* necfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* NEC<->IEEE Floating Point Conversion on host                              */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern int as_float_2_nec_4(as_float_t inp, LongWord *p_dest);

#endif /* _NECFLOAT_H */
