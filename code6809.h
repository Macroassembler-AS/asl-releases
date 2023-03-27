#ifndef _CODE6809_H
#define _CODE6809_H
/* code6809.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 6809/6309                                                   */
/*                                                                           */
/* Historie: 10.10.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern void code6809_init(void);

/* remaining stuff used by codeko09.c: */

extern void DecodeStack_6809(Word code);

typedef Boolean (*reg_decoder_6809_t)(const char *, Byte *);
extern void DecodeTFR_TFM_EXG_6809(Word code, Boolean allow_inc_dec, reg_decoder_6809_t reg_decoder, Boolean reverse);

#endif /* _CODE6809_H */
