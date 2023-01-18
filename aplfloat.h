#ifndef _APLFLOAT_H
#define _APLFLOAT_H
/* aplfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* APPLE<->IEEE Floating Point Conversion on host                            */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern int Double_2_apl4(Double inp, Word *p_dest);

struct sStrComp;
extern Boolean check_apl_fp_dispose_result(int ret, const struct sStrComp *p_arg);

#endif /* _APLFLOAT_H */
