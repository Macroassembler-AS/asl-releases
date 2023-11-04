/* as.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Hauptmodul                                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>

#include "version.h"
#include "be_le.h"
#include "bpemu.h"

#include "stdhandl.h"
#include "nls.h"
#include "nlmessages.h"
#include "as.rsc"
#ifdef _USE_MSH
# include "as.msh"
#endif
#include "ioerrs.h"
#include "strutil.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "msg_level.h"
#include "asmitree.h"
#include "trees.h"
#include "chunks.h"
#include "console.h"
#include "asminclist.h"
#include "asmfnums.h"
#include "asmdef.h"
#include "cpulist.h"
#include "asmerr.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmmac.h"
#include "asmstructs.h"
#include "asmif.h"
#include "asmcode.h"
#include "asmlist.h"
#include "asmlabel.h"
#include "asmdebug.h"
#include "asmrelocs.h"
#include "literals.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "codepseudo.h"
#include "as.h"

#include "code68k.h"
#include "code56k.h"
#include "code601.h"
#include "codepalm.h"
#include "codemcore.h"
#include "codexgate.h"
#include "code68.h"
#include "code6805.h"
#include "code6809.h"
#include "code6812.h"
#include "codes12z.h"
#include "code6816.h"
#include "code68rs08.h"
#include "codeh8_3.h"
#include "codeh8_5.h"
#include "code7000.h"
#include "codeko09.h"
#include "code65.h"
#include "codepps4.h"
#include "codeh16.h"
#include "code7700.h"
#include "codehmcs400.h"
#include "code4500.h"
#include "codem16.h"
#include "codem16c.h"
#include "codepdp11.h"
#include "codevax.h"
#include "code4004.h"
#include "code8008.h"
#include "code48.h"
#include "code51.h"
#include "code96.h"
#include "code85.h"
#include "code86.h"
#include "code960.h"
#include "code8x30x.h"
#include "code2650.h"
#include "codexa.h"
#include "codeavr.h"
#include "code29k.h"
#include "code166.h"
#include "codez80.h"
#include "codez8.h"
#include "codez8000.h"
#include "codekcpsm.h"
#include "codekcp3.h"
#include "codemic8.h"
#include "code96c141.h"
#include "code90c141.h"
#include "code87c800.h"
#include "code870c.h"
#include "code47c00.h"
#include "code97c241.h"
#include "code9331.h"
#include "code16c5x.h"
#include "code16c8x.h"
#include "code17c4x.h"
#include "codesx20.h"
#include "codepdk.h"
#include "codest6.h"
#include "codest7.h"
#include "codest9.h"
#include "code6804.h"
#include "code3201x.h"
#include "code3202x.h"
#include "code3203x.h"
#include "code3205x.h"
#include "code3254x.h"
#include "code3206x.h"
#include "code9900.h"
#include "codetms7.h"
#include "code370.h"
#include "codemsp.h"
#include "codetms1.h"
#include "codescmp.h"
#include "codeimp16.h"
#include "code807x.h"
#include "codecop4.h"
#include "codecop8.h"
#include "codesc14xxx.h"
#include "codens32k.h"
#include "codeace.h"
#include "codecp3f.h"
#include "codef8.h"
#include "code78c10.h"
#include "code75xx.h"
#include "code75k0.h"
#include "code78k0.h"
#include "code78k2.h"
#include "code78k3.h"
#include "code78k4.h"
#include "code7720.h"
#include "code77230.h"
#include "codev60.h"
#include "code53c8xx.h"
#include "codefmc8.h"
#include "codefmc16.h"
#include "codemn1610.h"
#include "codemn2610.h"
#include "codeol40.h"
#include "codeol50.h"
#include "code1802.h"
#include "codevector.h"
#include "codexcore.h"
#include "code1750.h"
#include "codekenbak.h"
#include "codecp1600.h"
#include "codenano.h"
#include "code6100.h"
#include "coderx.h"
/**          Code21xx};**/

static long StartTime, StopTime;
static Boolean GlobErrFlag;
static unsigned MacroNestLevel = 0;

/*=== Zeilen einlesen ======================================================*/


#if 0
# define dbgentry(str) printf("***enter %s\n", str);
# define dbgexit(str) printf("***exit %s\n", str);
#else
# define dbgentry(str) {}
# define dbgexit(str) {}
#endif

#define LEAVE goto func_exit

static void NULL_Restorer(PInputTag PInp)
{
  UNUSED(PInp);
}

static Boolean NULL_GetPos(PInputTag PInp, char *dest, size_t DestSize, Boolean ActGNUErrors)
{
  UNUSED(PInp);
  UNUSED(ActGNUErrors);

  if (DestSize)
    *dest = '\0';
  return False;
}

static Boolean INCLUDE_Processor(PInputTag PInp, as_dynstr_t *p_dest);

static PInputTag GenerateProcessor(void)
{
  PInputTag PInp = (PInputTag)malloc(sizeof(TInputTag));

  PInp->IsMacro = False;
  PInp->Next = NULL;
  PInp->First = True;
  PInp->OrigDoLst = DoLst;
  PInp->StartLine = CurrLine;
  PInp->ParCnt = 0; PInp->ParZ = 0;
  InitStringList(&(PInp->Params));
  PInp->LineCnt = PInp->ContLineCnt = 0; PInp->LineZ = 1;
  PInp->Lines = PInp->LineRun = NULL;
  StrCompMkTemp(&PInp->SpecName, PInp->SpecNameStr, sizeof(PInp->SpecNameStr));
  StrCompReset(&PInp->SpecName);
  PInp->AllArgs[0] = '\0';
  PInp->NumArgs[0] = '\0';
  PInp->IsEmpty = False;
  PInp->Buffer = NULL;
  PInp->Datei = NULL;
  PInp->IfLevel = SaveIFs();
  PInp->IncludeLevel = CurrIncludeLevel;
  PInp->Restorer = NULL_Restorer;
  PInp->GetPos = NULL_GetPos;
  PInp->Macro = NULL;
  PInp->SaveAttr[0] = '\0';
  PInp->SaveLabel[0] = '\0';
  PInp->GlobalSymbols = False;
  PInp->UsesNumArgs =
  PInp->UsesAllArgs = False;

  /* in case the input tag chain is empty, this must be the master file */

  PInp->FromFile = !FirstInputTag || (FirstInputTag->Processor == INCLUDE_Processor);

  return PInp;
}

static POutputTag GenerateOUTProcessor(SimpProc Processor, tErrorNum OpenErrMsg)
{
  POutputTag POut;

  POut = (POutputTag) malloc(sizeof(TOutputTag));
  POut->Processor = Processor;
  POut->NestLevel = 0;
  POut->Tag = NULL;
  POut->Mac = NULL;
  POut->ParamNames = NULL;
  POut->ParamDefVals = NULL;
  POut->PubSect = 0;
  POut->GlobSect = 0;
  POut->DoExport = False;
  POut->DoGlobCopy= False;
  POut->UsesNumArgs =
  POut->UsesAllArgs = False;
  *POut->GName = '\0';
  POut->OpenErrMsg = OpenErrMsg;

  return POut;
}

/*=========================================================================*/
/* Makroprozessor */

/*-------------------------------------------------------------------------*/
/* allgemein gebrauchte Subfunktionen */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* werden gebraucht, um festzustellen, ob innerhalb eines Makrorumpfes weitere
   Makroschachtelungen auftreten */

static Boolean MacroStart(void)
{
  return ((Memo("MACRO")) || (Memo("IRP")) || (Memo("IRPC")) || (Memo("REPT")) || (Memo("WHILE")));
}

static Boolean MacroEnd(void)
{
  if (Memo("ENDM"))
  {
    WasMACRO = True;
    return True;
  }
  else
    return False;
}

typedef void (*tMacroArgCallback)(Boolean CtrlArg, const tStrComp *pArg, void *pUser);

static void ProcessMacroArgs(tMacroArgCallback Callback, void *pUser)
{
  tStrComp *pArg;
  int l;

  for (pArg = ArgStr + 1; pArg <= ArgStr + ArgCnt; pArg++)
  {
    l = strlen(pArg->str.p_str);
    if ((l >= 2) && (pArg->str.p_str[0] == '{') && (pArg->str.p_str[l - 1] == '}'))
    {
      tStrComp Arg;

      StrCompRefRight(&Arg, pArg, 1);
      StrCompShorten(&Arg, 1);
      Callback(TRUE, &Arg, pUser);
    }
    else
    {
      Callback(FALSE, pArg, pUser);
    }
  }
}

/*-------------------------------------------------------------------------*/
/* Dieser Einleseprozessor dient nur dazu, eine fehlerhafte Makrodefinition
  bis zum Ende zu ueberlesen */

static void WaitENDM_Processor(void)
{
  POutputTag Tmp;

  if (MacroStart())
    FirstOutputTag->NestLevel++;
  else if (MacroEnd())
    FirstOutputTag->NestLevel--;
  if (FirstOutputTag->NestLevel <= -1)
  {
    Tmp = FirstOutputTag;
    FirstOutputTag = Tmp->Next;
    free(Tmp);
  }
}

static void AddWaitENDM_Processor(void)
{
  POutputTag Neu;

  Neu = GenerateOUTProcessor(WaitENDM_Processor, ErrNum_OpenMacro);
  Neu->Next = FirstOutputTag;
  FirstOutputTag = Neu;
}

/*-------------------------------------------------------------------------*/
/* normale Makros */

static void ComputeMacroStrings(PInputTag Tag)
{
  StringRecPtr Lauf;

  /* recompute # of params */

  if (Tag->UsesNumArgs)
    as_snprintf(Tag->NumArgs, sizeof(Tag->NumArgs), Integ32Format, Tag->ParCnt);

  /* recompute 'all string' parameter */

  if (Tag->UsesAllArgs)
  {
    Tag->AllArgs[0] = '\0';
    Lauf = Tag->Params;
    while (Lauf)
    {
      if (Tag->AllArgs[0] != '\0')
        strmaxcat(Tag->AllArgs, ",", STRINGSIZE);
      strmaxcat(Tag->AllArgs, Lauf->Content, STRINGSIZE);
      Lauf = Lauf->Next;
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Diese Routine leitet die Quellcodezeilen bei der Makrodefinition in den
   Makro-Record um */

static void MACRO_OutProcessor(void)
{
  POutputTag Tmp;
  int z;
  StringRecPtr l;
  PMacroRec GMacro;

  WasMACRO = True;

  /* write preprocessed output to file ? */

  if ((MacroOutput) && (FirstOutputTag->DoExport))
  {
    errno = 0;
    fprintf(MacroFile, "%s\n", OneLine.p_str);
    ChkIO(ErrNum_FileWriteError);
  }

  /* check for additional nested macros resp. end of definition */

  if (MacroStart())
    FirstOutputTag->NestLevel++;
  else if (MacroEnd())
    FirstOutputTag->NestLevel--;

  /* still lines to put into the macro body ? */

  if (FirstOutputTag->NestLevel != -1)
  {
    as_dynstr_t s;

    as_dynstr_ini_clone(&s, &OneLine);
    KillCtrl(s.p_str);

    /* compress into tokens */

    l = FirstOutputTag->ParamNames;
    for (z = 1; z <= FirstOutputTag->Mac->ParamCount; z++)
    {
      const char *p_param_name = GetStringListNext(&l);
      CompressLine(p_param_name ? p_param_name : "", z, &s, CaseSensitive);
    }

    /* reserved argument names are never case-sensitive */

    if (HasAttrs)
      CompressLine(AttrName, ArgCntMax + 1, &s, FALSE);
    if (CompressLine(ArgCName, ArgCntMax + 2, &s, FALSE) > 0)
      FirstOutputTag->UsesNumArgs = TRUE;
    if (CompressLine(AllArgName, ArgCntMax + 3, &s, FALSE) > 0)
      FirstOutputTag->UsesAllArgs = TRUE;
    if (FirstOutputTag->Mac->LocIntLabel)
      CompressLine(LabelName, ArgCntMax + 4, &s, FALSE);

    AddStringListLast(&(FirstOutputTag->Mac->FirstLine), s.p_str);
    as_dynstr_free(&s);
  }

  /* otherwise, finish definition */

  if (FirstOutputTag->NestLevel == -1)
  {
    if (IfAsm)
    {
      FirstOutputTag->Mac->UsesNumArgs = FirstOutputTag->UsesNumArgs;
      FirstOutputTag->Mac->UsesAllArgs = FirstOutputTag->UsesAllArgs;
      FirstOutputTag->Mac->ParamNames = FirstOutputTag->ParamNames;
      FirstOutputTag->ParamNames = NULL;
      FirstOutputTag->Mac->ParamDefVals = FirstOutputTag->ParamDefVals;
      FirstOutputTag->ParamDefVals = NULL;
      AddMacro(FirstOutputTag->Mac, FirstOutputTag->PubSect, True);
      if ((FirstOutputTag->DoGlobCopy) && (SectionStack))
      {
        GMacro = (PMacroRec) malloc(sizeof(MacroRec));
        GMacro->Name = as_strdup(FirstOutputTag->GName);
        GMacro->ParamCount = FirstOutputTag->Mac->ParamCount;
        GMacro->FirstLine = DuplicateStringList(FirstOutputTag->Mac->FirstLine);
        GMacro->ParamNames = DuplicateStringList(FirstOutputTag->Mac->ParamNames);
        GMacro->ParamDefVals = DuplicateStringList(FirstOutputTag->Mac->ParamDefVals);
        GMacro->UsesNumArgs = FirstOutputTag->Mac->UsesNumArgs;
        GMacro->UsesAllArgs = FirstOutputTag->Mac->UsesAllArgs;
        AddMacro(GMacro, FirstOutputTag->GlobSect, False);
      }
    }
    else
    {
      ClearMacroRec(&(FirstOutputTag->Mac), TRUE);
    }

    Tmp = FirstOutputTag;
    FirstOutputTag = Tmp->Next;
    ClearStringList(&(Tmp->ParamNames));
    ClearStringList(&(Tmp->ParamDefVals));
    free(Tmp);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Von hier her kommen bei einem Makroaufruf die expandierten Zeilen */

Boolean MACRO_Processor(PInputTag PInp, as_dynstr_t *p_dest)
{
  StringRecPtr Lauf;
  int z;
  Boolean Result;

  Result = True;

  /* run to current line */

  Lauf = PInp->Lines;
  for (z = 1; z <= PInp->LineZ - 1; z++)
    Lauf = Lauf->Next;
  as_dynstr_copy_c_str(p_dest, Lauf->Content);

  /* process parameters */

  Lauf = PInp->Params;
  for (z = 1; z <= PInp->ParCnt; z++)
  {
    ExpandLine(Lauf->Content, z, p_dest);
    Lauf = Lauf->Next;
  }

  /* process special parameters */

  if (HasAttrs)
    ExpandLine(PInp->SaveAttr, ArgCntMax + 1, p_dest);
  if (PInp->UsesNumArgs)
    ExpandLine(PInp->NumArgs, ArgCntMax + 2, p_dest);
  if (PInp->UsesAllArgs)
    ExpandLine(PInp->AllArgs, ArgCntMax + 3, p_dest);
  if (PInp->Macro->LocIntLabel)
    ExpandLine(PInp->SaveLabel, ArgCntMax + 4, p_dest);

  CurrLine = PInp->StartLine;
  InMacroFlag = True;

  /* before the first line, start a new local symbol space */

  if ((PInp->LineZ == 1) && (!PInp->GlobalSymbols))
    PushLocHandle(GetLocHandle());

  /* signal the end of the macro */

  if (++(PInp->LineZ) > PInp->LineCnt)
    Result = False;

  return Result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Initialisierung des Makro-Einleseprozesses */

static Boolean ReadMacro_SearchArg(const char *pTest, const char *pComp, Boolean *pErg)
{
  if (!as_strcasecmp(pTest, pComp))
  {
    *pErg = True;
    return True;
  }
  else if ((strlen(pTest) > 2) && (!as_strncasecmp(pTest, "NO", 2)) && (!as_strcasecmp(pTest + 2, pComp)))
  {
    *pErg = False;
    return True;
  }
  else
    return False;
}

static Boolean ReadMacro_SearchSect(char *Test_O, const char *Comp, Boolean *Erg, LongInt *Section)
{
  char *p;
  String Test, Sect;

  strmaxcpy(Test, Test_O, STRINGSIZE); KillBlanks(Test);
  p = strchr(Test, ':');
  if (!p)
    *Sect = '\0';
  else
  {
    strmaxcpy(Sect, p + 1, STRINGSIZE);
    *p = '\0';
  }
  if ((strlen(Test) > 2) && (!as_strncasecmp(Test, "NO", 2)) && (!as_strcasecmp(Test + 2, Comp)))
  {
    *Erg = False;
    return True;
  }
  else if (!as_strcasecmp(Test, Comp))
  {
    tStrComp TmpComp;

    *Erg = True;
    StrCompMkTemp(&TmpComp, Sect, sizeof(Sect));
    return (IdentifySection(&TmpComp, Section));
  }
  else
    return False;
}

typedef struct
{
  String PList;
  POutputTag pOutputTag;
  tLstMacroExpMod LstMacroExpMod;
  Boolean DoPublic, DoIntLabel, GlobalSymbols;
  Boolean ErrFlag;
  int ParamCount;
} tReadMacroContext;

static void ExpandPList(String PList, const char *pArg, Boolean CtrlArg)
{
  if (!*PList)
    strmaxcat(PList, ",", STRINGSIZE);
  if (CtrlArg)
    strmaxcat(PList, "{", STRINGSIZE);
  strmaxcat(PList, pArg, STRINGSIZE);
  if (CtrlArg)
    strmaxcat(PList, "}", STRINGSIZE);
}

static void ProcessMACROArgs(Boolean CtrlArg, const tStrComp *pArg, void *pUser)
{
  tReadMacroContext *pContext = (tReadMacroContext*)pUser;

  if (CtrlArg)
  {
    Boolean DoMacExp;

    if (ReadMacro_SearchArg(pArg->str.p_str, "EXPORT", &(pContext->pOutputTag->DoExport)));
    else if (ReadMacro_SearchArg(pArg->str.p_str, "GLOBALSYMBOLS", &pContext->GlobalSymbols));
    else if (ReadMacro_SearchArg(pArg->str.p_str, "EXPAND", &DoMacExp))
    {
      if (!AddLstMacroExpMod(&pContext->LstMacroExpMod, DoMacExp, eLstMacroExpAll))
        WrStrErrorPos(ErrNum_TooManyMacExpMod, pArg);
      ExpandPList(pContext->PList, pArg->str.p_str, CtrlArg);
    }
    else if (ReadMacro_SearchArg(pArg->str.p_str, "EXPIF", &DoMacExp))
    {
      if (!AddLstMacroExpMod(&pContext->LstMacroExpMod, DoMacExp, eLstMacroExpIf))
        WrStrErrorPos(ErrNum_TooManyMacExpMod, pArg);
      ExpandPList(pContext->PList, pArg->str.p_str, CtrlArg);
    }
    else if (ReadMacro_SearchArg(pArg->str.p_str, "EXPMACRO", &DoMacExp))
    {
      if (!AddLstMacroExpMod(&pContext->LstMacroExpMod, DoMacExp, eLstMacroExpMacro))
        WrStrErrorPos(ErrNum_TooManyMacExpMod, pArg);
      ExpandPList(pContext->PList, pArg->str.p_str, CtrlArg);
    }
    else if (ReadMacro_SearchArg(pArg->str.p_str, "EXPREST", &DoMacExp))
    {
      if (!AddLstMacroExpMod(&pContext->LstMacroExpMod, DoMacExp, eLstMacroExpRest))
        WrStrErrorPos(ErrNum_TooManyMacExpMod, pArg);
      ExpandPList(pContext->PList, pArg->str.p_str, CtrlArg);
    }
    else if (ReadMacro_SearchArg(pArg->str.p_str, "INTLABEL", &pContext->DoIntLabel))
    {
      ExpandPList(pContext->PList, pArg->str.p_str, CtrlArg);
    }
    else if (ReadMacro_SearchSect(pArg->str.p_str, "GLOBAL", &(pContext->pOutputTag->DoGlobCopy), &(pContext->pOutputTag->GlobSect)));
    else if (ReadMacro_SearchSect(pArg->str.p_str, "PUBLIC", &pContext->DoPublic, &(pContext->pOutputTag->PubSect)));
    else
    {
      WrStrErrorPos(ErrNum_UnknownMacArg, pArg);
      pContext->ErrFlag = True;
    }
  }
  else
  {
    char *pDefault;
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 0);
    ExpandPList(pContext->PList, Arg.str.p_str, CtrlArg);
    pDefault = QuotPos(Arg.str.p_str, '=');
    if (pDefault)
    {
      Arg.Pos.Len = pDefault - Arg.str.p_str;
      *pDefault++ = '\0';
      KillPostBlanksStrComp(&Arg);
      KillPrefBlanksStrComp(&Arg);
    }
    if (!ChkMacSymbName(Arg.str.p_str))
    {
      WrStrErrorPos(ErrNum_InvSymName, &Arg);
      pContext->ErrFlag = True;
    }
    if (!CaseSensitive)
      UpString(Arg.str.p_str);
    AddStringListLast(&(pContext->pOutputTag->ParamNames), Arg.str.p_str);
    AddStringListLast(&(pContext->pOutputTag->ParamDefVals), pDefault ? pDefault : "");
    pContext->ParamCount++;
  }
}

static void ReadMacro(void)
{
  PSaveSection RunSection;
  PMacroRec OneMacro;
  tReadMacroContext Context;
  LongInt HSect;
  String macro_name_buf;
  tStrComp macro_name;
  const tStrComp *p_macro_name;

  WasMACRO = True;

  CodeLen = 0;
  Context.ErrFlag = False;

  /* Makronamen pruefen */
  /* Definition nur im ersten Pass */

  StrCompMkTemp(&macro_name, macro_name_buf, sizeof(macro_name_buf));
  if (PassNo != 1)
  {
    Context.ErrFlag = True;
    p_macro_name = &LabPart;
  }
  else
  {
    p_macro_name = ExpandStrSymbol(&macro_name, &LabPart, False);
    if (!p_macro_name)
    {
      Context.ErrFlag = True;
      p_macro_name = &LabPart;
    }
    else if (!ChkSymbName(p_macro_name->str.p_str))
    {
      WrStrErrorPos(ErrNum_InvSymName, &LabPart);
      Context.ErrFlag = True;
    }
  }

  /* create tag */

  Context.pOutputTag = GenerateOUTProcessor(MACRO_OutProcessor, ErrNum_OpenMacro);
  Context.pOutputTag->Next = FirstOutputTag;

  /* check arguments, sort out control directives */

  Context.LstMacroExpMod = LstMacroExpModDefault;
  Context.DoPublic = False;
  Context.DoIntLabel = False;
  Context.GlobalSymbols = False;
  *Context.PList = '\0';
  Context.ParamCount = 0;
  ProcessMacroArgs(ProcessMACROArgs, &Context);

  /* contradicting macro expansion? */

  if (!ChkLstMacroExpMod(&Context.LstMacroExpMod))
  {
    WrError(ErrNum_ConflictingMacExpMod);
    Context.ErrFlag = True;
  }

  /* Abbruch bei Fehler */

  if (Context.ErrFlag)
  {
    ClearStringList(&(Context.pOutputTag->ParamNames));
    ClearStringList(&(Context.pOutputTag->ParamDefVals));
    free(Context.pOutputTag);
    AddWaitENDM_Processor();
    return;
  }

  /* Bei Globalisierung Namen des Extramakros ermitteln */

  if (Context.pOutputTag->DoGlobCopy)
  {
    strmaxcpy(Context.pOutputTag->GName, MacroName, STRINGSIZE);
    RunSection = SectionStack;
    HSect = MomSectionHandle;
    while ((HSect != Context.pOutputTag->GlobSect) && (RunSection != NULL))
    {
      strmaxprep(Context.pOutputTag->GName, "_", STRINGSIZE);
      strmaxprep(Context.pOutputTag->GName, GetSectionName(HSect), STRINGSIZE);
      HSect = RunSection->Handle;
      RunSection = RunSection->Next;
    }
  }
  if (!Context.DoPublic)
    Context.pOutputTag->PubSect = MomSectionHandle;

  /* chain in */

  OneMacro = (PMacroRec) calloc(1, sizeof(MacroRec));
  OneMacro->FirstLine =
  OneMacro->ParamNames =
  OneMacro->ParamDefVals = NULL;
  Context.pOutputTag->Mac = OneMacro;

  if ((MacroOutput) && (Context.pOutputTag->DoExport))
  {
    errno = 0;
    fprintf(MacroFile, "%s MACRO %s\n",
            Context.pOutputTag->DoGlobCopy ? Context.pOutputTag->GName : MacroName,
            Context.PList);
    ChkIO(ErrNum_FileWriteError);
  }

  OneMacro->UseCounter = 0;
  OneMacro->Name = as_strdup(p_macro_name->str.p_str);
  OneMacro->ParamCount = Context.ParamCount;
  OneMacro->FirstLine = NULL;
  OneMacro->LstMacroExpMod = Context.LstMacroExpMod;
  OneMacro->LocIntLabel = Context.DoIntLabel;
  OneMacro->GlobalSymbols = Context.GlobalSymbols;

  FirstOutputTag = Context.pOutputTag;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Beendigung der Expansion eines Makros */

static void MACRO_Cleanup(PInputTag PInp)
{
  ClearStringList(&(PInp->Params));
}

static Boolean MACRO_GetPos(PInputTag PInp, char *dest, size_t DestSize, Boolean ActGNUErrors)
{
  UNUSED(ActGNUErrors);
  as_snprintf(dest, DestSize, "%s(%lu) ", PInp->SpecName.str.p_str, (unsigned long)(PInp->LineZ - 1));
  return False;
}

static void MACRO_Restorer(PInputTag PInp)
{
  /* discard the local symbol space */

  if (!PInp->GlobalSymbols)
    PopLocHandle();

  /* undo the recursion counter by one */

  if ((PInp->Macro) && (PInp->Macro->UseCounter > 0))
    PInp->Macro->UseCounter--;

  /* restore list flag */

  DoLst = PInp->OrigDoLst;

  /* decrement macro nesting counter only if this actually was a macro */

  if (PInp->Processor == MACRO_Processor)
    MacroNestLevel--;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Dies initialisiert eine Makroexpansion */

static void ExpandMacro(PMacroRec OneMacro)
{
  int z1, z2;
  StringRecPtr Lauf, pDefault, pParamName, pArg;
  PInputTag Tag = NULL;
  Boolean NamedArgs;
  char *p;

  CodeLen = 0;

  if ((NestMax > 0) && (OneMacro->UseCounter > NestMax)) WrError(ErrNum_RekMacro);
  else
  {
    OneMacro->UseCounter++;

    /* 1. Tag erzeugen */

    Tag = GenerateProcessor();
    Tag->Processor = MACRO_Processor;
    Tag->Restorer  = MACRO_Restorer;
    Tag->Cleanup   = MACRO_Cleanup;
    Tag->GetPos    = MACRO_GetPos;
    Tag->Macro     = OneMacro;
    Tag->GlobalSymbols = OneMacro->GlobalSymbols;
    Tag->UsesNumArgs = OneMacro->UsesNumArgs;
    Tag->UsesAllArgs = OneMacro->UsesAllArgs;
    strmaxcpy(Tag->SpecName.str.p_str, OneMacro->Name, STRINGSIZE);
    strmaxcpy(Tag->SaveAttr, AttrPart.str.p_str, STRINGSIZE);
    if (OneMacro->LocIntLabel)
      strmaxcpy(Tag->SaveLabel, LabPart.str.p_str, STRINGSIZE);
    Tag->IsMacro   = True;

    /* 2. Store special parameters - in the original form.
          Omit this if they aren't used at all in the macro's body. */

    Tag->NumArgs[0] = '\0';
    if (Tag->UsesNumArgs)
      as_snprintf(Tag->NumArgs, sizeof(Tag->NumArgs), "%d", ArgCnt);
    Tag->AllArgs[0] = '\0';
    if (Tag->UsesAllArgs)
    {
      for (z1 = 1; z1 <= ArgCnt; z1++)
      {
        if (z1 != 1) strmaxcat(Tag->AllArgs, ",", STRINGSIZE);
        strmaxcat(Tag->AllArgs, ArgStr[z1].str.p_str, STRINGSIZE);
      }
    }
    Tag->ParCnt = OneMacro->ParamCount;

    /* 3. generate argument list */

    /* 3a. initialize with empty defaults - order is irrelevant at this point: */

    for (z1 = OneMacro->ParamCount; z1 >= 1; z1--)
      AddStringListFirst(&(Tag->Params), NULL);

    /* 3b. walk over given arguments */

    NamedArgs = False;
    for (z1 = 1; z1 <= ArgCnt; z1++)
    {
      if (!CaseSensitive) UpString(ArgStr[z1].str.p_str);

      /* explicit name given? */

      p = QuotPos(ArgStr[z1].str.p_str, '=');

      /* if parameter name given... */

      if (p)
      {
        /* split it off */

        *p++ = '\0';
        KillPostBlanksStrComp(&ArgStr[z1]);
        KillPrefBlanks(p);

        /* search parameter by name */

        for (pParamName = OneMacro->ParamNames, pArg = Tag->Params;
             pParamName; pParamName = pParamName->Next, pArg = pArg->Next)
          if (!strcmp(ArgStr[z1].str.p_str, pParamName->Content))
          {
            if (pArg->Content)
            {
              WrXError(ErrNum_MacArgRedef, pParamName->Content);
              free(pArg->Content);
            }
            pArg->Content = as_strdup(p);
            break;
          }
        if (!pParamName)
          WrStrErrorPos(ErrNum_UndefKeyArg, &ArgStr[z1]);

        /* set flag that no unnamed args are any longer allowed */

        NamedArgs = True;
      }

      /* do not mix unnamed with named arguments: */

      else if (NamedArgs)
        WrError(ErrNum_NoPosArg);

      /* empty positional parameters mean using defaults - insert non-empty args here: */

      else if ((z1 <= OneMacro->ParamCount) && (strlen(ArgStr[z1].str.p_str) > 0))
      {
        pArg = Tag->Params;
        pParamName = OneMacro->ParamNames;
        for (z2 = 0; z2 < z1 - 1; z2++)
        {
          pParamName = pParamName->Next;
          pArg = pArg->Next;
        }
        if (pArg->Content)
        {
          WrXError(ErrNum_MacArgRedef, pParamName->Content);
          free(pArg->Content);
        }
        pArg->Content = as_strdup(ArgStr[z1].str.p_str);
      }

      /* excess unnamed arguments: append at end of list */

      else if (z1 > OneMacro->ParamCount)
        AddStringListLast(&(Tag->Params), ArgStr[z1].str.p_str);
    }

    /* 3c. fill in defaults */

    for (pParamName = OneMacro->ParamNames, pArg = Tag->Params, pDefault = OneMacro->ParamDefVals;
             pParamName; pParamName = pParamName->Next, pArg = pArg->Next, pDefault = pDefault->Next)
      if (!pArg->Content)
        pArg->Content = as_strdup(pDefault->Content);

    /* 4. Zeilenliste anhaengen */

    Tag->Lines = OneMacro->FirstLine;
    Tag->IsEmpty = !OneMacro->FirstLine;
    Lauf = OneMacro->FirstLine;
    while (Lauf)
    {
      Tag->LineCnt++;
      Lauf = Lauf->Next;
    }
  }

  /* 5. anhaengen */

  if (Tag)
  {
    if (IfAsm)
    {
      /* override has higher prio, so apply as second */

      NextDoLst = ApplyLstMacroExpMod(DoLst, &OneMacro->LstMacroExpMod);
      NextDoLst = ApplyLstMacroExpMod(NextDoLst, &LstMacroExpModOverride);
      Tag->Next = FirstInputTag;
      FirstInputTag = Tag;
      MacroNestLevel++;
    }
    else
    {
      ClearStringList(&(Tag->Params)); free(Tag);
    }
  }
}

/*-------------------------------------------------------------------------*/
/* vorzeitiger Abbruch eines Makros */

static void ExpandEXITM(void)
{
  WasMACRO = True;

  if (!ChkArgCnt(0, 0));
  else if (!FirstInputTag) WrError(ErrNum_EXITMOutsideMacro);
  else if (!FirstInputTag->IsMacro) WrError(ErrNum_EXITMOutsideMacro);
  else if (IfAsm)
  {
    FirstInputTag->Cleanup(FirstInputTag);
    RestoreIFs(FirstInputTag->IfLevel);
    FirstInputTag->IsEmpty = True;
  }
}

/*-------------------------------------------------------------------------*/
/* discard first argument */

static void ExpandSHIFT(void)
{
  PInputTag RunTag;

  WasMACRO = True;

  if (!ChkArgCnt(0, 0));
  else if (!FirstInputTag) WrError(ErrNum_EXITMOutsideMacro);
  else if (!FirstInputTag->IsMacro) WrError(ErrNum_EXITMOutsideMacro);
  else if (IfAsm)
  {
    for (RunTag = FirstInputTag; RunTag; RunTag = RunTag->Next)
      if (RunTag->Processor == MACRO_Processor)
        break;

    if (RunTag && RunTag->Params)
    {
      char *p_arg = MoveAndCutStringListFirst(&RunTag->Params);
      if (p_arg)
        free(p_arg);
      RunTag->ParCnt--;
      ComputeMacroStrings(RunTag);
    }
  }
}

/*-------------------------------------------------------------------------*/
/*--- IRP (was das bei MASM auch immer heissen mag...)
      Ach ja: Individual Repeat! Danke Bernhard, jetzt hab'
      ich's gerafft! -----------------------*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Diese Routine liefert bei der Expansion eines IRP-Statements die expan-
  dierten Zeilen */

Boolean IRP_Processor(PInputTag PInp, as_dynstr_t *p_dest)
{
  StringRecPtr Lauf;
  int z;
  Boolean Result;

  Result = True;

  /* increment line counter only if contents came from a true file */

  CurrLine = PInp->StartLine;
  if (PInp->FromFile)
    CurrLine += PInp->LineZ;

  /* first line? Then open new symbol space and reset line pointer */

  if (PInp->LineZ == 1)
  {
    if (!PInp->GlobalSymbols)
    {
      if (!PInp->First) PopLocHandle();
      PushLocHandle(GetLocHandle());
    }
    PInp->First = False;
    PInp->LineRun = PInp->Lines;
  }

  /* extract line */

  as_dynstr_copy_c_str(p_dest, PInp->LineRun->Content);
  PInp->LineRun = PInp->LineRun->Next;

  /* expand iteration parameter */

  Lauf = PInp->Params; for (z = 1; z <= PInp->ParZ - 1; z++)
    Lauf = Lauf->Next;
  ExpandLine(Lauf->Content, 1, p_dest);

  /* end of body? then reset to line 1 and exit if this was the last iteration */

  if (++(PInp->LineZ) > PInp->LineCnt)
  {
    PInp->LineZ = 1;
    if (++(PInp->ParZ) > PInp->ParCnt)
      Result = False;
  }

  return Result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Aufraeumroutine IRP/IRPC */

static void IRP_Cleanup(PInputTag PInp)
{
  StringRecPtr Lauf;

  /* letzten Parameter sichern, wird evtl. noch fuer GetPos gebraucht!
     ... SaveAttr ist aber frei */
  if (PInp->Processor == IRP_Processor)
  {
    for (Lauf = PInp->Params; Lauf->Next; Lauf = Lauf->Next);
    strmaxcpy(PInp->SaveAttr, Lauf->Content, STRINGSIZE);
  }

  ClearStringList(&(PInp->Lines));
  ClearStringList(&(PInp->Params));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Posisionsangabe im IRP(C) fuer Fehlermeldungen */

static Boolean IRP_GetPos(PInputTag PInp, char *dest, size_t DestSize, Boolean ActGNUErrors)
{
  int z, ParZ = PInp->ParZ, LineZ = PInp->LineZ;
  const char *IRPType;
  char *IRPVal, tmp[20];

  UNUSED(ActGNUErrors);

  /* LineZ/ParZ already hopped to next line - step one back: */

  if (--LineZ <= 0)
  {
    LineZ = PInp->LineCnt;
    ParZ--;
  }

  if (PInp->Processor == IRP_Processor)
  {
    IRPType = "IRP";
    if (*PInp->SaveAttr != '\0')
      IRPVal = PInp->SaveAttr;
    else
    {
      StringRecPtr Lauf = PInp->Params;

      for (z = 1; z <= ParZ - 1; z++)
        Lauf = Lauf->Next;
      IRPVal = Lauf->Content;
    }
  }
  else
  {
    IRPType = "IRPC";
    as_snprintf(tmp, sizeof(tmp), "'%c'", PInp->SpecName.str.p_str[ParZ - 1]);
    IRPVal = tmp;
  }

  as_snprintf(dest, DestSize, "%s:%s(%lu) ", IRPType, IRPVal, (unsigned long)LineZ);

  return False;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Diese Routine sammelt waehrend der Definition eines IRP(C)-Statements die
  Quellzeilen ein */

static void IRP_OutProcessor(void)
{
  POutputTag Tmp;

  WasMACRO = True;

  /* Schachtelungen mitzaehlen */

  if (MacroStart())
    FirstOutputTag->NestLevel++;
  else if (MacroEnd())
    FirstOutputTag->NestLevel--;

  /* falls noch nicht zuende, weiterzaehlen */

  if (FirstOutputTag->NestLevel > -1)
  {
    as_dynstr_t s;
    StringRecPtr Dummy;
    const char *p_first_param = GetStringListFirst(FirstOutputTag->ParamNames, &Dummy);

    as_dynstr_ini_clone(&s, &OneLine); KillCtrl(s.p_str);
    CompressLine(p_first_param ? p_first_param : "", 1, &s, CaseSensitive);
    AddStringListLast(&(FirstOutputTag->Tag->Lines), s.p_str);
    as_dynstr_free(&s);
    FirstOutputTag->Tag->LineCnt++;
  }

  /* alles zusammen? Dann umhaengen */

  if (FirstOutputTag->NestLevel == -1)
  {
    Tmp = FirstOutputTag;
    FirstOutputTag = FirstOutputTag->Next;
    Tmp->Tag->IsEmpty = !Tmp->Tag->Lines;
    if (IfAsm)
    {
      NextDoLst = ApplyLstMacroExpMod(DoLst, &LstMacroExpModDefault);
      NextDoLst = ApplyLstMacroExpMod(NextDoLst, &LstMacroExpModOverride);
      Tmp->Tag->Next = FirstInputTag;
      FirstInputTag = Tmp->Tag;
    }
    else
    {
      ClearStringList(&(Tmp->Tag->Lines));
      ClearStringList(&(Tmp->Tag->Params));
      free(Tmp->Tag);
    }
    ClearStringList(&(Tmp->ParamNames));
    free(Tmp);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Initialisierung der IRP-Bearbeitung */

typedef struct
{
  Boolean ErrFlag;
  Boolean GlobalSymbols;
  int ArgCnt;
  POutputTag pOutputTag;
  StringList Params;
} tExpandIRPContext;

static void ProcessIRPArgs(Boolean CtrlArg, const tStrComp *pArg, void *pUser)
{
  tExpandIRPContext *pContext = (tExpandIRPContext*)pUser;

  if (CtrlArg)
  {
    if (ReadMacro_SearchArg(pArg->str.p_str, "GLOBALSYMBOLS", &pContext->GlobalSymbols));
    else
    {
      WrStrErrorPos(ErrNum_UnknownMacArg, pArg);
      pContext->ErrFlag = True;
    }
  }
  else
  {
    /* differentiate placeholder & arguments */

    if (0 == pContext->ArgCnt)
    {
      if (!ChkMacSymbName(pArg->str.p_str))
      {
        WrStrErrorPos(ErrNum_InvSymName, pArg);
        pContext->ErrFlag = True;
      }
      else
        AddStringListFirst(&(pContext->pOutputTag->ParamNames), pArg->str.p_str);
    }
    else
    {
      if (!CaseSensitive)
        UpString(pArg->str.p_str);
      AddStringListLast(&(pContext->Params), pArg->str.p_str);
    }
    pContext->ArgCnt++;
  }
}

static Boolean ExpandIRP(void)
{
  PInputTag Tag;
  tExpandIRPContext Context;

  WasMACRO = True;

  /* 0. terminate if conditional assembly bites */

  if (!IfAsm)
  {
    AddWaitENDM_Processor();
    return True;
  }

  /* 1. Parameter pruefen */

  Context.ErrFlag = False;
  Context.GlobalSymbols = False;
  Context.ArgCnt = 0;
  Context.Params = NULL;

  Context.pOutputTag = GenerateOUTProcessor(IRP_OutProcessor, ErrNum_OpenIRP);
  Context.pOutputTag->Next      = FirstOutputTag;
  ProcessMacroArgs(ProcessIRPArgs, &Context);

  /* at least parameter & one arg */

  if (!ChkArgCntExt(Context.ArgCnt, 2, ArgCntMax))
    Context.ErrFlag = True;
  if (Context.ErrFlag)
  {
    ClearStringList(&(Context.pOutputTag->ParamNames));
    ClearStringList(&(Context.pOutputTag->ParamDefVals));
    ClearStringList(&(Context.Params));
    free(Context.pOutputTag);
    AddWaitENDM_Processor();
    return False;
  }

  /* 2. Tag erzeugen */

  Tag = GenerateProcessor();
  Tag->ParCnt    = Context.ArgCnt - 1;
  Tag->Params    = Context.Params;
  Tag->Processor = IRP_Processor;
  Tag->Restorer  = MACRO_Restorer;
  Tag->Cleanup   = IRP_Cleanup;
  Tag->GetPos    = IRP_GetPos;
  Tag->GlobalSymbols = Context.GlobalSymbols;
  Tag->ParZ      = 1;
  Tag->IsMacro   = True;
  *Tag->SaveAttr = '\0';
  Context.pOutputTag->Tag = Tag;

  /* 4. einbetten */

  FirstOutputTag = Context.pOutputTag;

  return True;
}

/*--- IRPC: dito fuer Zeichen eines Strings ---------------------------------*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Diese Routine liefert bei der Expansion eines IRPC-Statements die expan-
  dierten Zeilen */

Boolean IRPC_Processor(PInputTag PInp, as_dynstr_t *p_dest)
{
  Boolean Result;
  char tmp[5];

  Result = True;

  /* increment line counter only if contents came from a true file */

  CurrLine = PInp->StartLine;
  if (PInp->FromFile)
    CurrLine += PInp->LineZ;

  /* first line? Then open new symbol space and reset line pointer */

  if (PInp->LineZ == 1)
  {
    if (!PInp->GlobalSymbols)
    {
      if (!PInp->First) PopLocHandle();
      PushLocHandle(GetLocHandle());
    }
    PInp->First = False;
    PInp->LineRun = PInp->Lines;
  }

  /* extract line */

  as_dynstr_copy_c_str(p_dest, PInp->LineRun->Content);
  PInp->LineRun = PInp->LineRun->Next;

  /* extract iteration parameter */

  *tmp = PInp->SpecName.str.p_str[PInp->ParZ - 1];
  tmp[1] = '\0';
  ExpandLine(tmp, 1, p_dest);

  /* end of body? then reset to line 1 and exit if this was the last iteration */

  if (++(PInp->LineZ) > PInp->LineCnt)
  {
    PInp->LineZ = 1;
    if (++(PInp->ParZ) > PInp->ParCnt)
      Result = False;
  }

  return Result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Initialisierung der IRPC-Bearbeitung */

typedef struct
{
  Boolean ErrFlag;
  Boolean GlobalSymbols;
  int ArgCnt;
  POutputTag pOutputTag;
  String ParameterStr;
  tStrComp Parameter;
} tExpandIRPCContext;

static void ProcessIRPCArgs(Boolean CtrlArg, const tStrComp *pArg, void *pUser)
{
  tExpandIRPCContext *pContext = (tExpandIRPCContext*)pUser;

  if (CtrlArg)
  {
    if (ReadMacro_SearchArg(pArg->str.p_str, "GLOBALSYMBOLS", &pContext->GlobalSymbols));
    else
    {
      WrStrErrorPos(ErrNum_UnknownMacArg, pArg);
      pContext->ErrFlag = True;
    }
  }
  else
  {
    if (0 == pContext->ArgCnt)
    {
      if (!ChkMacSymbName(pArg->str.p_str))
      {
        WrStrErrorPos(ErrNum_InvSymName, pArg);
        pContext->ErrFlag = True;
      }
      else
        AddStringListFirst(&(pContext->pOutputTag->ParamNames), pArg->str.p_str);
    }
    else
    {
      Boolean OK;

      EvalStrStringExpression(pArg, &OK, pContext->Parameter.str.p_str);
      pContext->Parameter.Pos = pArg->Pos;
      if (!OK)
        pContext->ErrFlag = True;
    }
    pContext->ArgCnt++;
  }
}

static Boolean ExpandIRPC(void)
{
  PInputTag Tag;
  tExpandIRPCContext Context;

  WasMACRO = True;

  /* 0. terminate if conditinal assembly bites */

  if (!IfAsm)
  {
    AddWaitENDM_Processor();
    return True;
  }

  /* 1.Parameter pruefen */

  Context.ErrFlag = False;
  Context.GlobalSymbols = False;
  Context.ArgCnt = 0;
  StrCompMkTemp(&Context.Parameter, Context.ParameterStr, sizeof(Context.ParameterStr));
  StrCompReset(&Context.Parameter);

  Context.pOutputTag = GenerateOUTProcessor(IRP_OutProcessor, ErrNum_OpenIRPC);
  Context.pOutputTag->Next = FirstOutputTag;
  ProcessMacroArgs(ProcessIRPCArgs, &Context);

  /* parameter & string */

  if (!ChkArgCntExt(Context.ArgCnt, 2, ArgCntMax))
    Context.ErrFlag = True;
  if (Context.ErrFlag)
  {
    ClearStringList(&(Context.pOutputTag->ParamNames));
    AddWaitENDM_Processor();
    return False;
  }

  /* 2. Tag erzeugen */

  Tag = GenerateProcessor();
  Tag->ParCnt    = strlen(Context.Parameter.str.p_str);
  Tag->Processor = IRPC_Processor;
  Tag->Restorer  = MACRO_Restorer;
  Tag->Cleanup   = IRP_Cleanup;
  Tag->GetPos    = IRP_GetPos;
  Tag->GlobalSymbols = Context.GlobalSymbols;
  Tag->ParZ      = 1;
  Tag->IsMacro   = True;
  *Tag->SaveAttr = '\0';
  StrCompCopy(&Tag->SpecName, &Context.Parameter);

  /* 4. einbetten */

  Context.pOutputTag->Tag = Tag;
  FirstOutputTag = Context.pOutputTag;

  return True;
}

/*--- Repetition -----------------------------------------------------------*/

static void REPT_Cleanup(PInputTag PInp)
{
  ClearStringList(&(PInp->Lines));
}

static Boolean REPT_GetPos(PInputTag PInp, char *dest, size_t DestSize, Boolean ActGNUErrors)
{
  int z1 = PInp->ParZ, z2 = PInp->LineZ;

  UNUSED(ActGNUErrors);

  if (--z2 <= 0)
  {
    z2 = PInp->LineCnt;
    z1--;
  }
  as_snprintf(dest, DestSize, "REPT %lu(%lu)", (unsigned long)z1, (unsigned long)z2);
  return False;
}

Boolean REPT_Processor(PInputTag PInp, as_dynstr_t *p_dest)
{
  Boolean Result;

  Result = True;

  /* increment line counter only if contents came from a true file */

  CurrLine = PInp->StartLine;
  if (PInp->FromFile)
    CurrLine += PInp->LineZ;

  /* first line? Then open new symbol space and reset line pointer */

  if (PInp->LineZ == 1)
  {
    if (!PInp->GlobalSymbols)
    {
      if (!PInp->First) PopLocHandle();
      PushLocHandle(GetLocHandle());
    }
    PInp->First = False;
    PInp->LineRun = PInp->Lines;
  }

  /* extract line */

  as_dynstr_copy_c_str(p_dest, PInp->LineRun->Content);
  PInp->LineRun = PInp->LineRun->Next;

  /* last line of body? Then increment count and stop if last iteration */

  if ((++PInp->LineZ) > PInp->LineCnt)
  {
    PInp->LineZ = 1;
    if ((++PInp->ParZ) > PInp->ParCnt)
      Result = False;
  }

  return Result;
}

static void REPT_OutProcessor(void)
{
  POutputTag Tmp;

  WasMACRO = True;

  /* Schachtelungen mitzaehlen */

  if (MacroStart())
    FirstOutputTag->NestLevel++;
  else if (MacroEnd())
    FirstOutputTag->NestLevel--;

  /* falls noch nicht zuende, weiterzaehlen */

  if (FirstOutputTag->NestLevel > -1)
  {
    AddStringListLast(&(FirstOutputTag->Tag->Lines), OneLine.p_str);
    FirstOutputTag->Tag->LineCnt++;
  }

  /* alles zusammen? Dann umhaengen */

  if (FirstOutputTag->NestLevel == -1)
  {
    Tmp = FirstOutputTag;
    FirstOutputTag = FirstOutputTag->Next;
    Tmp->Tag->IsEmpty = !Tmp->Tag->Lines;
    if ((IfAsm) && (Tmp->Tag->ParCnt > 0))
    {
      NextDoLst = ApplyLstMacroExpMod(DoLst, &LstMacroExpModDefault);
      NextDoLst = ApplyLstMacroExpMod(NextDoLst, &LstMacroExpModOverride);
      Tmp->Tag->Next = FirstInputTag;
      FirstInputTag = Tmp->Tag;
    }
    else
    {
      ClearStringList(&(Tmp->Tag->Lines));
      free(Tmp->Tag);
    }
    free(Tmp);
  }
}

typedef struct
{
  Boolean ErrFlag;
  Boolean GlobalSymbols;
  int ArgCnt;
  LongInt ReptCount;
} tExpandREPTContext;

static void ProcessREPTArgs(Boolean CtrlArg, const tStrComp *pArg, void *pUser)
{
  tExpandREPTContext *pContext = (tExpandREPTContext*)pUser;

  if (CtrlArg)
  {
    if (ReadMacro_SearchArg(pArg->str.p_str, "GLOBALSYMBOLS", &pContext->GlobalSymbols));
    else
    {
      WrStrErrorPos(ErrNum_UnknownMacArg, pArg);
      pContext->ErrFlag = True;
    }
  }
  else
  {
    Boolean ValOK;
    tSymbolFlags SymbolFlags;

    pContext->ReptCount = EvalStrIntExpressionWithFlags(pArg, Int32, &ValOK, &SymbolFlags);
    if (mFirstPassUnknown(SymbolFlags))
      WrStrErrorPos(ErrNum_FirstPassCalc, pArg);
    if (!ValOK || mFirstPassUnknown(SymbolFlags))
      pContext->ErrFlag = True;
    pContext->ArgCnt++;
  }
}

static Boolean ExpandREPT(void)
{
  PInputTag Tag;
  POutputTag Neu;
  tExpandREPTContext Context;

  WasMACRO = True;

  /* 0. skip everything when conditional assembly is off */

  if (!IfAsm)
  {
    AddWaitENDM_Processor();
    return True;
  }

  /* 1. Repetitionszahl ermitteln */

  Context.GlobalSymbols = False;
  Context.ReptCount = 0;
  Context.ErrFlag = False;
  Context.ArgCnt = 0;
  ProcessMacroArgs(ProcessREPTArgs, &Context);

  /* rept count must be present only once */

  if (!ChkArgCntExt(Context.ArgCnt, 1, 1))
    Context.ErrFlag = True;
  if (Context.ErrFlag)
  {
    AddWaitENDM_Processor();
    return False;
  }

  /* 2. Tag erzeugen */

  Tag = GenerateProcessor();
  Tag->ParCnt    = Context.ReptCount;
  Tag->Processor = REPT_Processor;
  Tag->Restorer  = MACRO_Restorer;
  Tag->Cleanup   = REPT_Cleanup;
  Tag->GetPos    = REPT_GetPos;
  Tag->GlobalSymbols = Context.GlobalSymbols;
  Tag->IsMacro   = True;
  Tag->ParZ      = 1;

  /* 3. einbetten */

  Neu = GenerateOUTProcessor(REPT_OutProcessor, ErrNum_OpenREPT);
  Neu->Next      = FirstOutputTag;
  Neu->Tag       = Tag;
  FirstOutputTag = Neu;

  return True;
}

/*- bedingte Wiederholung -------------------------------------------------------*/

static void WHILE_Cleanup(PInputTag PInp)
{
  ClearStringList(&(PInp->Lines));
}

static Boolean WHILE_GetPos(PInputTag PInp, char *dest, size_t DestSize, Boolean ActGNUErrors)
{
  int z1 = PInp->ParZ, z2 = PInp->LineZ;

  UNUSED(ActGNUErrors);

  if (--z2 <= 0)
  {
    z2 = PInp->LineCnt;
    z1--;
  }
  as_snprintf(dest, DestSize, "WHILE %lu/%lu", (unsigned long)z1, (unsigned long)z2);
  return False;
}

Boolean WHILE_Processor(PInputTag PInp, as_dynstr_t *p_dest)
{
  int z;
  Boolean OK, Result;

  /* increment line counter only if this came from a true file */

  CurrLine = PInp->StartLine;
  if (PInp->FromFile)
    CurrLine += PInp->LineZ;

  /* if this is the first line of the loop body, open a new handle
     for macro-local symbols and drop the old one if this was not the
     first pass through the body. */

  if (PInp->LineZ == 1)
  {
    if (!PInp->GlobalSymbols)
    {
      if (!PInp->First)
        PopLocHandle();
      PushLocHandle(GetLocHandle());
    }
    PInp->First = False;
    PInp->LineRun = PInp->Lines;
  }

  /* evaluate condition before first line */

  if (PInp->LineZ == 1)
  {
    z = EvalStrIntExpression(&PInp->SpecName, Int32, &OK);
    Result = (OK && (z != 0));
  }
  else
    Result = True;

  if (Result)
  {
    /* get line of body */

    as_dynstr_copy_c_str(p_dest, PInp->LineRun->Content);
    PInp->LineRun = PInp->LineRun->Next;

    /* in case this is the last line of the body, reset counters */

    if ((++PInp->LineZ) > PInp->LineCnt)
    {
      PInp->LineZ = 1;
      PInp->ParZ++;
    }
  }

  /* nasty last line... */

  else
    *p_dest->p_str = '\0';

  return Result;
}

static void WHILE_OutProcessor(void)
{
  POutputTag Tmp;
  Boolean OK;
  tSymbolFlags SymbolFlags;
  LongInt Erg;

  WasMACRO = True;

  /* Schachtelungen mitzaehlen */

  if (MacroStart())
    FirstOutputTag->NestLevel++;
  else if (MacroEnd())
    FirstOutputTag->NestLevel--;

  /* falls noch nicht zuende, weiterzaehlen */

  if (FirstOutputTag->NestLevel > -1)
  {
    AddStringListLast(&(FirstOutputTag->Tag->Lines), OneLine.p_str);
    FirstOutputTag->Tag->LineCnt++;
  }

  /* alles zusammen? Dann umhaengen */

  if (FirstOutputTag->NestLevel == -1)
  {
    Tmp = FirstOutputTag;
    FirstOutputTag = FirstOutputTag->Next;
    Tmp->Tag->IsEmpty = !Tmp->Tag->Lines;
    Erg = EvalStrIntExpressionWithFlags(&Tmp->Tag->SpecName, Int32, &OK, &SymbolFlags);
    if (mFirstPassUnknown(SymbolFlags))
    {
      WrError(ErrNum_FirstPassCalc);
      OK = False;
    }
    OK = (OK && (Erg != 0));
    if (IfAsm && OK)
    {
      NextDoLst = ApplyLstMacroExpMod(DoLst, &LstMacroExpModDefault);
      NextDoLst = ApplyLstMacroExpMod(NextDoLst, &LstMacroExpModOverride);
      Tmp->Tag->Next = FirstInputTag;
      FirstInputTag = Tmp->Tag;
    }
    else
    {
      ClearStringList(&(Tmp->Tag->Lines));
      free(Tmp->Tag);
    }
    free(Tmp);
  }
}

typedef struct
{
  Boolean ErrFlag;
  Boolean GlobalSymbols;
  int ArgCnt;
  String SpecNameStr;
  tStrComp SpecName;
} tExpandWHILEContext;

static void ProcessWHILEArgs(Boolean CtrlArg, const tStrComp *pArg, void *pUser)
{
  tExpandWHILEContext *pContext = (tExpandWHILEContext*)pUser;

  if (CtrlArg)
  {
    if (ReadMacro_SearchArg(pArg->str.p_str, "GLOBALSYMBOLS", &pContext->GlobalSymbols));
    else
    {
      WrStrErrorPos(ErrNum_UnknownMacArg, pArg);
      pContext->ErrFlag = True;
    }
  }
  else
  {
    StrCompCopy(&pContext->SpecName, pArg);
    pContext->ArgCnt++;
  }
}

static Boolean ExpandWHILE(void)
{
  PInputTag Tag;
  POutputTag Neu;
  tExpandWHILEContext Context;

  WasMACRO = True;

  /* 0. turned off ? */

  if (!IfAsm)
  {
    AddWaitENDM_Processor();
    return True;
  }

  /* 1. Bedingung ermitteln */

  Context.GlobalSymbols = False;
  Context.ErrFlag = False;
  Context.ArgCnt = 0;
  StrCompMkTemp(&Context.SpecName, Context.SpecNameStr, sizeof(Context.SpecNameStr));
  StrCompReset(&Context.SpecName);
  ProcessMacroArgs(ProcessWHILEArgs, &Context);

  /* condition must be present only once */

  if (!ChkArgCntExt(Context.ArgCnt, 1, 1))
    Context.ErrFlag = True;
  if (Context.ErrFlag)
  {
    AddWaitENDM_Processor();
    return False;
  }

  /* 2. Tag erzeugen */

  Tag = GenerateProcessor();
  Tag->Processor = WHILE_Processor;
  Tag->Restorer  = MACRO_Restorer;
  Tag->Cleanup   = WHILE_Cleanup;
  Tag->GetPos    = WHILE_GetPos;
  Tag->GlobalSymbols = Context.GlobalSymbols;
  Tag->IsMacro   = True;
  Tag->ParZ      = 1;
  StrCompCopy(&Tag->SpecName, &Context.SpecName);

  /* 3. einbetten */

  Neu = GenerateOUTProcessor(WHILE_OutProcessor, ErrNum_OpenWHILE);
  Neu->Next      = FirstOutputTag;
  Neu->Tag       = Tag;
  FirstOutputTag = Neu;

  return True;
}

/*--------------------------------------------------------------------------*/
/* Einziehen von Include-Files */

static void INCLUDE_Cleanup(PInputTag PInp)
{
  fclose(PInp->Datei);
  free(PInp->Buffer);

  /* if last line in file was continued, do not forget to add up
     continuation lines: */

  LineSum += PInp->LineZ + PInp->ContLineCnt;
  if ((*LstName != '\0') && (msg_level >= e_msg_level_normal))
  {
    String Tmp;

    as_snprintf(Tmp, sizeof(Tmp), "%s(%lu)", NamePart(CurrFileName), (unsigned long)CurrLine);
    WrConsoleLine(Tmp, True);
    fflush(stdout);
  }
  if (MakeIncludeList)
    PopInclude();
  CurrIncludeLevel = PInp->IncludeLevel;
}

static Boolean INCLUDE_GetPos(PInputTag PInp, char *dest, size_t DestSize, Boolean ActGNUErrors)
{
  UNUSED(PInp);

  as_snprintf(dest, DestSize, ActGNUErrors ? "%s:%lu" : "%s(%lu) ", NamePart(PInp->SpecName.str.p_str), (unsigned long)PInp->LineZ);
  return !GNUErrors;
}

Boolean INCLUDE_Processor(PInputTag PInp, as_dynstr_t *p_dest)
{
  Boolean Result;
  int Count = 1;

  /* add up # of continuation lines from previous source line */

  if (PInp->ContLineCnt)
  {
    CurrLine = (PInp->LineZ += PInp->ContLineCnt);
    PInp->ContLineCnt = 0;
  }

  Result = True;

  if (feof(PInp->Datei))
    *p_dest->p_str = '\0';
  else
  {
    Count = ReadLnCont(PInp->Datei, p_dest);
    /**ChkIO(ErrNum_FileReadError);**/
  }

  /* Even if we had continuation lines, only increment line counter
     by one at this place so the first line's # is the number of the
     concatenated line: */

  if (Count > 0)
  {
    PInp->LineZ++;
    CurrLine = PInp->LineZ;
    PInp->ContLineCnt = Count - 1;
  }
  if (feof(PInp->Datei))
    Result = False;

  return Result;
}

static void INCLUDE_Restorer(PInputTag PInp)
{
  CurrLine = PInp->StartLine;
  strmaxcpy(CurrFileName, PInp->SaveAttr, STRINGSIZE);
  IncDepth--;
}

/*!------------------------------------------------------------------------
 * \fn     ExpandINCLUDE_Core(const tStrComp *pArg, Boolean SearchPath)
 * \brief  The actual core code to open a source file for assembly
 * \param  pArg file's name to open
 * \param  SearchPath searhc file in include path?
 * ------------------------------------------------------------------------ */

static void ExpandINCLUDE_Core(const tStrComp *pArg, Boolean SearchPath)
{
  tStrComp FNameArg;
  String FNameArgStr;
  PInputTag Tag;

  StrCompMkTemp(&FNameArg, FNameArgStr, sizeof(FNameArgStr));
  INCLUDE_SearchCore(&FNameArg, pArg, SearchPath);

  /* Tag erzeugen */

  Tag = GenerateProcessor();
  Tag->Processor = INCLUDE_Processor;
  Tag->Restorer  = INCLUDE_Restorer;
  Tag->Cleanup   = INCLUDE_Cleanup;
  Tag->GetPos    = INCLUDE_GetPos;
  Tag->Buffer    = (void *) malloc(BufferArraySize);

  /* Sicherung alter Daten */

  Tag->StartLine = CurrLine;
  strmaxcpy(Tag->SpecName.str.p_str, FNameArg.str.p_str, STRINGSIZE);
  LineCompReset(&Tag->SpecName.Pos);
  strmaxcpy(Tag->SaveAttr, CurrFileName, STRINGSIZE);

  /* Datei oeffnen */

#ifdef __CYGWIN32__
  DeCygwinPath(FNameArg.str.p_str);
#endif
  Tag->Datei = fopen(FNameArg.str.p_str, "r");
  if (!Tag->Datei) ChkStrIO(ErrNum_OpeningFile, pArg);
  setvbuf(Tag->Datei, (char*)Tag->Buffer, _IOFBF, BufferArraySize);

  /* neu besetzen */

  strmaxcpy(CurrFileName, FNameArg.str.p_str, STRINGSIZE);
  Tag->LineZ = 0;
  AddFile(FNameArg.str.p_str);
  PushInclude(FNameArg.str.p_str);
  if (++CurrIncludeLevel > MaxIncludeLevel)
    WrStrErrorPos(ErrNum_MaxIncLevelExceeded, pArg);

  /* einhaengen */

  Tag->Next = FirstInputTag; FirstInputTag = Tag;
}

/*!------------------------------------------------------------------------
 * \fn     ExpandINCLUDE(void)
 * \brief  Handle INCLUDE statement
 * ------------------------------------------------------------------------ */

static void ExpandINCLUDE(void)
{
  if (!IfAsm)
    return;

  if (!ChkArgCnt(1, 1))
    return;

  ExpandINCLUDE_Core(&ArgStr[1], True);
  NextIncDepth++;
}

/*=========================================================================*/
/* Einlieferung von Zeilen */

static void GetNextLine(as_dynstr_t *pLine)
{
  PInputTag HTag;

  InMacroFlag = False;

  while (FirstInputTag && FirstInputTag->IsEmpty)
  {
    FirstInputTag->Cleanup(FirstInputTag);
    FirstInputTag->Restorer(FirstInputTag);
    HTag = FirstInputTag;
    FirstInputTag = HTag->Next;
    free(HTag);
  }

  if (!FirstInputTag)
  {
    *pLine->p_str = '\0';
    return;
  }

  if (!FirstInputTag->Processor(FirstInputTag, pLine))
  {
    FirstInputTag->IsEmpty = True;
  }

  MacLineSum++;
}

typedef struct
{
  char *pStr;
  size_t AllocLen;
} tAllocStr;

static void InitStr(tAllocStr *pStr)
{
  pStr->pStr = NULL;
  pStr->AllocLen = 0;
}

static void ReallocStr(tAllocStr *pStr, unsigned NewAllocLen)
{
  if (NewAllocLen > pStr->AllocLen)
  {
    char *pNewStr;

    /* round up, and implicitly avoid allocating 4/8 bytes (sizeof pointer)
       so size check in as_vsnprcatf() does not generate false positive: */

    NewAllocLen = (NewAllocLen + 15) &~15;
    pNewStr = pStr->AllocLen
            ? (char*)realloc(pStr->pStr, NewAllocLen)
            : (char*)malloc(NewAllocLen);

    if (pNewStr)
    {
      pStr->pStr = pNewStr;
      pStr->pStr[pStr->AllocLen] = '\0';
      pStr->AllocLen = NewAllocLen;
    }
  }
}

char *GetErrorPos(void)
{
  String ActPos;
  PInputTag RunTag;
  tAllocStr Str;
  int CurrStrLen, NewLen;
  Boolean Last;

  InitStr(&Str);
  CurrStrLen = 0;

  /* no file has yet been opened: */

  if (!FirstInputTag)
  {
    const char FixedName[] = "INTERNAL";

    ReallocStr(&Str, strlen(FixedName) + 1);
    strmaxcat(Str.pStr, FixedName, Str.AllocLen);
  }

  /* for GNU error message style: */

  else if (GNUErrors)
  {
    PInputTag pInnerTag = NULL;
    const char *pMsg;

    /* we only honor the include positions.  First, print the upper include layers... */

    for (RunTag = FirstInputTag; RunTag; RunTag = RunTag->Next)
      if (RunTag->GetPos == INCLUDE_GetPos)
      {
        if (!pInnerTag)
          pInnerTag = RunTag;
        else
        {
          Last = RunTag->GetPos(RunTag, ActPos, sizeof(ActPos), GNUErrors);
          if (!Str.AllocLen)
          {
            pMsg = getmessage(Num_GNUErrorMsg1);
            NewLen = strlen(pMsg) + 1 + strlen(ActPos) + 1;
            ReallocStr(&Str, NewLen);
            as_snprintf(Str.pStr, Str.AllocLen, "%s %s", pMsg, ActPos);
            CurrStrLen = NewLen;
          }
          else
          {
            pMsg = getmessage(Num_GNUErrorMsgN);
            NewLen = CurrStrLen + 2 + strlen(pMsg) + 1 + strlen(ActPos) + 1;
            ReallocStr(&Str, NewLen);
            as_snprcatf(Str.pStr, Str.AllocLen, ",\n%s %s", pMsg, ActPos);
            CurrStrLen = NewLen;
          }
        }
      }

    /* ...append something... */

    if (CurrStrLen > 0)
    {
      NewLen = CurrStrLen + 3;

      ReallocStr(&Str, NewLen);
      as_snprcatf(Str.pStr, Str.AllocLen, ":\n");
      CurrStrLen = NewLen;
    }

    /* ...then the innermost one */

    if (pInnerTag)
    {
      pInnerTag->GetPos(pInnerTag, ActPos, sizeof(ActPos), GNUErrors);
      NewLen = CurrStrLen + strlen(ActPos) + 1;
      ReallocStr(&Str, NewLen);
      as_snprcatf(Str.pStr, Str.AllocLen, "%s", ActPos);
      CurrStrLen = NewLen;
    }
  }

  /* otherwise the standard AS position generator: */

  else
  {
    int ThisLen;

    for (RunTag = FirstInputTag; RunTag; RunTag = RunTag->Next)
    {
      Last = RunTag->GetPos(RunTag, ActPos, sizeof(ActPos), GNUErrors);
      ThisLen = strlen(ActPos);
      ReallocStr(&Str, NewLen = CurrStrLen + ThisLen + 1);
      strmaxprep(Str.pStr, ActPos, Str.AllocLen);
      CurrStrLen = NewLen;
      if (Last)
        break;
    }
  }

  return Str.pStr;
}

static Boolean InputEnd(void)
{
  PInputTag Lauf;

  Lauf = FirstInputTag;
  while (Lauf)
  {
    if (!Lauf->IsEmpty)
      return False;
    Lauf = Lauf->Next;
  }

  return True;
}

/*=== Eine Quelldatei ( Haupt-oder Includedatei ) bearbeiten ===============*/

/*--- aus der zerlegten Zeile Code erzeugen --------------------------------*/

void WriteCode(void)
{
  unsigned z;

  for (z = 0; z < StopfZahl; z++)
  {
    switch (ActListGran)
    {
      case 4:
        DAsmCode[CodeLen >> 2] = NOPCode;
        break;
      case 2:
        WAsmCode[CodeLen >> 1] = NOPCode;
        break;
      case 1:
        BAsmCode[CodeLen] = NOPCode;
        break;
    }
    CodeLen += ActListGran/Granularity();
  }

  if ((ActPC != StructSeg) && (!ChkPC(PCs[ActPC] + CodeLen - 1)) && (CodeLen != 0))
    WrError(ErrNum_AdrOverflow);
  else
  {
    LargeWord NewPC = PCs[ActPC] + CodeLen;

    if ((!DontPrint) && (ActPC != StructSeg) && (CodeLen > 0))
      BookKeeping();
    if (ActPC == StructSeg)
    {
      if ((CodeLen != 0) && (!DontPrint)) WrError(ErrNum_NotInStruct);
      if (StructStack->StructRec->IsUnion)
      {
        BumpStructLength(StructStack->StructRec, CodeLen);
        CodeLen = 0;
        NewPC = 0;
      }
    }
    else if (CodeOutput)
    {
      PCsUsed[ActPC] = True;
      if (DontPrint)
        NewRecord(PCs[ActPC] + CodeLen);
      else
        WriteBytes();
    }
    PCs[ActPC] = NewPC;
  }
}

static void Produce_Code(void)
{
  PMacroRec OneMacro;
  PStructRec OneStruct;
  Boolean SearchMacros, Found, IsMacro = False, IsStruct = False, ResetLastLabel = True;
  tStrComp non_upper_case_op_part;
  String non_upper_case_op_part_buf;
  const tStrComp *p_search_op_part;

  ActListGran = ListGran();
  WasIF = WasMACRO = False;

  /* Makrosuche unterdruecken ? */

  /* We need the OpPart also in a variant not converted to all-uppercase,
     since structure and macro names may be case sensitive: */

  StrCompMkTemp(&non_upper_case_op_part, non_upper_case_op_part_buf, sizeof(non_upper_case_op_part_buf));
  if (*OpPart.str.p_str == '!')
  {
    SearchMacros = False;
    StrCompCutLeft(&OpPart, 1);
    StrCompCopy(&non_upper_case_op_part, &OpPart);
  }
  else
  {
    const tStrComp *p_lop_part;

    SearchMacros = True;
    p_lop_part = ExpandStrSymbol(&non_upper_case_op_part, &OpPart, False);
    if (p_lop_part && (p_lop_part != &OpPart))
      as_dynstr_copy(&OpPart.str, &p_lop_part->str);
  }
  NLS_UpString(OpPart.str.p_str);
  p_search_op_part = CaseSensitive ? &non_upper_case_op_part : &OpPart;

  /* Prozessor eingehaengt ? */

  if (FirstOutputTag)
  {
    FirstOutputTag->Processor();
    return;
  }

  /* otherwise generate code: check for macro/structs here */

  IsMacro = (SearchMacros) && (FoundMacro(&OneMacro, p_search_op_part));
  if (IsMacro)
    WasMACRO = True;
  if (!IsMacro)
    IsStruct = FoundStruct(&OneStruct, p_search_op_part->str.p_str);

  /* no longer at an address right after a BSR? */

  if (EProgCounter() != AfterBSRAddr)
    AfterBSRAddr = 0;

  /* evtl. voranstehendes Label ablegen */

  if (IfAsm && (!IsMacro || !OneMacro->LocIntLabel))
  {
    if (LabelPresent())
      LabelHandle(&LabPart, EProgCounter(), False);
  }

  Found = False;
  switch (*OpPart.str.p_str)
  {
    case 'I':
      /* Makroliste ? */
      Found = True;
      if (Memo("IRP")) ResetLastLabel = !ExpandIRP();
      else if (Memo("IRPC")) ResetLastLabel = !ExpandIRPC();
      else Found = False;
      break;
    case 'R':
      /* Repetition ? */
      Found = True;
      if (Memo("REPT")) ResetLastLabel = !ExpandREPT();
      else Found = False;
      break;
    case 'W':
      /* bedingte Repetition ? */
      Found = True;
      if (Memo("WHILE")) ResetLastLabel = !ExpandWHILE();
      else Found = False;
      break;
  }

  /* bedingte Assemblierung ? */

  if (!Found)
    WasIF = Found = CodeIFs();

  if (!Found)
    switch (*OpPart.str.p_str)
    {
      case 'M':
        /* Makrodefinition ? */
        Found = True;
        if (Memo("MACRO")) ReadMacro();
        else Found = False;
        break;
      case 'E':
        /* Abbruch Makroexpansion ? */
        Found = True;
        if (Memo("EXITM")) ExpandEXITM();
        else Found = False;
        break;
      case 'S':
        /* shift macro arguments ? */
        Found = True;
        if (memo_shift_pseudo() || (ShiftIsOccupied && Memo("SHFT"))) ExpandSHIFT();
        else Found = False;
        break;
      case 'I':
        /* Includefile? */
        Found = True;
        if (Memo("INCLUDE"))
          ExpandINCLUDE();
        else
          Found = False;
        break;
    }

  if (Found);

  /* Makroaufruf ? */

  else if (IsMacro)
  {
    ResetLastLabel = False;
    if (IfAsm)
    {
      ExpandMacro(OneMacro);
      if ((MacroNestLevel > 1) && (MacroNestLevel < 100))
        as_snprintf(ListLine, STRINGSIZE, "%*s(MACRO-%u)", MacroNestLevel - 1, "", MacroNestLevel);
      else
        strmaxcpy(ListLine, "(MACRO)", STRINGSIZE);

      /* Macro call itself must not appear in expanded output.  However, a label
         in the same line that is not consumed by the macro must.  In this case,
         dump the source line with the OpPart (macro's name) muted out. */

      if (MacProOutput && (LabPart.Pos.StartCol >= 0) && !OneMacro->LocIntLabel)
        PrintOneLineMuted(MacProFile, OneLine.p_str, &OpPart.Pos, &ArgPart.Pos);
    }
  }

  else
  {
    StopfZahl = 0;
    CodeLen = 0;
    DontPrint = False;

#ifdef PROFILE_MEMO
    NumMemo = 0;
#endif

    if (IfAsm)
    {
      /* structure declaration ? */

      if (IsStruct)
      {
        ExpandStruct(OneStruct, p_search_op_part->str.p_str);
        strmaxcpy(ListLine, OneStruct->IsUnion ? "(UNION)" : "(STRUCT)", STRINGSIZE);
      }
      else
      {
        AttrPartOpSize[0] = AttrPartOpSize[1] = eSymbolSizeUnknown;
        if (DecodeAttrPart ? DecodeAttrPart() : True)
        {
          if (!CodeGlobalPseudo())
          MakeCode();
        }
      }
      if (MacProOutput && ((*OpPart.str.p_str != '\0') || (*LabPart.str.p_str != '\0') || (*CommPart.str.p_str != '\0')))
      {
        errno = 0;
        fprintf(MacProFile, "%s\n", OneLine.p_str);
        ChkIO(ErrNum_ListWrError);
      }
    }

#ifdef PROFILE_MEMO
    NumMemoSum += NumMemo;
    NumMemoCnt++;
#endif

    WriteCode();
  }

  /* reset memory about previous label if it is a non-empty instruction */

  if (*OpPart.str.p_str && ResetLastLabel)
    LabelReset();

  /* dies ueberprueft implizit, ob von der letzten Eval...-Operation noch
     externe Referenzen liegengeblieben sind. */

  SetRelocs(NULL);
}

/*--- Zeile in Listing zerteilen -------------------------------------------*/

static void adjust_copy_comp(tStrComp *p_comp, const char *p_src, size_t newsz)
{
  if (newsz + 1 > p_comp->str.capacity)
    as_dynstr_realloc(&p_comp->str, as_dynstr_roundup_len(newsz));
  p_comp->Pos.Len = strmemcpy(p_comp->str.p_str, p_comp->str.capacity, p_src, newsz);
}

static void SplitLine(void)
{
  const char *pRun, *pEnd, *pPos;

  Retracted = False;

  /* run preprocessor */

  ExpandDefines(OneLine.p_str);
  pRun = OneLine.p_str;
  pEnd = pRun + strlen(pRun);

  /* If comment is present, ignore everything after it: */

  pPos = QuotSMultPosQualify(pRun, pCommentLeadIn, QualifyQuote);
  if (pPos)
  {
    adjust_copy_comp(&CommPart, pPos, pEnd - pPos);
    CommPart.Pos.StartCol = pPos - OneLine.p_str;
    pEnd = pPos;
  }
  else
    StrCompReset(&CommPart);

  /* Non-blank character in first column is always label: */

  if ((pRun < pEnd) && *pRun && !as_isspace(*pRun))
  {
    for (pPos = pRun; pPos < pEnd; pPos++)
      if (as_isspace(*pPos) || (*pPos == ':'))
        break;
    LabPart.Pos.StartCol = pRun - OneLine.p_str;
    if (pPos >= pEnd)
    {
      LabPart.Pos.Len = strmemcpy(LabPart.str.p_str, STRINGSIZE, pRun, pEnd - pRun);
      pRun = pEnd;
    }
    else
    {
      LabPart.Pos.Len = strmemcpy(LabPart.str.p_str, STRINGSIZE, pRun, pPos - pRun);
      pRun = pPos + 1;
    }
    if ((LabPart.Pos.Len > 0) && (LabPart.str.p_str[LabPart.Pos.Len - 1] == ':')) /* needed? */
      LabPart.str.p_str[--LabPart.Pos.Len] = '\0';
  }
  else
    StrCompReset(&LabPart);

  /* Opcode & Argument trennen */

  while (True)
  {
    for (; (pRun < pEnd) && as_isspace(*pRun); pRun++);
    for (pPos = pRun; (pPos < pEnd) && !as_isspace(*pPos); pPos++);

    /* If potential OpPart starts with argument divider,
       OpPart is empty and rest of line is all-arguments: */

    if (strchr(DivideChars, *pRun))
    {
      StrCompReset(&OpPart);
      adjust_copy_comp(&ArgPart, pRun, pEnd - pRun);
      ArgPart.Pos.StartCol = pRun - OneLine.p_str;
    }
    else
    {
      /* copy out OpPart */

      OpPart.Pos.StartCol = pRun - OneLine.p_str;
      OpPart.Pos.Len = strmemcpy(OpPart.str.p_str, OpPart.str.capacity, pRun, pPos - pRun);

      /* continue after OpPart separator */

      pRun = (pPos < pEnd) ? pPos + 1 : pEnd;

      /* Falls noch kein Label da war, kann es auch ein Label sein */

      if ((*LabPart.str.p_str == '\0') && OpPart.Pos.Len && (OpPart.str.p_str[OpPart.Pos.Len - 1] == ':'))
      {
        OpPart.str.p_str[--OpPart.Pos.Len] = '\0';
        StrCompCopy(&LabPart, &OpPart);
        continue; /* -> retry finding opcode */
      }

      /* save remainder to ArgPart */

      adjust_copy_comp(&ArgPart, pRun, pEnd - pRun);
      ArgPart.Pos.StartCol = pRun - OneLine.p_str;
    }
    break;
  }

  ArgCnt = 0;

  /* trailing separator on OpPart means we have to push in another empty argument */

  if (OpPart.Pos.Len && strchr(DivideChars, OpPart.str.p_str[OpPart.Pos.Len - 1]))
  {
    const char EmptyArg[] = "";

    OpPart.str.p_str[--OpPart.Pos.Len] = '\0';
    AppendArg(strlen(EmptyArg));
    strcpy(ArgStr[ArgCnt].str.p_str, EmptyArg);
    ArgStr[ArgCnt].Pos = ArgPart.Pos;
  }

  /* Attribut abspalten */

  oppart_leading_dot = False;
  if (HasAttrs)
  {
    const char *pActAttrChar;
    char *pAttrPos, *pActAttrPos;
    int Tries = 0;

again:
    pAttrPos = NULL; AttrSplit = ' ';
    for (pActAttrChar = AttrChars; *pActAttrChar; pActAttrChar++)
    {
      pActAttrPos = strchr(OpPart.str.p_str, *pActAttrChar);
      if (pActAttrPos && ((!pAttrPos) || (pActAttrPos < pAttrPos)))
        pAttrPos = pActAttrPos;
    }
    if (pAttrPos)
    {
      AttrSplit = (*pAttrPos);
      AttrPart.Pos.StartCol = OpPart.Pos.StartCol + (pAttrPos + 1 - OpPart.str.p_str);
      AttrPart.Pos.Len = strmemcpy(AttrPart.str.p_str, STRINGSIZE, pAttrPos + 1, strlen(pAttrPos + 1));
      *pAttrPos = '\0';
      OpPart.Pos.Len = pAttrPos - OpPart.str.p_str;

      /* The dot-prefixed OpPart may itself contain an attribute (.instr.attr).  So reiterate
         splitting off attribute, but only once ;-) */

      if ((*OpPart.str.p_str == '\0') && (*AttrPart.str.p_str != '\0'))
      {
        StrCompCopy(&OpPart, &AttrPart);
        StrCompReset(&AttrPart);
        if (!Tries && (AttrSplit == '.'))
          oppart_leading_dot = True;
        if (++Tries < 2)
          goto again;
      }
    }
    else
      StrCompReset(&AttrPart);
  }
  else
    StrCompReset(&AttrPart);

  KillPostBlanksStrComp(&ArgPart);

  /* Argumente zerteilen: */

  if (*ArgPart.str.p_str)
  {
    const char *pDivPos, *pActDiv, *pActDivPos;

    pRun = ArgPart.str.p_str;
    pEnd = pRun + strlen(pRun);
    pActDivPos = NULL;

    /* A separator found in the previous iteration forces another argument,
       even if it will be empty because the separator is right at the end: */

    while ((pRun < pEnd) || pActDivPos)
    {
      while (*pRun && as_isspace(*pRun))
        pRun++;
#if 0 /* TODO: should work, but doesn't yet */
      pDivPos = QuotMultPosFixup(pRun, DivideChars, NULL);
      if (!pDivPos)
        pDivPos = pEnd;
#endif
      pDivPos = pEnd;
      for (pActDiv = DivideChars; *pActDiv; pActDiv++)
      {
        pActDivPos = QuotPosQualify(pRun, *pActDiv, QualifyQuote);
        if (pActDivPos && (pActDivPos < pDivPos))
          pDivPos = pActDivPos;
      }
      if (ArgCnt >= ArgCntMax)
      {
        WrError(ErrNum_TooManyArgs);
        break;
      }
      AppendArg(pDivPos - pRun);
      adjust_copy_comp(&ArgStr[ArgCnt], pRun, pDivPos - pRun);
      ArgStr[ArgCnt].Pos.StartCol = ArgPart.Pos.StartCol + (pRun - ArgPart.str.p_str);
      KillPostBlanksStrComp(&ArgStr[ArgCnt]);
      pRun = (pDivPos < pEnd) ? pDivPos + 1 : pEnd;
    }
  }
}

/*------------------------------------------------------------------------*/

static void ProcessFile(char *pFileName)
{
  long NxtTime, ListTime;
  char *Run;
  tStrComp FileArg;

  dbgentry("ProcessFile");

  *OneLine.p_str = *CurrFileName = '\0';
  StrCompMkTemp(&FileArg, pFileName, 0);
  ExpandINCLUDE_Core(&FileArg, False);

  ListTime = GTime();

  while (!InputEnd() && !ENDOccured)
  {
    /* Zeile lesen */

    GetNextLine(&OneLine);

    /* Ergebnisfelder vorinitialisieren */

    DontPrint = False;
    CodeLen = 0;
    *ListLine = '\0';

    NextDoLst = DoLst;
    NextIncDepth = IncDepth;

    for (Run = OneLine.p_str; *Run != '\0'; Run++)
      if (!as_isspace(*Run))
        break;
    if (*Run == '#')
      Preprocess();
    else
    {
      SplitLine();
      Produce_Code();
    }

    MakeList(OneLine.p_str);
    DoLst = NextDoLst;
    IncDepth = NextIncDepth;

    /* Zeilenzaehler */

    if (msg_level >= e_msg_level_normal)
    {
      NxtTime = GTime();
      if (((!ListToStdout) || ((ListMask&1) == 0)) && (DTime(ListTime, NxtTime) > 50))
      {
        String Num;
        PInputTag p_input_tag;

        /* search innermost file currently being read, and request its position */

        for (p_input_tag = FirstInputTag; p_input_tag; p_input_tag = p_input_tag->Next)
          if (p_input_tag->Processor == INCLUDE_Processor)
            break;
        p_input_tag->GetPos(p_input_tag, Num, sizeof(Num), False);
        WrConsoleLine(Num, False);
        fflush(stdout);
        ListTime = NxtTime;
      }
    }

    /* bei Ende Makroprozessor ausraeumen
       OK - das ist eine Hauruckmethode... */

    if (ENDOccured)
      while (FirstInputTag)
        GetNextLine(&OneLine);
  }

  while (FirstInputTag)
    GetNextLine(&OneLine);

  /* irgendeine Makrodefinition nicht abgeschlossen ? */

  if (FirstOutputTag)
  {
    WrError(FirstOutputTag->OpenErrMsg);
  }

  dbgexit("ProcessFile");
}

/****************************************************************************/

static const char *TWrite_Plur(int n)
{
  return (n != 1) ? getmessage(Num_ListPlurName) : "";
}

static void TWrite(long DTime, char *dest, size_t DestSize)
{
  int h;

  *dest = '\0';
  h = DTime / 360000;
  DTime %= 360000;
  if (h > 0)
    as_snprcatf(dest, DestSize, "%d%s%s, ", h, getmessage(Num_ListHourName), TWrite_Plur(h));

  h = DTime / 6000;
  DTime %= 6000;
  if (h > 0)
    as_snprcatf(dest, DestSize, "%d%s%s, ", h, getmessage(Num_ListMinuName), TWrite_Plur(h));

  h = DTime / 100;
  DTime %= 100;
  as_snprcatf(dest, DestSize, "%d.%02d%s%s", h, (int)DTime, getmessage(Num_ListSecoName), TWrite_Plur(h));
}

/*--------------------------------------------------------------------------*/

static void AssembleFile_InitPass(void)
{
  static char DateS[31], TimeS[31];
  int z;
  String ArchVal;

  String TmpCompStr;
  tStrComp TmpComp;
  StrCompMkTemp(&TmpComp, TmpCompStr, sizeof(TmpCompStr));

  dbgentry("AssembleFile_InitPass");

  FirstInputTag = NULL;
  FirstOutputTag = NULL;

  MomLocHandle = -1;
  LocHandleCnt = 0;
  SectSymbolCounter = 0;

  SectionStack = NULL;
  FirstIfSave = NULL;
  FirstSaveState = NULL;
  StructStack =
  pInnermostNamedStruct = NULL;
  for (z = 0; z < SegCount; z++)
    pPhaseStacks[z] = NULL;

  InitPass();
  AsmLabelPassInit();

  ActPC = SegCode;
  PCs[ActPC] = 0;
  RelSegs = False;
  ENDOccured = False;
  ErrorCount = 0;
  WarnCount = 0;
  LineSum = 0;
  MacLineSum = 0;
  for (z = 1; z <= StructSeg; z++)
  {
    PCsUsed[z] = FALSE;
    Phases[z] = 0;
    InitChunk(SegChunks + z);
  }

  TransTables =
  CurrTransTable = (PTransTable) malloc(sizeof(TTransTable));
  CurrTransTable->Next = NULL;
  CurrTransTable->Name = as_strdup("STANDARD");
  CurrTransTable->p_table = as_chartrans_table_new();

  EnumSegment = SegNone;
  EnumIncrement = 1;
  EnumCurrentValue = 0;

  strmaxcpy(CurrFileName, "INTERNAL", STRINGSIZE);
  AddFile(CurrFileName);
  CurrLine = 0;

  IncDepth = 0;
  DoLst = eLstMacroExpAll;

  /* Pseudovariablen initialisieren */

  ResetSymbolDefines();
  ResetMacroDefines();
  ResetStructDefines();
  strmaxcpy(TmpCompStr, FlagTrueName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, 1, SegNone, True);
  strmaxcpy(TmpCompStr, FlagFalseName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, 0, SegNone, True);
  strmaxcpy(TmpCompStr, PiName, sizeof(TmpCompStr)); EnterFloatSymbol(&TmpComp, 4.0 * atan(1.0), True);
  strmaxcpy(TmpCompStr, VerName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, VerNo, SegNone, True);
  as_snprintf(ArchVal, sizeof(ArchVal), "%s-%s", ARCHPRNAME, ARCHSYSNAME);
  strmaxcpy(TmpCompStr, ArchName, sizeof(TmpCompStr)); EnterStringSymbol(&TmpComp, ArchVal, True);
  strmaxcpy(TmpCompStr, Has64Name, sizeof(TmpCompStr));
#ifdef HAS64
  EnterIntSymbol(&TmpComp, 1, SegNone, True);
#else
  EnterIntSymbol(&TmpComp, 0, SegNone, True);
#endif
  strmaxcpy(TmpCompStr, CaseSensName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, Ord(CaseSensitive), SegNone, True);
  if (PassNo == 0)
  {
    NLS_CurrDateString(DateS, sizeof(DateS));
    NLS_CurrTimeString(False, TimeS, sizeof(TimeS));
  }
  if (!FindDefSymbol(DateName))
  {
    strmaxcpy(TmpCompStr, DateName, sizeof(TmpCompStr));
    EnterStringSymbol(&TmpComp, DateS, True);
  }
  if (!FindDefSymbol(TimeName))
  {
    strmaxcpy(TmpCompStr, TimeName, sizeof(TmpCompStr));
    EnterStringSymbol(&TmpComp, TimeS, True);
  }

  if (*DefCPU == '\0')
    SetCPUByType(0, NULL);
  else
  {
    tStrComp TmpComp2;

    StrCompMkTemp(&TmpComp2, DefCPU, sizeof(DefCPU));
    if (!SetCPUByName(&TmpComp2))
      SetCPUByType(0, NULL);
  }

  strmaxcpy(TmpCompStr, ListOnName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, ListOn = 1, SegNone, True);
  SetLstMacroExp(eLstMacroExpAll);
  InitLstMacroExpMod(&LstMacroExpModOverride);
  InitLstMacroExpMod(&LstMacroExpModDefault);
  SetFlag(&RelaxedMode, RelaxedName, DefRelaxedMode);
  SetIntConstRelaxedMode(DefRelaxedMode);
  strmaxcpy(TmpCompStr, NestMaxName, sizeof(TmpCompStr)); EnterIntSymbol(&TmpComp, NestMax = DEF_NESTMAX, SegNone, True);
  CopyDefSymbols();

  /* initialize counter for temp symbols here after implicit symbols
     have been defined, so counter starts at a value as low as possible */

  InitTmpSymbols();

  ResetPageCounter();

  StartAdrPresent = False;

  AfterBSRAddr = 0;

  Repass = False;
  PassNo++;

#ifdef PROFILE_MEMO
  NumMemoSum = 0;
  NumMemoCnt = 0;
#endif

  dbgexit("AssembleFile_InitPass");
}

static void AssembleFile_ExitPass(void)
{
  tSavePhase *pSavePhase;
  int z;

#if 0
  if CurrIncludeLevel)
    WrXError(ErrNum_InternalError, "open include");
#endif

  UnsetCPU();
  ClearLocStack();
  ClearStacks();
  AsmErrPassExit();
  for (z = 0; z < SegCount; z++)
    while (pPhaseStacks[z])
    {
      pSavePhase = pPhaseStacks[z];
      pPhaseStacks[z] = pSavePhase->pNext;
      free(pSavePhase);
    }
  if (FirstIfSave)
    WrError(ErrNum_MissEndif);
  if (FirstSaveState)
    WrError(ErrNum_NoRestoreFrame);
  if (SectionStack)
    WrError(ErrNum_MissingEndSect);
  if (StructStack)
    WrXError(ErrNum_OpenStruct, StructStack->Name);
}

static void AssembleFile_WrSummary(const char *pStr)
{
  if (msg_level >= e_msg_level_normal)
    WrConsoleLine(pStr, True);
  if (ListMode == 2)
    WrLstLine(pStr);
}

static void AssembleFile(char *Name)
{
  char *p_out_name;
  String s, Tmp;

  dbgentry("AssembleFile");

  strmaxcpy(SourceFile, Name, STRINGSIZE);
  if (MakeDebug)
    fprintf(Debug, "File %s\n", SourceFile);

  /* Untermodule initialisieren */

  AsmDefInit();
  AsmParsInit();
  AsmIFInit();
  InitFileList();
  ResetStack();

  /* Kommandozeilenoptionen verarbeiten */

  p_out_name = MoveFromOutListFirst();
  strmaxcpy(OutName, p_out_name ? p_out_name : "", STRINGSIZE);
  free(p_out_name);
  if (OutName[0] == '\0')
  {
    strmaxcpy(OutName, SourceFile, STRINGSIZE);
    KillSuffix(OutName);
    AddSuffix(OutName, PrgSuffix);
  }

  if (*ErrorPath == '\0')
  {
    strmaxcpy(ErrorName, SourceFile, STRINGSIZE);
    KillSuffix(ErrorName);
    AddSuffix(ErrorName, LogSuffix);
    unlink(ErrorName);
  }

  switch (ListMode)
  {
    case 0:
      strmaxcpy(LstName, NULLDEV, STRINGSIZE);
      break;
    case 1:
      strmaxcpy(LstName, "!1", STRINGSIZE);
      break;
    case 2:
    {
      char *p_lst_name = MoveFromListOutListFirst();

      strmaxcpy(LstName, p_lst_name ? p_lst_name : "", STRINGSIZE);
      if (p_lst_name) free(p_lst_name);
      if (*LstName == '\0')
      {
        strmaxcpy(LstName, SourceFile, STRINGSIZE);
        KillSuffix(LstName);
        AddSuffix(LstName, LstSuffix);
      }
      break;
    }
  }
  ListToStdout = !strcmp(LstName, "!1");
  ListToNull = !strcmp(LstName, NULLDEV);

  if (ShareMode != 0)
  {
    char *p_share_name = MoveFromShareOutListFirst();

    strmaxcpy(ShareName, p_share_name ? p_share_name : "", STRINGSIZE);
    if (p_share_name)
      free(p_share_name);
    if (*ShareName == '\0')
    {
      strmaxcpy(ShareName, SourceFile, STRINGSIZE);
      KillSuffix(ShareName);
      switch (ShareMode)
      {
        case 1:
          AddSuffix(ShareName, ".inc");
          break;
        case 2:
          AddSuffix(ShareName, ".h");
          break;
        case 3:
          AddSuffix(ShareName, IncSuffix);
          break;
      }
    }
  }

  if (MacProOutput)
  {
    strmaxcpy(MacProName, SourceFile, STRINGSIZE);
    KillSuffix(MacProName);
    AddSuffix(MacProName, PreSuffix);
  }

  if (MacroOutput)
  {
    strmaxcpy(MacroName, SourceFile, STRINGSIZE);
    KillSuffix(MacroName);
    AddSuffix(MacroName, MacSuffix);
  }

  ClearIncludeList();
  CurrIncludeLevel = 0;

  if (DebugMode != DebugNone)
    InitLineInfo();

  /* Variablen initialisieren */

  StartTime = GTime();

  PassNo = 0;

  /* Listdatei eroeffnen */

  if (msg_level >= e_msg_level_normal)
    printf("%s%s\n", getmessage(Num_InfoMessAssembling), SourceFile);

  do
  {
    /* Durchlauf initialisieren */

    AssembleFile_InitPass();
    AsmSubPassInit();
    AsmErrPassInit();
    if (msg_level >= e_msg_level_normal)
    {
      as_snprintf(Tmp, sizeof(Tmp), "%s", getmessage(Num_InfoMessPass));
      as_snprcatf(Tmp, sizeof(Tmp), IntegerFormat, PassNo);
      WrConsoleLine(Tmp, True);
    }

    /* Dateien oeffnen */

    if (CodeOutput)
      OpenFile();

    if (ShareMode != 0)
    {
      ShareFile = fopen(ShareName, "w");
      if (!ShareFile)
        ChkXIO(ErrNum_OpeningFile, ShareName);
      errno = 0;
      switch (ShareMode)
      {
        case 1:
          fprintf(ShareFile, "(* %s-Include File for CONST Section *)\n", SourceFile);
          break;
        case 2:
          fprintf(ShareFile, "/* %s-Include File for C Program */\n", SourceFile);
          break;
        case 3:
          fprintf(ShareFile, "; %s-Include File for Assembler Program\n", SourceFile);
          break;
      }
      ChkIO(ErrNum_ListWrError);
    }

    if (MacProOutput)
    {
      MacProFile = fopen(MacProName, "w");
      if (!MacProFile)
        ChkXIO(ErrNum_OpeningFile, MacProName);
    }

    if ((MacroOutput) && (PassNo == 1))
    {
      MacroFile = fopen(MacroName, "w");
      if (!MacroFile)
        ChkXIO(ErrNum_OpeningFile, MacroName);
    }

    /* Listdatei oeffnen */

    OpenWithStandard(&LstFile, LstName);
    if (!LstFile)
      ChkXIO(ErrNum_OpeningFile, LstName);
    if (!ListToNull)
    {
      errno = 0;
      fprintf(LstFile, "%s", PrtInitString);
      ChkIO(ErrNum_ListWrError);
    }
    if ((ListMask & 1) != 0)
      NewPage(0, False);

    /* assemblieren */

    ProcessFile(SourceFile);
    AssembleFile_ExitPass();

    /* Dateien schliessen */

    if (CodeOutput)
      CloseFile();

    if (ShareMode != 0)
    {
      errno = 0;
      switch (ShareMode)
      {
        case 1:
          fprintf(ShareFile, "(* Ende Include File for CONST Section *)\n");
          break;
        case 2:
          fprintf(ShareFile, "/* Ende Include File for C Program */\n");
          break;
        case 3:
          fprintf(ShareFile, "; Ende Include File for Assembler Program\n");
          break;
      }
      ChkIO(ErrNum_ListWrError);
      CloseIfOpen(&ShareFile);
    }

    if (MacProOutput)
      CloseIfOpen(&MacProFile);
    if (MacroOutput && (PassNo == 1))
      CloseIfOpen(&MacroFile);

    /* evtl. fuer naechsten Durchlauf aufraeumen */

    if ((ErrorCount == 0) && (Repass))
    {
      CloseIfOpen(&LstFile);
      if (CodeOutput)
        unlink(OutName);
      ClearCodepages();
      if (MakeUseList)
        ClearUseList();
      if (MakeCrossList)
        ClearCrossList();
      ClearDefineList();
      if (DebugMode != DebugNone)
        ClearLineInfo();
      ClearIncludeList();
      if (DebugMode != DebugNone)
      {
        ResetAddressRanges();
        ClearSectionUsage();
      }
    }
  }
  while ((ErrorCount == 0) && (Repass));

  /* bei Fehlern loeschen */

  if (ErrorCount != 0)
  {
    if (CodeOutput)
      unlink(OutName);
    if (MacProOutput)
      unlink(MacProName);
    if ((MacroOutput) && (PassNo == 1))
      unlink(MacroName);
    if (ShareMode != 0)
      unlink(ShareName);
    GlobErrFlag = True;
  }

  /* Debug-Ausgabe muss VOR die Symbollistenausgabe, weil letztere die
     Symbolliste loescht */

  if (DebugMode != DebugNone)
  {
    if (ErrorCount == 0)
      DumpDebugInfo();
    ClearLineInfo();
  }

  /* Listdatei abschliessen */

  if  (strcmp(LstName, NULLDEV))
  {
    if (ListMask & 2)
      PrintSymbolList();

    if (ListMask & 64)
      PrintRegDefs();

    if (ListMask & 4)
      PrintMacroList();

    if (ListMask & 256)
      PrintStructList();

    if (ListMask & 8)
      PrintFunctionList();

    if (ListMask & 32)
      PrintDefineList();

    if (ListMask & 128)
      PrintCodepages();

    if (MakeUseList)
    {
      NewPage(ChapDepth, True);
      PrintUseList();
    }

    if (MakeCrossList)
    {
      NewPage(ChapDepth, True);
      PrintCrossList();
    }

    if (MakeSectionList)
      PrintSectionList();

    if (MakeIncludeList)
      PrintIncludeList();

    if (!ListToNull)
    {
      errno = 0;
      fprintf(LstFile, "%s", PrtExitString);
      ChkIO(ErrNum_ListWrError);
    }
  }

  if (MakeUseList)
    ClearUseList();

  if (MakeCrossList)
    ClearCrossList();

  ClearSectionList();

  ClearIncludeList();

  if (!*ErrorPath)
    CloseIfOpen(&ErrorFile);

  ClearUp();

  /* Statistik ausgeben */

  StopTime = GTime();
  TWrite(DTime(StartTime, StopTime), s, sizeof(s));
  strmaxcat(s, getmessage(Num_InfoMessAssTime), STRINGSIZE);
  if (msg_level >= e_msg_level_normal)
  {
    WrConsoleLine("", True);
    WrConsoleLine(s, True);
    WrConsoleLine("", True);
  }
  if (ListMode == 2)
  {
    WrLstLine("");
    WrLstLine(s);
    WrLstLine("");
  }

  as_snprintf(s, sizeof(s), "%7" PRILongInt "%s", LineSum,
              getmessage((LineSum == 1) ? Num_InfoMessAssLine : Num_InfoMessAssLines), STRINGSIZE);
  AssembleFile_WrSummary(s);

  if (LineSum != MacLineSum)
  {
    as_snprintf(s, sizeof(s), "%7" PRILongInt "%s", MacLineSum,
                getmessage((MacLineSum == 1) ? Num_InfoMessMacAssLine : Num_InfoMessMacAssLines), STRINGSIZE);
    AssembleFile_WrSummary(s);
  }

  as_snprintf(s, sizeof(s), "%7d%s", (int)PassNo,
              getmessage((PassNo == 1) ? Num_InfoMessPassCnt : Num_InfoMessPPassCnt), STRINGSIZE);
  AssembleFile_WrSummary(s);

  if ((ErrorCount > 0) && (Repass) && (ListMode != 0))
    WrLstLine(getmessage(Num_InfoMessNoPass));

#ifdef __TURBOC__
  as_snprintf(s, sizeof(s), "%7lu%s", coreleft() >> 10,
              getmessage(Num_InfoMessRemainMem));
  AssembleFile_WrSummary(s);

  as_snprintf(s, sizeof(s), "%7lu%s", (unsigned long)StackRes(),
              getmessage(Num_InfoMessRemainStack));
  AssembleFile_WrSummary(s);
#endif

  as_snprintf(s, sizeof(s), "%7u%s%s", (unsigned)ErrorCount,
              getmessage(Num_InfoMessErrCnt),
              (ErrorCount == 1) ? "" : getmessage(Num_InfoMessErrPCnt));
  AssembleFile_WrSummary(s);

  as_snprintf(s, sizeof(s), "%7u%s%s", (unsigned)WarnCount,
              getmessage(Num_InfoMessWarnCnt),
              (WarnCount == 1) ? "" : getmessage(Num_InfoMessWarnPCnt));
  AssembleFile_WrSummary(s);

#ifdef PROFILE_MEMO
  {
    unsigned long Sum = (NumMemoSum * 100) / NumMemoCnt;

    as_snprintf(s, sizeof(s), "%4lu.%02lu%s", Sum / 100, Sum % 100, " Oppart Compares");
    if (msg_level >= e_msg_level_normal)
      WrConsoleLine(s, True);
    if (ListMode == 2)
      WrLstLine(s);
  }
#endif

  CloseIfOpen(&LstFile);

  /* verstecktes */

  if (MakeDebug)
    PrintSymbolDepth();

  /* Speicher freigeben */

  ClearSymbolList();
  ClearCodepages();
  ClearMacroList();
  ClearFunctionList();
  ClearDefineList();
  ClearFileList();
  ClearStructList();

  dbgentry("AssembleFile");
}

static void AssembleGroup(const char *pFileMask)
{
  String FileMask;

  strmaxcpy(FileMask, pFileMask, sizeof(FileMask));
  AddSuffix(FileMask, SrcSuffix);
  if (!DirScan(FileMask, AssembleFile))
    fprintf(stderr, "%s%s\n", FileMask, getmessage(Num_InfoMessNFilesFound));
}

/*-------------------------------------------------------------------------*/

static int LineZ, screen_height = 0;
static Boolean write_cpu_list_exit;

static void write_console_next(const char *p_line)
{
  WrConsoleLine(p_line, True);
  if (screen_height && (++LineZ >= screen_height))
  {
    LineZ = 0;
    WrConsoleLine(getmessage(Num_KeyWaitMsg), False);
    fflush(stdout);
    while (getchar() != '\n');
    printf("%s", CursUp);
  }
}

static as_cmd_result_t CMD_SharePascal(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (!Negate)
    ShareMode = 1;
  else if (ShareMode == 1)
    ShareMode = 0;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_ShareC(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (!Negate)
    ShareMode = 2;
  else if (ShareMode == 2)
    ShareMode = 0;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_ShareAssembler(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (!Negate)
    ShareMode = 3;
  else if (ShareMode == 3)
    ShareMode = 0;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_DebugMode(Boolean Negate, const char *pArg)
{
  String Arg;

  strmaxcpy(Arg, pArg, STRINGSIZE);
  UpString(Arg);

  if (Negate)
  {
    if (Arg[0] != '\0')
      return e_cmd_err;
    else
    {
      DebugMode = DebugNone;
      return e_cmd_ok;
    }
  }
  else if (!strcmp(Arg, ""))
  {
    DebugMode = DebugMAP;
    return e_cmd_ok;
  }
  else if (!strcmp(Arg, "ATMEL"))
  {
    DebugMode = DebugAtmel;
    return e_cmd_arg;
  }
  else if (!strcmp(Arg, "MAP"))
  {
    DebugMode = DebugMAP;
    return e_cmd_arg;
  }
  else if (!strcmp(Arg, "NOICE"))
  {
    DebugMode = DebugNoICE;
    return e_cmd_arg;
  }
#if 0
  else if (!strcmp(Arg, "A.OUT"))
  {
    DebugMode = DebugAOUT;
    return e_cmd_arg;
  }
  else if (!strcmp(Arg, "COFF"))
  {
    DebugMode = DebugCOFF;
    return e_cmd_arg;
  }
  else if (!strcmp(Arg, "ELF"))
  {
    DebugMode = DebugELF;
    return e_cmd_arg;
  }
#endif
  else
    return e_cmd_err;

#if 0
  if (Negate)
    DebugMode = DebugNone;
  else
    DebugMode = DebugMAP;
  return e_cmd_ok;
#endif
}

static as_cmd_result_t CMD_ListConsole(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (!Negate)
    ListMode = 1;
  else if (ListMode == 1)
    ListMode = 0;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_ListRadix(Boolean Negate, const char *Arg)
{
  Boolean OK, new_zero_pad = False;
  LargeWord NewListRadixBase;

  if (Negate)
  {
    ListRadixBase = 16;
    ListPCZeroPad = False;
    return e_cmd_ok;
  }

  if (*Arg == '0')
  {
    new_zero_pad = True;
    Arg++;
  }
  NewListRadixBase = ConstLongInt(Arg, &OK, 10);
  if (!OK || (NewListRadixBase < 2) || (NewListRadixBase > 36))
    return e_cmd_err;
  ListRadixBase = NewListRadixBase;
  ListPCZeroPad = new_zero_pad;
  return e_cmd_arg;
}

static as_cmd_result_t CMD_screen_height(Boolean negate, const char *p_arg)
{
  Boolean ok;
  int new_screen_height;

  if (negate)
  {
    screen_height = 0;
    return e_cmd_ok;
  }
  new_screen_height = ConstLongInt(p_arg, &ok, 10);
  if (!ok)
    return e_cmd_err;
  screen_height = new_screen_height;
  return e_cmd_arg;
}

static as_cmd_result_t CMD_ListFile(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (!Negate)
    ListMode = 2;
  else if (ListMode == 2)
    ListMode = 0;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_SuppWarns(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  SuppWarns = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_UseList(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  MakeUseList = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_CrossList(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  MakeCrossList = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_SectionList(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  MakeSectionList = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_BalanceTree(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  BalanceTrees = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_MakeDebug(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (!Negate)
  {
    MakeDebug = True;
    errno = 0;
    Debug = fopen("as.deb", "w");
    if (!Debug)
      ChkIO(ErrNum_ListWrError);
  }
  else if (MakeDebug)
  {
    MakeDebug = False;
    CloseIfOpen(&Debug);
  }
  return e_cmd_ok;
}

static as_cmd_result_t CMD_MacProOutput(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  MacProOutput = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_MacroOutput(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  MacroOutput = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_MakeIncludeList(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  MakeIncludeList = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_CodeOutput(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  CodeOutput = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_MsgIfRepass(Boolean Negate, const char *Arg)
{
  Boolean OK;
  UNUSED(Arg);

  MsgIfRepass = !Negate;
  if (MsgIfRepass)
  {
    if (Arg[0] == '\0')
    {
      PassNoForMessage = 1;
      return e_cmd_ok;
    }
    else
    {
      PassNoForMessage = ConstLongInt(Arg, &OK, 10);
      if (!OK)
      {
        PassNoForMessage = 1;
        return e_cmd_ok;
      }
      else if (PassNoForMessage < 1)
        return e_cmd_err;
      else
        return e_cmd_arg;
    }
  }
  else
    return e_cmd_ok;
}

static as_cmd_result_t CMD_Relaxed(Boolean Negate, const char *pArg)
{
  UNUSED(pArg);

  DefRelaxedMode = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_ExtendErrors(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if ((Negate) && (ExtendErrors > 0))
    ExtendErrors--;
  else if ((!Negate) && (ExtendErrors < 2))
    ExtendErrors++;

  return e_cmd_ok;
}

static as_cmd_result_t CMD_NumericErrors(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  NumericErrors = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_HexLowerCase(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  HexStartCharacter = Negate ? 'A' : 'a';
  return e_cmd_ok;
}

static as_cmd_result_t CMD_SplitByte(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (Negate)
  {
    SplitByteCharacter = '\0';
    return e_cmd_ok;
  }
  else if (*Arg)
  {
    if (strlen(Arg) != 1)
      return e_cmd_err;
    SplitByteCharacter = *Arg;
    return e_cmd_arg;
  }
  else
  {
    SplitByteCharacter = '.';
    return e_cmd_ok;
  }
}

static as_cmd_result_t CMD_ThrowErrors(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  ThrowErrors = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_CaseSensitive(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  CaseSensitive = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_GNUErrors(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  GNUErrors  =  !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_IncludeList(Boolean Negate, const char *Arg)
{
  char *p;
  String Copy, part;

  if (*Arg == '\0') return e_cmd_err;
  else
  {
    strmaxcpy(Copy, Arg, STRINGSIZE);
    do
    {
      p = strrchr(Copy, DIRSEP);
      if (!p)
      {
        strmaxcpy(part, Copy, STRINGSIZE);
        *Copy = '\0';
      }
      else
      {
        *p = '\0';
        strmaxcpy(part, p + 1, STRINGSIZE);
      }
      if (Negate)
        RemoveIncludeList(part);
      else
        AddIncludeList(part);
    }
    while (Copy[0] != '\0');
    return e_cmd_arg;
  }
}

static as_cmd_result_t CMD_ListMask(Boolean Negate, const char *Arg)
{
  Word erg;
  Boolean OK;

  if (Arg[0] == '\0')
    return e_cmd_err;
  else
  {
    erg = ConstLongInt(Arg, &OK, 10);
    if ((!OK) || (erg > 511))
      return e_cmd_err;
    else
    {
      ListMask = Negate ? (ListMask & ~erg) : (ListMask | erg);
      return e_cmd_arg;
    }
  }
}

static as_cmd_result_t CMD_DefSymbol(Boolean Negate, const char *Arg)
{
  String Copy, Part, Name;
  char *p;
  as_cmd_result_t Result = e_cmd_err;

  TempResult t;
  as_tempres_ini(&t);

  if (Arg[0] == '\0')
    LEAVE;

  strmaxcpy(Copy, Arg, STRINGSIZE);
  do
  {
    p = QuotPos(Copy, ',');
    if (!p)
    {
      strmaxcpy(Part, Copy, STRINGSIZE);
      Copy[0] = '\0';
    }
    else
    {
      *p = '\0';
      strmaxcpy(Part, Copy, STRINGSIZE);
      strmov(Copy, p + 1);
    }
   if (!CaseSensitive)
     UpString(Part);
   p = QuotPos(Part, '=');
   if (!p)
   {
     strmaxcpy(Name, Part, STRINGSIZE);
     Part[0] = '\0';
   }
   else
   {
     *p = '\0';
     strmaxcpy(Name, Part, STRINGSIZE);
     strmov(Part, p + 1);
   }
   if (!ChkSymbName(Name))
     LEAVE;
   if (Negate)
     RemoveDefSymbol(Name);
   else
   {
     AsmParsInit();
     if (Part[0] != '\0')
     {
       EvalExpression(Part, &t);
       if ((t.Typ == TempNone) || mFirstPassUnknown(t.Flags))
         return e_cmd_err;
     }
     else
       as_tempres_set_int(&t, 1);
     AddDefSymbol(Name, &t);
   }
  }
  while (Copy[0] != '\0');

  Result = e_cmd_arg;
func_exit:
  as_tempres_free(&t);
  return Result;
}

static as_cmd_result_t CMD_ErrorPath(Boolean Negate, const char *Arg)
{
  if (Negate)
    return e_cmd_err;
  else if (Arg[0] == '\0')
  {
    ErrorPath[0] = '\0';
    return e_cmd_ok;
  }
  else
  {
    strmaxcpy(ErrorPath, Arg, STRINGSIZE);
    return e_cmd_arg;
  }
}

static as_cmd_result_t CMD_HardRanges(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  HardRanges = Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_OutFile(Boolean Negate, const char *Arg)
{
  if (Arg[0] == '\0')
  {
    if (Negate)
    {
      ClearOutList();
      return e_cmd_ok;
    }
    else
      return e_cmd_err;
  }
  else
  {
    if (Negate)
      RemoveFromOutList(Arg);
    else
      AddToOutList(Arg);
    return e_cmd_arg;
  }
}

static as_cmd_result_t CMD_ShareOutFile(Boolean Negate, const char *Arg)
{
  if (Arg[0] == '\0')
  {
    if (Negate)
    {
      ClearShareOutList();
      return e_cmd_ok;
    }
    else
      return e_cmd_err;
  }
  else
  {
    if (Negate)
      RemoveFromShareOutList(Arg);
    else
      AddToShareOutList(Arg);
    return e_cmd_arg;
  }
}

static as_cmd_result_t CMD_ListOutFile(Boolean Negate, const char *Arg)
{
  if (Arg[0] == '\0')
  {
    if (Negate)
    {
      ClearListOutList();
      return e_cmd_ok;
    }
    else
      return e_cmd_err;
  }
  else
  {
    if (Negate)
      RemoveFromListOutList(Arg);
    else
      AddToListOutList(Arg);
    return e_cmd_arg;
  }
}

static Boolean CMD_CPUAlias_ChkCPUName(char *s)
{
  int z;

  for (z = 0; z < (int)strlen(s); z++)
    if (!isalnum((unsigned int) s[z]))
      return False;
  return True;
}

static as_cmd_result_t CMD_CPUAlias(Boolean Negate, const char *Arg)
{
  const char *p;
  String s1, s2;

  if (Negate)
    return e_cmd_err;
  else if (Arg[0] == '\0')
    return e_cmd_err;
  else
  {
    p = strchr(Arg, '=');
    if (!p)
      return e_cmd_err;
    else
    {
      strmemcpy(s1, STRINGSIZE, Arg, p - Arg);
      UpString(s1);
      strmaxcpy(s2, p + 1, STRINGSIZE);
      UpString(s2);
      if (!(CMD_CPUAlias_ChkCPUName(s1) && CMD_CPUAlias_ChkCPUName(s2)))
        return e_cmd_err;
      else if (!AddCPUAlias(s2, s1))
        return e_cmd_err;
      else
        return e_cmd_arg;
    }
  }
}

static as_cmd_result_t CMD_SetCPU(Boolean Negate, const char *Arg)
{
  if (Negate)
  {
    *DefCPU = '\0';
    return e_cmd_ok;
  }
  else
  {
    if (*Arg == '\0')
      return e_cmd_err;

    if (!as_strcasecmp(Arg, "?") || !as_strcasecmp(Arg, "LIST"))
    {
      write_cpu_list_exit = True;
      return e_cmd_arg;
    }

    strmaxcpy(DefCPU, Arg, sizeof(DefCPU) - 1);
    NLS_UpString(DefCPU);

    if (!LookupCPUDefByName(DefCPU))
    {
      *DefCPU = '\0';
      return e_cmd_err;
    }
    return e_cmd_arg;
  }
}

static as_cmd_result_t CMD_NoICEMask(Boolean Negate, const char *Arg)
{
  Word erg;
  Boolean OK;

  if (Negate)
  {
    NoICEMask = 1 << SegCode;
    return e_cmd_ok;
  }
  else if (Arg[0] == '\0')
    return e_cmd_err;
  else
  {
    erg = ConstLongInt(Arg, &OK, 10);
    if (!OK || (erg >= (1 << SegCount)))
      return e_cmd_err;
    else
    {
      NoICEMask = erg;
      return e_cmd_arg;
    }
  }
}

static as_cmd_result_t CMD_MaxErrors(Boolean Negate, const char *Arg)
{
  if (Negate)
  {
    MaxErrors = 0;
    return e_cmd_ok;
  }
  else if (Arg[0] == '\0')
    return e_cmd_err;
  else
  {
    Boolean OK;
    LongWord NewMaxErrors = ConstLongInt(Arg, &OK, 10);

    if (!OK)
      return e_cmd_err;
    MaxErrors = NewMaxErrors;
    return e_cmd_arg;
  }
}

/*!------------------------------------------------------------------------
 * \fn     CMD_MaxIncludeLevel(Boolean Negate, const char *pArg)
 * \brief  set maximum include nesting level
 * \param  Negate back to default?
 * \param  pArg numeric argument
 * \return exec result
 * ------------------------------------------------------------------------ */

#define DEFAULT_MAXINCLUDELEVEL 200

static as_cmd_result_t CMD_MaxIncludeLevel(Boolean Negate, const char *pArg)
{
  if (Negate)
  {
    MaxErrors = DEFAULT_MAXINCLUDELEVEL;
    return e_cmd_ok;
  }
  else if (pArg[0] == '\0')
    return e_cmd_err;
  else
  {
    Boolean OK;
    Integer NewMaxIncludeLevel = ConstLongInt(pArg, &OK, 10);

    if (!OK)
      return e_cmd_err;
    MaxIncludeLevel = NewMaxIncludeLevel;
    return e_cmd_arg;
  }
}

static const as_cmd_rec_t ASParams[] =
{
  { "A"             , CMD_BalanceTree     },
  { "alias"         , CMD_CPUAlias        },
  { "a"             , CMD_ShareAssembler  },
  { "C"             , CMD_CrossList       },
  { "c"             , CMD_ShareC          },
  { "CPU"           , CMD_SetCPU          },
  { "D"             , CMD_DefSymbol       },
  { "E"             , CMD_ErrorPath       },
  { "g"             , CMD_DebugMode       },
  { "G"             , CMD_CodeOutput      },
  { "gnuerrors"     , CMD_GNUErrors       },
  { "h"             , CMD_HexLowerCase    },
  { "i"             , CMD_IncludeList     },
  { "I"             , CMD_MakeIncludeList },
  { "L"             , CMD_ListFile        },
  { "l"             , CMD_ListConsole     },
  { "listradix"     , CMD_ListRadix       },
  { "splitbyte"     , CMD_SplitByte       },
  { "M"             , CMD_MacroOutput     },
  { "maxerrors"     , CMD_MaxErrors       },
  { "maxinclevel"   , CMD_MaxIncludeLevel },
  { "n"             , CMD_NumericErrors   },
  { "noicemask"     , CMD_NoICEMask       },
  { "o"             , CMD_OutFile         },
  { "P"             , CMD_MacProOutput    },
  { "p"             , CMD_SharePascal     },
  { "r"             , CMD_MsgIfRepass     },
  { RelaxedName     , CMD_Relaxed         },
  { "s"             , CMD_SectionList     },
  { "screenheight"  , CMD_screen_height   },
  { "shareout"      , CMD_ShareOutFile    },
  { "olist"         , CMD_ListOutFile     },
  { "t"             , CMD_ListMask        },
  { "u"             , CMD_UseList         },
  { "U"             , CMD_CaseSensitive   },
  { "w"             , CMD_SuppWarns       },
  { "warnranges"    , CMD_HardRanges      },
  { "x"             , CMD_ExtendErrors    },
  { "X"             , CMD_MakeDebug       },
  { "Y"             , CMD_ThrowErrors     }
};

/*--------------------------------------------------------------------------*/

#ifdef __sunos__

extern void on_exit(void (*procp)(int status, caddr_t arg),caddr_t arg);

static void GlobExitProc(int status, caddr_t arg)
{
  if (MakeDebug)
    CloseIfOpen(&Debug);
}

#else

/* Might no longer need this with newer TCC versions: */

#ifdef __TINYC__
void * __dso_handle __attribute((visibility("hidden"))) = &__dso_handle;
#endif

static void GlobExitProc(void)
{
  if (MakeDebug)
    CloseIfOpen(&Debug);
}

#endif

int main(int argc, char **argv)
{
  char *Env;
  String Dummy;
  static Boolean First = TRUE;
  as_cmd_results_t cmd_results;

  if (First)
  {
    be_le_init();
    nls_init();
    bpemu_init();
    stdhandl_init();
    strutil_init();
    chunks_init();
    if (!NLS_Initialize(&argc, argv))
      exit(4);

#ifdef _USE_MSH
    nlmessages_init_buffer(as_msh_data, sizeof(as_msh_data), MsgId1, MsgId2);
#else
    nlmessages_init_file("as.msg", *argv, MsgId1, MsgId2);
#endif
    ioerrs_init(*argv);
    as_cmdarg_init(*argv);
    msg_level_init();
    as_cmd_register(ASParams, as_array_size(ASParams));

    asmfnums_init();
    asminclist_init();
    asmitree_init();

    asmdef_init();
    cpulist_init();
    asmsub_init();
    asmpars_init();
    intformat_init();

    asmmac_init();
    asmstruct_init();
    asmif_init();
    asmerr_init();
    asmcode_init();
    asmlabel_init();
    asmdebug_init();

    codeallg_init();
    onoff_common_init();
    literals_init();

    code68k_init();
    code56k_init();
    code601_init();
    codepalm_init();
    codemcore_init();
    codexgate_init();
    code68_init();
    code6805_init();
    code6809_init();
    code6812_init();
    codes12z_init();
    code6816_init();
    code68rs08_init();
    codeh8_3_init();
    codeh8_5_init();
    code7000_init();
    codeko09_init();
    code65_init();
    codepps4_init();
    codeh16_init();
    code7700_init();
    codehmcs400_init();
    code4500_init();
    codem16_init();
    codem16c_init();
    codepdp11_init();
    codevax_init();
    code4004_init();
    code8008_init();
    code48_init();
    code51_init();
    code96_init();
    code85_init();
    code86_init();
    code960_init();
    code8x30x_init();
    code2650_init();
    codexa_init();
    codeavr_init();
    code29k_init();
    code166_init();
    codez80_init();
    codez8_init();
    codez8000_init();
    codekcpsm_init();
    codekcpsm3_init();
    codemico8_init();
    code96c141_init();
    code90c141_init();
    code87c800_init();
    code870c_init();
    code47c00_init();
    code97c241_init();
    code9331_init();
    code16c5x_init();
    code16c8x_init();
    code17c4x_init();
    codesx20_init();
    codepdk_init();
    codest6_init();
    codest7_init();
    codest9_init();
    code6804_init();
    code3201x_init();
    code3202x_init();
    code3203x_init();
    code3205x_init();
    code32054x_init();
    code3206x_init();
    code9900_init();
    codetms7_init();
    code370_init();
    codemsp_init();
    codetms1_init();
    code78c10_init();
    code75xx_init();
    code75k0_init();
    code78k0_init();
    code78k2_init();
    code78k3_init();
    code78k4_init();
    code7720_init();
    code77230_init();
    codev60_init();
    codescmp_init();
    codeimp16_init();
    code807x_init();
    codecop4_init();
    codecop8_init();
    codesc14xxx_init();
    codens32k_init();
    codeace_init();
    codecp3f_init();
    codef8_init();
    code53c8xx_init();
    codef2mc8_init();
    codef2mc16_init();
    codemn1610_init();
    codemn2610_init();
    codeolms40_init();
    codeolms50_init();
    code1802_init();
    codevector_init();
    codexcore_init();
    code1750_init();
    codekenbak_init();
    codecp1600_init();
    codenano_init();
    code6100_init();
    coderx_init();
    First = FALSE;
  }

#ifdef __sunos__
  on_exit(GlobExitProc, (caddr_t) NULL);
#else
# ifndef __MUNIX__
  atexit(GlobExitProc);
# endif
#endif

  *CursUp = '\0';
  switch (Redirected)
  {
    case NoRedir:
      Env = getenv("USEANSI");
      strmaxcpy(Dummy, Env ? Env : "Y", STRINGSIZE);
      if (as_toupper(Dummy[0]) == 'N')
      {
      }
      else
      {
        strcpy(CursUp, " [A"); CursUp[0] = Char_ESC;
      }
      break;
    case RedirToDevice:
      break;
    case RedirToFile:
      break;
  }

  ShareMode = 0;
  ListMode = 0;
  IncludeList[0] = '\0';
  SuppWarns = False;
  MakeUseList = False;
  MakeCrossList = False;
  MakeSectionList = False;
  MakeIncludeList = False;
  ListMask = 0x1ff;
  MakeDebug = False;
  ExtendErrors = 0;
  DefRelaxedMode = False;
  MacroOutput = False;
  MacProOutput = False;
  CodeOutput = True;
  strcpy(ErrorPath,  "!2");
  MsgIfRepass = False;
  NumericErrors = False;
  DebugMode = DebugNone;
  CaseSensitive = False;
  ThrowErrors = False;
  HardRanges = True;
  NoICEMask = 1 << SegCode;
  GNUErrors = False;
  MaxErrors = 0;
  ListRadixBase = 16;
  ListPCZeroPad = False;
  MaxIncludeLevel = DEFAULT_MAXINCLUDELEVEL;
  write_cpu_list_exit = False;

  LineZ = 0;
  screen_height = 0;

#if defined(INCDIR)
  CMD_IncludeList(False, INCDIR);
#endif
  if (e_cmd_err == as_cmd_process(argc, argv, EnvName, &cmd_results))
  {
    printf("%s%s\n", getmessage(cmd_results.error_arg_in_env ? Num_ErrMsgInvEnvParam : Num_ErrMsgInvParam), cmd_results.error_arg);
    exit(4);
  }

  if ((msg_level >= e_msg_level_verbose) || cmd_results.write_version_exit)
  {
    String Tmp;

    as_snprintf(Tmp, sizeof(Tmp), "%s%s", getmessage(Num_InfoMessMacroAss), Version);
    write_console_next(Tmp);
    as_snprintf(Tmp, sizeof(Tmp), "(%s-%s)", ARCHPRNAME, ARCHSYSNAME);
    write_console_next(Tmp);
    write_console_next(InfoMessCopyright);
    WriteCopyrights(write_console_next);
    write_console_next("");
  }

  if (cmd_results.write_help_exit)
  {
    char *ph1, *ph2;
    String tmp;
    as_snprintf(tmp, sizeof(tmp), "%s%s%s", getmessage(Num_InfoMessHead1), as_cmdarg_get_executable_name(), getmessage(Num_InfoMessHead2));
    write_console_next(tmp);
    for (ph1 = getmessage(Num_InfoMessHelp), ph2 = strchr(ph1, '\n'); ph2; ph1 = ph2 + 1, ph2 = strchr(ph1, '\n'))
    {
      *ph2 = '\0';
      write_console_next(ph1);
      *ph2 = '\n';
    }
  }

  if (write_cpu_list_exit)
  {
    write_console_next(getmessage(Num_InfoMessCPUList));
    PrintCPUList(write_console_next);
    ClearCPUList();
  }

  if (cmd_results.write_version_exit || write_cpu_list_exit || cmd_results.write_help_exit)
    exit(0);

  /* ListRadixBase must have been set */

  asmlist_init();

  GlobErrFlag = False;
  if (ErrorPath[0] != '\0')
  {
    strcpy(ErrorName, ErrorPath);
    unlink(ErrorName);
  }

  if (StringListEmpty(cmd_results.file_arg_list))
  {
    fprintf(stderr, "%s: %s\n", as_cmdarg_get_executable_name(), getmessage(Num_ErrMessNoInputFiles));
    exit(1);
  }
  else
  {
    char *pFile;

    while (True)
    {
      pFile = MoveAndCutStringListFirst(&cmd_results.file_arg_list);
      if (!pFile)
        break;
      if (*pFile)
        AssembleGroup(pFile);
      free(pFile);
    }
  }

  if (*ErrorPath)
    CloseIfOpen(&ErrorFile);

  ClearCPUList();

  return GlobErrFlag ? 2 : 0;
}
