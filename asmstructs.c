/* asmstructs.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* structure handling                                                        */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "nls.h"
#include "nlmessages.h"
#include "strutil.h"

#include "trees.h"
#include "errmsg.h"
#include "symbolsize.h"

#include "as.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmlabel.h"
#include "asmstructs.h"
#include "errmsg.h"
#include "codepseudo.h"

#include "as.rsc"

/*****************************************************************************/

typedef struct sStructNode
{
  TTree Tree;
  Boolean Defined;
  PStructRec StructRec;
} TStructNode, *PStructNode;

/*****************************************************************************/

PStructStack StructStack,        /* momentan offene Strukturen */
             pInnermostNamedStruct;
as_addrspace_t StructSaveSeg;    /* gesichertes Segment waehrend Strukturdef.*/
PStructNode StructRoot = NULL;

/*****************************************************************************/

PStructRec CreateStructRec(void)
{
  PStructRec Neu;

  Neu = (PStructRec) malloc(sizeof(TStructRec));
  if (Neu)
  {
    Neu->TotLen = 0;
    Neu->Elems = NULL;
    Neu->ExtChar = '\0';
    Neu->DoExt = Neu->IsUnion = False;
  }
  return Neu;
}

/*!------------------------------------------------------------------------
 * \fn     DestroyStructElem(PStructElem pStructElem)
 * \brief  destroy/free struct element
 * \param  pStructElem element to destroy
 * ------------------------------------------------------------------------ */

void DestroyStructElem(PStructElem pStructElem)
{
  if (pStructElem->pElemName) free(pStructElem->pElemName);
  if (pStructElem->pRefElemName) free(pStructElem->pRefElemName);
  free(pStructElem);
}

void DestroyStructRec(PStructRec StructRec)
{
  PStructElem Old;

  while (StructRec->Elems)
  {
    Old = StructRec->Elems;
    StructRec->Elems = Old->Next;
    DestroyStructElem(Old);
  }
  free(StructRec);
}

/*!------------------------------------------------------------------------
 * \fn     CreateStructElem(const char *pElemName)
 * \brief  create a new entry for a structure definition
 * \param  pElemName name of the structure element
 * \return * to element or NULL if allocation failed
 * ------------------------------------------------------------------------ */

void ExpandStructStd(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
{
  LabelHandle(pVarName, Base + pStructElem->Offset, True);
  if (pStructElem->OpSize != eSymbolSizeUnknown)
  {
    String ext_name_buf;
    tStrComp ext_name;
    const tStrComp *p_ext_name;

    StrCompMkTemp(&ext_name, ext_name_buf, sizeof(ext_name_buf));
    p_ext_name = ExpandStrSymbol(&ext_name, pVarName, !CaseSensitive);
    if (p_ext_name)
      SetSymbolOrStructElemSize(p_ext_name, pStructElem->OpSize);
  }
}

PStructElem CreateStructElem(const tStrComp *pElemName)
{
  String ext_name_buf;
  tStrComp ext_name;
  const tStrComp *p_ext_name;
  PStructElem pNeu;

  StrCompMkTemp(&ext_name, ext_name_buf, sizeof(ext_name_buf));
  p_ext_name = ExpandStrSymbol(&ext_name, pElemName, !CaseSensitive);
  if (!p_ext_name)
    return NULL;

  pNeu = (PStructElem) calloc(1, sizeof(TStructElem));
  if (pNeu)
  {
    pNeu->pElemName = as_strdup(p_ext_name->str.p_str);
    pNeu->pRefElemName = NULL;
    pNeu->ExpandFnc = ExpandStructStd;
    pNeu->Offset = 0;
    pNeu->BitPos = pNeu->BitWidthM1 = -1;
    pNeu->OpSize = eSymbolSizeUnknown;
    pNeu->Next = NULL;
  }
  return pNeu;
}

/*!------------------------------------------------------------------------
 * \fn     CloneStructElem(const struct sStrComp *pCloneElemName, const struct sStructElem *pSrc)
 * \brief  generate copy of struct element, with different name
 * \param  pCloneElemName new element's name
 * \param  pSrc source to clone from
 * \return * to new element or NULL
 * ------------------------------------------------------------------------ */

PStructElem CloneStructElem(const struct sStrComp *pCloneElemName, const struct sStructElem *pSrc)
{
  PStructElem pResult = CreateStructElem(pCloneElemName);
  if (!pResult)
    return pResult;

  pResult->Offset = pSrc->Offset;
  pResult->OpSize = pSrc->OpSize;
  pResult->BitPos = pSrc->BitPos;
  pResult->ExpandFnc = pSrc->ExpandFnc;
  pResult->pRefElemName = as_strdup(pSrc->pRefElemName);
  return pResult;
}

/*!------------------------------------------------------------------------
 * \fn     AddStructElem(PStructRec pStructRec, PStructElem pElement)
 * \brief  add a new element to a structure definition
 * \param  pStructRec structure to extend
 * \param  pElement new element
 * \return True if element was added
 * ------------------------------------------------------------------------ */

Boolean AddStructElem(PStructRec pStructRec, PStructElem pElement)
{
  PStructElem pRun, pPrev;
  Boolean Duplicate = False;

  if (!CaseSensitive && pElement->pRefElemName)
    NLS_UpString(pElement->pRefElemName);

  for (pPrev = NULL, pRun = pStructRec->Elems; pRun; pPrev = pRun, pRun = pRun->Next)
  {
    Duplicate = CaseSensitive ? !strcmp(pElement->pElemName, pRun->pElemName) : !as_strcasecmp(pElement->pElemName, pRun->pElemName);
    if (Duplicate)
    {
      WrXError(ErrNum_DuplicateStructElem, pElement->pElemName);
      break;
    }
  }

  if (!Duplicate)
  {
    if (pPrev)
      pPrev->Next = pElement;
    else
      pStructRec->Elems = pElement;
  }
  BumpStructLength(pStructRec, pElement->Offset);
  if (Duplicate)
    DestroyStructElem(pElement);
  return !Duplicate;
}

/*!------------------------------------------------------------------------
 * \fn     SetStructElemSize(PStructRec pStructRec, const tStrComp *pElemName, tSymbolSize Size)
 * \brief  set the operand size of a structure's element
 * \param  pStructRec structure the element is contained in
 * \param  pElemName element's name
 * \param  Size operand size to set
 * ------------------------------------------------------------------------ */

void SetStructElemSize(PStructRec pStructRec, const tStrComp *pElemName, tSymbolSize Size)
{
  PStructElem pRun;
  String exp_name_buf;
  tStrComp exp_name;
  const tStrComp *p_exp_name;

  StrCompMkTemp(&exp_name, exp_name_buf, sizeof(exp_name_buf));
  p_exp_name = ExpandStrSymbol(&exp_name, pElemName, !CaseSensitive);
  if (!p_exp_name)
    return;
  for (pRun = pStructRec->Elems; pRun; pRun = pRun->Next)
  {
    if (!strcmp(p_exp_name->str.p_str, pRun->pElemName))
    {
      pRun->OpSize = Size;
      return;
    }
  }
  fprintf(stderr, "SetStructElemSize: cannot set size of '%s', something wicked is going on\n", p_exp_name->str.p_str);
}

/*!------------------------------------------------------------------------
 * \fn     ResolveStructReferences(PStructRec pStructRec)
 * \brief  resolve referenced elements in structure
 * \param  pStructRec structure to work on
 * ------------------------------------------------------------------------ */

void ResolveStructReferences(PStructRec pStructRec)
{
  Boolean AllResolved, DidResolve;
  PStructElem pRun, pRef;

  /* iterate over list until all symbols resolved, or we failed to resolve at least one per pass */

  do
  {
    AllResolved = True;
    DidResolve = False;

    for (pRun = pStructRec->Elems; pRun; pRun = pRun->Next)
      if (pRun->pRefElemName)
      {
        /* Only resolve to elements that already have been resolved.
           That's why we may need more than one pass.  */

        for (pRef = pStructRec->Elems; pRef; pRef = pRef->Next)
        {
          if (!strcmp(pRun->pRefElemName, pRef->pElemName) && !pRef->pRefElemName)
          {
            pRun->Offset = pRef->Offset;
            if (pRun->OpSize == eSymbolSizeUnknown)
              pRun->OpSize = pRef->OpSize;
            free(pRun->pRefElemName);
            pRun->pRefElemName = NULL;
            DidResolve = True;
            break;
          }
        }
        if (!pRef)
          AllResolved = False;
      }
  }
  while (!AllResolved && DidResolve);
  if (!AllResolved)
  {
    String Str;

    for (pRun = pStructRec->Elems; pRun; pRun = pRun->Next)
      if (pRun->pRefElemName)
      {
        strmaxcpy(Str, pRun->pElemName, STRINGSIZE);
        strmaxcat(Str, " -> ", STRINGSIZE);
        strmaxcat(Str, pRun->pRefElemName, STRINGSIZE);
        WrXError(ErrNum_UnresolvedStructRef, Str);
      }
  }
}

void BuildStructName(char *pResult, unsigned ResultLen, const char *pName)
{
  PStructStack ZStruct;
  String tmp2;

  strmaxcpy(pResult, pName, ResultLen);
  for (ZStruct = StructStack; ZStruct; ZStruct = ZStruct->Next)
    if (ZStruct->StructRec->DoExt && ZStruct->Name[0])
    {
      as_snprintf(tmp2, sizeof(tmp2), "%s%c", ZStruct->pBaseName, ZStruct->StructRec->ExtChar);
      strmaxprep(pResult, tmp2, ResultLen);
    }
}

void AddStructSymbol(const char *pName, LargeWord Value)
{
  PStructStack ZStruct;

  /* what we get is offset/length in current structure.  Add to
     it all offsets in parent structures, i.e. leave out SaveCurrPC
     of bottom of stack which contains saved PC of non-struct segment: */

  for (ZStruct = StructStack; ZStruct->Next; ZStruct = ZStruct->Next)
    Value += ZStruct->SaveCurrPC;

  {
    String tmp;
    tStrComp TmpComp;

    as_snprintf(tmp, sizeof(tmp), "%s%c%s", pInnermostNamedStruct->Name, pInnermostNamedStruct->StructRec->ExtChar, pName);
    StrCompMkTemp(&TmpComp, tmp, sizeof(tmp));
    EnterIntSymbol(&TmpComp, Value, SegNone, False);
  }
}

void BumpStructLength(PStructRec StructRec, LongInt Length)
{
  if (StructRec->TotLen < Length)
    StructRec->TotLen = Length;
}

static Boolean StructAdder(PTree *PDest, PTree Neu, void *pData)
{
  PStructNode NewNode = (PStructNode) Neu, *Node;
  Boolean Protest = *((Boolean*)pData), Result = False;

  if (!PDest)
  {
    NewNode->Defined = TRUE;
    return True;
  }

  Node = (PStructNode*) PDest;
  if ((*Node)->Defined)
  {
    if (Protest)
      WrXError(ErrNum_DoubleStruct, Neu->Name);
    else
    {
      DestroyStructRec((*Node)->StructRec);
      (*Node)->StructRec = NewNode->StructRec;
    }
  }
  else
  {
    DestroyStructRec((*Node)->StructRec);
    (*Node)->StructRec = NewNode->StructRec;
    (*Node)->Defined = True;
    return True;
  }
  return Result;
}

void AddStruct(PStructRec StructRec, char *Name, Boolean Protest)
{
  PStructNode Node;
  PTree TreeRoot;

  Node = (PStructNode) malloc(sizeof(TStructNode));
  if (Node)
  {
    Node->Tree.Left = Node->Tree.Right = NULL;
    Node->Tree.Name = as_strdup(Name);
    if (!CaseSensitive)
      NLS_UpString(Node->Tree.Name);
    Node->Tree.Attribute = MomSectionHandle;
    Node->Defined = FALSE;
    Node->StructRec = StructRec;
    TreeRoot = &(StructRoot->Tree);
    EnterTree(&TreeRoot, &(Node->Tree), StructAdder, &Protest);
    StructRoot = (PStructNode)TreeRoot;
  }
}

static PStructRec FoundStruct_FNode(LongInt Handle, const char *Name)
{
  PStructNode Lauf;

  Lauf = (PStructNode) SearchTree((PTree)StructRoot, Name, Handle);
  return Lauf ? Lauf->StructRec : NULL;
}

Boolean FoundStruct(PStructRec *Erg, const char *pName)
{
  PSaveSection Lauf;

  *Erg = FoundStruct_FNode(MomSectionHandle, pName);
  if (*Erg)
    return True;
  Lauf = SectionStack;
  while (Lauf)
  {
    *Erg = FoundStruct_FNode(Lauf->Handle, pName);
    if (*Erg)
      return True;
    Lauf = Lauf->Next;
  }
  return False;
}

static void ResDef(PTree Tree, void *pData)
{
  UNUSED(pData);

  ((PStructNode)Tree)->Defined = FALSE;
}

void ResetStructDefines(void)
{
  IterTree((PTree)StructRoot, ResDef, NULL);
}

typedef struct
{
  LongInt Sum;
  as_dynstr_t num_str;
} TPrintContext;

static void PrintDef(PTree Tree, void *pData)
{
  PStructNode Node = (PStructNode)Tree;
  PStructElem Elem;
  TPrintContext *pContext = (TPrintContext*)pData;
  String s;
  char NumStr[30];
  TempResult t;

  UNUSED(pData);
  as_tempres_ini(&t);

  WrLstLine("");
  pContext->Sum++;
  strmaxcpy(s, Node->Tree.Name, STRINGSIZE);
  if (Node->Tree.Attribute != -1)
  {
    strmaxcat(s, "[", STRINGSIZE);
    strmaxcat(s, GetSectionName(Node->Tree.Attribute), STRINGSIZE);
    strmaxcat(s, "]", STRINGSIZE);
  }
  WrLstLine(s);
  for (Elem = Node->StructRec->Elems; Elem; Elem = Elem->Next)
  {
    as_tempres_set_int(&t, Elem->Offset);
    StrSym(&t, False, &pContext->num_str, ListRadixBase);
    as_snprintf(s, sizeof(s), "%3s", pContext->num_str.p_str);
    if (Elem->BitPos >= 0)
    {
      if (Elem->BitWidthM1 >= 0)
        as_snprintf(NumStr, sizeof(NumStr), ".%d-%d", Elem->BitPos, Elem->BitPos + Elem->BitWidthM1);
      else
        as_snprintf(NumStr, sizeof(NumStr), ".%d", Elem->BitPos);
    }
    else
      *NumStr = '\0';
    as_snprcatf(s, sizeof(s), "%-6s", NumStr);
    if (Elem->OpSize != eSymbolSizeUnknown)
      as_snprcatf(s, sizeof(s), "(%s)", GetSymbolSizeName(Elem->OpSize));
    else
      strmaxcat(s, "   ", STRINGSIZE);
    as_snprcatf(s, sizeof(s), " %s", Elem->pElemName);
    WrLstLine(s);
  }
  as_tempres_free(&t);
}

void PrintStructList(void)
{
  TPrintContext Context;
  String s;

  if (!StructRoot)
    return;

  NewPage(ChapDepth, True);
  WrLstLine(getmessage(Num_ListStructListHead1));
  WrLstLine(getmessage(Num_ListStructListHead2));

  Context.Sum = 0;
  as_dynstr_ini(&Context.num_str, STRINGSIZE);
  IterTree((PTree)StructRoot, PrintDef, &Context);
  as_dynstr_free(&Context.num_str);
  as_snprintf(s, sizeof(s), "%" PRILongInt "%s",
              Context.Sum,
              getmessage((Context.Sum == 1) ? Num_ListStructSumMsg : Num_ListStructSumsMsg));
  WrLstLine(s);
}

static void ClearNode(PTree Tree, void *pData)
{
  PStructNode Node = (PStructNode) Tree;
  UNUSED(pData);

  DestroyStructRec(Node->StructRec);
}

void ClearStructList(void)
{
  PTree TreeRoot;

  TreeRoot = &(StructRoot->Tree);
  StructRoot = NULL;
  DestroyTree(&TreeRoot, ClearNode, NULL);
}

static void ExpandStruct_One(PStructRec StructRec, char *pVarPrefix, char *pStructPrefix, LargeWord Base)
{
  PStructElem StructElem;
  int VarLen, RemVarLen, StructLen, RemStructLen;
  tStrComp TmpComp;

  VarLen = strlen(pVarPrefix);
  pVarPrefix[VarLen] = StructRec->ExtChar;
  RemVarLen = STRINGSIZE - 3 - VarLen;

  StructLen = strlen(pStructPrefix);
  pStructPrefix[StructLen] = StructRec->ExtChar;
  RemStructLen = STRINGSIZE - 3 - StructLen;

  if ((RemVarLen > 1) && (RemStructLen > 1))
  {
    for (StructElem = StructRec->Elems; StructElem; StructElem = StructElem->Next)
    {
      strmaxcpy(pVarPrefix + VarLen + 1, StructElem->pElemName, RemVarLen);
      StrCompMkTemp(&TmpComp, pVarPrefix, 0);
      StructElem->ExpandFnc(&TmpComp, StructElem, Base);
      if (StructElem->IsStruct)
      {
        TStructRec *pSubStruct;
        Boolean Found;

        strmaxcpy(pStructPrefix + StructLen + 1, StructElem->pElemName, RemStructLen);
        Found = FoundStruct(&pSubStruct, pStructPrefix);
        if (Found)
          ExpandStruct_One(pSubStruct, pVarPrefix, pStructPrefix, Base + StructElem->Offset);
      }
    }
  }
  pVarPrefix[VarLen] = '\0';
  pStructPrefix[StructLen] = '\0';
}

/*!------------------------------------------------------------------------
 * \fn     ExpandStruct(PStructRec StructRec)
 * \brief  expand a defined structure
 * \param  StructRec structure to expand
 * ------------------------------------------------------------------------ */

#define DIMENSION_MAX 3

void ExpandStruct(PStructRec StructRec, const char *p_struct_name)
{
  String CompVarName, CompStructName;
  int z;
  unsigned DimensionCnt = 0, Dim;
  LargeInt Dimensions[DIMENSION_MAX];
  tStrComp Arg;
  tEvalResult EvalResult;

  if (!LabPart.str.p_str[0])
  {
    WrError(ErrNum_StructNameMissing);
    return;
  }

  /* currently, we only support array dimensions as arguments */

  for (z = 1; z <= ArgCnt; z++)
    if (IsIndirectGen(ArgStr[z].str.p_str, "[]"))
    {
      if (DimensionCnt >= DIMENSION_MAX)
      {
        WrStrErrorPos(ErrNum_TooManyArrayDimensions, &ArgStr[z]);
        return;
      }
      StrCompRefRight(&Arg, &ArgStr[z], 1);
      StrCompShorten(&Arg, 1);
      Dimensions[DimensionCnt++] = EvalStrIntExpressionWithResult(&Arg, UInt32, &EvalResult);
      if (!EvalResult.OK)
        return;
      if (EvalResult.Flags & eSymbolFlag_FirstPassUnknown)
      {
        WrStrErrorPos(ErrNum_FirstPassCalc, &Arg);
        return;
      }
      if (Dimensions[DimensionCnt - 1] <= 0)
      {
        WrStrErrorPos(ErrNum_UnderRange, &Arg);
        return;
      }
    }
    else
    {
      WrStrErrorPos(ErrNum_InvStructArgument, &ArgStr[z]);
      return;
    }

  strmaxcpy(CompStructName, p_struct_name, sizeof(CompStructName));
  strmaxcpy(CompVarName, LabPart.str.p_str, sizeof(CompVarName));
  if (!DimensionCnt)
  {
    ExpandStruct_One(StructRec, CompVarName, CompStructName, EProgCounter());
    CodeLen = StructRec->TotLen;
  }
  else
  {
    LargeInt Indices[DIMENSION_MAX];
    int CompVarNameLens[DIMENSION_MAX];
    tStrComp LabelComp;

    /* Start with element [0,...,0] and build associated names.
       Store length of names up to given dimension so we don't
       have to rebuild the name with all indices upon every elemnt: */

    for (Dim = 0; Dim < DimensionCnt; Dim++)
    {
      Indices[Dim] = 0;
      CompVarNameLens[Dim] = strlen(CompVarName);
      as_snprcatf(CompVarName, sizeof(CompVarName), "_%llu", (LargeWord)Indices[Dim]);
    }
    while (Indices[0] < Dimensions[0])
    {
      StrCompMkTemp(&LabelComp, CompVarName, sizeof(CompVarName));
      LabelHandle(&LabelComp, EProgCounter() + CodeLen, True);
      ExpandStruct_One(StructRec, CompVarName, CompStructName, EProgCounter() + CodeLen);
      CodeLen += StructRec->TotLen;

      /* increase indices, ripple through 'carry' from minor to major indices */

      Indices[DimensionCnt - 1]++;
      for (Dim = DimensionCnt - 1; Dim > 0; Dim--)
        if (Indices[Dim] >= Dimensions[Dim])
        {
          Indices[Dim] = 0;
          Indices[Dim - 1]++;
        }
        else
          break;

      /* Dim now holds the most major (leftmost) index that changed.  Build up new element
         name, starting from that: */

      CompVarName[CompVarNameLens[Dim]] = '\0';
      for (; Dim < DimensionCnt; Dim++)
      {
        as_snprcatf(CompVarName, sizeof(CompVarName), "_%llu", (LargeWord)Indices[Dim]);
        if (Dim + 1 < DimensionCnt)
          CompVarNameLens[Dim + 1] = strlen(CompVarName);
      }
    }
  }
  BookKeeping();
  DontPrint = True;
}

void asmstruct_init(void)
{
   StructRoot = NULL;
}
