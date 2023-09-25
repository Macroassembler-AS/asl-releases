#ifndef _ASMDEF_H
#define _ASMDEF_H
/* asmdef.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* global benutzte Variablen und Definitionen                                */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>
#include <stdio.h>

#include "chunks.h"
#include "fileformat.h"
#include "dynstr.h"
#include "intformat.h"
#include "strcomp.h"
#include "lstmacroexp.h"
#include "cpulist.h"
#include "tempresult.h"
#include "addrspace.h"
#include "chartrans.h"

typedef struct _TCrossRef
{
  struct _TCrossRef *Next;
  Byte FileNum;
  LongInt LineNum;
  Integer OccNum;
} TCrossRef,*PCrossRef;


typedef struct _TPatchEntry
{
  struct _TPatchEntry *Next;
  LargeWord Address;
  char *Ref;
  Word len;
  LongWord RelocType;
} TPatchEntry, *PPatchEntry;

typedef struct _TExportEntry
{
  struct _TExportEntry *Next;
  char *Name;
  Word len;
  LongWord Flags;
  LargeWord Value;
} TExportEntry, *PExportEntry;

typedef enum
{
  DebugNone,
  DebugMAP,
  DebugAOUT,
  DebugCOFF,
  DebugELF,
  DebugAtmel,
  DebugNoICE
} DebugType;

#define Char_NUL 0
#define Char_BEL '\a'
#define Char_BS '\b'
#define Char_HT 9
#define Char_LF '\n'
#define Char_FF 12
#define Char_CR 13
#define Char_ESC 27

#define ListMask_FormFeed         (1 << 0)
#define ListMask_SymbolList       (1 << 1)
#define ListMask_MacroList        (1 << 2)
#define ListMask_FunctionList     (1 << 3)
#define ListMask_LineNums         (1 << 4)
#define ListMask_DefineList       (1 << 5)
#define ListMask_RegDefList       (1 << 6)
#define ListMask_Codepages        (1 << 7)
#define ListMask_StructList       (1 << 8)

extern char SrcSuffix[],IncSuffix[],PrgSuffix[],LstSuffix[],
            MacSuffix[],PreSuffix[],LogSuffix[],MapSuffix[],
            OBJSuffix[];

#define MomCPUName       "MOMCPU"     /* mom. Prozessortyp */
#define MomCPUIdentName  "MOMCPUNAME" /* mom. Prozessortyp */
#define MomFPUIdentName  "MOMFPUNAME" /* mom. Koprozessortyp */
#define MomPMMUIdentName  "MOMPMMUNAME" /* mom. MMU-Typ */
#define DoPaddingName    "PADDING"    /* Padding an */
#define PackingName      "PACKING"    /* gepackte Ablage an */
#define ListOnName       "LISTON"     /* Listing an/aus */
#define RelaxedName      "RELAXED"    /* alle Zahlenschreibweisen zugelassen */
#define BranchExtName    "BRANCHEXT"  /* Spruenge autom. verlaengern */
#define FlagTrueName     "TRUE"	      /* Flagkonstanten */
#define FlagFalseName    "FALSE"
#define PiName           "CONSTPI"    /* Zahl Pi */
#define DateName         "DATE"       /* Datum & Zeit */
#define TimeName         "TIME"
#define VerName          "VERSION"    /* speichert Versionsnummer */
#define CaseSensName     "CASESENSITIVE" /* zeigt Gross/Kleinunterscheidung an */
#define Has64Name        "HAS64"         /* arbeitet Parser mit 64-Bit-Integers ? */
#define ArchName         "ARCHITECTURE"  /* Zielarchitektur von AS */
#define AttrName         "ATTRIBUTE"  /* Attributansprache in Makros */
#define LabelName        "__LABEL__"  /* Labelansprache in Makros */
#define ArgCName         "ARGCOUNT"   /* Argumentzahlansprache in Makros */
#define AllArgName       "ALLARGS"    /* Ansprache Argumentliste in Makros */
#define DefStackName     "DEFSTACK"   /* Default-Stack */
#define NestMaxName      "NESTMAX"    /* max. nesting level of a macro */
#define DottedStructsName "DOTTEDSTRUCTS" /* struct elements by default with . */

extern const char *EnvName;

/* This results from the tokenized representation of macro arguments
   in macro bodys: (31*16) - 4 for special arguments: */

#define ArgCntMax 476

#define ChapMax 4

#define AscOfs '0'

#define MaxCodeLen_Ini 256
#define MaxCodeLen_Max 65535
extern LongWord MaxCodeLen;

#define DEF_NESTMAX 256

typedef void (*SimpProc)(
#ifdef __PROTOS__
void
#endif
);

typedef void (*DissectBitProc)(
#ifdef __PROTOS__
char *pDest, size_t DestSize, LargeWord Inp
#endif
);

typedef Boolean (*tQualifyQuoteFnc)(const char *pStart, const char *pQuotePos);

typedef Word WordField[6];          /* fuer Zahlenumwandlung */
typedef struct _TTransTable
{
  struct _TTransTable *Next;
  char *Name;
  as_chartrans_table_t *p_table;
} TTransTable, *PTransTable;

typedef struct _TSaveState
{
  struct _TSaveState *Next;
  CPUVar SaveCPU;
  char *pSaveCPUArgs;
  as_addrspace_t SavePC;
  Byte SaveListOn;
  tLstMacroExp SaveLstMacroExp;
  tLstMacroExpMod SaveLstMacroExpModDefault,
                  SaveLstMacroExpModOverride;
  PTransTable SaveTransTable;
  Integer SaveEnumSegment;
  LongInt SaveEnumCurrentValue, SaveEnumIncrement;
} TSaveState,*PSaveState;

typedef struct _TForwardSymbol
{
  struct _TForwardSymbol *Next;
  StringPtr Name;
  LongInt DestSection;
  StringPtr pErrorPos;
} TForwardSymbol, *PForwardSymbol;

typedef struct _TSaveSection
{
  struct _TSaveSection *Next;
  PForwardSymbol LocSyms, GlobSyms, ExportSyms;
  LongInt Handle;
} TSaveSection, *PSaveSection;

typedef struct sSavePhase
{
  struct sSavePhase *pNext;
  LargeWord SaveValue;
} tSavePhase;

typedef struct _TDefinement
{
  struct _TDefinement *Next;
  StringPtr TransFrom, TransTo;
  Byte Compiled[256];
} TDefinement, *PDefinement;

typedef struct _ASSUMERec
{
  const char *Name;
  LongInt *Dest;
  LongInt Min,Max;
  LongInt NothingVal;
  void (*pPostProc)(void);
} ASSUMERec;

extern StringPtr SourceFile;

extern StringPtr CursUp;

extern LargeWord *PCs;
extern Boolean RelSegs;
extern LargeWord StartAdr;
extern LargeWord AfterBSRAddr;
extern Boolean StartAdrPresent;
extern LargeWord *Phases;
extern Word Grans[SegCountPlusStruct];
extern Word ListGrans[SegCountPlusStruct];
extern ChunkList SegChunks[SegCountPlusStruct];
extern as_addrspace_t ActPC;
extern Boolean PCsUsed[SegCountPlusStruct];
extern LargeWord *SegInits;
extern LargeWord *SegLimits;
extern LongInt ValidSegs;
extern Boolean ENDOccured;
extern Boolean Retracted;
extern Boolean ListToStdout,ListToNull;

extern unsigned ASSUMERecCnt;
extern const ASSUMERec *pASSUMERecs;
extern void (*pASSUMEOverride)(void);

extern Integer PassNo;
extern Integer JmpErrors;
extern Boolean ThrowErrors;
extern Boolean Repass;
extern Byte MaxSymPass;
extern Byte ShareMode;
extern DebugType DebugMode;
extern Word NoICEMask;
extern Byte ListMode;
extern Byte ListOn;
extern Integer MaxIncludeLevel;
extern Boolean MakeUseList;
extern Boolean MakeCrossList;
extern Boolean MakeSectionList;
extern Boolean MakeIncludeList;
extern Boolean DefRelaxedMode;
extern Word ListMask;
extern ShortInt ExtendErrors;
extern Integer EnumSegment;
extern LongInt EnumIncrement, EnumCurrentValue;
extern LongWord MaxErrors;

extern LongInt MomSectionHandle;
extern PSaveSection SectionStack;
extern tSavePhase *pPhaseStacks[SegCount];

extern tSymbolSize AttrPartOpSize[2];
extern LongInt CodeLen;
extern Byte *BAsmCode;
extern Word *WAsmCode;
extern LongWord *DAsmCode;

extern Boolean DontPrint;
extern Word ActListGran;

extern Boolean NumericErrors;
extern Boolean CodeOutput;
extern Boolean MacProOutput;
extern Boolean MacroOutput;
extern Boolean HardRanges;
extern const char *DivideChars;
extern Boolean HasAttrs;
extern const char *AttrChars;
extern Boolean MsgIfRepass;
extern Integer PassNoForMessage;
extern Boolean CaseSensitive;
extern LongInt NestMax;
extern Boolean GNUErrors;

extern FILE *PrgFile;

extern StringPtr ErrorPath,ErrorName;
extern StringPtr OutName;
extern Integer CurrIncludeLevel;
extern StringPtr CurrFileName;
extern LongInt CurrLine;
extern LongInt LineSum;
extern LongInt MacLineSum;

extern LongInt NOPCode;
extern Boolean TurnWords;
extern Byte HeaderID;
extern const char *PCSymbol;
extern tIntConstMode IntConstMode;
extern Boolean IntConstModeIBMNoTerm;
extern Boolean (*SetIsOccupiedFnc)(void),
               (*SaveIsOccupiedFnc)(void),
               (*RestoreIsOccupiedFnc)(void);
extern Boolean SwitchIsOccupied, PageIsOccupied, ShiftIsOccupied;
extern Boolean multi_char_le;
extern Boolean (*DecodeAttrPart)(void);
extern void (*MakeCode)(void);
extern Boolean (*ChkPC)(LargeWord Addr);
extern Boolean (*IsDef)(void);
extern void (*SwitchFrom)(void);
extern void (*InternSymbol)(char *Asc, TempResult *Erg);
extern DissectBitProc DissectBit;
extern DissectRegProc DissectReg;
extern tQualifyQuoteFnc QualifyQuote;

extern StringPtr IncludeList;
extern Integer IncDepth,NextIncDepth;
extern FILE *ErrorFile;
extern FILE *LstFile;
extern FILE *ShareFile;
extern FILE *MacProFile;
extern FILE *MacroFile;
extern Boolean InMacroFlag;
extern StringPtr LstName, MacroName, MacProName;
extern tLstMacroExp DoLst, NextDoLst;
extern StringPtr ShareName;
extern CPUVar MomCPU, MomVirtCPU;
extern StringPtr MomCPUArgs;
extern char DefCPU[20];
extern char MomCPUIdent[20],
            MomFPUIdent[20],
            MomPMMUIdent[20];

extern int OutRadixBase, ListRadixBase;
extern Boolean ListPCZeroPad;
extern const char *pCommentLeadIn;

extern tStrComp *ArgStr;
extern tStrComp LabPart, CommPart, ArgPart, OpPart, AttrPart;
extern char AttrSplit;
extern Boolean oppart_leading_dot;
extern int ArgCnt;
extern as_dynstr_t OneLine;
#ifdef PROFILE_MEMO
extern unsigned NumMemo;
extern unsigned long NumMemoSum, NumMemoCnt;
#endif

#define forallargs(pArg, cond) \
        for (pArg = ArgStr + 1; (cond) && (pArg <= ArgStr + ArgCnt); pArg++)

extern Byte LstCounter;
extern Word PageCounter[ChapMax+1];
extern Byte ChapDepth;
extern StringPtr ListLine;
extern Byte PageLength, PageWidth;
extern tLstMacroExpMod LstMacroExpModOverride, LstMacroExpModDefault;
extern Boolean DottedStructs;
extern StringPtr PrtInitString;
extern StringPtr PrtExitString;
extern StringPtr PrtTitleString;

extern Byte StopfZahl;

extern Boolean SuppWarns;

extern PTransTable TransTables, CurrTransTable;

extern PDefinement FirstDefine;

extern PSaveState FirstSaveState;

extern Boolean MakeDebug;
extern FILE *Debug;

extern Boolean WasIF, WasMACRO;

extern void AsmDefInit(void);

extern void NullProc(void);

extern int SetMaxCodeLen(LongWord NewMaxCodeLen);

extern void Default_InternSymbol(char *Asc, TempResult *Erg);

extern void Default_DissectBit(char *pDest, size_t DestSize, LargeWord BitSpec);

extern void AppendArg(size_t ReqSize);
extern void InsertArg(int Index, size_t ReqSize);

extern Boolean memo_set_pseudo(void);
extern Boolean is_set_pseudo(void);
extern Boolean is_save_pseudo(void);
extern Boolean is_restore_pseudo(void);
extern Boolean memo_switch_pseudo(void);
extern Boolean memo_shift_pseudo(void);
extern Boolean is_page_pseudo(void);

extern void free_forward_symbol(PForwardSymbol p_symbol);

extern void asmdef_init(void);
#endif /* _ASMDEF_H */
