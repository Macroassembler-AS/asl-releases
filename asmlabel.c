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
  const char *p_op_part = OpPart.str.p_str;

  if (!*LabPart.str.p_str)
    return False;
  if (IsDef())
    return False;

  if (!HasAttrs && (*p_op_part == '.'))
    p_op_part++;

  switch (*p_op_part)
  {
    case '=':
      return !Memo("=");
    case ':':
      return !Memo(":=");
    case 'M':
      return !!strcmp(p_op_part, "MACRO");
    case 'F':
      return !!strcmp(p_op_part, "FUNCTION");
    case 'L':
      return !!strcmp(p_op_part, "LABEL");
    case 'S':
      return !(!strcmp(p_op_part, "SET") && is_set_pseudo())
          && strcmp(p_op_part, "STRUCT")
          && strcmp(p_op_part, "STRUC");
    case 'E':
      return strcmp(p_op_part, "EQU")
          && strcmp(p_op_part, "ENDSTRUCT")
          && strcmp(p_op_part, "ENDS")
          && strcmp(p_op_part, "ENDSTRUC")
          && strcmp(p_op_part, "ENDUNION")
          && strcmp(p_op_part, "EVAL");
    case 'U':
      return !!strcmp(p_op_part, "UNION");
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
 * \fn     initpass_asmlabel(void)
 * \brief  Initializations before passing through sources
 * ------------------------------------------------------------------------ */

static void initpass_asmlabel(void)
{
  LabelReset();
}

/*!------------------------------------------------------------------------
 * \fn     asmlabel_init(void)
 * \brief  Initializations once at startup
 * ------------------------------------------------------------------------ */

void asmlabel_init(void)
{
  initpass_asmlabel();
  AddInitPassProc(initpass_asmlabel);
}
