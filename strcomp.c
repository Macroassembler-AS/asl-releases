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

void StrCompAlloc(tStrComp *pComp)
{
  pComp->Str = (char*)malloc(STRINGSIZE);
  StrCompReset(pComp);
}

void StrCompReset(tStrComp *pComp)
{
  LineCompReset(&pComp->Pos);
  *pComp->Str = '\0';
}

void LineCompReset(tLineComp *pComp)
{
  pComp->StartCol = -1;
  pComp->Len = 0;
}

void StrCompFree(tStrComp *pComp)
{
  LineCompReset(&pComp->Pos);
  if (pComp->Str)
    free(pComp->Str);
  pComp->Str = NULL;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompMkTemp(tStrComp *pComp, char *pStr)
 * \brief  create a dummy StrComp from plain string
 * \param  pComp comp to create
 * \param  pStr string to use
 * ------------------------------------------------------------------------ */

void StrCompMkTemp(tStrComp *pComp, char *pStr)
{
  LineCompReset(&pComp->Pos);
  pComp->Str = pStr;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompRefRight(tStrComp *pDest, const tStrComp *pSrc, int StartOffs)
 * \brief  create a right-aligned substring component of string component (no copy)
 * \param  pDest destination component
 * \param  pSrc source component
 * \param  StartOffs how many characters to omit at the left
 * ------------------------------------------------------------------------ */

void StrCompRefRight(tStrComp *pDest, const tStrComp *pSrc, int StartOffs)
{
  pDest->Str = pSrc->Str + StartOffs;
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
  strcpy(pDest->Str, pSrc->Str);
}

/*!------------------------------------------------------------------------
 * \fn     StrCompCopySub(tStrComp *pDest, const tStrComp *pSrc, unsigned Start, unsigned Count)
 * \brief  copy substring
 * \param  pDest destination
 * \param  pSrc source
 * \param  Start start index to copy from
 * \param  Count # of characters to copy
 * ------------------------------------------------------------------------ */

void StrCompCopySub(tStrComp *pDest, const tStrComp *pSrc, unsigned Start, unsigned Count)
{
  unsigned l = strlen(pSrc->Str);

  if (Start >= l)
    Count = 0;
  else if (Start + Count > l)
    Count = l - Start;
  memcpy(pDest->Str, pSrc->Str + Start, Count);
  pDest->Str[Count] = '\0';
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
  strcpy(pDest->Str, pSrcSplitPos + 1);
  pDest->Pos.StartCol = pSrc->Pos.StartCol + pSrcSplitPos + 1 - pSrc->Str;
  pDest->Pos.Len = strlen(pDest->Str);
  *pSrcSplitPos = '\0';
  pSrc->Pos.Len = pSrcSplitPos - pSrc->Str;
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
  *pSrcSplitPos = '\0';
  strcpy(pDest->Str, pSrc->Str);
  pDest->Pos.StartCol = pSrc->Pos.StartCol;
  pDest->Pos.Len = pSrcSplitPos - pSrc->Str;
  strmov(pSrc->Str, pSrcSplitPos + 1);
  pSrc->Pos.StartCol += pSrcSplitPos - pSrc->Str + 1;
  pSrc->Pos.Len = strlen(pSrc->Str);
}

void StrCompSplitCopy(tStrComp *pLeft, tStrComp *pRight, const tStrComp *pSrc, char *pSplitPos)
{
  pLeft->Pos.StartCol = pSrc->Pos.StartCol;
  pLeft->Pos.Len = pSplitPos - pSrc->Str;
  memmove(pLeft->Str, pSrc->Str, pLeft->Pos.Len);
  pLeft->Str[pLeft->Pos.Len] = '\0';

  pRight->Pos.StartCol = pSrc->Pos.StartCol + (pLeft->Pos.Len + 1);
  pRight->Pos.Len = pSrc->Pos.Len - (pLeft->Pos.Len + 1);
  strcpy(pRight->Str, pSplitPos + 1);
}

char StrCompSplitRef(tStrComp *pLeft, tStrComp *pRight, const tStrComp *pSrc, char *pSplitPos)
{
  char Old = *pSplitPos;
  /* save because pLeft and pSrc might be equal */
  tLineComp SrcPos = pSrc->Pos;

  *pSplitPos = '\0';
  pLeft->Str = pSrc->Str;
  pLeft->Pos.StartCol = SrcPos.StartCol;
  pLeft->Pos.Len = pSplitPos - pLeft->Str;
  pRight->Str = pSrc->Str + (pLeft->Pos.Len + 1);
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
  int Delta = KillPrefBlanks(pComp->Str);
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
  while (isspace(*pComp->Str))
  {
    pComp->Str++;
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
  pComp->Pos.Len -= KillPostBlanks(pComp->Str);
}

/*!------------------------------------------------------------------------
 * \fn     StrCompShorten(struct sStrComp *pComp, int Delta)
 * \brief  shorten string component by n characters
 * \param  pComp component to shorten
 * \param  Delta # of characters to chop off (no checks!)
 * ------------------------------------------------------------------------ */

void StrCompShorten(struct sStrComp *pComp, int Delta)
{
  pComp->Str[strlen(pComp->Str) - Delta] = '\0';
  pComp->Pos.Len -= Delta;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompCutLeft(struct sStrComp *pComp, int Delta)
 * \brief  remove n characters at start of component
 * \param  pComp component to shorten
 * \param  Delta # of characters to cut off
 * \return actual # of characters cut off
 * ------------------------------------------------------------------------ */

int StrCompCutLeft(struct sStrComp *pComp, int Delta)
{
  int len = strlen(pComp->Str);

  if (Delta > len)
    Delta = len;
  if (Delta > (int)pComp->Pos.Len)
    Delta = pComp->Pos.Len;
  strmov(pComp->Str, pComp->Str + Delta);
  pComp->Pos.StartCol += Delta;
  pComp->Pos.Len -= Delta;
  return Delta;
}

/*!------------------------------------------------------------------------
 * \fn     StrCompIncRefLeft(struct sStrComp *pComp, int Delta)
 * \brief  move start of component by n characters
 * \param  pComp component to shorten
 * \param  Delta # of characters to move by (no checks!)
 * ------------------------------------------------------------------------ */

void StrCompIncRefLeft(struct sStrComp *pComp, int Delta)
{
  pComp->Str += Delta;
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
  fprintf(stderr, "%s: @ col %u len %u '%s'\n", pTitle, pComp->Pos.StartCol, pComp->Pos.Len, pComp->Str);
}
