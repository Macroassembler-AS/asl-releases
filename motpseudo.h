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

enum
{
 e_moto_8_le = 0 << 0,
 e_moto_8_be = 1 << 0,
 e_moto_8_db = 1 << 1,
 e_moto_8_dw = 1 << 2,
 e_moto_8_ds = 1 << 3,
 e_moto_8_ddb = 1 << 4,
 e_moto_8_dcm = 1 << 5
};

struct sInstTable;
extern void init_moto8_pseudo(struct sInstTable *p_inst_table, unsigned moto8_flags);
extern Boolean decode_moto8_pseudo(void);
extern void deinit_moto8_pseudo(void);

extern void ConvertMotoFloatDec(Double F, Byte *pDest, Boolean NeedsBig);

extern void AddMoto16PseudoONOFF(Boolean default_paddding_value);

extern void DecodeMotoDC(tSymbolSize OpSize, Boolean Turn);

extern Boolean DecodeMoto16Pseudo(tSymbolSize OpSize, Boolean BigEndian);

extern Boolean DecodeMoto16AttrSize(char SizeSpec, tSymbolSize *pResult, Boolean Allow24);

extern Boolean DecodeMoto16AttrSizeStr(const struct sStrComp *pSizeSpec, tSymbolSize *pResult, Boolean Allow24);

#endif /* _MOTPSEUDO_H */
