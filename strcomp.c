/* strcomp.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Macro Assembler AS                                                        */
/*                                                                           */
/* Definition of a source line's component present after parsing             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include "datatypes.h"
#include "strutil.h"
#include "strcomp.h"

static void check_capacity(const tStrComp *pComp)
{
  if (!pComp->str.capacity)
  {
    fprintf(stderr, "copy to zero-capacity StrComp\n");
    abort();
  }
}

static void check_no_capacity(const tStrComp *pComp)
{
  if (pComp->str.capacity)
  {
    fprintf(stderr, "ptr move on non-zero-capacity StrComp\n");
    abort();
  }
}

static size_t check_realloc(as_dynstr_t *p_str, size_t req_count)
{
  if ((req_count >= p_str->capacity) && p_str->dynamic)
    as_dynstr_realloc(p_str, as_dynstr_roundup_len(req_count));
  if (req_count >= p_str->capacity)
    req_count = p_str->capacity - 1;
  return req_count;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompAlloc(tStrComp *pComp, size_t capacity)
 * \brief  initialize string component with dynamic buffer
 * \param  pComp component to fill
 * \param  capacity requested capacity of buffer
 * ------------------------------------------------------------------------ */

void StrCompAlloc(tStrComp *pComp, size_t capacity)
{
  as_dynstr_ini(&pComp->str, capacity);
  StrCompReset(pComp);
}

/*!------------------------------------------------------------------------
 * \fn     StrCompReset(tStrComp *pComp)
 * \brief  reset string component to "empty" state
 * \param  pComp component to set
 * ------------------------------------------------------------------------ */

void StrCompReset(tStrComp *pComp)
{
  LineCompReset(&pComp->Pos);
  *pComp->str.p_str = '\0';
}

/*!------------------------------------------------------------------------
 * \fn     LineCompReset(tLineComp *pComp)
 * \brief  reset line position to "empty" state
 * \param  pComp component to set
 * ------------------------------------------------------------------------ */

void LineCompReset(tLineComp *pComp)
{
  pComp->StartCol = -1;
  pComp->Len = 0;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompFree(tStrComp *pComp)
 * \brief  free/clean string component
 * \param  pComp string component to be cleared
 * ------------------------------------------------------------------------ */

void StrCompFree(tStrComp *pComp)
{
  LineCompReset(&pComp->Pos);
  as_dynstr_free(&pComp->str);
}

/*!------------------------------------------------------------------------
 * \fn     StrCompMkTemp(tStrComp *pComp, char *pStr, size_t capacity)
 * \brief  create a dummy StrComp from plain string
 * \param  pComp comp to create
 * \param  pStr string to use
 * \param  capacity string's capacity
 * ------------------------------------------------------------------------ */

void StrCompMkTemp(tStrComp *pComp, char *pStr, size_t capacity)
{
  LineCompReset(&pComp->Pos);
  pComp->str.p_str = pStr;
  pComp->str.capacity = capacity;
  pComp->str.dynamic = 0;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompRefRight(tStrComp *pDest, const tStrComp *pSrc, size_t StartOffs)
 * \brief  create a right-aligned substring component of string component (no copy)
 * \param  pDest destination component
 * \param  pSrc source component
 * \param  StartOffs how many characters to omit at the left
 * ------------------------------------------------------------------------ */

void StrCompRefRight(tStrComp *pDest, const tStrComp *pSrc, size_t StartOffs)
{
  pDest->str.p_str = pSrc->str.p_str + StartOffs;
  pDest->str.capacity = 0;
  pDest->str.dynamic = 0;
  pDest->Pos.StartCol = pSrc->Pos.StartCol + StartOffs;
  pDest->Pos.Len = pSrc->Pos.Len - StartOffs;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompCopy(tStrComp *pDest, tStrComp *pSrc)
 * \brief  copy string component
 * \param  pDest destination component
 * \param  pSrc source component
 * ------------------------------------------------------------------------ */

void StrCompCopy(tStrComp *pDest, const tStrComp *pSrc)
{
  pDest->Pos = pSrc->Pos;
  pDest->Pos.Len = as_dynstr_copy(&pDest->str, &pSrc->str);
}

/*!------------------------------------------------------------------------
 * \fn     StrCompCopySub(tStrComp *pDest, const tStrComp *pSrc, size_t Start, size_t Count)
 * \brief  copy substring
 * \param  pDest destination
 * \param  pSrc source
 * \param  Start start index to copy from
 * \param  Count # of characters to copy
 * ------------------------------------------------------------------------ */

void StrCompCopySub(tStrComp *pDest, const tStrComp *pSrc, size_t Start, size_t Count)
{
  unsigned l = strlen(pSrc->str.p_str);

  if (Start >= l)
    Count = 0;
  else if (Start + Count > l)
    Count = l - Start;
  check_capacity(pDest);
  Count = check_realloc(&pDest->str, Count);
  memcpy(pDest->str.p_str, pSrc->str.p_str + Start, Count);
  pDest->str.p_str[Count] = '\0';
  pDest->Pos.StartCol = pSrc->Pos.StartCol + Start;
  pDest->Pos.Len = Count;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompSplitRight(tStrComp *pSrc, tStrComp *pDest, char *pSrcSplitPos)
 * \brief  split off another component at the right of the source
 * \param  pSrc source to split off
 * \param  pDest where to put part splitted off
 * \param  pSrcSplitPos split position (not included in source or dest any more)
 * ------------------------------------------------------------------------ */

void StrCompSplitRight(tStrComp *pSrc, tStrComp *pDest, char *pSrcSplitPos)
{
  size_t SrcLen = strlen(pSrcSplitPos + 1);

  check_capacity(pDest);
  SrcLen = check_realloc(&pDest->str, SrcLen);
  memcpy(pDest->str.p_str, pSrcSplitPos + 1, SrcLen);
  pDest->str.p_str[SrcLen] = '\0';
  pDest->Pos.StartCol = pSrc->Pos.StartCol + pSrcSplitPos + 1 - pSrc->str.p_str;
  pDest->Pos.Len = SrcLen;
  *pSrcSplitPos = '\0';
  pSrc->Pos.Len = pSrcSplitPos - pSrc->str.p_str;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompSplitLeft(tStrComp *pSrc, tStrComp *pDest, char *pSrcSplitPos)
 * \brief  split off another component at the left of the source
 * \param  pSrc source to split off
 * \param  pDest where to put part splitted off
 * \param  pSrcSplitPos split position (not included in source or dest any more)
 * ------------------------------------------------------------------------ */

void StrCompSplitLeft(tStrComp *pSrc, tStrComp *pDest, char *pSrcSplitPos)
{
  size_t SrcLen;

  *pSrcSplitPos = '\0';
  SrcLen = strlen(pSrc->str.p_str);
  check_capacity(pDest);
  SrcLen = check_realloc(&pDest->str, SrcLen);

  memcpy(pDest->str.p_str, pSrc->str.p_str, SrcLen);
  pDest->str.p_str[SrcLen] = '\0';
  pDest->Pos.StartCol = pSrc->Pos.StartCol;
  pDest->Pos.Len = pSrcSplitPos - pSrc->str.p_str;

  strmov(pSrc->str.p_str, pSrcSplitPos + 1);
  pSrc->Pos.StartCol += pSrcSplitPos - pSrc->str.p_str + 1;
  pSrc->Pos.Len -= pSrcSplitPos - pSrc->str.p_str + 1;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompSplitCopy(tStrComp *pLeft, tStrComp *pRight, const tStrComp *pSrc, char *pSplitPos)
 * \brief  copy left & right part of string to new components
 * \param  pLeft dest for left part
 * \param  pRight dest for right part
 * \param  pSrc character source
 * \param  pSplitPos split position in source
 * ------------------------------------------------------------------------ */

void StrCompSplitCopy(tStrComp *pLeft, tStrComp *pRight, const tStrComp *pSrc, char *pSplitPos)
{
  /* pLeft may be equal to pSrc, use memmove() and save SrcLen */

  size_t SrcLen = pSrc->Pos.Len;

  check_capacity(pLeft);
  pLeft->Pos.StartCol = pSrc->Pos.StartCol;
  pLeft->Pos.Len = check_realloc(&pLeft->str, pSplitPos - pSrc->str.p_str);
  memmove(pLeft->str.p_str, pSrc->str.p_str, pLeft->Pos.Len);
  pLeft->str.p_str[pLeft->Pos.Len] = '\0';

  check_capacity(pRight);
  pRight->Pos.StartCol = pSrc->Pos.StartCol + (pLeft->Pos.Len + 1);
  pRight->Pos.Len = check_realloc(&pRight->str, SrcLen - (pLeft->Pos.Len + 1));
  memcpy(pRight->str.p_str, pSplitPos + 1, pRight->Pos.Len);
  pRight->str.p_str[pRight->Pos.Len] = '\0';
}

/*!------------------------------------------------------------------------
 * \fn     StrCompSplitRef(tStrComp *pLeft, tStrComp *pRight, const tStrComp *pSrc, char *pSplitPos)
 * \brief  split string into left & right and form references
 * \param  pLeft dest for left part
 * \param  pRight dest for right part
 * \param  pSrc character source
 * \param  pSplitPos split position in source
 * ------------------------------------------------------------------------ */

char StrCompSplitRef(tStrComp *pLeft, tStrComp *pRight, const tStrComp *pSrc, char *pSplitPos)
{
  char Old = *pSplitPos;
  /* save because pLeft and pSrc might be equal */
  tLineComp SrcPos = pSrc->Pos;

  *pSplitPos = '\0';
  pLeft->str.p_str = pSrc->str.p_str;
  if (pLeft != pSrc)
  {
    pLeft->str.capacity = 0;
    pLeft->str.dynamic = 0;
  }
  pLeft->Pos.StartCol = SrcPos.StartCol;
  pLeft->Pos.Len = pSplitPos - pLeft->str.p_str;
  pRight->str.p_str = pSrc->str.p_str + (pLeft->Pos.Len + 1);
  pRight->str.capacity = 0;
  pRight->str.dynamic = 0;
  pRight->Pos.StartCol = SrcPos.StartCol + (pLeft->Pos.Len + 1);
  pRight->Pos.Len = SrcPos.Len - (pLeft->Pos.Len + 1);

  return Old;
}

/*!------------------------------------------------------------------------
 * \fn     KillPrefBlanksStrComp(struct sStrComp *pComp)
 * \brief  remove leading spaces on string component
 * \param  pComp component to handle
 * ------------------------------------------------------------------------ */

void KillPrefBlanksStrComp(struct sStrComp *pComp)
{
  int Delta = KillPrefBlanks(pComp->str.p_str);
  pComp->Pos.StartCol += Delta;
  pComp->Pos.Len -= Delta;
}

/*!------------------------------------------------------------------------
 * \fn     KillPrefBlanksStrCompRef(struct sStrComp *pComp)
 * \brief  remove leading spaces on string component by inc'ing pointer
 * \param  pComp component to handle
 * ------------------------------------------------------------------------ */

void KillPrefBlanksStrCompRef(struct sStrComp *pComp)
{
  check_no_capacity(pComp);
  while (isspace(*pComp->str.p_str))
  {
    pComp->str.p_str++;
    if (pComp->str.capacity > 0)
      pComp->str.capacity--;
    pComp->Pos.StartCol++;
    pComp->Pos.Len--;
  }
}

/*!------------------------------------------------------------------------
 * \fn     KillPostBlanksStrComp(struct sStrComp *pComp)
 * \brief  remove trailing spaces on string component
 * \param  pComp component to handle
 * ------------------------------------------------------------------------ */

void KillPostBlanksStrComp(struct sStrComp *pComp)
{
  pComp->Pos.Len -= KillPostBlanks(pComp->str.p_str);
}

/*!------------------------------------------------------------------------
 * \fn     StrCompShorten(struct sStrComp *pComp, size_t Delta)
 * \brief  shorten string component by n characters
 * \param  pComp component to shorten
 * \param  Delta # of characters to chop off (no checks!)
 * ------------------------------------------------------------------------ */

void StrCompShorten(struct sStrComp *pComp, size_t Delta)
{
  pComp->str.p_str[strlen(pComp->str.p_str) - Delta] = '\0';
  pComp->Pos.Len -= Delta;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompCutLeft(struct sStrComp *pComp, size_t Delta)
 * \brief  remove n characters at start of component
 * \param  pComp component to shorten
 * \param  Delta # of characters to cut off
 * \return actual # of characters cut off
 * ------------------------------------------------------------------------ */

size_t StrCompCutLeft(struct sStrComp *pComp, size_t Delta)
{
  size_t len = strlen(pComp->str.p_str);

  if (Delta > len)
    Delta = len;
  if (Delta > pComp->Pos.Len)
    Delta = pComp->Pos.Len;
  strmov(pComp->str.p_str, pComp->str.p_str + Delta);
  pComp->Pos.StartCol += Delta;
  pComp->Pos.Len -= Delta;
  return Delta;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompIncRefLeft(struct sStrComp *pComp, size_t Delta)
 * \brief  move start of component by n characters
 * \param  pComp component to shorten
 * \param  Delta # of characters to move by (no checks!)
 * ------------------------------------------------------------------------ */

void StrCompIncRefLeft(struct sStrComp *pComp, size_t Delta)
{
  check_no_capacity(pComp);
  pComp->str.p_str += Delta;
  pComp->str.capacity = (pComp->str.capacity > Delta) ? pComp->str.capacity - Delta : 0;
  pComp->Pos.StartCol += Delta;
  pComp->Pos.Len -= Delta;
}

/*!------------------------------------------------------------------------
 * \fn     DumpStrComp(const char *pTitle, const struct sStrComp *pComp)
 * \brief  debug dump of component
 * \param  pTitle description of component
 * \param  pComp component to dump
 * ------------------------------------------------------------------------ */

void DumpStrComp(const char *pTitle, const struct sStrComp *pComp)
{
  fprintf(stderr, "%s: @ col %u len %u '%s'\n", pTitle, pComp->Pos.StartCol, pComp->Pos.Len, pComp->str.p_str);
}
