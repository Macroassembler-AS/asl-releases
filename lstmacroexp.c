/* lstmacroexp.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Functions & variables regarding macro expansion in listing                */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "strutil.h"
#include "strcomp.h"
#include "asmdef.h"
#include "asmpars.h"
#include "lstmacroexp.h"

#define MODIFIER_CLR 0x80
#define LstMacroExpName  "MACEXP"     /* expandierte Makros anzeigen */

static tLstMacroExp LstMacroExp;

/*!------------------------------------------------------------------------
 * \fn     SetLstMacroExp(tLstMacroExp NewLstMacroExp)
 * \brief  Set a new value for the 'effective' value of what is expanded in listing
 * \param  NewLstMacroExp new value to be set
 * ------------------------------------------------------------------------ */

void SetLstMacroExp(tLstMacroExp NewLstMacroExp)
{
  tStrComp TmpComp;
  String TmpCompStr;
  StrCompMkTemp(&TmpComp, TmpCompStr, sizeof(TmpCompStr));

  LstMacroExp = NewLstMacroExp;
  strmaxcpy(TmpCompStr, LstMacroExpName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, NewLstMacroExp, SegNone, True);
  if (LstMacroExp == eLstMacroExpAll)
    strcpy(ListLine, "ALL");
  else if (LstMacroExp == eLstMacroExpNone)
    strcpy(ListLine, "NONE");
  else
  {
    if (LstMacroExp & eLstMacroExpMacro)
    {
      strmaxcat(ListLine, *ListLine ? "+" : "=", STRINGSIZE);
      strmaxcat(ListLine, "MACRO", STRINGSIZE);
    }
    if (LstMacroExp & eLstMacroExpIf)
    {
      strmaxcat(ListLine, *ListLine ? "+" : "=", STRINGSIZE);
      strmaxcat(ListLine, "IF", STRINGSIZE);
    }
    if (LstMacroExp & eLstMacroExpRest)
    {
      strmaxcat(ListLine, *ListLine ? "+" : "=", STRINGSIZE);
      strmaxcat(ListLine, "REST", STRINGSIZE);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     GetLstMacroExp(void)
 * \brief  Retrieve value of what is currently 'effectively' expanded in listing
 * \return value
 * ------------------------------------------------------------------------ */

tLstMacroExp GetLstMacroExp(void)
{
  return LstMacroExp;
}

/*!------------------------------------------------------------------------
 * \fn     Inverse(Byte Mod1, Byte Mod2)
 * \brief  check whether one modifier is the exact inverse of another modifier
 * \param  Mod1, Mod2 modifiers to be analyzed
 * \return True of Mod1 is the exact inverse of Mod2
 * ------------------------------------------------------------------------ */

static Boolean Inverse(Byte Mod1, Byte Mod2)
{
  return (Mod1 ^ Mod2) == MODIFIER_CLR;
}

/*!------------------------------------------------------------------------
 * \fn     InitLstMacroExpMod(tLstMacroExpMod *pLstMacroExpMod)
 * \brief  initialize/clear list of macro expansion modifiers
 * \param  pLstMacroExpMod list to be initialized
 * ------------------------------------------------------------------------ */

void InitLstMacroExpMod(tLstMacroExpMod *pLstMacroExpMod)
{
  pLstMacroExpMod->Count = 0;
}

/*!------------------------------------------------------------------------
 * \fn     AddLstMacroExpMod(tLstMacroExpMod *pLstMacroExpMod, Boolean Set, tLstMacroExp Mod)
 * \brief  extend/modify a list of modifiers
 * \param  pLstMacroExpMod list to be updated
 * \param  Set is the modifier a set or clear modifier?
 * \param  Mod component to be set/cleared
 * \return True if modifier could be added, otherwise list is full (should not occur?)
 * ------------------------------------------------------------------------ */

Boolean AddLstMacroExpMod(tLstMacroExpMod *pLstMacroExpMod, Boolean Set, tLstMacroExp Mod)
{
  Byte NewModifier = Mod | (Set ? 0 : MODIFIER_CLR);
  unsigned z, dest;

  /* trim out any inverse modifier that is totally neutralized by the new one */

  for (z = dest = 0; z < pLstMacroExpMod->Count; z++)
    if (!Inverse(pLstMacroExpMod->Modifiers[z], NewModifier))
      pLstMacroExpMod->Modifiers[dest++] = pLstMacroExpMod->Modifiers[z];
  pLstMacroExpMod->Count = dest;

  /* add the new one */

  if (pLstMacroExpMod->Count >= LSTMACROEXPMOD_MAX)
    return False;
  else
  {
    pLstMacroExpMod->Modifiers[pLstMacroExpMod->Count++] = NewModifier;
    return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkLstMacroExpMod(const tLstMacroExpMod *pLstMacroExpMod)
 * \brief  check whether a modifier list is contradiction-free
 * \param  pLstMacroExpMod list to be checked
 * \return True if list contains no contradictions
 * ------------------------------------------------------------------------ */

Boolean ChkLstMacroExpMod(const tLstMacroExpMod *pLstMacroExpMod)
{
  unsigned z1, z2;

  for (z1 = 0; z1 < pLstMacroExpMod->Count; z1++)
    for (z2 = z1 + 1; z2 < pLstMacroExpMod->Count; z2++)
      if (Inverse(pLstMacroExpMod->Modifiers[z1], pLstMacroExpMod->Modifiers[z2]))
        return False;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DumpLstMacroExpMod(const tLstMacroExpMod *pLstMacroExpMod, char *pDest, int DestLen)
 * \brief  transform modifier list to human-readable form
 * \param  pLstMacroExpMod list to be transformed
 * \param  pDest where to write human-readable form
 * \param  DestLen size of dest buffer
 * ------------------------------------------------------------------------ */

void DumpLstMacroExpMod(const tLstMacroExpMod *pLstMacroExpMod, char *pDest, int DestLen)
{
  unsigned z;

  for (z = 0; z < pLstMacroExpMod->Count; z++)
  {
    if (z)
      strmaxcat(pDest, ",", DestLen);
    strmaxcat(pDest, (pLstMacroExpMod->Modifiers[z] & MODIFIER_CLR) ? "NOEXP" : "EXP", DestLen);
    switch (pLstMacroExpMod->Modifiers[z] & ~MODIFIER_CLR)
    {
      case eLstMacroExpRest:
        strmaxcat(pDest, "REST", DestLen); break;
      case eLstMacroExpIf:
        strmaxcat(pDest, "IF", DestLen); break;
      case eLstMacroExpMacro:
        strmaxcat(pDest, "MACRO", DestLen); break;
      case eLstMacroExpAll:
        strmaxcat(pDest, "AND", DestLen); break;
      default:
        strmaxcat(pDest, "?", DestLen);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     ApplyLstMacroExpMod(tLstMacroExp Src, const tLstMacroExpMod *pLstMacroExpMod)
 * \brief  apply a list of modifiers to a listing macro expansion bit field
 * \param  Src original value of bit field
 * \param  pLstMacroExpMod list of modifiers
 * \return resulting new mask
 * ------------------------------------------------------------------------ */

tLstMacroExp ApplyLstMacroExpMod(tLstMacroExp Src, const tLstMacroExpMod *pLstMacroExpMod)
{
  unsigned z;

  for (z = 0; z < pLstMacroExpMod->Count; z++)
  {
    if (pLstMacroExpMod->Modifiers[z] & MODIFIER_CLR)
      Src &= ~((tLstMacroExp)pLstMacroExpMod->Modifiers[z] & eLstMacroExpAll);
    else
      Src |= (tLstMacroExp)pLstMacroExpMod->Modifiers[z];
  }
  return Src;
}
