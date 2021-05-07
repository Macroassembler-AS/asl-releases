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

enum
{
  eIntPseudoFlag_None = 0,
  eIntPseudoFlag_BigEndian = 1 << 0,
  eIntPseudoFlag_AllowInt = 1 << 1,
  eIntPseudoFlag_AllowFloat = 1 << 2
};

extern void DecodeIntelDN(Word Flags);
extern void DecodeIntelDB(Word Flags);
extern void DecodeIntelDW(Word Flags);
extern void DecodeIntelDD(Word Flags);
extern void DecodeIntelDQ(Word Flags);
extern void DecodeIntelDT(Word Flags);

extern Boolean DecodeIntelPseudo(Boolean BigEndian);

extern void DecodeZ80SYNTAX(Word Code);

extern Boolean ChkZ80Syntax(tZ80Syntax InstrSyntax);

extern void intpseudo_init(void);

#endif /* _INTPSEUDO_H */
