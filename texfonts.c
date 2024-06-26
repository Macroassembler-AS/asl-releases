/* texfonts.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX-->ASCII/HTML Converter: Font Stuff                                    */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>

#include "datatypes.h"
#include "texfonts.h"
#include "texutil.h"

/*--------------------------------------------------------------------------*/

typedef struct sFontSave
{
  struct sFontSave *pNext;
  int FontFlags;
  tFontSize FontSize;
} tFontSave, *tpFontSave;

/*--------------------------------------------------------------------------*/

int CurrFontFlags;
tFontSize CurrFontSize;
tFontType CurrFontType;

static tpFontSave pFontStack;

/*!------------------------------------------------------------------------
 * \fn     InitFont(void)
 * \brief  initialize font state
 * ------------------------------------------------------------------------ */

void InitFont(void)
{
  pFontStack = NULL;
  curr_tex_env_data.FontNest = 0;
  CurrFontSize = FontNormalSize;
  CurrFontType = FontStandard;
  CurrFontFlags = 0;
}

/*!------------------------------------------------------------------------
 * \fn     SaveFont(void)
 * \brief  push font size & flags to stack
 * ------------------------------------------------------------------------ */

void SaveFont(void)
{
  tpFontSave pNewSave;

  pNewSave = (tpFontSave) malloc(sizeof(*pNewSave));
  pNewSave->pNext = pFontStack;
  pNewSave->FontSize = CurrFontSize;
  pNewSave->FontFlags = CurrFontFlags;
  pFontStack = pNewSave;
  curr_tex_env_data.FontNest++;
}

/*!------------------------------------------------------------------------
 * \fn     RestoreFont(void)
 * \brief  push font size & flags to stack
 * ------------------------------------------------------------------------ */

extern void PrFontDiff(int OldFlags, int NewFlags);
extern void PrFontSize(tFontSize Type, Boolean On);

void RestoreFont(void)
{
  tpFontSave pOldSave;

  if (!pFontStack)
    return;

  PrFontDiff(CurrFontFlags, pFontStack->FontFlags);
  PrFontSize(CurrFontSize, False);

  pOldSave = pFontStack;
  pFontStack = pFontStack->pNext;
  CurrFontSize = pOldSave->FontSize;
  CurrFontFlags = pOldSave->FontFlags;
  free(pOldSave);
  curr_tex_env_data.FontNest--;
}

/*!------------------------------------------------------------------------
 * \fn     FreeFontStack(void)
 * \brief  dispose pushed font settings
 * ------------------------------------------------------------------------ */

void FreeFontStack(void)
{
  while (pFontStack)
  {
    tpFontSave pOld = pFontStack;
    pFontStack = pOld->pNext;
    free(pOld);
  }
}
