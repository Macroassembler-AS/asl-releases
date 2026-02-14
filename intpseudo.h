#ifndef _INTPSEUDO_H
#define _INTPSEUDO_H
/* intpseudo.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Commonly used 'Intel Style' Pseudo Instructions                           */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef enum
{
  eSyntaxNeither = 0,
  eSyntax808x = 1,
  eSyntaxZ80 = 2,
  eSyntaxBoth = 3
} tZ80Syntax;

extern tZ80Syntax CurrZ80Syntax;

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

typedef enum
{
  eIntPseudoFlag_None = 0,
  eIntPseudoFlag_EndianMask = 3 << 0,
  eIntPseudoFlag_NoEndian = 0 << 0,
  eIntPseudoFlag_LittleEndian = 1 << 0,
  eIntPseudoFlag_BigEndian = 2 << 0,
  eIntPseudoFlag_DynEndian = 3 << 0,
  eIntPseudoFlag_AllowInt = 1 << 2,
  eIntPseudoFlag_AllowFloat = 1 << 3,
  eIntPseudoFlag_AllowString = 1 << 4,
  eIntPseudoFlag_DECFormat = 1 << 5,
  eIntPseudoFlag_Turn = 1 << 6,
  eIntPseudoFlag_DECGFormat = 1 << 7,
  eIntPseudoFlag_DECFormats = eIntPseudoFlag_DECFormat | eIntPseudoFlag_DECGFormat,
  eIntPseudoFlag_ASCIZ = 1 << 8,
  eIntPseudoFlag_ASCIC = 1 << 9,
  eIntPseudoFlag_ASCID = eIntPseudoFlag_ASCIZ | eIntPseudoFlag_ASCIC,
  eIntPseudoFlag_ASCIAll = eIntPseudoFlag_ASCIZ | eIntPseudoFlag_ASCIC,
  eIntPseudoFlag_MotoRep = 1 << 10,
  eIntPseudoFlag_TMS340Format = 1 << 11
} int_pseudo_flags_t;

#ifdef __cplusplus
#include "intpseudo.hpp"
#endif

struct sInstTable;

extern void DecodeIntelD1(Word Flags);
extern void DecodeIntelDN(Word Flags);
extern void DecodeIntelDB(Word Flags);
extern void DecodeIntelDW(Word Flags);
extern void DecodeIntelDD(Word Flags);
extern void DecodeIntelDM(Word Flags);
extern void DecodeIntelDQ(Word Flags);
extern void DecodeIntelDT(Word Flags);
extern void DecodeIntelDO(Word Flags);
extern void DecodeIntelDS(Word Flags);

extern void AddIntelPseudo(struct sInstTable *p_inst_table, int_pseudo_flags_t flags);

extern void AddZ80Syntax(struct sInstTable *InstTable);

extern Boolean ChkZ80Syntax(tZ80Syntax InstrSyntax);

#endif /* _INTPSEUDO_H */
