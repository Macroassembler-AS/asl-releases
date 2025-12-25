/* intpseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Commonly Used Intel-Style Pseudo Instructions                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "bpemu.h"
#include "be_le.h"
#include "strutil.h"
#include "nls.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmcode.h"
#include "asmpars.h"
#include "asmitree.h"
#include "onoff_common.h"
#include "chartrans.h"
#include "errmsg.h"
#include "ieeefloat.h"
#include "decfloat.h"

#include "intpseudo.h"

#define LEAVE goto func_exit

/*****************************************************************************
 * Local Types
 *****************************************************************************/

struct sLayoutCtx;

typedef Boolean (*TLayoutFunc)(
#ifdef __PROTOS__
                               const tStrComp *pArg, struct sLayoutCtx *pCtx
#endif
                               );

typedef enum
{
  DSNone, DSConstant, DSSpace
} tDSFlag;

struct sCurrCodeFill
{
  LongInt FullWordCnt;
  int LastWordFill;
};
typedef struct sCurrCodeFill tCurrCodeFill;

struct sLayoutCtx
{
  tDSFlag DSFlag;
  int_pseudo_flags_t flags;
  TLayoutFunc LayoutFunc;
  int BaseElemLenBits, FullWordSize, ElemsPerFullWord, ListGran;
  Boolean (*Put1I)(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put4I)(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put8I)(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put16I)(Word w, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put16F)(as_float_t f, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put32I)(LongWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put32F)(as_float_t f, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put48I)(LargeWord q, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put48F)(as_float_t f, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put64I)(LargeWord q, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put64F)(as_float_t f, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put80I)(LargeWord t, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx);
  Boolean (*Put80F)(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Put128I)(LargeWord q, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx);
  Boolean (*Put128F)(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx);
  Boolean (*Replicate)(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx);
  tCurrCodeFill CurrCodeFill, FillIncPerElem;
  const tStrComp *pCurrComp;
  int LoHiMap;
};
typedef struct sLayoutCtx tLayoutCtx;

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

static char Z80SyntaxName[] = "Z80SYNTAX";
tZ80Syntax CurrZ80Syntax;

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

void _DumpCodeFill(const char *pTitle, const tCurrCodeFill *pFill)
{
  fprintf(stderr, "%s %u %d\n", pTitle, (unsigned)pFill->FullWordCnt, pFill->LastWordFill);
}

/*!------------------------------------------------------------------------
 * \fn     Boolean SetDSFlag(struct sLayoutCtx *pCtx, tDSFlag Flag)
 * \brief  check set data disposition/reservation flag in context
 * \param  pCtx context
 * \param  Flag operation to be set
 * \return True if operation could be set or was alreday set
 * ------------------------------------------------------------------------ */

static Boolean SetDSFlag(struct sLayoutCtx *pCtx, tDSFlag Flag)
{
  if ((pCtx->DSFlag != DSNone) && (pCtx->DSFlag != Flag))
  {
    WrStrErrorPos(ErrNum_MixDBDS, pCtx->pCurrComp);
    return False;
  }
  pCtx->DSFlag = Flag;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     IncMaxCodeLen(struct sLayoutCtx *pCtx, LongWord NumFullWords)
 * \brief  assure xAsmCode has space for at least n more full words
 * \param  pCtxcontext
 * \param  NumFullWords # of additional words intended to write
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean IncMaxCodeLen(struct sLayoutCtx *pCtx, LongWord NumFullWords)
{
  if (SetMaxCodeLen((pCtx->CurrCodeFill.FullWordCnt + NumFullWords) * pCtx->FullWordSize))
  {
    WrStrErrorPos(ErrNum_CodeOverflow, pCtx->pCurrComp);
    return False;
  }
  else
    return True;
}

static Byte BitInNibble(Byte n, int Pos)
{
  return (n & 1) << Pos;
}

static Byte BitInByte(Byte n, int Pos)
{
  return (n & 1) << Pos;
}

static Word BitInWord(Byte n, int Pos)
{
  return (Word)(n & 1) << Pos;
}

static LongWord BitInDWord(Byte n, int Pos)
{
  return (LongWord)(n & 1) << Pos;
}

static Byte NibbleInByte(Byte n, int Pos)
{
  return (n & 15) << (Pos << 2);
}

static Word NibbleInWord(Byte n, int Pos)
{
  return ((Word)(n & 15)) << (Pos << 2);
}

static LongWord NibbleInDWord(Byte n, int Pos)
{
  return ((LongWord)(n & 15)) << (Pos << 2);
}

static Word ByteInWord(Byte b, int Pos)
{
  return ((Word)b) << (Pos << 3);
}

static LongWord ByteInDWord(Byte b, int Pos)
{
  return ((LongWord)b) << (Pos << 3);
}

static LongWord WordInDWord(Word w, int Pos)
{
  return ((LongWord)w) << (Pos << 4);
}

static Byte BitFromNibble(Byte b, int Pos)
{
  return (b >> Pos) & 0x01;
}

static Byte BitFromByte(Byte b, int Pos)
{
  return (b >> Pos) & 0x01;
}

static Byte BitFromWord(Word w, int Pos)
{
  return (w >> Pos) & 0x01;
}

static Byte BitFromDWord(LongWord l, int Pos)
{
  return (l >> Pos) & 0x01;
}

static Byte NibbleFromByte(Byte b, int Pos)
{
  return (b >> (Pos << 2)) & 0x0f;
}

static Byte NibbleFromWord(Word w, int Pos)
{
  return (w >> (Pos << 2)) & 0x0f;
}

static Byte NibbleFromDWord(LongWord d, int Pos)
{
  return (d >> (Pos << 2)) & 0x0f;
}

static Byte ByteFromWord(Word w, int Pos)
{
  return (w >> (Pos << 3)) & 0xff;
}

static Byte ByteFromDWord(LongWord d, int Pos)
{
  return (d >> (Pos << 3)) & 0xff;
}

static Word WordFromDWord(LongWord d, int Pos)
{
  return (d >> (Pos << 4)) & 0xffff;
}

/*!------------------------------------------------------------------------
 * \fn     SubCodeFill
 * \brief  perform 'c = a - b' on tCurrCodeFill structures
 * \param  c result
 * \param  b, c arguments
 * ------------------------------------------------------------------------ */

static void SubCodeFill(tCurrCodeFill *c, const tCurrCodeFill *a, const tCurrCodeFill *b, struct sLayoutCtx *pCtx)
{
  c->FullWordCnt = a->FullWordCnt - b->FullWordCnt;
  if ((c->LastWordFill = a->LastWordFill - b->LastWordFill) < 0)
  {
    c->LastWordFill += pCtx->ElemsPerFullWord;
    c->FullWordCnt--;
  }
}

/*!------------------------------------------------------------------------
 * \fn     MultCodeFill(tCurrCodeFill *b, LongWord a, struct sLayoutCtx *pCtx)
 * \brief  perform 'b *= a' on tCurrCodeFill structures
 * \param  b what to multiply
 * \param  a scaling factor
 * ------------------------------------------------------------------------ */

static void MultCodeFill(tCurrCodeFill *b, LongWord a, struct sLayoutCtx *pCtx)
{
  b->FullWordCnt *= a;
  b->LastWordFill *= a;
  if (pCtx->ElemsPerFullWord > 1)
  {
    LongWord div = b->LastWordFill / pCtx->ElemsPerFullWord,
             mod = b->LastWordFill % pCtx->ElemsPerFullWord;
    b->FullWordCnt += div;
    b->LastWordFill = mod;
  }
}

/*!------------------------------------------------------------------------
 * \fn     IncCodeFill(tCurrCodeFill *a, struct sLayoutCtx *pCtx)
 * \brief  advance tCurrCodeFill pointer by one base element
 * \param  a pointer to increment
 * \param  pCtx context
 * ------------------------------------------------------------------------ */

static void IncCodeFill(tCurrCodeFill *a, struct sLayoutCtx *pCtx)
{
  if (++a->LastWordFill >= pCtx->ElemsPerFullWord)
  {
    a->LastWordFill -= pCtx->ElemsPerFullWord;
    a->FullWordCnt++;
  }
}

/*!------------------------------------------------------------------------
 * \fn     IncCurrCodeFill(struct sLayoutCtx *pCtx)
 * \brief  advance CodeFill pointer in context and reserve memory
 * \param  pCtx context
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean IncCurrCodeFill(struct sLayoutCtx *pCtx)
{
  LongInt OldFullWordCnt = pCtx->CurrCodeFill.FullWordCnt;

  IncCodeFill(&pCtx->CurrCodeFill, pCtx);
  if (OldFullWordCnt == pCtx->CurrCodeFill.FullWordCnt)
    return True;
  else if (!IncMaxCodeLen(pCtx, 1))
    return False;
  else
  {
    switch (pCtx->FullWordSize)
    {
      case 8:
        BAsmCode[pCtx->CurrCodeFill.FullWordCnt] = 0;
        break;
      case 16:
        WAsmCode[pCtx->CurrCodeFill.FullWordCnt] = 0;
        break;
      case 32:
        DAsmCode[pCtx->CurrCodeFill.FullWordCnt] = 0;
        break;
    }
    return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     IncCodeFillBy(tCurrCodeFill *a, const tCurrCodeFill *inc, struct sLayoutCtx *pCtx)
 * \brief  perform 'a += inc' on tCurrCodeFill structures
 * \param  a what to advance
 * \param  inc by what to advance
 * \param  pCtx context
 * ------------------------------------------------------------------------ */

static void IncCodeFillBy(tCurrCodeFill *a, const tCurrCodeFill *inc, struct sLayoutCtx *pCtx)
{
  a->LastWordFill += inc->LastWordFill;
  if ((pCtx->ElemsPerFullWord > 1) && (a->LastWordFill >= pCtx->ElemsPerFullWord))
  {
    a->LastWordFill -= pCtx->ElemsPerFullWord;
    a->FullWordCnt++;
  }
  a->FullWordCnt += inc->FullWordCnt;
}

/*****************************************************************************
 * Function:    LayoutBit
 * Purpose:     parse argument, interprete as bit,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put1I_To_1(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0x01);
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt] = b & 0x01;
  pCtx->CurrCodeFill.FullWordCnt++;
  return True;
}

static Boolean Replicate1_To_1(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  LongInt this_count = pEndPos->FullWordCnt - pStartPos->FullWordCnt;

  if (!IncMaxCodeLen(pCtx, this_count))
    return False;
  memcpy(&BAsmCode[pCtx->CurrCodeFill.FullWordCnt], &BAsmCode[pStartPos->FullWordCnt], this_count);
  pCtx->CurrCodeFill.FullWordCnt += this_count;

  return True;
}

static Boolean Put1I_To_4(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_b_guessed(flags, Pos.FullWordCnt, 1, BitInNibble(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    BAsmCode[Pos.FullWordCnt] = BitInNibble(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_b_guessed(flags, Pos.FullWordCnt, 1, BitInNibble(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    BAsmCode[Pos.FullWordCnt] |= BitInNibble(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate1_To_4(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = BitFromNibble(BAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = BitFromNibble(get_basmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put1I_To_4(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put1I_To_8(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_b_guessed(flags, Pos.FullWordCnt, 1, BitInByte(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    BAsmCode[Pos.FullWordCnt] = BitInByte(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_b_guessed(flags, Pos.FullWordCnt, 1, BitInByte(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    BAsmCode[Pos.FullWordCnt] |= BitInByte(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate1_To_8(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = BitFromByte(BAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = BitFromByte(get_basmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put1I_To_8(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put1I_To_16(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_w_guessed(flags, Pos.FullWordCnt, 1, BitInWord(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    WAsmCode[Pos.FullWordCnt] = BitInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_w_guessed(flags, Pos.FullWordCnt, 1, BitInWord(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    WAsmCode[Pos.FullWordCnt] |= BitInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate1_To_16(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = BitFromWord(WAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = BitFromWord(get_wasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put1I_To_16(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put1I_To_32(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_d_guessed(flags, Pos.FullWordCnt, 1, BitInDWord(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] = BitInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_d_guessed(flags, Pos.FullWordCnt, 1, BitInDWord(0x1, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] |= BitInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate1_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = BitFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = BitFromDWord(get_dasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put1I_To_32(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean LayoutBit(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  TempResult t;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  switch (t.Typ)
  {
    case TempInt:
      if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0x1;
      if (!mSymbolQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, UInt1)) WrStrErrorPos(ErrNum_OverRange, pExpr);
      else
      {
        if (!pCtx->Put1I(t.Contents.Int, t.Flags, pCtx))
          LEAVE;
        Result = True;
      }
      break;
    case TempFloat:
      WrStrErrorPos(ErrNum_IntButFloat, pExpr);
      break;
    case TempString:
      WrStrErrorPos(ErrNum_IntButString, pExpr);
      break;
    default:
      break;
  }

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutNibble
 * Purpose:     parse argument, interprete as nibble,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put4I_To_1(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  
  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0x01);
  for (z = 0; z < 4; z++, b >>= 1)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = b & 0x01;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put4I_To_4(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0x0f);
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt] = b & 0x0f;
  pCtx->CurrCodeFill.FullWordCnt++;
  return True;
}

static Boolean Replicate4_To_4(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  LongInt this_count = pEndPos->FullWordCnt - pStartPos->FullWordCnt;

  if (!IncMaxCodeLen(pCtx, this_count))
    return False;
  memcpy(&BAsmCode[pCtx->CurrCodeFill.FullWordCnt], &BAsmCode[pStartPos->FullWordCnt], this_count);
  pCtx->CurrCodeFill.FullWordCnt += this_count;

  return True;
}

static Boolean Put4I_To_8(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_b_guessed(flags, Pos.FullWordCnt, 1, NibbleInByte(0xf, Pos.LastWordFill ^ pCtx->LoHiMap));
    BAsmCode[Pos.FullWordCnt] = NibbleInByte(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_b_guessed(flags, Pos.FullWordCnt, 1, NibbleInByte(0xf, Pos.LastWordFill ^ pCtx->LoHiMap));
    BAsmCode[Pos.FullWordCnt] |= NibbleInByte(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate4_To_8(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = NibbleFromByte(BAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = NibbleFromByte(get_basmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put4I_To_8(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put4I_To_16(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_w_guessed(flags, Pos.FullWordCnt, 1, NibbleInWord(0xf, Pos.LastWordFill ^ pCtx->LoHiMap));
    WAsmCode[Pos.FullWordCnt] = NibbleInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_w_guessed(flags, Pos.FullWordCnt, 1, NibbleInWord(0xf, Pos.LastWordFill ^ pCtx->LoHiMap));
    WAsmCode[Pos.FullWordCnt] |= NibbleInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate4_To_16(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = NibbleFromWord(WAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = NibbleFromWord(get_wasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put4I_To_16(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put4I_To_32(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_d_guessed(flags, Pos.FullWordCnt, 1, NibbleInWord(0xf, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] = NibbleInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_d_guessed(flags, Pos.FullWordCnt, 1, NibbleInWord(0xf, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] |= NibbleInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate4_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = NibbleFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = NibbleFromDWord(get_dasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put4I_To_32(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean LayoutNibble(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  TempResult t;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  switch (t.Typ)
  {
    case TempInt:
      if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0xf;
      if (!mSymbolQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int4)) WrStrErrorPos(ErrNum_OverRange, pExpr);
      else
      {
        if (!pCtx->Put4I(t.Contents.Int, t.Flags, pCtx))
          LEAVE;
        Result = True;
      }
      break;
    case TempFloat:
      WrStrErrorPos(ErrNum_IntButFloat, pExpr);
      break;
    case TempString:
      WrStrErrorPos(ErrNum_IntButString, pExpr);
      break;
    default:
      break;
  }

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutByte
 * Purpose:     parse argument, interprete as byte,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put8I_To_1(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  
  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0x01);
  for (z = 0; z < 8; z++, b >>= 1)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ (pCtx->LoHiMap & 7))] = b & 0x01;
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put8I_To_4(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0x0f);
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (b >> 0) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (b >> 4) & 0x0f;
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put8I_To_8(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  if ((pCtx->ListGran == 1) || !(pCtx->CurrCodeFill.FullWordCnt & 1))
  {
    set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xff);
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt] = b;
  }
  else if (pCtx->flags & eIntPseudoFlag_Turn)
  {
    or_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0x00ffu);
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt >> 1] = (((Word)BAsmCode[pCtx->CurrCodeFill.FullWordCnt - 1]) << 8) | b;
  }
  else
  {
    or_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xff00u);
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt >> 1] = (((Word)b) << 8) | BAsmCode[pCtx->CurrCodeFill.FullWordCnt - 1];
  }
  pCtx->CurrCodeFill.FullWordCnt++;
  return True;
}

static Boolean Put8I_To_16(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_w_guessed(flags, Pos.FullWordCnt, 1, ByteInWord(0xff, Pos.LastWordFill ^ pCtx->LoHiMap));
    WAsmCode[Pos.FullWordCnt] = ByteInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_w_guessed(flags, Pos.FullWordCnt, 1, ByteInWord(0xff, Pos.LastWordFill ^ pCtx->LoHiMap));
    WAsmCode[Pos.FullWordCnt] |= ByteInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Put8I_To_32(Byte b, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_d_guessed(flags, Pos.FullWordCnt, 1, ByteInDWord(0xff, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] = ByteInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_d_guessed(flags, Pos.FullWordCnt, 1, ByteInDWord(0xff, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] |= ByteInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Replicate8ToN_To_8(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos;

  if (!IncMaxCodeLen(pCtx, pEndPos->FullWordCnt - pStartPos->FullWordCnt))
    return False;

  for (Pos = *pStartPos; Pos.FullWordCnt < pEndPos->FullWordCnt; Pos.FullWordCnt += pCtx->BaseElemLenBits / 8)
  {
    copy_basmcode_guessed(pCtx->CurrCodeFill.FullWordCnt, Pos.FullWordCnt, pCtx->BaseElemLenBits / 8);
    memcpy(&BAsmCode[pCtx->CurrCodeFill.FullWordCnt], &BAsmCode[Pos.FullWordCnt], pCtx->BaseElemLenBits / 8);
    pCtx->CurrCodeFill.FullWordCnt += pCtx->BaseElemLenBits / 8;
  }
  if (Pos.FullWordCnt != pEndPos->FullWordCnt)
  {
    WrXError(ErrNum_InternalError, "DUP replication inconsistency");
    return False;
  }

  return True;
}

static Boolean Replicate8_To_16(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = ByteFromWord(WAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = ByteFromWord(get_wasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put8I_To_16(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Replicate8_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = ByteFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = ByteFromDWord(get_dasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put8I_To_32(b, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean LayoutByte(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  const Boolean allow_int = !!(pCtx->flags & eIntPseudoFlag_AllowInt),
                allow_string = !!(pCtx->flags & eIntPseudoFlag_AllowString);
  TempResult t;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  switch (t.Typ)
  {
    case TempInt:
    ToInt:
      if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0xff;
      if (!allow_int) WrStrErrorPos(ErrNum_StringButInt, pExpr);
      else if (!mSymbolQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int8)) WrStrErrorPos(ErrNum_OverRange, pExpr);
      else
      {
        if (!pCtx->Put8I(t.Contents.Int, t.Flags, pCtx))
          LEAVE;
        Result = True;
      }
      break;
    case TempFloat:
      WrStrErrorPos((allow_int && allow_string)
                   ? ErrNum_StringOrIntButFloat
                   : (allow_int ? ErrNum_IntButFloat : ErrNum_IntButString), pExpr);
      break;
    case TempString:
    {
      unsigned ch;
      const char *p_run;
      size_t run_len;
      int ret;
      unsigned ascii_flags = pCtx->flags & eIntPseudoFlag_ASCIAll;

      if (allow_int && MultiCharToInt(&t, 1))
        goto ToInt;

      if (!allow_string)
      {
        WrStrErrorPos(ErrNum_IntButString, pExpr);
        LEAVE;
      }

      p_run = t.Contents.str.p_str;
      run_len = t.Contents.str.len;
      if (ascii_flags == eIntPseudoFlag_ASCIC)
      {
        if (run_len > 255)
        {
          WrStrErrorPos(ErrNum_StringTooLong, pExpr);
          LEAVE;
        }
        if (!pCtx->Put8I(run_len, t.Flags, pCtx))
          LEAVE;
      }
      while (!(ret = as_chartrans_xlate_next(CurrTransTable->p_table, &ch, &p_run, &run_len)))
      {
        if (!pCtx->Put8I(ch, t.Flags, pCtx))
          LEAVE;
      }
      if (ENOENT == ret)
      {
        WrStrErrorPos(ErrNum_UnmappedChar, pExpr);
        LEAVE;
      }
      if (ascii_flags == eIntPseudoFlag_ASCIZ)
      {
        if (!pCtx->Put8I('\0', eSymbolFlag_None, pCtx))
          LEAVE;
      }

      Result = True;
      break;
    }
    default:
      break;
  }

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutWord
 * Purpose:     parse argument, interprete as 16-bit word,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put16I_To_1(Word w, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;

  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 16, 0x01);
  for (z = 0; z < 16; z++, w >>= 1)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = w & 0x01;
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put16F_To_1(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret, z;
  Byte tmp[2];

  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  if ((ret = as_float_2_ieee2(t, tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  for (z = 0; z < 2; z++)
    Put8I_To_1(tmp[z ^ (pCtx->LoHiMap & 1)], flags, pCtx);

  return True;
}

static Boolean Put16I_To_4(Word w, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0x0f);
  for (z = 0; z < 4; z++, w >>= 4)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = w & 0x0f;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put16F_To_4(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;
  Byte tmp[2];
  LongInt dest;
  unsigned z;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  if ((ret = as_float_2_ieee2(t, tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0x0f);
  if (pCtx->LoHiMap)
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt + 4; z < 2; z++)
    {
      BAsmCode[--dest] = (tmp[z] >> 0) & 0xf;
      BAsmCode[--dest] = (tmp[z] >> 4) & 0xf;
    }
  else
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt; z < 2; z++)
    {
      BAsmCode[dest++] = (tmp[z] >> 0) & 0xf;
      BAsmCode[dest++] = (tmp[z] >> 4) & 0xf;
    }
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put16I_To_8(Word w, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if (pCtx->ListGran == 2)
  {
    set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt >> 1, 1, 0xffffu);
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt >> 1] = w;
  }
  else
  {
    set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xff);
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = Lo(w);
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = Hi(w);
  }
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put16F_To_8(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if ((ret = as_float_2_ieee2(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  else
  {
    if (pCtx->ListGran == 2)
      set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt >> 1, 1, 0xffffu);
    else
      set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xff);
    pCtx->CurrCodeFill.FullWordCnt += 2;
    return True;
  }
}

static Boolean Put16I_To_16(Word w, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt++] = w;
  return True;
}

static Boolean Put16F_To_16(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[2];
  int ret;

  if (!IncMaxCodeLen(pCtx, 1))
    return False;

  if ((ret = as_float_2_ieee2(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ pCtx->LoHiMap) | ByteInWord(Tmp[1], 1 ^ pCtx->LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 1;
  return True;
}

static Boolean Put16I_To_32(Word w, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
  {
    set_d_guessed(flags, Pos.FullWordCnt, 1, WordInDWord(0xffffu, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] = WordInDWord(w, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  else
  {
    or_d_guessed(flags, Pos.FullWordCnt, 1, WordInDWord(0xffffu, Pos.LastWordFill ^ pCtx->LoHiMap));
    DAsmCode[Pos.FullWordCnt] |= WordInDWord(w, Pos.LastWordFill ^ pCtx->LoHiMap);
  }
  return True;
}

static Boolean Put16F_To_32(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[2];
  int ret;

  if ((ret = as_float_2_ieee2(t, Tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  return Put16I_To_32((((Word) Tmp[1]) << 8) | Tmp[0], flags, pCtx);
}

static Boolean Replicate16_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Word w, u;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    w = WordFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    u = WordFromDWord(get_dasmcode_guessed(CurrPos.FullWordCnt), CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put16I_To_32(w, u ? eSymbolFlag_FirstPassUnknown : eSymbolFlag_None, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Replicate16ToN_To_16(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos;

  if (!IncMaxCodeLen(pCtx, pEndPos->FullWordCnt - pStartPos->FullWordCnt))
    return False;

  for (Pos = *pStartPos; Pos.FullWordCnt < pEndPos->FullWordCnt; Pos.FullWordCnt += pCtx->BaseElemLenBits / 16)
  {
    copy_wasmcode_guessed(pCtx->CurrCodeFill.FullWordCnt, Pos.FullWordCnt, pCtx->BaseElemLenBits / 16);
    memcpy(&WAsmCode[pCtx->CurrCodeFill.FullWordCnt], &WAsmCode[Pos.FullWordCnt], pCtx->BaseElemLenBits / 8);
    pCtx->CurrCodeFill.FullWordCnt += pCtx->BaseElemLenBits / 16;
  }
  if (Pos.FullWordCnt != pEndPos->FullWordCnt)
  {
    WrXError(ErrNum_InternalError, "DUP replication inconsistency");
    return False;
  }

  return True;
}

static Boolean LayoutWord(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  const Boolean allow_string = !!(pCtx->flags & eIntPseudoFlag_AllowString),
                allow_int = !!(pCtx->flags & eIntPseudoFlag_AllowInt),
                allow_float = !!pCtx->Put16F;
  TempResult t;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  Result = True;
  switch (t.Typ)
  {
    case TempInt:
    ToInt:
      if (pCtx->Put16I)
      {
        if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0xffff;
        if (!mSymbolQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int16)) WrStrErrorPos(ErrNum_OverRange, pExpr);
        else
        {
          if (!pCtx->Put16I(t.Contents.Int, t.Flags, pCtx))
            LEAVE;
          Result = True;
        }
        break;
      }
      else
        TempResultToFloat(&t);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(allow_string ? ErrNum_StringOrIntButFloat : ErrNum_IntButFloat, pExpr);
      else
      {
        if (!pCtx->Put16F(t.Contents.Float, t.Flags, pCtx))
          LEAVE;
        Result = True;
      }
      break;
    case TempString:
      if (allow_int && MultiCharToInt(&t, 2))
        goto ToInt;

      if (!allow_string) WrStrErrorPos(allow_float ? ErrNum_IntOrFloatButString : ErrNum_IntButString, pExpr);
      else
      {
        unsigned z;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pExpr))
          LEAVE;

        for (z = 0; z < t.Contents.str.len; z++)
          if (!pCtx->Put16I(t.Contents.str.p_str[z], t.Flags, pCtx))
            LEAVE;

        Result = True;
      }
      break;
    case TempReg:
      if (allow_float && allow_string)
        WrStrErrorPos(ErrNum_StringOrIntOrFloatButReg, pExpr);
      else if (allow_float)
        WrStrErrorPos(ErrNum_IntOrFloatButReg, pExpr);
      else if (allow_string)
        WrStrErrorPos(ErrNum_IntOrStringButReg, pExpr);
      else
        WrStrErrorPos(ErrNum_IntButReg, pExpr);
      break;
    default:
      break;
  }

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutDoubleWord
 * Purpose:     parse argument, interprete as 32-bit word or
                single precision float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put32I_To_1(LongWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;

  if (!IncMaxCodeLen(pCtx, 32))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 32, 0x01);
  for (z = 0; z < 32; z++, l >>= 1)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0x01;
  pCtx->CurrCodeFill.FullWordCnt += 32;
  return True;
}

static Boolean Put32F_To_1(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret, z;
  Byte tmp[4];

  if (!IncMaxCodeLen(pCtx, 32))
    return False;
  if ((ret = as_float_2_ieee4(t, tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  for (z = 0; z < 4; z++)
    Put8I_To_1(tmp[z ^ (pCtx->LoHiMap & 3)], flags, pCtx);

  return True;
}

static Boolean Put32I_To_4(LongWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0x0f);
  for (z = 0; z < 8; z++, l >>= 4)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0xf;
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put32F_To_4(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte tmp[4];
  int ret;
  unsigned z;
  LongInt dest;

  if (!IncMaxCodeLen(pCtx, 8))
    return False;

  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    Word tmp_w[2];

    ret = as_float_2_dec_f(t, tmp_w);
    if (ret >= 0)
    {
      tmp[0] = Lo(tmp_w[0]);
      tmp[1] = Hi(tmp_w[0]);
      tmp[2] = Lo(tmp_w[1]);
      tmp[3] = Hi(tmp_w[1]);
    }
  }
  else
    ret = as_float_2_ieee4(t, tmp, False);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0x0f);
  if (pCtx->LoHiMap)
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt + 8; z < 4; z++)
    {
      BAsmCode[--dest] = (tmp[z] >> 0) & 0xf;
      BAsmCode[--dest] = (tmp[z] >> 4) & 0xf;
    }
  else
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt; z < 4; z++)
    {
      BAsmCode[dest++] = (tmp[z] >> 0) & 0xf;
      BAsmCode[dest++] = (tmp[z] >> 4) & 0xf;
    }
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put32I_To_8(LongWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0xff);
  for (z = 0; z < 4; z++, l >>= 8)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0xff;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put32F_To_8(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;

  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    Word *p_dest = WAsmCode + (pCtx->CurrCodeFill.FullWordCnt / 2);

    ret = as_float_2_dec_f(t, p_dest);
    if ((ret >= 0) && (HostBigEndian && (ListGran() == 1)))
      WSwap(p_dest, 4);
  }
  else
    ret = as_float_2_ieee4(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  else
  {
    set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0xff);
    pCtx->CurrCodeFill.FullWordCnt += 4;
    return True;
  }
}

static Boolean Put32I_To_16(LongWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = LoWord(l);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = HiWord(l);
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put32F_To_16(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[4];
  int ret;

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if ((ret = as_float_2_ieee4(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ pCtx->LoHiMap) | ByteInWord(Tmp[1], 1 ^ pCtx->LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[2], 0 ^ pCtx->LoHiMap) | ByteInWord(Tmp[3], 1 ^ pCtx->LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put32I_To_32(LongWord w, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xfffffffful);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt++] = w;
  return True;
}

static Boolean Put32F_To_32(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[4];
  int ret;

  if (!IncMaxCodeLen(pCtx, 1))
    return False;

  if ((ret = as_float_2_ieee4(t, Tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xfffffffful);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = 
    ByteInDWord(Tmp[0], 0) |
    ByteInDWord(Tmp[1], 1) |
    ByteInDWord(Tmp[2], 2) |
    ByteInDWord(Tmp[3], 3);
  pCtx->CurrCodeFill.FullWordCnt += 1;
  return True;
}

static Boolean Replicate32ToN_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos;
  /* need roundup for DT since 80 bit value needs 3 dwords: */
  size_t roundup_words_per_element = (pCtx->BaseElemLenBits + 31) / 32,
         roundup_bytes_per_element = roundup_words_per_element * 4;

  if (!IncMaxCodeLen(pCtx, pEndPos->FullWordCnt - pStartPos->FullWordCnt))
    return False;

  for (Pos = *pStartPos; Pos.FullWordCnt < pEndPos->FullWordCnt; Pos.FullWordCnt += roundup_words_per_element)
  {
    copy_dasmcode_guessed(pCtx->CurrCodeFill.FullWordCnt, Pos.FullWordCnt, roundup_words_per_element);
    memcpy(&DAsmCode[pCtx->CurrCodeFill.FullWordCnt], &DAsmCode[Pos.FullWordCnt], roundup_bytes_per_element);
    pCtx->CurrCodeFill.FullWordCnt += roundup_words_per_element;
  }
  if (Pos.FullWordCnt != pEndPos->FullWordCnt)
  {
    WrXError(ErrNum_InternalError, "DUP replication inconsistency");
    return False;
  }

  return True;
}

static Boolean LayoutDoubleWord(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  TempResult t;
  Boolean Result = False;
  const Boolean allow_string = !!(pCtx->flags & eIntPseudoFlag_AllowString),
                allow_float = !!pCtx->Put32F;
  Word Cnt = 0;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  Result = False;
  switch (t.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put32I)
      {
        if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0xfffffffful;
        if (!mSymbolQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int32)) WrStrErrorPos(ErrNum_OverRange, pExpr);
        else
        {
          if (!pCtx->Put32I(t.Contents.Int, t.Flags, pCtx))
            LEAVE;
          Cnt = 4;
          Result = True;
        }
        break;
      }
      else
        TempResultToFloat(&t);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(allow_string ? ErrNum_StringOrIntButFloat : ErrNum_IntButFloat, pExpr);
      else
      {
        if (!pCtx->Put32F(t.Contents.Float, t.Flags, pCtx))
          LEAVE;
        Cnt = 4;
        Result = True;
      }
      break;
    case TempString:
      if (!allow_string) WrStrErrorPos(allow_float ? ErrNum_IntOrFloatButString : ErrNum_IntButString, pExpr);
      else
      {
        unsigned z;

        if (MultiCharToInt(&t, 4))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pExpr))
          WrStrErrorPos(ErrNum_UnmappedChar, pExpr);

        for (z = 0; z < t.Contents.str.len; z++)
          if (!pCtx->Put32I(t.Contents.str.p_str[z], t.Flags, pCtx))
            LEAVE;

        Cnt = t.Contents.str.len * 4;
        Result = True;
      }
      break;
    case TempReg:
      if (allow_float && allow_string)
        WrStrErrorPos(ErrNum_StringOrIntOrFloatButReg, pExpr);
      else if (allow_float)
        WrStrErrorPos(ErrNum_IntOrFloatButReg, pExpr);
      else if (allow_string)
        WrStrErrorPos(ErrNum_IntOrStringButReg, pExpr);
      else
        WrStrErrorPos(ErrNum_IntButReg, pExpr);
      break;
    case TempAll:
      assert(0);
  }
  (void)Cnt;

func_exit:
  as_tempres_free(&t);
  return Result;
}


/*****************************************************************************
 * Function:    LayoutMacAddr
 * Purpose:     parse argument, interprete as 48-bit word or
                float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put48I_To_1(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  Byte *p_dest, highest_src;

  if (!IncMaxCodeLen(pCtx, 48))
    return False;

  p_dest = BAsmCode + pCtx->CurrCodeFill.FullWordCnt + (pCtx->LoHiMap ? 48 : 0);
  for (z = 0; z < min(48, LARGEBITS); z++, l >>= 1)
  {
    highest_src = l & 0x1;
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = highest_src;
  }
  /* TempResult is TempInt, so sign-extend */
  for (; z < 48; z++)
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = (highest_src & 0x1) ? 0x1 : 0x0;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 48, 0x1);
  pCtx->CurrCodeFill.FullWordCnt += 48;
  return True;
}

static Boolean Put48I_To_4(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  Byte *p_dest, highest_src;

  if (!IncMaxCodeLen(pCtx, 12))
    return False;

  p_dest = BAsmCode + pCtx->CurrCodeFill.FullWordCnt + (pCtx->LoHiMap ? 12 : 0);
  for (z = 0; z < min(12, LARGEBITS / 4); z++, l >>= 4)
  {
    highest_src = l & 0xf;
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = highest_src;
  }
  /* TempResult is TempInt, so sign-extend */
  for (; z < 12; z++)
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = (highest_src & 0x8) ? 0xf : 0x0;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 12, 0xf);
  pCtx->CurrCodeFill.FullWordCnt += 12;
  return True;
}

static Boolean Put48I_To_8(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  Byte *p_dest, highest_src = 0;

  if (!IncMaxCodeLen(pCtx, 6))
    return False;

  p_dest = BAsmCode + pCtx->CurrCodeFill.FullWordCnt + (pCtx->LoHiMap ? 6 : 0);
  for (z = 0; z < min(6, LARGEBITS / 8); z++, l >>= 8)
  {
    highest_src = l & 0xff;
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = highest_src;
  }
  /* TempResult is TempInt, so sign-extend */
  for (; z < 6; z++)
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = (highest_src & 0x80) ? 0xff : 0x0;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 6, 0xff);
  pCtx->CurrCodeFill.FullWordCnt += 6;
  return True;
}

static Boolean Put48F_To_8(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  /* make space for 8 bytes - last word of D format float is truncated */
  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    /* LoHiMap (endianess) is ignored */
    int ret = as_float_2_dec_d(t, WAsmCode + (pCtx->CurrCodeFill.FullWordCnt / 2));
    if (ret < 0)
    {
      asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
      return False;
    }
  }
  else
    assert(0);  /* no 6 byte IEEE float format */
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 12, 0xf);
  pCtx->CurrCodeFill.FullWordCnt += 6;
  return True;
}

static Boolean Put48I_To_16(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  Word *p_dest, highest_src = 0;

  if (!IncMaxCodeLen(pCtx, 3))
    return False;

  p_dest = WAsmCode + pCtx->CurrCodeFill.FullWordCnt + (pCtx->LoHiMap ? 3 : 0);
  for (z = 0; z < min(3, LARGEBITS / 16); z++, l >>= 16)
  {
    highest_src = l & 0xffff;
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = highest_src;
  }
  /* TempResult is TempInt, so sign-extend */
  for (; z < 3; z++)
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = (highest_src & 0x8000) ? 0xffff : 0x0000;
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 3, 0xffffu);
  pCtx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean Put48F_To_16(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 3))
    return False;
  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    Word Tmp[4];
    int ret = as_float_2_dec_d(t, Tmp);
    if (ret < 0)
    {
      asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
      return False;
    }
    /* LoHiMap (endianess) is ignored */
    memcpy(&WAsmCode[pCtx->CurrCodeFill.FullWordCnt], Tmp, 6);
  }
  else
    assert(0);  /* no 6 byte IEEE float format */
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 3, 0xffffu);
  pCtx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean Put48I_To_32(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;
  LongWord highest_src = 0, *p_dest;

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  p_dest = DAsmCode + pCtx->CurrCodeFill.FullWordCnt + (pCtx->LoHiMap ? 2 : 0);
  for (z = 0; z < min(2, LARGEBITS / 32); z++, largeint_shr32(l))
  {
    highest_src = l & 0xfffffffful;
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = highest_src & (z ? 0xfffffffful : 0x0000fffful);
  }
  for (; z < 2; z++)
    *(pCtx->LoHiMap ? --p_dest : p_dest++) = ((highest_src & 0x80000000ul) ? 0xfffffffful : 0x00000000ul) & (z ? 0xfffffffful : 0x0000fffful);
  if (pCtx->LoHiMap)
  {
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt    , 1, 0xffffu);
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt + 1, 1, 0xfffffffful);
  }
  else
  {
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt    , 1, 0xfffffffful);
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt + 1, 1, 0xffffu);
  }
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put48F_To_32(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    Word Tmp[4];
    int ret = as_float_2_dec_d(t, Tmp);
    if (ret < 0)
    {
      asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
      return False;
    }
    /* LoHiMap (endianess) is ignored */
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = WordInDWord(Tmp[0], 0);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = WordInDWord(Tmp[1], 1) | WordInDWord(Tmp[2], 2);
  }
  else
    assert(0);  /* no 6 byte IEEE float format */
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt    , 1, 0xffffu);
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt + 1, 1, 0xfffffffful);
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean LayoutMacAddr(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  TempResult t;
  Word Cnt  = 0;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  Result = False;
  switch(t.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put48I)
      {
        if (!pCtx->Put48I(t.Contents.Int, t.Flags, pCtx))
          LEAVE;
        Cnt = 6;
        Result = True;
        break;
      }
      else
        TempResultToFloat(&t);
      /* fall-through */
    case TempFloat:
      if (!pCtx->Put48F) WrStrErrorPos(ErrNum_StringOrIntButFloat, pExpr);
      else if (!pCtx->Put48F(t.Contents.Float, t.Flags, pCtx))
        LEAVE;
      Cnt = 6;
      Result = True;
      break;
    case TempString:
    {
      unsigned z;

      if (MultiCharToInt(&t, 6))
        goto ToInt;

      if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pExpr))
        LEAVE;

      for (z = 0; z < t.Contents.str.len; z++)
        if (!pCtx->Put48I(t.Contents.str.p_str[z], t.Flags, pCtx))
          LEAVE;

      Cnt = t.Contents.str.len * 6;
      Result = True;
      break;
    }
    case TempReg:
      WrStrErrorPos(ErrNum_StringOrIntOrFloatButReg, pExpr);
      break;
    case TempAll:
      assert(0);
  }
  (void)Cnt;

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutQuadWord
 * Purpose:     parse argument, interprete as 64-bit word or
                double precision float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put64I_To_1(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  Byte highest_src;

  if (!IncMaxCodeLen(pCtx, 64))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 64, 0x01);

  for (z = 0; z < min(64, LARGEBITS); z++, l >>= 1)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = highest_src = l & 0x01;
  for (; z < 64; z++)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = (highest_src & 0x1) ? 0x1: 0x0;

  pCtx->CurrCodeFill.FullWordCnt += 64;
  return True;
}

static Boolean Put64F_To_1(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret, z;
  Byte tmp[8];

  if (!IncMaxCodeLen(pCtx, 64))
    return False;
  if ((ret = as_float_2_ieee8(t, tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  for (z = 0; z < 8; z++)
    Put8I_To_1(tmp[z ^ (pCtx->LoHiMap & 7)], flags, pCtx);

  return True;
}

static Boolean Put64I_To_4(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  unsigned z;
  Byte highest_src = 0;

  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 16, 0xf);
  for (z = 0; z < min(16, LARGEBITS / 4); z++, l >>= 4)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = highest_src = l & 0xf;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 16; z++)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = (highest_src & 0x8) ? 0xf : 0x0;
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put64F_To_4(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte tmp[8];
  int ret;
  unsigned z;
  LongInt dest;

  if (!IncMaxCodeLen(pCtx, 16))
    return False;

  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    Word tmp_w[4];

    ret = (pCtx->flags & eIntPseudoFlag_DECGFormat)
        ? as_float_2_dec_g(t, tmp_w)
        : as_float_2_dec_d(t, tmp_w);
    if (ret >= 0)
    {
      tmp[0] = Lo(tmp_w[0]);
      tmp[1] = Hi(tmp_w[0]);
      tmp[2] = Lo(tmp_w[1]);
      tmp[3] = Hi(tmp_w[1]);
      tmp[4] = Lo(tmp_w[2]);
      tmp[5] = Hi(tmp_w[2]);
      tmp[6] = Lo(tmp_w[3]);
      tmp[7] = Hi(tmp_w[3]);
    }
  }
  else
    ret = as_float_2_ieee8(t, tmp, False);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 16, 0xf);
  if (pCtx->LoHiMap)
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt + 16; z < 8; z++)
    {
      BAsmCode[--dest] = (tmp[z] >> 0) & 0xf;
      BAsmCode[--dest] = (tmp[z] >> 4) & 0xf;
    }
  else
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt; z < 8; z++)
    {
      BAsmCode[dest++] = (tmp[z] >> 0) & 0xf;
      BAsmCode[dest++] = (tmp[z] >> 4) & 0xf;
    }
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put64I_To_8(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;
  Byte higest_src = 0x00;

  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0xff);
  for (z = 0; z < min(8, LARGEBITS / 8); z++, l >>= 8)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = higest_src = l & 0xff;
  for (; z < 8; z++)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = (higest_src & 0x80) ? 0xff : 0x00;
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put64F_To_8(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 8))
    return False;

  if (pCtx->flags & eIntPseudoFlag_DECFormats)
  {
    Word *p_dest = WAsmCode + (pCtx->CurrCodeFill.FullWordCnt / 2);
    ret = (pCtx->flags & eIntPseudoFlag_DECGFormat)
        ? as_float_2_dec_g(t, p_dest)
        : as_float_2_dec_d(t, p_dest);
    if ((ret >= 0) && (HostBigEndian && (ListGran() == 1)))
      WSwap(p_dest, 8);
  }
  else
    ret = as_float_2_ieee8(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  else
  {
    set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0xff);
    pCtx->CurrCodeFill.FullWordCnt += 8;
    return True;
  }
}

static Boolean Put64I_To_16(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;
  Word highest_src = 0;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0xffffu);
  for (z = 0; z < min(4, LARGEBITS / 16); z++, l >>= 16)
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = highest_src = l & 0xffff;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 4; z++)
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = (highest_src & 0x8000) ? 0xffff : 0x0000;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put64F_To_16(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[8];
  int ret;
  int LoHiMap = pCtx->LoHiMap & 1;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  if ((ret = as_float_2_ieee8(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ LoHiMap) | ByteInWord(Tmp[1], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[2], 0 ^ LoHiMap) | ByteInWord(Tmp[3], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInWord(Tmp[4], 0 ^ LoHiMap) | ByteInWord(Tmp[5], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 3] = ByteInWord(Tmp[6], 0 ^ LoHiMap) | ByteInWord(Tmp[7], 1 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put64I_To_32(LargeWord l, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int z;
  LongWord highest_src;

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xfffffffful);
  for (z = 0; z < min(2, LARGEBITS / 32); z++, largeint_shr32(l))
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = highest_src = l & 0xfffffffful;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 2; z++)
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = (highest_src & 0x80000000ul) ? 0xfffffffful : 0x00000000ul;
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put64F_To_32(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[8];
  int ret;
  int LoHiMap = pCtx->LoHiMap ? 3 : 0;

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if ((ret = as_float_2_ieee8(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xfffffffful);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInDWord(Tmp[0], 0 ^ LoHiMap) | ByteInDWord(Tmp[1], 1 ^ LoHiMap) | ByteInDWord(Tmp[2], 2 ^ LoHiMap) | ByteInDWord(Tmp[3], 3 ^ LoHiMap);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInDWord(Tmp[4], 0 ^ LoHiMap) | ByteInDWord(Tmp[5], 1 ^ LoHiMap) | ByteInDWord(Tmp[6], 2 ^ LoHiMap) | ByteInDWord(Tmp[7], 3 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean LayoutQuadWord(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  const Boolean allow_string = !!(pCtx->flags & eIntPseudoFlag_AllowString),
                allow_float = !!pCtx->Put64F;
  TempResult t;
  Word Cnt  = 0;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  Result = False;
  switch (t.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put64I)
      {
        if (!mFirstPassUnknownOrQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int64)) WrStrErrorPos(ErrNum_OverRange, pExpr);
        else
        {
          if (!pCtx->Put64I(t.Contents.Int, t.Flags, pCtx))
            LEAVE;
          Cnt = 8;
          Result = True;
        }
        break;
      }
      else
        TempResultToFloat(&t);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(ErrNum_StringOrIntButFloat, pExpr);
      else if (!pCtx->Put64F(t.Contents.Float, t.Flags, pCtx))
        LEAVE;
      Cnt = 8;
      Result = True;
      break;
    case TempString:
      if (!allow_string) WrStrErrorPos(allow_float ? ErrNum_IntOrFloatButString : ErrNum_IntButString, pExpr);
      else
      {
         unsigned z;

        if (MultiCharToInt(&t, 8))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pExpr))
          LEAVE;

        for (z = 0; z < t.Contents.str.len; z++)
          if (!pCtx->Put64I(t.Contents.str.p_str[z], t.Flags, pCtx))
            LEAVE;

        Cnt = t.Contents.str.len * 8;
        Result = True;
      }
      break;
    case TempReg:
      WrStrErrorPos(ErrNum_StringOrIntOrFloatButReg, pExpr);
      break;
    case TempAll:
      assert(0);
  }
  (void)Cnt;

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutTenBytes
 * Purpose:     parse argument, interprete extended precision float,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put80I_To_4(LargeWord t, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *p_ctx)
{
  unsigned dest;
  Byte digit;

  if (!IncMaxCodeLen(p_ctx, 20))
    return False;
  memset(&BAsmCode[p_ctx->CurrCodeFill.FullWordCnt], 0, 20);

  if (orig_negative)
    t = (LargeWord)(0 - ((LargeInt)t));
  dest = 0;
  while (t)
  {
    digit = (t % 10);
    t /= 10;
    if (p_ctx->LoHiMap)
      BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + (19 - dest)] = digit;
    else
      BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + dest] |= digit;
    dest++;
    if ((dest >= 18) && t)
    {
      WrError(orig_negative ? ErrNum_UnderRange : ErrNum_OverRange);
      return False;
    }
  }
  digit = !!orig_negative << 3;
  if (p_ctx->LoHiMap)
    BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 0] |= digit;
  else
    BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 19] |= digit;
  set_b_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt, 20, 0xf);
  p_ctx->CurrCodeFill.FullWordCnt += 20;
  return True;
}

static Boolean Put80I_To_8(LargeWord t, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *p_ctx)
{
  unsigned dest, bit_pos;
  Byte digit;

  if (!IncMaxCodeLen(p_ctx, 10))
    return False;
  memset(&BAsmCode[p_ctx->CurrCodeFill.FullWordCnt], 0, 10);

  if (orig_negative)
    t = (LargeWord)(0 - ((LargeInt)t));
  dest = bit_pos = 0;
  while (t)
  {
    digit = (t % 10) << bit_pos;
    t /= 10;
    if (p_ctx->LoHiMap)
      BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + (9 - dest)] |= digit;
    else
      BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + dest] |= digit;
    if ((bit_pos += 4) >= 8)
    {
      bit_pos = 0;
      dest++;
    }
    if ((dest >= 9) && t)
    {
      WrError(orig_negative ? ErrNum_UnderRange : ErrNum_OverRange);
      return False;
    }
  }
  digit = !!orig_negative << 7;
  if (p_ctx->LoHiMap)
    BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 0] |= digit;
  else
    BAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 9] |= digit;
  set_b_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt, 10, 0xff);
  p_ctx->CurrCodeFill.FullWordCnt += 10;
  return True;
}

static Boolean Put80I_To_16(LargeWord t, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *p_ctx)
{
  unsigned dest, bit_pos;
  Word digit;

  if (!IncMaxCodeLen(p_ctx, 5))
    return False;
  memset(&WAsmCode[p_ctx->CurrCodeFill.FullWordCnt], 0, 10);

  if (orig_negative)
    t = (LargeWord)(0 - ((LargeInt)t));
  dest = bit_pos = 0;
  while (t)
  {
    digit = (t % 10) << bit_pos;
    t /= 10;
    if (p_ctx->LoHiMap)
      WAsmCode[p_ctx->CurrCodeFill.FullWordCnt + (4 - dest)] |= digit;
    else
      WAsmCode[p_ctx->CurrCodeFill.FullWordCnt + dest] |= digit;
    if ((bit_pos += 4) >= 16)
    {
      bit_pos = 0;
      dest++;
    }
    if ((dest >= 4) && (bit_pos >= 8) && t)
    {
      WrError(orig_negative ? ErrNum_UnderRange : ErrNum_OverRange);
      return False;
    }
  }
  digit = !!orig_negative << 15;
  if (p_ctx->LoHiMap)
    WAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 0] |= digit;
  else
    WAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 4] |= digit;
  set_w_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt, 5, 0xffffu);
  p_ctx->CurrCodeFill.FullWordCnt += 5;
  return True;
}

static Boolean Put80I_To_32(LargeWord t, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *p_ctx)
{
  unsigned dest, bit_pos;
  LongWord digit;

  if (!IncMaxCodeLen(p_ctx, 3))
    return False;
  memset(&DAsmCode[p_ctx->CurrCodeFill.FullWordCnt], 0, 12);

  if (orig_negative)
    t = (LargeWord)(0 - ((LargeInt)t));
  dest = bit_pos = 0;
  while (t)
  {
    digit = (t % 10) << bit_pos;
    t /= 10;
    if (p_ctx->LoHiMap)
      DAsmCode[p_ctx->CurrCodeFill.FullWordCnt + (2 - dest)] |= digit;
    else
      DAsmCode[p_ctx->CurrCodeFill.FullWordCnt + dest] |= digit;
    if ((bit_pos += 4) >= 32)
    {
      bit_pos = 0;
      dest++;
    }
    if ((dest >= 2) && (bit_pos >= 8) && t)
    {
      WrError(orig_negative ? ErrNum_UnderRange : ErrNum_OverRange);
      return False;
    }
  }
  digit = ((LongWord)!!orig_negative) << 15;
  if (p_ctx->LoHiMap)
  {
    set_d_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt, 1, 0xffffu);
    set_d_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt + 1, 2, 0xfffffffful);
    DAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 0] |= digit;
  }
  else
  {
    set_d_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt, 2, 0xfffffffful);
    set_d_guessed(flags, p_ctx->CurrCodeFill.FullWordCnt + 2, 1, 0xffffu);
    DAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 2] |= digit;
  }
  p_ctx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean Put80F_To_4(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte tmp[10];
  int ret;
  LongInt dest;
  unsigned z;

  if (!IncMaxCodeLen(pCtx, 20))
    return False;

  ret = as_float_2_ieee10(t, tmp, False);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  if (pCtx->LoHiMap)
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt + 20; z < 10; z++)
    {
      BAsmCode[--dest] = (tmp[z] >> 0) & 0xf;
      BAsmCode[--dest] = (tmp[z] >> 4) & 0xf;
    }
  else
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt; z < 10; z++)
    {
      BAsmCode[dest++] = (tmp[z] >> 0) & 0xf;
      BAsmCode[dest++] = (tmp[z] >> 4) & 0xf;
    }
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 20, 0xf);
  pCtx->CurrCodeFill.FullWordCnt += 20;
  return True;
}

static Boolean Put80F_To_8(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 10))
    return False;
  if ((ret = as_float_2_ieee10(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  else
  {
    set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 10, 0xff);
    pCtx->CurrCodeFill.FullWordCnt += 10;
    return True;
  }
}

static Boolean Put80F_To_16(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;
  Byte Tmp[10];
  int LoHiMap = pCtx->LoHiMap & 1;

  if (!IncMaxCodeLen(pCtx, 5))
    return False;
  if ((ret = as_float_2_ieee10(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 5, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ LoHiMap) | ByteInWord(Tmp[1], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[2], 0 ^ LoHiMap) | ByteInWord(Tmp[3], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInWord(Tmp[4], 0 ^ LoHiMap) | ByteInWord(Tmp[5], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 3] = ByteInWord(Tmp[6], 0 ^ LoHiMap) | ByteInWord(Tmp[7], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 4] = ByteInWord(Tmp[8], 0 ^ LoHiMap) | ByteInWord(Tmp[9], 1 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 5;
  return True;
}

static Boolean Put80F_To_32(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;
  Byte Tmp[10];

  if (!IncMaxCodeLen(pCtx, 3))
    return False;
  if ((ret = as_float_2_ieee10(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  if (pCtx->LoHiMap)
  {
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 1, 0xffffu);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInDWord(Tmp[0], 1) | ByteInDWord(Tmp[1], 0);
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt + 1, 2, 0xfffffffful);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInDWord(Tmp[2], 3) | ByteInDWord(Tmp[3], 2) | ByteInDWord(Tmp[4], 1) | ByteInDWord(Tmp[5], 0);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInDWord(Tmp[6], 3) | ByteInDWord(Tmp[7], 2) | ByteInDWord(Tmp[8], 1) | ByteInDWord(Tmp[9], 0);
  }
  else
  {
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 2, 0xfffffffful);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInDWord(Tmp[0], 0) | ByteInDWord(Tmp[1], 1) | ByteInDWord(Tmp[2], 2) | ByteInDWord(Tmp[3], 3);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInDWord(Tmp[4], 0) | ByteInDWord(Tmp[5], 1) | ByteInDWord(Tmp[6], 2) | ByteInDWord(Tmp[7], 3);
    set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt + 2, 1, 0xffffu);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInDWord(Tmp[8], 0) | ByteInDWord(Tmp[9], 1);
  }
  pCtx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean LayoutTenBytes(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  TempResult t;
  Word Cnt;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  Result = False;
  switch(t.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->flags & eIntPseudoFlag_AllowInt)
      {
        if (!pCtx->Put80I(t.Contents.Int, t.Flags, t.Contents.Int < 0, pCtx))
          LEAVE;
        Cnt = 10;
        Result = True;
        break;
      }
      else
      {
        TempResultToFloat(&t);
        goto ToFloat;
      }
    case TempFloat:
    ToFloat:
      if (!pCtx->Put80F(t.Contents.Float, t.Flags, pCtx))
        LEAVE;
      Cnt = 10;
      Result = True;
      break;
    case TempString:
    {
      Boolean ret;
      unsigned z;

      if (MultiCharToInt(&t, 4))
        goto ToInt;

      if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pExpr))
        LEAVE;

      for (z = 0; z < t.Contents.str.len; z++)
      {
        ret = (pCtx->flags & eIntPseudoFlag_AllowInt)
            ? pCtx->Put80I(t.Contents.str.p_str[z], t.Flags, False, pCtx)
            : pCtx->Put80F(t.Contents.str.p_str[z], t.Flags, pCtx);
        if (!ret)
          LEAVE;
      }

      Cnt = t.Contents.str.len * 10;
      Result = True;
      break;
    }
    case TempReg:
      WrStrErrorPos(ErrNum_StringOrIntOrFloatButReg, pExpr);
      break;
    case TempAll:
      assert(0);
  }
  (void)Cnt;

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutOctaWord
 * Purpose:     parse argument, interprete as 128-bit word or
                double precision float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put128I_To_1(LargeWord l, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 128))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 128, 0x01);

  for (z = 0; z < min(128, LARGEBITS); z++, l >>= 1)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0x01;
  for (; z < 128; z++)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = orig_negative ? 0x01 : 0x00;

  pCtx->CurrCodeFill.FullWordCnt += 128;
  return True;
}

static Boolean Put128F_To_1(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret, z;
  Byte tmp[16];

  if (!IncMaxCodeLen(pCtx, 128))
    return False;
  if ((ret = as_float_2_ieee16(t, tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  for (z = 0; z < 16; z++)
    Put8I_To_1(tmp[z ^ (pCtx->LoHiMap & 15)], flags, pCtx);

  return True;
}

static Boolean Put128I_To_4(LargeWord l, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 32))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 32, 0xf);
  for (z = 0; z < min(32, LARGEBITS / 4); z++, l >>= 4)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0xf;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 32; z++)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = orig_negative ? 0xff : 0x00;

  pCtx->CurrCodeFill.FullWordCnt += 32;
  return True;
}

static Boolean Put128F_To_4(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte tmp[16];
  int ret;
  unsigned z;
  LongInt dest;

  if (!IncMaxCodeLen(pCtx, 32))
    return False;

  if (pCtx->flags & eIntPseudoFlag_DECFormat)
  {
    Word tmp_w[8];

    ret = as_float_2_dec_h(t, tmp_w);
    if (ret >= 0)
    {
      tmp[ 0] = Lo(tmp_w[0]);
      tmp[ 1] = Hi(tmp_w[0]);
      tmp[ 2] = Lo(tmp_w[1]);
      tmp[ 3] = Hi(tmp_w[1]);
      tmp[ 4] = Lo(tmp_w[2]);
      tmp[ 5] = Hi(tmp_w[2]);
      tmp[ 6] = Lo(tmp_w[3]);
      tmp[ 7] = Hi(tmp_w[3]);
      tmp[ 8] = Lo(tmp_w[4]);
      tmp[ 9] = Hi(tmp_w[4]);
      tmp[10] = Lo(tmp_w[5]);
      tmp[11] = Hi(tmp_w[5]);
      tmp[12] = Lo(tmp_w[6]);
      tmp[13] = Hi(tmp_w[6]);
      tmp[14] = Lo(tmp_w[7]);
      tmp[15] = Hi(tmp_w[7]);
    }
  }
  else
    ret = as_float_2_ieee16(t, tmp, False);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }

  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 32, 0xf);
  if (pCtx->LoHiMap)
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt + 32; z < 16; z++)
    {
      BAsmCode[--dest] = (tmp[z] >> 0) & 0xf;
      BAsmCode[--dest] = (tmp[z] >> 4) & 0xf;
    }
  else
    for (z = 0, dest = pCtx->CurrCodeFill.FullWordCnt; z < 16; z++)
    {
      BAsmCode[dest++] = (tmp[z] >> 0) & 0xf;
      BAsmCode[dest++] = (tmp[z] >> 4) & 0xf;
    }
  pCtx->CurrCodeFill.FullWordCnt += 32;
  return True;
}


static Boolean Put128I_To_8(LargeWord l, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 16, 0xff);
  for (z = 0; z < min(16, LARGEBITS / 8); z++, l >>= 8)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0xff;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 16; z++)
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = orig_negative ? 0xff : 0x00;
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put128F_To_8(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  if (pCtx->flags & eIntPseudoFlag_DECFormats)
  {
    Word *p_dest = WAsmCode + (pCtx->CurrCodeFill.FullWordCnt / 2);
    ret = as_float_2_dec_h(t, p_dest);
    if ((ret >= 0) && (HostBigEndian && (ListGran() == 1)))
      WSwap(p_dest, 16);
  }
  else
    ret = as_float_2_ieee16(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap);

  if (ret < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  else
  {
    set_b_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 16, 0xff);
    pCtx->CurrCodeFill.FullWordCnt += 16;
    return True;
  }
}

static Boolean Put128I_To_16(LargeWord l, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0xffffu);
  for (z = 0; z < min(8, LARGEBITS / 16); z++, l >>= 16)
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0xffff;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 8; z++)
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = orig_negative ? 0xffff : 0x0000;
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put128F_To_16(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[16];
  int LoHiMap = pCtx->LoHiMap & 1;
  int ret;

  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  if ((ret = as_float_2_ieee16(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_w_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 8, 0xffffu);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[ 0], 0 ^ LoHiMap) | ByteInWord(Tmp[ 1], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[ 2], 0 ^ LoHiMap) | ByteInWord(Tmp[ 3], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInWord(Tmp[ 4], 0 ^ LoHiMap) | ByteInWord(Tmp[ 5], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 3] = ByteInWord(Tmp[ 6], 0 ^ LoHiMap) | ByteInWord(Tmp[ 7], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 4] = ByteInWord(Tmp[ 8], 0 ^ LoHiMap) | ByteInWord(Tmp[ 9], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 5] = ByteInWord(Tmp[10], 0 ^ LoHiMap) | ByteInWord(Tmp[11], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 6] = ByteInWord(Tmp[12], 0 ^ LoHiMap) | ByteInWord(Tmp[13], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 7] = ByteInWord(Tmp[14], 0 ^ LoHiMap) | ByteInWord(Tmp[15], 1 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put128I_To_32(LargeWord l, tSymbolFlags flags, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  int z;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0xfffffffful);
  for (z = 0; z < min(4, LARGEBITS / 32); z++, largeint_shr32(l))
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = l & 0xfffffffful;
  /* TempResult is TempInt, so sign-extend */
  for (; z < 4; z++)
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (z ^ pCtx->LoHiMap)] = orig_negative ? 0xfffffffful : 0x00000000;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put128F_To_32(as_float_t t, tSymbolFlags flags, struct sLayoutCtx *pCtx)
{
  Byte Tmp[16];
  int LoHiMap = pCtx->LoHiMap ? 3 : 0;
  int ret;

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  if ((ret = as_float_2_ieee16(t, Tmp, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  set_d_guessed(flags, pCtx->CurrCodeFill.FullWordCnt, 4, 0xfffffffful);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInDWord(Tmp[ 0], 0 ^ LoHiMap) | ByteInDWord(Tmp[ 1], 1 ^ LoHiMap) | ByteInDWord(Tmp[ 2], 2 ^ LoHiMap) | ByteInDWord(Tmp[ 3], 3 ^ LoHiMap);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInDWord(Tmp[ 4], 0 ^ LoHiMap) | ByteInDWord(Tmp[ 5], 1 ^ LoHiMap) | ByteInDWord(Tmp[ 6], 2 ^ LoHiMap) | ByteInDWord(Tmp[ 7], 3 ^ LoHiMap);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInDWord(Tmp[ 8], 0 ^ LoHiMap) | ByteInDWord(Tmp[ 9], 1 ^ LoHiMap) | ByteInDWord(Tmp[10], 2 ^ LoHiMap) | ByteInDWord(Tmp[11], 3 ^ LoHiMap);
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 3] = ByteInDWord(Tmp[12], 0 ^ LoHiMap) | ByteInDWord(Tmp[13], 1 ^ LoHiMap) | ByteInDWord(Tmp[14], 2 ^ LoHiMap) | ByteInDWord(Tmp[15], 3 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean LayoutOctaWord(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  const Boolean allow_string = !!(pCtx->flags & eIntPseudoFlag_AllowString),
                allow_float = !!pCtx->Put128F;
  TempResult t;
  Word Cnt  = 0;

  as_tempres_ini(&t);
  EvalStrExpression(pExpr, &t);
  Result = False;
  switch (t.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put128I)
      {
        if (!mFirstPassUnknownOrQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int128)) WrStrErrorPos(ErrNum_OverRange, pExpr);
        else
        {
          if (!pCtx->Put128I(t.Contents.Int, t.Flags, t.Contents.Int < 0, pCtx))
            LEAVE;
          Cnt = 16;
          Result = True;
        }
        break;
      }
      else
        TempResultToFloat(&t);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(ErrNum_StringOrIntButFloat, pExpr);
      else if (!pCtx->Put128F(t.Contents.Float, t.Flags, pCtx))
        LEAVE;
      Cnt = 16;
      Result = True;
      break;
    case TempString:
      if (!allow_string) WrStrErrorPos(allow_float ? ErrNum_IntOrFloatButString : ErrNum_IntButString, pExpr);
      else
      {
         unsigned z;

        if (MultiCharToInt(&t, 16))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pExpr))
          LEAVE;

        for (z = 0; z < t.Contents.str.len; z++)
          if (!pCtx->Put128I(t.Contents.str.p_str[z], t.Flags, False, pCtx))
            LEAVE;

        Cnt = t.Contents.str.len * 16;
        Result = True;
      }
      break;
    case TempReg:
      WrStrErrorPos(ErrNum_StringOrIntOrFloatButReg, pExpr);
      break;
    case TempAll:
      assert(0);
  }
  (void)Cnt;

func_exit:
  as_tempres_free(&t);
  return Result;
}

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

/*****************************************************************************
 * Function:    DecodeIntelPseudo
 * Purpose:     handle Intel-style pseudo instructions
 * Result:      TRUE if mnemonic was handled
 *****************************************************************************/

static Boolean DecodeIntelPseudo_ValidSymChar(char ch)
{
  ch = as_toupper(ch);

  return (((ch >= 'A') && (ch <= 'Z'))
       || ((ch >= '0') && (ch <= '9'))
       || (ch == '_')
       || (ch == '.'));
}

static void DecodeIntelPseudo_HandleQuote(int *pDepth, Byte *pQuote, char Ch)
{
  switch (Ch)
  {
    case '(':
      if (!(*pQuote))
        (*pDepth)++;
      break;
    case ')':
      if (!(*pQuote))
        (*pDepth)--;
      break;
    case '\'':
      if (!((*pQuote) & 2))
        (*pQuote) ^= 1;
      break;
    case '"':
      if (!((*pQuote) & 1))
        (*pQuote) ^= 2;
      break;
  }
}

static Boolean DecodeIntelPseudo_LayoutMult(const tStrComp *pArg, struct sLayoutCtx *pCtx)
{
  int z, Depth, Len, LastNonBlank;
  Boolean OK, LastValid, Result, DupIsRep;
  Byte Quote;
  const char *pDupFnd, *pRun;
  const tStrComp *pSaveComp;

  pSaveComp = pCtx->pCurrComp;
  pCtx->pCurrComp = pArg;

  /* search for DUP:
     - Exclude parts in parentheses, and parts in quotation marks.
     - Assure there is some (non-blank) token before DUP, so if there
       is e.g. a plain DUP as argument, it will not be interpreted as
       DUP operator. */

  Depth = Quote = 0;
  LastValid = FALSE;
  LastNonBlank = -1;
  pDupFnd = NULL; DupIsRep = False;
  Len = strlen(pArg->str.p_str);
  for (pRun = pArg->str.p_str; pRun < pArg->str.p_str + Len - 2; pRun++)
  {
    DecodeIntelPseudo_HandleQuote(&Depth, &Quote, *pRun);
    if (!Depth && !Quote)
    {
      if (!LastValid
       && (LastNonBlank >= 0)
       && !DecodeIntelPseudo_ValidSymChar(pRun[3])
       && !as_strncasecmp(pRun, "DUP", 3))
      {
        pDupFnd = pRun;
        break;
      }
      if (!as_isspace(*pRun))
        LastNonBlank = pRun - pArg->str.p_str;
    }
    LastValid = DecodeIntelPseudo_ValidSymChar(*pRun);
  }

  if (!pDupFnd && (pCtx->flags & eIntPseudoFlag_MotoRep))
  {
    if (*pArg->str.p_str == '[')
    {
      pDupFnd = QuotPos(pArg->str.p_str + 1, ']');
      if (pDupFnd)
        DupIsRep = True;
    }
  }

  /* found DUP: */

  if (pDupFnd)
  {
    LongInt DupCnt;
    char *pSep, *pRun;
    String CopyStr;
    tStrComp Copy, DupArg, RemArg, ThisRemArg;
    tCurrCodeFill DUPStartFill, DUPEndFill;
    tSymbolFlags Flags;

    /* operate on copy */

    StrCompMkTemp(&Copy, CopyStr, sizeof(CopyStr));
    StrCompCopy(&Copy, pArg);
    pSep = Copy.str.p_str + (pDupFnd - pArg->str.p_str);
    StrCompSplitRef(&DupArg, &RemArg, &Copy, pSep);
    if (DupIsRep)
      StrCompIncRefLeft(&DupArg, 1);
    else
      StrCompIncRefLeft(&RemArg, 2);

    /* evaluate count */

    DupCnt = EvalStrIntExpressionWithFlags(&DupArg, Int32, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
    {
      WrStrErrorPos(ErrNum_FirstPassCalc, &DupArg); return False;
    }
    if (!OK)
    {
      Result = False;
      goto func_exit;
    }

    /* catch invalid counts */

    if (DupCnt <= 0)
    {
      if (DupCnt < 0)
        WrStrErrorPos(ErrNum_NegDUP, &DupArg);
      Result = True;
      goto func_exit;
    }

    /* split into parts and evaluate */

    KillPrefBlanksStrCompRef(&RemArg);
    Len = strlen(RemArg.str.p_str);
    if ((Len >= 2) && (*RemArg.str.p_str == '(') && (RemArg.str.p_str[Len - 1] == ')'))
    {
      StrCompIncRefLeft(&RemArg, 1);
      StrCompShorten(&RemArg, 1);
      Len -= 2;
    }
    DUPStartFill = pCtx->CurrCodeFill;
    do
    {
      pSep = NULL; Quote = Depth = 0;
      for (pRun = RemArg.str.p_str; *pRun; pRun++)
      {
        DecodeIntelPseudo_HandleQuote(&Depth, &Quote, *pRun);
        if (!Depth && !Quote && (*pRun == ','))
        {
          pSep = pRun;
          break;
        }
      }
      if (pSep)
        StrCompSplitRef(&RemArg, &ThisRemArg, &RemArg, pSep);
      KillPrefBlanksStrCompRef(&RemArg);
      KillPostBlanksStrComp(&RemArg);
      if (!DecodeIntelPseudo_LayoutMult(&RemArg, pCtx))
      {
        Result = False;
        goto func_exit;
      }
      if (pSep)
        RemArg = ThisRemArg;
    }
    while (pSep);
    DUPEndFill = pCtx->CurrCodeFill;

    /* replicate result (data or reserve) */

    switch (pCtx->DSFlag)
    {
      case DSConstant:
        for (z = 1; z <= DupCnt - 1; z++)
          if (!pCtx->Replicate(&DUPStartFill, &DUPEndFill, pCtx))
          {
            Result = False;
            goto func_exit;
          }
        break;
      case DSSpace:
      {
        tCurrCodeFill Diff;

        SubCodeFill(&Diff, &DUPEndFill, &DUPStartFill, pCtx);
        MultCodeFill(&Diff, DupCnt - 1, pCtx);
        IncCodeFillBy(&pCtx->CurrCodeFill, &Diff, pCtx);
        break;
      }
      default:
        Result = False;
        goto func_exit;
    }

    Result = True;
  }

  /* no DUP: simple expression.  Differentiate space reservation & data disposition */

  else if (!strcmp(pArg->str.p_str, "?"))
  {
    Result = SetDSFlag(pCtx, DSSpace);
    if (Result)
      IncCodeFillBy(&pCtx->CurrCodeFill, &pCtx->FillIncPerElem, pCtx);
  }

  else
    Result = SetDSFlag(pCtx, DSConstant) && pCtx->LayoutFunc(pArg, pCtx);

func_exit:
  pCtx->pCurrComp = pSaveComp;
  return Result;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDx(tLayoutCtx *pLayoutCtx)
 * \brief  Intel-style constant disposition
 * \param  pLayoutCtx layout infos & context
 * ------------------------------------------------------------------------ */

static void DecodeIntelDx(tLayoutCtx *pLayoutCtx)
{
  tStrComp *pArg;
  Boolean OK;
  int full_word_bits;

  pLayoutCtx->DSFlag = DSNone;
  pLayoutCtx->FullWordSize = Grans[ActPC];
  full_word_bits = (8 * pLayoutCtx->FullWordSize) - grans_bits_unused[ActPC];
  if ((pLayoutCtx->FullWordSize == 1) && !(pLayoutCtx->flags & eIntPseudoFlag_DECFormats))
    pLayoutCtx->ListGran = 1;
  else
    pLayoutCtx->ListGran = ActListGran;
  pLayoutCtx->ElemsPerFullWord = full_word_bits / pLayoutCtx->BaseElemLenBits;
  if (pLayoutCtx->ElemsPerFullWord > 1)
  {
    pLayoutCtx->FillIncPerElem.FullWordCnt = 0;
    pLayoutCtx->FillIncPerElem.LastWordFill = 1;
  }
  else
  {
    pLayoutCtx->FillIncPerElem.FullWordCnt = pLayoutCtx->BaseElemLenBits / full_word_bits;
    pLayoutCtx->FillIncPerElem.LastWordFill = 0;
  }

  OK = True;
  forallargs(pArg, OK)
  {
    if (!*pArg->str.p_str)
    {
      OK = FALSE;
      WrStrErrorPos(ErrNum_EmptyArgument, pArg);
    }
    else
      OK = DecodeIntelPseudo_LayoutMult(pArg, pLayoutCtx);
  }

  /* Finalize: add optional padding if fractions of full words
     remain unused & set code length */

  if (OK)
  {
    if (pLayoutCtx->CurrCodeFill.LastWordFill)
    {
      WrError(ErrNum_PaddingAdded);
      pLayoutCtx->CurrCodeFill.LastWordFill = 0;
      pLayoutCtx->CurrCodeFill.FullWordCnt++;
    }
    CodeLen = pLayoutCtx->CurrCodeFill.FullWordCnt;
  }

  DontPrint = (pLayoutCtx->DSFlag == DSSpace);
  if (DontPrint)
  {
    BookKeeping();
    if (!CodeLen && OK) WrError(ErrNum_NullResMem);
  }
  if (OK)
    ActListGran = pLayoutCtx->ListGran;
}

/*!------------------------------------------------------------------------
 * \fn     resolve_flags(int_pseudo_flags_t flags)
 * \brief  resolve actual endianess
 * \param  flags given by caller
 * \return effective flags
 * ------------------------------------------------------------------------ */

static int_pseudo_flags_t resolve_flags(int_pseudo_flags_t flags)
{
  switch (flags & eIntPseudoFlag_EndianMask)
  {
    case eIntPseudoFlag_DynEndian:
      flags &= ~eIntPseudoFlag_EndianMask;
      flags |= TargetBigEndian ? eIntPseudoFlag_BigEndian : eIntPseudoFlag_LittleEndian;
      break;
    case eIntPseudoFlag_LittleEndian:
    case eIntPseudoFlag_BigEndian:
      break;
    default:
      fprintf(stderr, "%s: undefined endianess\n", OpPart.str.p_str);
      abort();
  }
  return flags;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelD1(Word Flags)
 * \brief  Intel-style constant disposition - bits
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelD1(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutBit;
  LayoutCtx.BaseElemLenBits = 1;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put1I = Put1I_To_1;
          LayoutCtx.Replicate = Replicate1_To_1;
          break;
        case 4:
          LayoutCtx.Put1I = Put1I_To_4;
          LayoutCtx.Replicate = Replicate1_To_4;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
          break;
        case 0:
          LayoutCtx.Put1I = Put1I_To_8;
          LayoutCtx.Replicate = Replicate1_To_8;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
          break;
        default:
          goto unhandled;
      }
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put1I = Put1I_To_16;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 15 : 0;
      LayoutCtx.Replicate = Replicate1_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put1I = Put1I_To_32;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 31 : 0;
      LayoutCtx.Replicate = Replicate1_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DN for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDN(Word Flags)
 * \brief  Intel-style constant disposition - nibbles
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDN(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutNibble;
  LayoutCtx.BaseElemLenBits = 4;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put4I = Put4I_To_1;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
          LayoutCtx.Replicate = Replicate4_To_4;
          break;
        case 4:
          LayoutCtx.Put4I = Put4I_To_4;
          LayoutCtx.Replicate = Replicate4_To_4;
          break;
        case 0:
          LayoutCtx.Put4I = Put4I_To_8;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
          LayoutCtx.Replicate = Replicate4_To_8;
          break;
        default:
          goto unhandled;
      }
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put4I = Put4I_To_16;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
      LayoutCtx.Replicate = Replicate4_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put4I = Put4I_To_32;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
      LayoutCtx.Replicate = Replicate4_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DN for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDB(Word BigEndian)
 * \brief  Intel-style constant disposition - bytes
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDB(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutByte;
  LayoutCtx.BaseElemLenBits = 8;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put8I = Put8I_To_1;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
          break;
        case 4:
          LayoutCtx.Put8I = Put8I_To_4;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
          break;
        case 0:
          LayoutCtx.Put8I = Put8I_To_8;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put8I = Put8I_To_16;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate8_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put8I = Put8I_To_32;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
      LayoutCtx.Replicate = Replicate8_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DB for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize8Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDW(Word Flags)
 * \brief  Intel-style constant disposition - words
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDW(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutWord;
  LayoutCtx.BaseElemLenBits = 16;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put16I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put16I_To_1 : NULL;
          LayoutCtx.Put16F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_1 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 15 : 0;
          break;
        case 4:
          LayoutCtx.Put16I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put16I_To_4 : NULL;
          LayoutCtx.Put16F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_4 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
          break;
        case 0:
          LayoutCtx.Put16I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put16I_To_8 : NULL;
          LayoutCtx.Put16F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_8 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put16I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put16I_To_16 : NULL;
      LayoutCtx.Put16F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_16 : NULL;
      LayoutCtx.Replicate = Replicate16ToN_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put16I = (Flags & eIntPseudoFlag_AllowInt) ? Put16I_To_32 : NULL;
      LayoutCtx.Put16F = (Flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_32 : NULL;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate16_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DW for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize16Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDD(Word Flags)
 * \brief  Intel-style constant disposition - 32-bit words
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDD(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutDoubleWord;
  LayoutCtx.BaseElemLenBits = 32;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put32I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put32I_To_1 : NULL;
          LayoutCtx.Put32F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_1 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 31 : 0;
          break;
        case 4:
          LayoutCtx.Put32I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put32I_To_4 : NULL;
          LayoutCtx.Put32F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_4 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
          break;
        case 0:
          LayoutCtx.Put32I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put32I_To_8 : NULL;
          LayoutCtx.Put32F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_8 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put32I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put32I_To_16 : NULL;
      LayoutCtx.Put32F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_16 : NULL;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate16ToN_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put32I = (Flags & eIntPseudoFlag_AllowInt) ? Put32I_To_32 : NULL;
      LayoutCtx.Put32F = (Flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_32 : NULL;
      LayoutCtx.Replicate = Replicate32ToN_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DD for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize32Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDM(Word Flags)
 * \brief  Intel-style constant disposition - 48-bit words
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDM(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutMacAddr;
  LayoutCtx.BaseElemLenBits = 48;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put48I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put48I_To_1 : NULL;
          LayoutCtx.Put48F = NULL;
          break;
        case 4:
          LayoutCtx.Put48I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put48I_To_4 : NULL;
          LayoutCtx.Put48F = NULL;
          break;
        case 0:
          LayoutCtx.Put48I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put48I_To_8 : NULL;
          LayoutCtx.Put48F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put48F_To_8 : NULL;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 5 : 0;
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put48I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put48I_To_16 : NULL;
      LayoutCtx.Put48F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put48F_To_16 : NULL;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 2 : 0;
      LayoutCtx.Replicate = Replicate16ToN_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put48I = (Flags & eIntPseudoFlag_AllowInt) ? Put48I_To_32 : NULL;
      LayoutCtx.Put48F = (Flags & eIntPseudoFlag_AllowFloat) ? Put48F_To_32 : NULL;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate32ToN_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DM for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize48Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDQ(Word Flags)
 * \brief  Intel-style constant disposition - 64-bit words
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDQ(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutQuadWord;
  LayoutCtx.BaseElemLenBits = 64;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put64I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put64I_To_1 : NULL;
          LayoutCtx.Put64F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_1 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 63 : 0;
          break;
        case 4:
          LayoutCtx.Put64I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put64I_To_4 : NULL;
          LayoutCtx.Put64F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_4 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 15 : 0;
          break;
        case 0:
          LayoutCtx.Put64I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put64I_To_8 : NULL;
          LayoutCtx.Put64F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_8 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put64I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put64I_To_16 : NULL;
      LayoutCtx.Put64F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_16 : NULL;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
      LayoutCtx.Replicate = Replicate16ToN_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put64I = (Flags & eIntPseudoFlag_AllowInt) ? Put64I_To_32 : NULL;
      LayoutCtx.Put64F = (Flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_32 : NULL;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate32ToN_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DQ for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize64Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDT(Word Flags)
 * \brief  Intel-style constant disposition - 80-bit words
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDT(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutTenBytes;
  LayoutCtx.BaseElemLenBits = 80;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 4:
          LayoutCtx.Put80F = Put80F_To_4;
          LayoutCtx.Put80I = Put80I_To_4;
          break;
        case 0:
          LayoutCtx.Put80F = Put80F_To_8;
          LayoutCtx.Put80I = Put80I_To_8;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put80F = Put80F_To_16;
      LayoutCtx.Put80I = Put80I_To_16;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate16ToN_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put80F = Put80F_To_32;
      LayoutCtx.Put80I = Put80I_To_32;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      LayoutCtx.Replicate = Replicate32ToN_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DT for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize80Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDO(Word Flags)
 * \brief  Intel-style constant disposition - 128-bit words
 * \param  Flags Data Type & Endianess Flags
 * ------------------------------------------------------------------------ */

void DecodeIntelDO(Word Flags)
{
  tLayoutCtx LayoutCtx;

  memset(&LayoutCtx, 0, sizeof(LayoutCtx));
  LayoutCtx.LayoutFunc = LayoutOctaWord;
  LayoutCtx.BaseElemLenBits = 128;
  LayoutCtx.flags = resolve_flags((int_pseudo_flags_t)Flags);
  switch (Grans[ActPC])
  {
    case 1:
      switch (grans_bits_unused[ActPC])
      {
        case 7:
          LayoutCtx.Put128I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put128I_To_1 : NULL;
          LayoutCtx.Put128F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_1 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 127 : 0;
          break;
        case 4:
          LayoutCtx.Put128I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put128I_To_4 : NULL;
          LayoutCtx.Put128F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_4 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 31 : 0;
          break;
        case 0:
          LayoutCtx.Put128I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put128I_To_8 : NULL;
          LayoutCtx.Put128F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_8 : NULL;
          LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 15 : 0;
          break;
        default:
          goto unhandled;
      }
      LayoutCtx.Replicate = Replicate8ToN_To_8;
      break;
    case 2:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put128I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put128I_To_16 : NULL;
      LayoutCtx.Put128F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_16 : NULL;
      LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
      LayoutCtx.Replicate = Replicate16ToN_To_16;
      break;
    case 4:
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put128I = (Flags & eIntPseudoFlag_AllowInt) ? Put128I_To_32 : NULL;
      LayoutCtx.Put128F = (Flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_32 : NULL;
      LayoutCtx.LoHiMap = (Flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
      LayoutCtx.Replicate = Replicate32ToN_To_32;
      break;
    default:
    unhandled:
      fprintf(stderr, "implement DO for %u-bit words\n",
              (unsigned)(Grans[ActPC] * 8 - grans_bits_unused[ActPC]));
      exit(255);
  }
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, eSymbolSize128Bit);
  DecodeIntelDx(&LayoutCtx);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIntelDS(Word item_size)
 * \brief  Intel-style memory reservation
 * \param  item_size # of bytes per reserved item
 * ------------------------------------------------------------------------ */

void DecodeIntelDS(Word item_size)
{
  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;
    Boolean OK;
    LongInt HVal = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);

    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    else if (OK)
    {
      DontPrint = True;
      CodeLen = HVal * (LongInt)item_size;
      if (!HVal)
        WrError(ErrNum_NullResMem);
      BookKeeping();
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     AddIntelPseudo(PInstTable p_inst_table, int_pseudo_flags_t flags)
 * \brief  add Intelstyle pseudo instructions to hash table
 * \param  p_inst_table table to augment
 * \param  flags additional flags to parse to functions
 * ------------------------------------------------------------------------ */

void AddIntelPseudo(PInstTable p_inst_table, int_pseudo_flags_t flags)
{
  AddInstTable(p_inst_table, "DN", flags | eIntPseudoFlag_AllowInt, DecodeIntelDN);
  AddInstTable(p_inst_table, "DB", flags | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDB);
  AddInstTable(p_inst_table, "DW", flags | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_AllowFloat, DecodeIntelDW);
  AddInstTable(p_inst_table, "DD", flags | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_AllowFloat, DecodeIntelDD);
  AddInstTable(p_inst_table, "DQ", flags | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_AllowFloat, DecodeIntelDQ);
  AddInstTable(p_inst_table, "DT", flags | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_AllowFloat, DecodeIntelDT);
  AddInstTable(p_inst_table, "DO", flags | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_AllowFloat, DecodeIntelDO);
  AddInstTable(p_inst_table, "DS", 1, DecodeIntelDS);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeZ80SYNTAX(Word Code)
 * \brief  change Z80 syntax support for target
 * ------------------------------------------------------------------------ */

static void DecodeZ80SYNTAX(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tStrComp TmpComp;

    StrCompMkTemp(&TmpComp, Z80SyntaxName, 0);
    NLS_UpString(ArgStr[1].str.p_str);
    if (!as_strcasecmp(ArgStr[1].str.p_str, "OFF"))
    {
      CurrZ80Syntax = eSyntax808x;
      EnterIntSymbol(&TmpComp, 0, SegNone, True);
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "ON"))
    {
      CurrZ80Syntax = eSyntaxBoth;
      EnterIntSymbol(&TmpComp, 1, SegNone, True);
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "EXCLUSIVE"))
    {
      CurrZ80Syntax = eSyntaxZ80;
      EnterIntSymbol(&TmpComp, 2, SegNone, True);
    }
    else
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkZ80Syntax(tZ80Syntax InstrSyntax)
 * \brief  check whether instruction's syntax (808x/Z80) fits to selected one
 * \param  InstrSyntax instruction syntax
 * \return True if all fine
 * ------------------------------------------------------------------------ */

Boolean ChkZ80Syntax(tZ80Syntax InstrSyntax)
{
  if ((InstrSyntax == eSyntax808x) && (!(CurrZ80Syntax & eSyntax808x)))
  {
    WrStrErrorPos(ErrNum_Z80SyntaxExclusive, &OpPart);
    return False;
  }
  else if ((InstrSyntax == eSyntaxZ80) && (!(CurrZ80Syntax & eSyntaxZ80)))
  {
    WrStrErrorPos(ErrNum_Z80SyntaxNotEnabled, &OpPart);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     AddZ80Syntax(struct sInstTable *InstTable)
 * \brief  add Z80SYNTAX instruction to list & possibly set default
 * \param  InstTable table to add to
 * ------------------------------------------------------------------------ */

void AddZ80Syntax(struct sInstTable *InstTable)
{
  if (!onoff_test_and_set(e_onoff_reg_z80syntax))
  {
    tStrComp TmpComp;

    CurrZ80Syntax = eSyntax808x;
    StrCompMkTemp(&TmpComp, Z80SyntaxName, 0);
    EnterIntSymbol(&TmpComp, 0, SegNone, True);
  }
  AddInstTable(InstTable, "Z80SYNTAX", 0, DecodeZ80SYNTAX);
}
