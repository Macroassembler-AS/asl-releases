/* codepseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Haeufiger benutzte Pseudo-Befehle                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "nls.h"
#include "bpemu.h"
#include "be_le.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmitree.h"
#include "errmsg.h"

#include "codepseudo.h"
#include "motpseudo.h"

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

/*****************************************************************************
 * Function:    IsIndirect
 * Purpose:     check whether argument is syntactically 'indirect', i.e.
 *              enclosed in 'extra' parentheses.
 * Result:      TRUE if indirect
 *****************************************************************************/

Boolean IsIndirectGen(const char *Asc, const char *pBeginEnd)
{
  int z,Level,l;

  if (((l = strlen(Asc)) <= 2)
   || (Asc[0] != pBeginEnd[0])
   || (Asc[l - 1] != pBeginEnd[1]))
    return False;

  Level = 0;
  for (z = 1; z <= l - 2; z++)
  {
    if (Asc[z] == pBeginEnd[0]) Level++;
    if (Asc[z] == pBeginEnd[1]) Level--;
    if (Level < 0) return False;
  }

  return True;
}

Boolean IsIndirect(const char *Asc)
{
  return IsIndirectGen(Asc, "()");
}

/*!------------------------------------------------------------------------
 * \fn     FindDispBaseSplitWithQualifier(const char *pArg, int *pArgLen, tDispBaseSplitQualifier Qualifier, const char *pBracks)
 * \brief  check for argument of type xxx(yyyy)
 * \param  pArg argument to check
 * \param  pArgLen returns argument length
 * \param  Qualifier possible qualifier to allow more positive decisions
 * \param  pBracks Opening/Closing Parentheses
 * \return index to opening parenthese or -1 if not like pattern
 * ------------------------------------------------------------------------ */

typedef struct
{
  as_quoted_iterator_cb_data_t data;
  int nest, split_pos, last_nonspace_pos;
  char last_nonspace;
  tDispBaseSplitQualifier qualifier;
  const char *p_bracks;
} disp_base_split_cb_data_t;

/* We are looking for expressions of the form xxx(yyy),
   but we want to avoid false positives on things like
   xxx+(yyy*zzz).  So we look at the (non-blank) character
   right before the opening parenthese in question.  If it is
   something that might be the last letter of an identifier,
   or another parenthized expression, and not an operator,
   it might be OK...

   We generally look for the last candidate in the string,
   i.e. we continue to search after a finding.
 */

static int disp_base_split_cb(const char *p_pos, as_quoted_iterator_cb_data_t *p_cb_data)
{
  disp_base_split_cb_data_t *p_data = (disp_base_split_cb_data_t*)p_cb_data;
  int pos = p_pos - p_cb_data->p_str;

  if (*p_pos == p_data->p_bracks[0])
  {
    if (!p_data->nest)
    {
      if ((p_data->last_nonspace_pos < 0) || as_isalnum(p_data->last_nonspace) || (p_data->last_nonspace == ')') || (p_data->last_nonspace == '\'') || (p_data->last_nonspace == '"'))
        p_data->split_pos = pos;
      else if (p_data->qualifier)
      {
        int qual = p_data->qualifier(p_cb_data->p_str, p_data->last_nonspace_pos, pos);
        if (qual >= 0)
          p_data->split_pos = qual;
      }
    }
    p_data->nest++;
  }
  else if (*p_pos == p_data->p_bracks[1])
    p_data->nest--;
  if (!as_isspace(*p_pos))
  {
    p_data->last_nonspace_pos = pos;
    p_data->last_nonspace = *p_pos;
  }
  return 0;
}

int FindDispBaseSplitWithQualifier(const char *pArg, int *pArgLen, tDispBaseSplitQualifier Qualifier, const char *pBracks)
{
  disp_base_split_cb_data_t data;

  *pArgLen = strlen(pArg);

  if (!*pArgLen || (pArg[*pArgLen - 1] != pBracks[1]))
    return -1;

  data.data.callback_before = False;
  data.data.qualify_quote = QualifyQuote;
  data.nest = 0;
  data.split_pos = -1;
  data.last_nonspace_pos = -1;
  data.last_nonspace = ' ';
  data.qualifier = Qualifier;
  data.p_bracks = pBracks;

  as_iterate_str_quoted(pArg, disp_base_split_cb, &data.data);

  return data.split_pos;
}

/*****************************************************************************
 * Function:    CodeEquate
 * Purpose:     EQU for different segment
 * Result:      -
 *****************************************************************************/

void CodeEquate(as_addrspace_t DestSeg, LargeInt Min, LargeInt Max)
{
  Boolean OK;
  tSymbolFlags Flags;
  LargeInt Erg;

  if (ChkArgCnt(1, 1))
  {
    Erg = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);
    if (OK && !mFirstPassUnknown(Flags))
    {
      if (Min > Erg) WrError(ErrNum_UnderRange);
      else if (
#ifndef HAS64
        (!(Max & 0x80000000ul)) &&   /* cannot check >=2G range if LargeInt is 32 bits */
#endif
        (Erg > Max)) WrError(ErrNum_OverRange);
      else
      {
        TempResult t;

        PushLocHandle(-1);
        EnterIntSymbol(&LabPart, Erg, DestSeg, False);
        PopLocHandle();
        if (MakeUseList)
          if (AddChunk(SegChunks + DestSeg, Erg, 1, False)) WrError(ErrNum_Overlap);

        as_tempres_ini(&t);
        as_tempres_set_int(&t, Erg);
        SetListLineVal(&t);
        as_tempres_free(&t);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     QualifyQuote_SingleQuoteConstant(const char *pStart, const char *pQuotePos)
 * \brief  check whether ' in source is lead-in to character string or int constant
 * \param  pStart complete string
 * \param  pQuotePos single quote position
 * \return True if this is NO lead-in of int constant
 * ------------------------------------------------------------------------ */

Boolean QualifyQuote_SingleQuoteConstant(const char *pStart, const char *pQuotePos)
{
  const char *pRun;
  Boolean OK;
  int Base;

  /* previous character must be H X B O */

  if (pQuotePos == pStart)
    return True;
  switch (as_toupper(*(pQuotePos - 1)))
  {
    case 'B':
      Base = 2; break;
    case 'O':
      Base = 8; break;
    case 'X':
    case 'H':
      Base = 16; break;
    default:
      return True;
  }

  /* Scan for following valid (binary/octal/hex) character(s) */

  for (pRun = pQuotePos + 1; *pRun; pRun++)
  {
    switch (Base)
    {
      case 16: OK = as_isxdigit(*pRun); break;
      case 8: OK = as_isdigit(*pRun) && (*pRun < '8'); break;
      case 2: OK = as_isdigit(*pRun) && (*pRun < '2'); break;
      default: OK = False;
    }
    if (!OK)
      break;
  }

  /* none? -> bad */

  if (pRun <= pQuotePos + 1)
    return True;

  /* If we've hit another ' after them, it is the "harmless" x'...' form,
     and no special treatment is needed */

  if ('\'' == *pRun)
    return True;

  /* Other token or string continues -> cannot be such a constant, otherwise we
     have a match and the ' does NOT lead in a character string: */

  return as_isalnum(*pRun);
}

/*!------------------------------------------------------------------------
 * \fn     string_2_xasm_code(const struct as_nonz_dynstr *p_str, int bytes_per_dword, Boolean big_endian)
 * \brief  put characters from string into xx bit words of machine code - translation done outside!
 * \param  p_str source string
 * \param  bytes_per_dword # of characters in a word
 * \param  big_endian fill words starting with MSB?
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

#define declare_string_2_xasm_code(NAME, TYPE, VAR) \
int NAME(const struct as_nonz_dynstr *p_str, int bytes_per_dword, Boolean big_endian) \
{ \
  int byte_fill, ret; \
  const char *p_ch, *p_end; \
  TYPE character; \
 \
  for (byte_fill = 0, p_ch = p_str->p_str, p_end = p_ch + p_str->len; \
       p_ch < p_end; p_ch++) \
  { \
    if (!byte_fill) \
    { \
      ret = SetMaxCodeLen((CodeLen + 1) * sizeof(TYPE)); \
      if (ret) \
        return ret; \
      VAR[CodeLen++] = 0; \
    } \
    character = *p_ch & 0xff; \
    if (big_endian) \
      VAR[CodeLen - 1] = (VAR[CodeLen - 1] << 8) | character; \
    else \
      VAR[CodeLen - 1] |= character << (byte_fill * 8); \
    if (++byte_fill >= bytes_per_dword) \
      byte_fill = 0; \
  } \
  if (byte_fill && big_endian) \
    VAR[CodeLen - 1] <<= 8 * (bytes_per_dword - byte_fill); \
  return 0; \
}

declare_string_2_xasm_code(string_2_dasm_code, LongWord, DAsmCode)
declare_string_2_xasm_code(string_2_wasm_code, Word, WAsmCode)
