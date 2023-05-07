/* asmlabel.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS Port                                                                   */
/*                                                                           */
/* Handle Label on Source Line                                               */
/*                                                                           */
/*****************************************************************************/

/* ------------------------------------------------------------------------
 * Includes
 * ------------------------------------------------------------------------ */

#include "stdinc.h"
#include <string.h>

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmstructs.h"
#include "strcomp.h"

#include "asmlabel.h"

/* ------------------------------------------------------------------------
 * Local Variables
 * ------------------------------------------------------------------------ */

static PStructElem pLabelElement;
static struct sSymbolEntry *pLabelEntry;
static LargeWord LabelValue;

/* ------------------------------------------------------------------------
 * Global Functions
 * ------------------------------------------------------------------------ */

/*!------------------------------------------------------------------------
 * \fn     LabelReset(void)
 * \brief  reset state about most recent label
 * ------------------------------------------------------------------------ */

void LabelReset(void)
{
  pLabelElement = NULL;
  pLabelEntry = NULL;
  LabelValue = (LargeWord)-1;
}

/*!------------------------------------------------------------------------
 * \fn     LabelPresent(void)
 * \brief  check whether line contains a label
 * \return True if label is present
 * ------------------------------------------------------------------------ */

Boolean LabelPresent(void)
{
  if (!*LabPart.str.p_str)
    return False;
  if (IsDef())
    return False;

  switch (*OpPart.str.p_str)
  {
    case '=':
      return !Memo("=");
    case ':':
      return !Memo(":=");
    case '.':
      return !Memo(".SET") && !Memo(".EQU");
    case 'M':
      return !Memo("MACRO");
    case 'F':
      return !Memo("FUNCTION");
    case 'L':
      return !Memo("LABEL");
    case 'S':
      return !memo_set_pseudo() && (!(Memo("STRUCT") || Memo("STRUC")));
    case 'E':
      if (Memo("EQU") || Memo("ENDSTRUCT") || Memo("ENDS") || Memo("ENDSTRUC") || Memo("ENDUNION"))
        return False;
      return !Memo("EVAL");
    case 'U':
      return !Memo("UNION");
    default:
      return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     LabelHandle(const tStrComp *pName, LargeWord Value, Boolean ForceGlobal)
 * \brief  process new label
 * \param  pName label's name
 * \param  Value label's value
 * \param  ForceGlobal force visibility outside macro body
 * ------------------------------------------------------------------------ */

void LabelHandle(const tStrComp *pName, LargeWord Value, Boolean ForceGlobal)
{
  pLabelElement = NULL;
  pLabelEntry = NULL;

  /* structure element ? */

  if (pInnermostNamedStruct)
  {
    pLabelElement = CreateStructElem(pName);
    if (!pLabelElement)
      return;

    pLabelElement->Offset = Value;
    if (AddStructElem(pInnermostNamedStruct->StructRec, pLabelElement))
      AddStructSymbol(pLabelElement->pElemName, Value);
  }

  /* normal label */

  else
  {
    if (ForceGlobal)
      PushLocHandle(-1);
    if (RelSegs)
      pLabelEntry = EnterRelSymbol(pName, Value, (as_addrspace_t)ActPC, False);
    else
    {
      pLabelEntry = EnterIntSymbolWithFlags(pName, Value, (as_addrspace_t)ActPC, False,
                              eSymbolFlag_Label |
                              (Value == AfterBSRAddr ? eSymbolFlag_NextLabelAfterBSR : eSymbolFlag_None));
    }
    if (ForceGlobal)
      PopLocHandle();
  }
  LabelValue = Value;
}

/*!------------------------------------------------------------------------
 * \fn     LabelModify(LargeWord OldValue, LargeWord NewValue)
 * \brief  change value of current label
 * \param  OldValue current value to check consistency
 * \param  NewValue new value to assign
 * ------------------------------------------------------------------------ */

void LabelModify(LargeWord OldValue, LargeWord NewValue)
{
  if (OldValue == LabelValue)
  {
    if (pLabelElement)
      pLabelElement->Offset = NewValue;
    if (pLabelEntry)
      ChangeSymbol(pLabelEntry, NewValue);
    LabelValue = NewValue;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AsmLabelPassInit(void)
 * \brief  Initializations before passing through sources
 * ------------------------------------------------------------------------ */

void AsmLabelPassInit(void)
{
  LabelReset();
}

/*!------------------------------------------------------------------------
 * \fn     asmlabel_init(void)
 * \brief  Initializations once at startup
 * ------------------------------------------------------------------------ */

void asmlabel_init(void)
{
  LabelReset();
}
