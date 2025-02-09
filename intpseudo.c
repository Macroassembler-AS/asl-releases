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
  Boolean (*Put4I)(Byte b, struct sLayoutCtx *pCtx);
  Boolean (*Put8I)(Byte b, struct sLayoutCtx *pCtx);
  Boolean (*Put16I)(Word w, struct sLayoutCtx *pCtx);
  Boolean (*Put16F)(as_float_t f, struct sLayoutCtx *pCtx);
  Boolean (*Put32I)(LongWord l, struct sLayoutCtx *pCtx);
  Boolean (*Put32F)(as_float_t f, struct sLayoutCtx *pCtx);
  Boolean (*Put48I)(LargeWord q, struct sLayoutCtx *pCtx);
  Boolean (*Put48F)(as_float_t f, struct sLayoutCtx *pCtx);
  Boolean (*Put64I)(LargeWord q, struct sLayoutCtx *pCtx);
  Boolean (*Put64F)(as_float_t f, struct sLayoutCtx *pCtx);
  Boolean (*Put80I)(LargeWord t, Boolean orig_negative, struct sLayoutCtx *pCtx);
  Boolean (*Put80F)(as_float_t t, struct sLayoutCtx *pCtx);
  Boolean (*Put128I)(LargeWord q, Boolean orig_negative, struct sLayoutCtx *pCtx);
  Boolean (*Put128F)(as_float_t t, struct sLayoutCtx *pCtx);
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
 * \brief  assure xAsmCode has space for at moleast n more full words
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

static LongWord WordInDWord(Word b, int Pos)
{
  return ((LongWord)b) << (Pos << 4);
}

static Byte NibbleFromByte(Byte b, int Pos)
{
  return (b >> (Pos << 2)) & 0x0f;
}

static Byte NibbleFromWord(Word w, int Pos)
{
  return (w >> (Pos << 2)) & 0x0f;
}

static Byte NibbleFromDWord(LongWord w, int Pos)
{
  return (w >> (Pos << 2)) & 0x0f;
}

static Byte ByteFromWord(Word w, int Pos)
{
  return (w >> (Pos << 3)) & 0xff;
}

static Byte ByteFromDWord(LongWord w, int Pos)
{
  return (w >> (Pos << 3)) & 0xff;
}

static Word WordFromDWord(LongWord w, int Pos)
{
  return (w >> (Pos << 4)) & 0xffff;
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
 * Function:    LayoutNibble
 * Purpose:     parse argument, interprete as nibble,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put4I_To_4(Byte b, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
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

static Boolean Put4I_To_8(Byte b, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
    BAsmCode[Pos.FullWordCnt] = NibbleInByte(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  else
    BAsmCode[Pos.FullWordCnt] |= NibbleInByte(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  return True;
}

static Boolean Replicate4_To_8(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = NibbleFromByte(BAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put4I_To_8(b, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put4I_To_16(Byte b, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
    WAsmCode[Pos.FullWordCnt] = NibbleInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  else
    WAsmCode[Pos.FullWordCnt] |= NibbleInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  return True;
}

static Boolean Replicate4_To_16(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = NibbleFromWord(WAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put4I_To_16(b, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Put4I_To_32(Byte b, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
    DAsmCode[Pos.FullWordCnt] = NibbleInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  else
    DAsmCode[Pos.FullWordCnt] |= NibbleInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  return True;
}

static Boolean Replicate4_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = NibbleFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put4I_To_32(b, pCtx))
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
        if (!pCtx->Put4I(t.Contents.Int, pCtx))
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

static Boolean Put8I_To_4(Byte b, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (b >> 0) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (b >> 4) & 0x0f;
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put8I_To_8(Byte b, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  if ((pCtx->ListGran == 1) || !(pCtx->CurrCodeFill.FullWordCnt & 1))
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt] = b;
  else if (pCtx->flags & eIntPseudoFlag_Turn)
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt >> 1] = (((Word)BAsmCode[pCtx->CurrCodeFill.FullWordCnt - 1]) << 8) | b;
  else
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt >> 1] = (((Word)b) << 8) | BAsmCode[pCtx->CurrCodeFill.FullWordCnt - 1];
  pCtx->CurrCodeFill.FullWordCnt++;
  return True;
}

static Boolean Put8I_To_16(Byte b, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
    WAsmCode[Pos.FullWordCnt] = ByteInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  else
    WAsmCode[Pos.FullWordCnt] |= ByteInWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  return True;
}

static Boolean Put8I_To_32(Byte b, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
    DAsmCode[Pos.FullWordCnt] = ByteInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  else
    DAsmCode[Pos.FullWordCnt] |= ByteInDWord(b, Pos.LastWordFill ^ pCtx->LoHiMap);
  return True;
}

static Boolean Replicate8ToN_To_8(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos;

  if (!IncMaxCodeLen(pCtx, pEndPos->FullWordCnt - pStartPos->FullWordCnt))
    return False;

  for (Pos = *pStartPos; Pos.FullWordCnt < pEndPos->FullWordCnt; Pos.FullWordCnt += pCtx->BaseElemLenBits / 8)
  {
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
  Byte b;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = ByteFromWord(WAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put8I_To_16(b, pCtx))
      return False;
    IncCodeFill(&CurrPos, pCtx);
  }

  return True;
}

static Boolean Replicate8_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Byte b;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    b = ByteFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put8I_To_32(b, pCtx))
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
        if (!pCtx->Put8I(t.Contents.Int, pCtx))
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
        if (!pCtx->Put8I(run_len, pCtx))
          LEAVE;
      }
      while (!(ret = as_chartrans_xlate_next(CurrTransTable->p_table, &ch, &p_run, &run_len)))
      {
        if (!pCtx->Put8I(ch, pCtx))
          LEAVE;
      }
      if (ENOENT == ret)
      {
        WrStrErrorPos(ErrNum_UnmappedChar, pExpr);
        LEAVE;
      }
      if (ascii_flags == eIntPseudoFlag_ASCIZ)
      {
        if (!pCtx->Put8I('\0', pCtx))
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

static Boolean Put16I_To_4(Word w, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (w >>  0) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (w >>  4) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (w >>  8) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (w >> 12) & 0x0f;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put16F_To_4(as_float_t t, struct sLayoutCtx *pCtx)
{
  int ret;
  Byte tmp[2];

  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  if ((ret = as_float_2_ieee2(t, tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (tmp[0] >> 0) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (tmp[0] >> 4) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (tmp[1] >> 0) & 0x0f;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (tmp[1] >> 4) & 0x0f;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put16I_To_8(Word w, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if (pCtx->ListGran == 2)
    WAsmCode[pCtx->CurrCodeFill.FullWordCnt >> 1] = w;
  else
  {
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = Lo(w);
    BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = Hi(w);
  }
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put16F_To_8(as_float_t t, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  if ((ret = as_float_2_ieee2(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put16I_To_16(Word w, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt++] = w;
  return True;
}

static Boolean Put16F_To_16(as_float_t t, struct sLayoutCtx *pCtx)
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
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ pCtx->LoHiMap) | ByteInWord(Tmp[1], 1 ^ pCtx->LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 1;
  return True;
}

static Boolean Put16I_To_32(Word w, struct sLayoutCtx *pCtx)
{
  tCurrCodeFill Pos = pCtx->CurrCodeFill;
  if (!IncCurrCodeFill(pCtx))
    return False;
  if (!Pos.LastWordFill)
    DAsmCode[Pos.FullWordCnt] = WordInDWord(w, Pos.LastWordFill ^ pCtx->LoHiMap);
  else
    DAsmCode[Pos.FullWordCnt] |= WordInDWord(w, Pos.LastWordFill ^ pCtx->LoHiMap);
  return True;
}

static Boolean Put16F_To_32(as_float_t t, struct sLayoutCtx *pCtx)
{
  Byte Tmp[2];
  int ret;

  if ((ret = as_float_2_ieee2(t, Tmp, False)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  return Put16I_To_32((((Word) Tmp[1]) << 8) | Tmp[0], pCtx);
}

static Boolean Replicate16_To_32(const tCurrCodeFill *pStartPos, const tCurrCodeFill *pEndPos, struct sLayoutCtx *pCtx)
{
  Word w;
  tCurrCodeFill CurrPos;

  CurrPos = *pStartPos;
  while ((CurrPos.FullWordCnt != pEndPos->FullWordCnt) || (CurrPos.LastWordFill != pEndPos->LastWordFill))
  {
    w = WordFromDWord(DAsmCode[CurrPos.FullWordCnt], CurrPos.LastWordFill ^ pCtx->LoHiMap);
    if (!Put16I_To_32(w, pCtx))
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
          if (!pCtx->Put16I(t.Contents.Int, pCtx))
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
        if (!pCtx->Put16F(t.Contents.Float, pCtx))
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
          if (!pCtx->Put16I(t.Contents.str.p_str[z], pCtx))
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

static Boolean Put32I_To_4(LongWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >>  4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (l >>  8) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l >> 12) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] = (l >> 16) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] = (l >> 20) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] = (l >> 24) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = (l >> 28) & 0xf;
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put32F_To_4(as_float_t t, struct sLayoutCtx *pCtx)
{
  Byte tmp[4];
  int ret;

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

  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (tmp[0] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (tmp[0] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (tmp[1] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (tmp[1] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] = (tmp[2] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] = (tmp[2] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] = (tmp[3] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = (tmp[3] >> 4) & 0xf;
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put32I_To_8(LongWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >>  8) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (l >> 16) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l >> 24) & 0xff;
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put32F_To_8(as_float_t t, struct sLayoutCtx *pCtx)
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

  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put32I_To_16(LongWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = LoWord(l);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = HiWord(l);
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put32F_To_16(as_float_t t, struct sLayoutCtx *pCtx)
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
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ pCtx->LoHiMap) | ByteInWord(Tmp[1], 1 ^ pCtx->LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[2], 0 ^ pCtx->LoHiMap) | ByteInWord(Tmp[3], 1 ^ pCtx->LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put32I_To_32(LongWord w, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 1))
    return False;
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt++] = w;
  return True;
}

static Boolean Put32F_To_32(as_float_t t, struct sLayoutCtx *pCtx)
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
  TempResult erg;
  Boolean Result = False;
  const Boolean allow_string = !!(pCtx->flags & eIntPseudoFlag_AllowString),
                allow_float = !!pCtx->Put32F;
  Word Cnt = 0;

  as_tempres_ini(&erg);
  EvalStrExpression(pExpr, &erg);
  Result = False;
  switch (erg.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put32I)
      {
        if (mFirstPassUnknown(erg.Flags)) erg.Contents.Int &= 0xfffffffful;
        if (!mSymbolQuestionable(erg.Flags) && !RangeCheck(erg.Contents.Int, Int32)) WrStrErrorPos(ErrNum_OverRange, pExpr);
        else
        {
          if (!pCtx->Put32I(erg.Contents.Int, pCtx))
            LEAVE;
          Cnt = 4;
          Result = True;
        }
        break;
      }
      else
        TempResultToFloat(&erg);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(allow_string ? ErrNum_StringOrIntButFloat : ErrNum_IntButFloat, pExpr);
      else
      {
        if (!pCtx->Put32F(erg.Contents.Float, pCtx))
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

        if (MultiCharToInt(&erg, 4))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &erg.Contents.str, pExpr))
          WrStrErrorPos(ErrNum_UnmappedChar, pExpr);

        for (z = 0; z < erg.Contents.str.len; z++)
          if (!pCtx->Put32I(erg.Contents.str.p_str[z], pCtx))
            LEAVE;

        Cnt = erg.Contents.str.len * 4;
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
  as_tempres_free(&erg);
  return Result;
}


/*****************************************************************************
 * Function:    LayoutMacAddr
 * Purpose:     parse argument, interprete as 48-bit word or
                float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put48I_To_8(LargeWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 6))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)      ] = (l      ) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)      ] = (l >>  8) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ (pCtx->LoHiMap & 1))] = (l >> 16) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ (pCtx->LoHiMap & 1))] = (l >> 24) & 0xff;
#ifdef HAS64
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)      ] = (l >> 32) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)      ] = (l >> 40) & 0xff;
#else
  /* TempResult is TempInt, so sign-extend */
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)      ] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)      ] = (l & 0x80000000ul) ? 0xff : 0x00;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 6;
  return True;
}

static Boolean Put48F_To_8(as_float_t t, struct sLayoutCtx *pCtx)
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
  pCtx->CurrCodeFill.FullWordCnt += 6;
  return True;
}

static Boolean Put48I_To_16(LargeWord l, struct sLayoutCtx *pCtx)
{
  int LoHiMap = pCtx->LoHiMap ? 2 : 0; /* 5 or 0 -> 2 or 0 */

  if (!IncMaxCodeLen(pCtx, 3))
    return False;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ LoHiMap)      ] = (l      ) & 0xffff;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ (LoHiMap & 1))] = (l >> 16) & 0xffff;
#ifdef HAS64
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ LoHiMap)      ] = (l >> 32) & 0xffff;
#else
  /* TempResult is TempInt, so sign-extend */
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ LoHiMap)      ] = (l & 0x80000000ul) ? 0xffff : 0x0000;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean Put48F_To_16(as_float_t t, struct sLayoutCtx *pCtx)
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
  pCtx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean Put48I_To_32(LargeWord l, struct sLayoutCtx *pCtx)
{
  int LoHiMap = pCtx->LoHiMap ? 2 : 0; /* 5 or 0 -> 2 or 0 */

  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ LoHiMap)] = (l      ) & 0xfffffffful;
#ifdef HAS64
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ LoHiMap)] = (l >> 32) & 0x0000fffful;
#else
  /* TempResult is TempInt, so sign-extend */
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ LoHiMap)] = (l & 0x80000000ul) ? 0xfffful : 0x0000ul;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put48F_To_32(as_float_t t, struct sLayoutCtx *pCtx)
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
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean LayoutMacAddr(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  TempResult erg;
  Word Cnt  = 0;

  as_tempres_ini(&erg);
  EvalStrExpression(pExpr, &erg);
  Result = False;
  switch(erg.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put48I)
      {
        if (!pCtx->Put64I(erg.Contents.Int, pCtx))
          LEAVE;
        Cnt = 6;
        Result = True;
        break;
      }
      else
        TempResultToFloat(&erg);
      /* fall-through */
    case TempFloat:
      if (!pCtx->Put48F) WrStrErrorPos(ErrNum_StringOrIntButFloat, pExpr);
      else if (!pCtx->Put48F(erg.Contents.Float, pCtx))
        LEAVE;
      Cnt = 6;
      Result = True;
      break;
    case TempString:
    {
      unsigned z;

      if (MultiCharToInt(&erg, 6))
        goto ToInt;

      if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &erg.Contents.str, pExpr))
        LEAVE;

      for (z = 0; z < erg.Contents.str.len; z++)
        if (!pCtx->Put48I(erg.Contents.str.p_str[z], pCtx))
          LEAVE;

      Cnt = erg.Contents.str.len * 6;
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
  as_tempres_free(&erg);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutQuadWord
 * Purpose:     parse argument, interprete as 64-bit word or
                double precision float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put64I_To_4(LargeWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 0 ^ pCtx->LoHiMap)] = (l >>  0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 1 ^ pCtx->LoHiMap)] = (l >>  4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 2 ^ pCtx->LoHiMap)] = (l >>  8) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 3 ^ pCtx->LoHiMap)] = (l >> 12) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 4 ^ pCtx->LoHiMap)] = (l >> 16) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 5 ^ pCtx->LoHiMap)] = (l >> 20) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 6 ^ pCtx->LoHiMap)] = (l >> 24) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 7 ^ pCtx->LoHiMap)] = (l >> 28) & 0xf;
#ifdef HAS64
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 8 ^ pCtx->LoHiMap)] = (l >> 32) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 9 ^ pCtx->LoHiMap)] = (l >> 36) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] = (l >> 40) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] = (l >> 44) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] = (l >> 48) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] = (l >> 52) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] = (l >> 56) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = (l >> 60) & 0xf;
#else
  /* TempResult is TempInt, so sign-extend */
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 8 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 9 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = (l & 0x80000000ul) ? 0xf : 0x0;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put64F_To_4(as_float_t t, struct sLayoutCtx *pCtx)
{
  Byte tmp[8];
  int ret;

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

  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 0 ^ pCtx->LoHiMap)] = (tmp[0] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 1 ^ pCtx->LoHiMap)] = (tmp[0] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 2 ^ pCtx->LoHiMap)] = (tmp[1] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 3 ^ pCtx->LoHiMap)] = (tmp[1] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 4 ^ pCtx->LoHiMap)] = (tmp[2] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 5 ^ pCtx->LoHiMap)] = (tmp[2] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 6 ^ pCtx->LoHiMap)] = (tmp[3] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 7 ^ pCtx->LoHiMap)] = (tmp[3] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 8 ^ pCtx->LoHiMap)] = (tmp[4] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 9 ^ pCtx->LoHiMap)] = (tmp[4] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] = (tmp[5] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] = (tmp[5] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] = (tmp[6] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] = (tmp[6] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] = (tmp[7] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = (tmp[7] >> 4) & 0xf;
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put64I_To_8(LargeWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >>  8) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (l >> 16) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l >> 24) & 0xff;
#ifdef HAS64
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] = (l >> 32) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] = (l >> 40) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] = (l >> 48) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = (l >> 56) & 0xff;
#else
  /* TempResult is TempInt, so sign-extend */
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = (l & 0x80000000ul) ? 0xff : 0x00;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put64F_To_8(as_float_t t, struct sLayoutCtx *pCtx)
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

  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put64I_To_16(LargeWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xffff;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >> 16) & 0xffff;
#ifdef HAS64
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (l >> 32) & 0xffff;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l >> 48) & 0xffff;
#else
  /* TempResult is TempInt, so sign-extend */
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l & 0x80000000ul) ? 0xffff : 0x0000;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put64F_To_16(as_float_t t, struct sLayoutCtx *pCtx)
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
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ LoHiMap) | ByteInWord(Tmp[1], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[2], 0 ^ LoHiMap) | ByteInWord(Tmp[3], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInWord(Tmp[4], 0 ^ LoHiMap) | ByteInWord(Tmp[5], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 3] = ByteInWord(Tmp[6], 0 ^ LoHiMap) | ByteInWord(Tmp[7], 1 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put64I_To_32(LargeWord l, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 2))
    return False;
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xfffffffful;
#ifdef HAS64
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >> 32) & 0xfffffffful;
#else
  /* TempResult is TempInt, so sign-extend */
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l & 0x80000000ul) ? 0xfffffffful : 0x00000000ul;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 2;
  return True;
}

static Boolean Put64F_To_32(as_float_t t, struct sLayoutCtx *pCtx)
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
  TempResult erg;
  Word Cnt  = 0;

  as_tempres_ini(&erg);
  EvalStrExpression(pExpr, &erg);
  Result = False;
  switch (erg.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put64I)
      {
        if (!pCtx->Put64I(erg.Contents.Int, pCtx))
          LEAVE;
        Cnt = 8;
        Result = True;
        break;
      }
      else
        TempResultToFloat(&erg);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(ErrNum_StringOrIntButFloat, pExpr);
      else if (!pCtx->Put64F(erg.Contents.Float, pCtx))
        LEAVE;
      Cnt = 8;
      Result = True;
      break;
    case TempString:
      if (!allow_string) WrStrErrorPos(allow_float ? ErrNum_IntOrFloatButString : ErrNum_IntButString, pExpr);
      else
      {
         unsigned z;

        if (MultiCharToInt(&erg, 8))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &erg.Contents.str, pExpr))
          LEAVE;

        for (z = 0; z < erg.Contents.str.len; z++)
          if (!pCtx->Put64I(erg.Contents.str.p_str[z], pCtx))
            LEAVE;

        Cnt = erg.Contents.str.len * 8;
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
  as_tempres_free(&erg);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutTenBytes
 * Purpose:     parse argument, interprete extended precision float,
 *              and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put80I_To_8(LargeWord t, Boolean orig_negative, struct sLayoutCtx *p_ctx)
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
  p_ctx->CurrCodeFill.FullWordCnt += 10;
  return True;
}

static Boolean Put80I_To_16(LargeWord t, Boolean orig_negative, struct sLayoutCtx *p_ctx)
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
  p_ctx->CurrCodeFill.FullWordCnt += 5;
  return True;
}

static Boolean Put80I_To_32(LargeWord t, Boolean orig_negative, struct sLayoutCtx *p_ctx)
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
    DAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 0] |= digit;
  else
    DAsmCode[p_ctx->CurrCodeFill.FullWordCnt + 2] |= digit;
  p_ctx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean Put80F_To_8(as_float_t t, struct sLayoutCtx *pCtx)
{
  int ret;

  if (!IncMaxCodeLen(pCtx, 10))
    return False;
  if ((ret = as_float_2_ieee10(t, BAsmCode + pCtx->CurrCodeFill.FullWordCnt, !!pCtx->LoHiMap)) < 0)
  {
    asmerr_check_fp_dispose_result(ret, pCtx->pCurrComp);
    return False;
  }
  pCtx->CurrCodeFill.FullWordCnt += 10;
  return True;
}

static Boolean Put80F_To_16(as_float_t t, struct sLayoutCtx *pCtx)
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
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInWord(Tmp[0], 0 ^ LoHiMap) | ByteInWord(Tmp[1], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInWord(Tmp[2], 0 ^ LoHiMap) | ByteInWord(Tmp[3], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInWord(Tmp[4], 0 ^ LoHiMap) | ByteInWord(Tmp[5], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 3] = ByteInWord(Tmp[6], 0 ^ LoHiMap) | ByteInWord(Tmp[7], 1 ^ LoHiMap);
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + 4] = ByteInWord(Tmp[8], 0 ^ LoHiMap) | ByteInWord(Tmp[9], 1 ^ LoHiMap);
  pCtx->CurrCodeFill.FullWordCnt += 5;
  return True;
}

static Boolean Put80F_To_32(as_float_t t, struct sLayoutCtx *pCtx)
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
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInDWord(Tmp[0], 1) | ByteInDWord(Tmp[1], 0);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInDWord(Tmp[2], 3) | ByteInDWord(Tmp[3], 2) | ByteInDWord(Tmp[4], 1) | ByteInDWord(Tmp[5], 0);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInDWord(Tmp[6], 3) | ByteInDWord(Tmp[7], 2) | ByteInDWord(Tmp[8], 1) | ByteInDWord(Tmp[9], 0);
  }
  else
  {
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 0] = ByteInDWord(Tmp[0], 0) | ByteInDWord(Tmp[1], 1) | ByteInDWord(Tmp[2], 2) | ByteInDWord(Tmp[3], 3);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 1] = ByteInDWord(Tmp[4], 0) | ByteInDWord(Tmp[5], 1) | ByteInDWord(Tmp[6], 2) | ByteInDWord(Tmp[7], 3);
    DAsmCode[pCtx->CurrCodeFill.FullWordCnt + 2] = ByteInDWord(Tmp[8], 0) | ByteInDWord(Tmp[9], 1);
  }
  pCtx->CurrCodeFill.FullWordCnt += 3;
  return True;
}

static Boolean LayoutTenBytes(const tStrComp *pExpr, struct sLayoutCtx *pCtx)
{
  Boolean Result = False;
  TempResult erg;
  Word Cnt;

  as_tempres_ini(&erg);
  EvalStrExpression(pExpr, &erg);
  Result = False;
  switch(erg.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->flags & eIntPseudoFlag_AllowInt)
      {
        if (!pCtx->Put80I(erg.Contents.Int, erg.Contents.Int < 0, pCtx))
          LEAVE;
        Cnt = 10;
        Result = True;
        break;
      }
      else
      {
        TempResultToFloat(&erg);
        goto ToFloat;
      }
    case TempFloat:
    ToFloat:
      if (!pCtx->Put80F(erg.Contents.Float, pCtx))
        LEAVE;
      Cnt = 10;
      Result = True;
      break;
    case TempString:
    {
      Boolean ret;
      unsigned z;

      if (MultiCharToInt(&erg, 4))
        goto ToInt;

      if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &erg.Contents.str, pExpr))
        LEAVE;

      for (z = 0; z < erg.Contents.str.len; z++)
      {
        ret = (pCtx->flags & eIntPseudoFlag_AllowInt)
            ? pCtx->Put80I(erg.Contents.str.p_str[z], False, pCtx)
            : pCtx->Put80F(erg.Contents.str.p_str[z], pCtx);
        if (!ret)
          LEAVE;
      }

      Cnt = erg.Contents.str.len * 10;
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
  as_tempres_free(&erg);
  return Result;
}

/*****************************************************************************
 * Function:    LayoutOctaWord
 * Purpose:     parse argument, interprete as 128-bit word or
                double precision float, and put into result buffer
 * Result:      TRUE if no errors occured
 *****************************************************************************/

static Boolean Put128I_To_4(LargeWord l, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 32))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 0 ^ pCtx->LoHiMap)] = (l >>  0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 1 ^ pCtx->LoHiMap)] = (l >>  4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 2 ^ pCtx->LoHiMap)] = (l >>  8) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 3 ^ pCtx->LoHiMap)] = (l >> 12) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 4 ^ pCtx->LoHiMap)] = (l >> 16) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 5 ^ pCtx->LoHiMap)] = (l >> 20) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 6 ^ pCtx->LoHiMap)] = (l >> 24) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 7 ^ pCtx->LoHiMap)] = (l >> 28) & 0xf;
#ifdef HAS64
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 8 ^ pCtx->LoHiMap)] = (l >> 32) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 9 ^ pCtx->LoHiMap)] = (l >> 36) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] = (l >> 40) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] = (l >> 44) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] = (l >> 48) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] = (l >> 52) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] = (l >> 56) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = (l >> 60) & 0xf;
#else
  /* TempResult is TempInt, so sign-extend */
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 8 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 9 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] =
#endif
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (16 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (17 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (18 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (19 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (20 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (21 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (22 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (23 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (24 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (25 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (26 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (27 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (28 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (29 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (30 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (31 ^ pCtx->LoHiMap)] = orig_negative ? 0xff : 0x00;

  pCtx->CurrCodeFill.FullWordCnt += 32;
  return True;
}

static Boolean Put128F_To_4(as_float_t t, struct sLayoutCtx *pCtx)
{
  Byte tmp[16];
  int ret;

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

  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 0 ^ pCtx->LoHiMap)] = (tmp[ 0] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 1 ^ pCtx->LoHiMap)] = (tmp[ 0] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 2 ^ pCtx->LoHiMap)] = (tmp[ 1] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 3 ^ pCtx->LoHiMap)] = (tmp[ 1] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 4 ^ pCtx->LoHiMap)] = (tmp[ 2] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 5 ^ pCtx->LoHiMap)] = (tmp[ 2] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 6 ^ pCtx->LoHiMap)] = (tmp[ 3] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 7 ^ pCtx->LoHiMap)] = (tmp[ 3] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 8 ^ pCtx->LoHiMap)] = (tmp[ 4] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + ( 9 ^ pCtx->LoHiMap)] = (tmp[ 4] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] = (tmp[ 5] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] = (tmp[ 5] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] = (tmp[ 6] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] = (tmp[ 6] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] = (tmp[ 7] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = (tmp[ 7] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (16 ^ pCtx->LoHiMap)] = (tmp[ 8] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (17 ^ pCtx->LoHiMap)] = (tmp[ 8] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (18 ^ pCtx->LoHiMap)] = (tmp[ 9] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (19 ^ pCtx->LoHiMap)] = (tmp[ 9] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (20 ^ pCtx->LoHiMap)] = (tmp[10] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (21 ^ pCtx->LoHiMap)] = (tmp[10] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (22 ^ pCtx->LoHiMap)] = (tmp[11] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (23 ^ pCtx->LoHiMap)] = (tmp[11] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (24 ^ pCtx->LoHiMap)] = (tmp[12] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (25 ^ pCtx->LoHiMap)] = (tmp[12] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (26 ^ pCtx->LoHiMap)] = (tmp[13] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (27 ^ pCtx->LoHiMap)] = (tmp[13] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (28 ^ pCtx->LoHiMap)] = (tmp[14] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (29 ^ pCtx->LoHiMap)] = (tmp[14] >> 4) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (30 ^ pCtx->LoHiMap)] = (tmp[15] >> 0) & 0xf;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (31 ^ pCtx->LoHiMap)] = (tmp[15] >> 4) & 0xf;
  pCtx->CurrCodeFill.FullWordCnt += 32;
  return True;
}


static Boolean Put128I_To_8(LargeWord l, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 16))
    return False;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >>  8) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (l >> 16) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l >> 24) & 0xff;
#ifdef HAS64
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] = (l >> 32) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] = (l >> 40) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] = (l >> 48) & 0xff;
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = (l >> 56) & 0xff;
  /* TempResult is TempInt, so sign-extend */
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (8 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (9 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = orig_negative ? 0xff : 0x00;
#else
  /* TempResult is TempInt, so sign-extend */
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (8 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (9 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (10 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (11 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (12 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (13 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (14 ^ pCtx->LoHiMap)] =
  BAsmCode[pCtx->CurrCodeFill.FullWordCnt + (15 ^ pCtx->LoHiMap)] = orig_negative ? 0xff : 0x00;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put128F_To_8(as_float_t t, struct sLayoutCtx *pCtx)
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

  pCtx->CurrCodeFill.FullWordCnt += 16;
  return True;
}

static Boolean Put128I_To_16(LargeWord l, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 8))
    return False;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xffff;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >> 16) & 0xffff;
#ifdef HAS64
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] = (l >> 32) & 0xffff;
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = (l >> 48) & 0xffff;
  /* TempResult is TempInt, so sign-extend */
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = orig_negative ? 0xffff : 0x0000;
#else
  /* TempResult is TempInt, so sign-extend */
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (4 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (5 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (6 ^ pCtx->LoHiMap)] =
  WAsmCode[pCtx->CurrCodeFill.FullWordCnt + (7 ^ pCtx->LoHiMap)] = orig_negative ? 0xffff : 0x0000;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 8;
  return True;
}

static Boolean Put128F_To_16(as_float_t t, struct sLayoutCtx *pCtx)
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

static Boolean Put128I_To_32(LargeWord l, Boolean orig_negative, struct sLayoutCtx *pCtx)
{
  if (!IncMaxCodeLen(pCtx, 4))
    return False;
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (0 ^ pCtx->LoHiMap)] = (l      ) & 0xfffffffful;
#ifdef HAS64
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] = (l >> 32) & 0xfffffffful;
  /* TempResult is TempInt, so sign-extend */
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] =
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = orig_negative ? 0xfffffffful : 0x00000000ul;
#else
  /* TempResult is TempInt, so sign-extend */
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (1 ^ pCtx->LoHiMap)] =
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (2 ^ pCtx->LoHiMap)] =
  DAsmCode[pCtx->CurrCodeFill.FullWordCnt + (3 ^ pCtx->LoHiMap)] = orig_negative ? 0xfffffffful : 0x00000000ul;
#endif
  pCtx->CurrCodeFill.FullWordCnt += 4;
  return True;
}

static Boolean Put128F_To_32(as_float_t t, struct sLayoutCtx *pCtx)
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
  TempResult erg;
  Word Cnt  = 0;

  as_tempres_ini(&erg);
  EvalStrExpression(pExpr, &erg);
  Result = False;
  switch (erg.Typ)
  {
    case TempNone:
      break;
    case TempInt:
    ToInt:
      if (pCtx->Put128I)
      {
        if (!pCtx->Put128I(erg.Contents.Int, erg.Contents.Int < 0, pCtx))
          LEAVE;
        Cnt = 16;
        Result = True;
        break;
      }
      else
        TempResultToFloat(&erg);
      /* fall-through */
    case TempFloat:
      if (!allow_float) WrStrErrorPos(ErrNum_StringOrIntButFloat, pExpr);
      else if (!pCtx->Put128F(erg.Contents.Float, pCtx))
        LEAVE;
      Cnt = 16;
      Result = True;
      break;
    case TempString:
      if (!allow_string) WrStrErrorPos(allow_float ? ErrNum_IntOrFloatButString : ErrNum_IntButString, pExpr);
      else
      {
         unsigned z;

        if (MultiCharToInt(&erg, 16))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &erg.Contents.str, pExpr))
          LEAVE;

        for (z = 0; z < erg.Contents.str.len; z++)
          if (!pCtx->Put128I(erg.Contents.str.p_str[z], False, pCtx))
            LEAVE;

        Cnt = erg.Contents.str.len * 16;
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
  as_tempres_free(&erg);
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

  pLayoutCtx->DSFlag = DSNone;
  pLayoutCtx->FullWordSize = Grans[ActPC];
  if ((pLayoutCtx->FullWordSize == 1) && !(pLayoutCtx->flags & eIntPseudoFlag_DECFormats))
    pLayoutCtx->ListGran = 1;
  else
    pLayoutCtx->ListGran = ActListGran;
  pLayoutCtx->ElemsPerFullWord = (8 * pLayoutCtx->FullWordSize) / pLayoutCtx->BaseElemLenBits;
  if (pLayoutCtx->ElemsPerFullWord > 1)
  {
    pLayoutCtx->FillIncPerElem.FullWordCnt = 0;
    pLayoutCtx->FillIncPerElem.LastWordFill = 1;
  }
  else
  {
    pLayoutCtx->FillIncPerElem.FullWordCnt = pLayoutCtx->BaseElemLenBits / (8 * pLayoutCtx->FullWordSize);
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
      if (grans_bits_unused[ActPC] >= 4)
      {
         LayoutCtx.Put4I = Put4I_To_4;
         LayoutCtx.Replicate = Replicate4_To_4;
      }
      else
      {
        LayoutCtx.Put4I = Put4I_To_8;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
        LayoutCtx.Replicate = Replicate4_To_8;
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
      if (grans_bits_unused[ActPC] >= 4)
      {
        LayoutCtx.Put8I = Put8I_To_4;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
      }
      else
        LayoutCtx.Put8I = Put8I_To_8;
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
      if (grans_bits_unused[ActPC] >= 4)
      {
        LayoutCtx.Put16I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put16I_To_4 : NULL;
        LayoutCtx.Put16F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_4 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
      }
      else
      {
        LayoutCtx.Put16I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put16I_To_8 : NULL;
        LayoutCtx.Put16F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put16F_To_8 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 1 : 0;
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
      if (grans_bits_unused[ActPC])
        goto unhandled;
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
      if (grans_bits_unused[ActPC] >= 4)
      {
        LayoutCtx.Put32I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put32I_To_4 : NULL;
        LayoutCtx.Put32F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_4 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
      }
      else
      {
        LayoutCtx.Put32I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put32I_To_8 : NULL;
        LayoutCtx.Put32F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put32F_To_8 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 3 : 0;
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
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put48I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put48I_To_8 : NULL;
      LayoutCtx.Put48F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put48F_To_8 : NULL;
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
      if (grans_bits_unused[ActPC] >= 4)
      {
        LayoutCtx.Put64I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put64I_To_4 : NULL;
        LayoutCtx.Put64F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_4 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 15 : 0;
      }
      else
      {
        LayoutCtx.Put64I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put64I_To_8 : NULL;
        LayoutCtx.Put64F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put64F_To_8 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 7 : 0;
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
      if (grans_bits_unused[ActPC])
        goto unhandled;
      LayoutCtx.Put80F = Put80F_To_8;
      LayoutCtx.Put80I = Put80I_To_8;
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
      if (grans_bits_unused[ActPC] >= 4)
      {
        LayoutCtx.Put128I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put128I_To_4 : NULL;
        LayoutCtx.Put128F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_4 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 31 : 0;
      }
      else
      {
        LayoutCtx.Put128I = (LayoutCtx.flags & eIntPseudoFlag_AllowInt) ? Put128I_To_8 : NULL;
        LayoutCtx.Put128F = (LayoutCtx.flags & eIntPseudoFlag_AllowFloat) ? Put128F_To_8 : NULL;
        LayoutCtx.LoHiMap = (LayoutCtx.flags & eIntPseudoFlag_BigEndian) ? 15 : 0;
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
