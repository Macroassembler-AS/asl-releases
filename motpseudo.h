#ifndef _MOTPSEUDO_H
#define _MOTPSEUDO_H
/* motpseudo.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Haeufiger benutzte Motorola-Pseudo-Befehle                                */
/*                                                                           */
/*****************************************************************************/

#include "symbolsize.h"

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

struct sStrComp;

extern void DecodeMotoBYT(Word Code);
extern void DecodeMotoADR(Word Code);
extern void DecodeMotoDFS(Word Code);

extern Boolean DecodeMotoPseudo(Boolean Turn);

extern void ConvertMotoFloatDec(Double F, Byte *pDest, Boolean NeedsBig);

extern void AddMoto16PseudoONOFF(void);

extern void DecodeMotoDC(tSymbolSize OpSize, Boolean Turn);

extern Boolean DecodeMoto16Pseudo(tSymbolSize OpSize, Boolean Turn);

extern Boolean DecodeMoto16AttrSize(char SizeSpec, tSymbolSize *pResult, Boolean Allow24);

extern Boolean DecodeMoto16AttrSizeStr(const struct sStrComp *pSizeSpec, tSymbolSize *pResult, Boolean Allow24);

#endif /* _MOTPSEUDO_H */
