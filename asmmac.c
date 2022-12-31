/* asmmac.c  */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Unterroutinen des Makroprozessors                                         */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "nlmessages.h"
#include "as.rsc"
#include "stringlists.h"
#include "strutil.h"
#include "chunks.h"
#include "trees.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmif.h"

#include "asmmac.h"


PInputTag FirstInputTag;
POutputTag FirstOutputTag;

/*=== Praeprozessor =======================================================*/

/*-------------------------------------------------------------------------*/
/* Verwaltung define-Symbole */

static void FreeDefine(PDefinement P)
{
  free(P->TransFrom);
  free(P->TransTo);
  free(P);
}

static void EnterDefine(char *Name, char *Definition)
{
  PDefinement Neu;
  int z, l;

  if (!ChkSymbName(Name))
  {
    WrXError(ErrNum_InvSymName, Name);
    return;
  };

  Neu = FirstDefine;
  while (Neu)
  {
    if (!strcmp(Neu->TransFrom, Name))
    {
      if (PassNo == 1)
        WrXError(ErrNum_DoubleDef, Name);
      return;
    }
    Neu = Neu->Next;
  }

  Neu = (PDefinement) malloc(sizeof(TDefinement));
  Neu->Next = FirstDefine;
  Neu->TransFrom = as_strdup(Name);
  if (!CaseSensitive)
    NLS_UpString(Neu->TransFrom);
  Neu->TransTo = as_strdup(Definition);
  l = strlen(Name);
  for (z = 0; z < 256; Neu->Compiled[z++] = l);
  for (z = 0; z < l - 1; z++)
    Neu->Compiled[(unsigned int)Neu->TransFrom[z]] = l - (z + 1);
  FirstDefine = Neu;
}

static void RemoveDefine(char *Name_O)
{
  PDefinement Lauf, Del;
  String Name;

  strmaxcpy(Name, Name_O, STRINGSIZE);
  if (!CaseSensitive)
    NLS_UpString(Name);

  Del = NULL;

  if (FirstDefine)
  {
    if (!strcmp(FirstDefine->TransFrom, Name))
    {
      Del = FirstDefine;
      FirstDefine = FirstDefine->Next;
    }
    else
    {
      Lauf = FirstDefine;
      while ((Lauf->Next) && (strcmp(Lauf->Next->TransFrom, Name)))
        Lauf = Lauf->Next;
      if (Lauf->Next)
      {
        Del = Lauf->Next;
        Lauf->Next = Del->Next;
      }
    }
   }

   if (!Del)
     WrXError(ErrNum_SymbolUndef, Name);
   else
     FreeDefine(Del);
}

void PrintDefineList(void)
{
  PDefinement Lauf;
  String OneS;

  if (!FirstDefine)
    return;

  NewPage(ChapDepth, True);
  WrLstLine(getmessage(Num_ListDefListHead1));
  WrLstLine(getmessage(Num_ListDefListHead2));
  WrLstLine("");

  Lauf = FirstDefine;
  while (Lauf)
  {
    strmaxcpy(OneS, Lauf->TransFrom, STRINGSIZE);
    strmaxcat(OneS, Blanks(10 - (strlen(Lauf->TransFrom)%10)), STRINGSIZE);
    strmaxcat(OneS, " = ", STRINGSIZE);
    strmaxcat(OneS, Lauf->TransTo, STRINGSIZE);
    WrLstLine(OneS);
    Lauf = Lauf->Next;
  }
  WrLstLine("");
}

void ClearDefineList(void)
{
  PDefinement Temp;

  while (FirstDefine)
  {
    Temp = FirstDefine;
    FirstDefine = FirstDefine->Next;
    FreeDefine(Temp);
  }
}

/*------------------------------------------------------------------------*/
/* Interface */

void Preprocess(void)
{
  String h, Cmd, Arg;
  char *p;

  p = strchr(OneLine.p_str, '#') + 1;
  strmaxcpy(h, p, STRINGSIZE);
  p = FirstBlank(h);
  if (!p)
  {
    strmaxcpy(Cmd, h, STRINGSIZE);
    *h = '\0';
  }
  else
    SplitString(h, Cmd, h, p);

  KillPrefBlanks(h);
  KillPostBlanks(h);

  if (!IfAsm)
    return;

  if (!as_strcasecmp(Cmd, "DEFINE"))
  {
    p = FirstBlank(h);
    if (p)
    {
      SplitString(h, Arg, h, p);
      KillPrefBlanks(h);
      EnterDefine(Arg, h);
    }
  }
  else if (!as_strcasecmp(Cmd, "UNDEF"))
    RemoveDefine(h);
  else
    WrXError(ErrNum_InvalidPrepDir, Cmd);

  CodeLen = 0;
}

static Boolean ExpandDefines_NErl(char inp)
{
  return (((inp >= '0') && (inp <= '9')) || ((inp >= 'A') && (inp <= 'Z')) || ((inp >= 'a') && (inp <= 'z')));
}

#define t_toupper(ch) ((CaseSensitive) ? (ch) : (as_toupper(ch)))

void ExpandDefines(char *Line)
{
  PDefinement Lauf;
  sint LPos, Diff, p, p2, p3, z, z2, FromLen, ToLen, LineLen;

  Lauf = FirstDefine;
  while (Lauf)
  {
    LPos = 0;
    FromLen = strlen(Lauf->TransFrom);
    ToLen = strlen(Lauf->TransTo);
    Diff = ToLen - FromLen;
    do
    {
      /* Stelle, ab der verbatim, suchen -->p */
      p = LPos;
      while ((p < (int)strlen(Line)) && (Line[p] != '\'') && (Line[p] != '"'))
        p++;
      /* nach Quellstring suchen, ersetzen, bis keine Treffer mehr */
      p2 = LPos;
      do
      {
        z2 = 0;
        while ((z2 >= 0) && (p2 <= p - FromLen))
        {
          z2 = FromLen - 1;
          z = p2 + z2;
          while ((z2 >= 0) && (t_toupper(Line[z]) == Lauf->TransFrom[z2]))
          {
            z2--;
            z--;
          }
          if (z2 >= 0)
            p2 += Lauf->Compiled[(unsigned int)t_toupper(Line[p2 + FromLen - 1])];
        }
        if (z2 == -1)
        {
          if (((p2 == 0) || (!ExpandDefines_NErl(Line[p2 - 1])))
           && ((p2 + FromLen == p) || (!ExpandDefines_NErl(Line[p2 + FromLen]))))
          {
            if (Diff != 0)
              memmove(Line + p2 + ToLen, Line + p2 + FromLen, strlen(Line) - p2 - FromLen + 1);
            memcpy(Line + p2, Lauf->TransTo, ToLen);
            p += Diff; /* !!! */
            p2 += ToLen;
          }
          else
            p2 += FromLen;
        }
      }
      while (z2 == -1);

      /* Endposition verbatim suchen */

      p3 = p + 1;
      LineLen = strlen(Line);
      while ((p3 < LineLen) && (Line[p3] != Line[p]))
        p3++;
      /* Zaehler entsprechend herauf */
      LPos = p3 + 1;
    }
    while (LPos <= LineLen - FromLen);
    Lauf = Lauf->Next;
  }
}

/*=== Makrolistenverwaltung ===============================================*/

typedef struct sMacroNode
{
  TTree Tree;
  Boolean Defined;
  PMacroRec Contents;
} TMacroNode, *PMacroNode;

static PMacroNode MacroRoot;

static Boolean MacroAdder(PTree *PDest, PTree Neu, void *pData)
{
  PMacroNode NewNode = (PMacroNode) Neu, *Node;
  Boolean Protest = *((Boolean*)pData), Result = False;

  if (!PDest)
  {
    NewNode->Defined = TRUE;
    return True;
  }

  Node = (PMacroNode*) PDest;
  if ((*Node)->Defined)
  {
    if (Protest) WrXError(ErrNum_DoubleMacro, Neu->Name);
    else
    {
      ClearMacroRec(&((*Node)->Contents), TRUE);
     (*Node)->Contents = NewNode->Contents;
    }
  }
  else
  {
    ClearMacroRec(&((*Node)->Contents), TRUE);
    (*Node)->Contents = NewNode->Contents;
    (*Node)->Defined = True;
    return True;
  }
  return Result;
}

void AddMacro(PMacroRec Neu, LongInt DefSect, Boolean Protest)
{
  PMacroNode NewNode;
  PTree TreeRoot;

  if (!CaseSensitive)
    NLS_UpString(Neu->Name);
  NewNode = (PMacroNode) malloc(sizeof(TMacroNode));
  NewNode->Tree.Left = NULL;
  NewNode->Tree.Right = NULL;
  NewNode->Tree.Name = as_strdup(Neu->Name);
  NewNode->Tree.Attribute = DefSect;
  NewNode->Contents = Neu;

  TreeRoot = &(MacroRoot->Tree);
  EnterTree(&TreeRoot, &(NewNode->Tree), MacroAdder, &Protest);
  MacroRoot = (PMacroNode)TreeRoot;
}

static PMacroRec FoundMacro_FNode(LongInt Handle, char *Part)
{
  PMacroNode Lauf;
  PMacroRec Result = NULL;

  Lauf = (PMacroNode) SearchTree((PTree)MacroRoot, Part, Handle);
  if (Lauf)
    Result = Lauf->Contents;
  return Result;
}

Boolean FoundMacro(PMacroRec *Erg, const tStrComp *p_name)
{
  PSaveSection Lauf;

  *Erg = FoundMacro_FNode(MomSectionHandle, p_name->str.p_str);
  if (*Erg)
    return True;
  Lauf = SectionStack;
  while (Lauf)
  {
    *Erg = FoundMacro_FNode(Lauf->Handle, p_name->str.p_str);
    if (*Erg)
      return True;
    Lauf = Lauf->Next;
  }
  return False;
}

static void ClearMacroList_ClearNode(PTree Tree, void *pData)
{
  PMacroNode Node = (PMacroNode) Tree;
  UNUSED(pData);

  ClearMacroRec(&(Node->Contents), TRUE);
}

void ClearMacroList(void)
{
  PTree TreeRoot;

  TreeRoot = &(MacroRoot->Tree);
   MacroRoot = NULL;
  DestroyTree(&TreeRoot, ClearMacroList_ClearNode, NULL);
}

static void ResetMacroDefines_ResetNode(PTree Tree, void *pData)
{
  PMacroNode Node = (PMacroNode)Tree;
  UNUSED(pData);

  Node->Defined = False;
}

void ResetMacroDefines(void)
{
  IterTree((PTree)MacroRoot, ResetMacroDefines_ResetNode, NULL);
}

void ClearMacroRec(PMacroRec *Alt, Boolean Complete)
{
  if ((*Alt)->Name)
  {
    free((*Alt)->Name);
    (*Alt)->Name = NULL;
  }
  ClearStringList(&((*Alt)->FirstLine));
  ClearStringList(&((*Alt)->ParamNames));
  ClearStringList(&((*Alt)->ParamDefVals));

  if (Complete)
  {
    free(*Alt);
    *Alt = NULL;
  }
}

typedef struct
{
  LongInt Sum;
  Boolean cnt;
  String OneS;
} TMacroListContext;

static void PrintMacroList_PNode(PTree Tree, void *pData)
{
  PMacroNode Node = (PMacroNode)Tree;
  TMacroListContext *pContext = (TMacroListContext*) pData;
  String h;

  strmaxcpy(h, Node->Contents->Name, STRINGSIZE);
  if (Node->Tree.Attribute != -1)
  {
    strmaxcat(h, "[", STRINGSIZE);
    strmaxcat(h, GetSectionName(Node->Tree.Attribute), STRINGSIZE);
    strmaxcat(h, "]", STRINGSIZE);
  }
  if (Node->Contents->LstMacroExpMod.Count)
  {
    strmaxcat(h, "{", STRINGSIZE);
    DumpLstMacroExpMod(&Node->Contents->LstMacroExpMod, h + strlen(h), sizeof(h) - strlen(h));
    strmaxcat(h, "}", STRINGSIZE);
  }
  strmaxcat(pContext->OneS, h, STRINGSIZE);
  if (strlen(h) < 37)
    strmaxcat(pContext->OneS, Blanks(37 - strlen(h)), STRINGSIZE);
  if (!(pContext->cnt))
    strmaxcat(pContext->OneS, " | ", STRINGSIZE);
  else
  {
    WrLstLine(pContext->OneS);
    pContext->OneS[0] = '\0';
  }
  pContext->cnt = !pContext->cnt;
  pContext->Sum++;
}

void PrintMacroList(void)
{
  TMacroListContext Context;

  if (!MacroRoot)
    return;

  NewPage(ChapDepth, True);
  WrLstLine(getmessage(Num_ListMacListHead1));
  WrLstLine(getmessage(Num_ListMacListHead2));
  WrLstLine("");

  Context.OneS[0] = '\0';
  Context.cnt = False;
  Context.Sum = 0;
  IterTree((PTree)MacroRoot, PrintMacroList_PNode, &Context);
  if (Context.cnt)
  {
    Context.OneS[strlen(Context.OneS) - 1] = '\0';
    WrLstLine(Context.OneS);
  }
  WrLstLine("");
  as_snprintf(Context.OneS, sizeof(Context.OneS), "%7lu%s",
              (unsigned long)Context.Sum,
              getmessage((Context.Sum == 1) ? Num_ListMacSumMsg : Num_ListMacSumsMsg));
  WrLstLine(Context.OneS);
  WrLstLine("");
}

/*=== Eingabefilter Makroprozessor ========================================*/

void asmmac_init(void)
{
  MacroRoot = NULL;
}
