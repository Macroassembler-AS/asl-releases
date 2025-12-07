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

extern void DecodeMotoBYT(Word flags);
extern void DecodeMotoADR(Word flags);
extern void DecodeMotoDFS(Word flags);

typedef enum
{
 e_moto_pseudo_flags_none = 0,
 e_moto_pseudo_flags_le = 0 << 0,
 e_moto_pseudo_flags_be = 1 << 0,
#if 0
 e_moto_pseudo_flags_db = 1 << 1,
 e_moto_pseudo_flags_dw = 1 << 2,
#endif
 e_moto_pseudo_flags_ds = 1 << 3,
 e_moto_pseudo_flags_ddb = 1 << 4,
 e_moto_pseudo_flags_dcm = 1 << 5
} moto_pseudo_flags_t;

#ifdef __cplusplus
#include "motpseudo.hpp"
#endif

struct sInstTable;
extern void add_moto8_pseudo(struct sInstTable *p_inst_table, moto_pseudo_flags_t flags);

extern int ConvertMotoFloatDec(as_float_t F, Byte *pDest, Boolean NeedsBig);

extern void AddMoto16PseudoONOFF(Boolean default_paddding_value);

extern void DecodeMotoDC(Word flags);
extern void DecodeMotoDS(Word flags);

extern void AddMoto16Pseudo(struct sInstTable *p_inst_table, moto_pseudo_flags_t flags);

extern Boolean DecodeMoto16AttrSize(char SizeSpec, tSymbolSize *pResult, Boolean Allow24);

extern Boolean DecodeMoto16AttrSizeStr(const struct sStrComp *pSizeSpec, tSymbolSize *pResult, Boolean Allow24);

extern void mot_64_to_16(Word *p_dest, LargeWord src, Boolean big_endian);

#endif /* _MOTPSEUDO_H */
