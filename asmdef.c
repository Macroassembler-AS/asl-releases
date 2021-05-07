/* asmdef.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* global benutzte Variablen                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <errno.h>

#include "strutil.h"
#include "stringlists.h"
#include "chunks.h"

#include "asmdef.h"
#include "asmsub.h"
#include "errmsg.h"

char SrcSuffix[] = ".asm";             /* Standardendungen: Hauptdatei */
char IncSuffix[] = ".inc";             /* Includedatei */
char PrgSuffix[] = ".p";               /* Programmdatei */
char LstSuffix[] = ".lst";             /* Listingdatei */
char MacSuffix[] = ".mac";             /* Makroausgabe */
char PreSuffix[] = ".i";               /* Ausgabe Makroprozessor */
char LogSuffix[] = ".log";             /* Fehlerdatei */
char MapSuffix[] = ".map";             /* Debug-Info/Map-Format */
char OBJSuffix[] = ".obj";

const char *EnvName = "ASCMD";         /* Environment-Variable fuer Default-
                                          Parameter */

const char *SegNames[PCMax + 2] =
{
  "NOTHING", "CODE", "DATA", "IDATA", "XDATA", "YDATA",
  "BITDATA", "IO", "REG", "ROMDATA", "EEDATA", "STRUCT"
};
char SegShorts[PCMax + 2] =
{
  '-','C','D','I','X','Y','B','P','R','O','E','S'
};

StringPtr SourceFile;                    /* Hauptquelldatei */

StringPtr CursUp;		            /*   "     "  Cursor hoch */

LargeWord *PCs;                          /* Programmzaehler */
Boolean RelSegs;                         /* relokatibles Segment ? */
LargeWord StartAdr;                      /* Programmstartadresse */
Boolean StartAdrPresent;                 /*          "           definiert? */
LargeWord AfterBSRAddr;                  /* address right behind last BSR */
LargeWord *Phases;                       /* Verschiebungen */
Word Grans[StructSeg + 1];               /* Groesse der Adressierungselemente */
Word ListGrans[StructSeg + 1];           /* Wortgroesse im Listing */
ChunkList SegChunks[StructSeg + 1];      /* Belegungen */
Integer ActPC;                           /* gewaehlter Programmzaehler */
Boolean PCsUsed[StructSeg + 1];          /* PCs bereits initialisiert ? */
LargeWord *SegInits;                     /* Segmentstartwerte */
LargeWord *SegLimits;                    /* Segmentgrenzwerte */
LongInt ValidSegs;                       /* erlaubte Segmente */
Boolean ENDOccured;	                 /* END-Statement aufgetreten ? */
Boolean Retracted;	                 /* Codes zurueckgenommen ? */
Boolean ListToStdout, ListToNull;        /* Listing auf Konsole/Nulldevice ? */

unsigned ASSUMERecCnt;
const ASSUMERec *pASSUMERecs;
void (*pASSUMEOverride)(void);

Integer PassNo;                          /* Durchlaufsnummer */
Integer JmpErrors;                       /* Anzahl fraglicher Sprungfehler */
Boolean ThrowErrors;                     /* Fehler verwerfen bei Repass ? */
LongWord MaxErrors;                      /* terminate upon n errors? */
Boolean TreatWarningsAsErrors;           /* treat warnings like erros? */
Boolean Repass;		                 /* noch ein Durchlauf erforderlich */
Byte MaxSymPass;	                 /* Pass, nach dem Symbole definiert sein muessen */
Byte ShareMode;                          /* 0=kein SHARED,1=Pascal-,2=C-Datei, 3=ASM-Datei */
DebugType DebugMode;                     /* Ausgabeformat Debug-Datei */
Word NoICEMask;                          /* which symbols to use in NoICE dbg file */
Byte ListMode;                           /* 0=kein Listing,1=Konsole,2=auf Datei */
Byte ListOn;		    	         /* Listing erzeugen ? */
Integer MaxIncludeLevel;                 /* maximum include nesting level */
Boolean MakeUseList;                     /* Belegungsliste ? */
Boolean MakeCrossList;	                 /* Querverweisliste ? */
Boolean MakeSectionList;                 /* Sektionsliste ? */
Boolean MakeIncludeList;                 /* Includeliste ? */
Boolean DefRelaxedMode;                  /* alle Integer-Syntaxen zulassen ? */
Boolean DefCompMode, CompMode;           /* enable compatibility mode */
Word ListMask;                           /* Listingmaske */
ShortInt ExtendErrors;	                 /* erweiterte Fehlermeldungen */
Integer EnumSegment;                     /* ENUM state & config */
LongInt EnumIncrement, EnumCurrentValue;
Boolean NumericErrors;                   /* Fehlermeldungen mit Nummer */
Boolean CodeOutput;		         /* Code erzeugen */
Boolean MacProOutput;                    /* Makroprozessorausgabe schreiben */
Boolean MacroOutput;                     /* gelesene Makros schreiben */
Boolean QuietMode;                       /* keine Meldungen */
Boolean HardRanges;                      /* Bereichsfehler echte Fehler ? */
const char *DivideChars;                 /* Trennzeichen fuer Parameter. Inhalt Read Only! */
Boolean HasAttrs;                        /* Opcode hat Attribut */
const char *AttrChars;                   /* Zeichen, mit denen Attribut abgetrennt wird */
Boolean MsgIfRepass;                     /* Meldungen, falls neuer Pass erforderlich */
Integer PassNoForMessage;                /* falls ja: ab welchem Pass ? */
Boolean CaseSensitive;                   /* Gross/Kleinschreibung unterscheiden ? */
LongInt NestMax;                         /* max. nesting level of a macro */
Boolean GNUErrors;                       /* GNU-error-style messages ? */

FILE *PrgFile = NULL;                    /* Codedatei */

StringPtr ErrorPath, ErrorName;          /* Ausgabedatei Fehlermeldungen */
StringPtr OutName;                       /* Name Code-Datei */
Integer CurrIncludeLevel;                /* current include nesting level */
StringPtr CurrFileName;                  /* mom. bearbeitete Datei */
LongInt MomLineCounter;                  /* Position in mom. Datei */
LongInt CurrLine;       	         /* virtuelle Position */
LongInt LineSum;                         /* Gesamtzahl Quellzeilen */
LongInt MacLineSum;                      /* inkl. Makroexpansion */

LongInt NOPCode;                         /* Maschinenbefehl NOP zum Stopfen */
Boolean TurnWords;                       /* TRUE  = Motorola-Wortformat */
                                         /* FALSE = Intel-Wortformat */
Byte HeaderID;	                         /* Kennbyte des Codeheaders */
const char *PCSymbol;	                 /* Symbol, womit Programmzaehler erreicht wird. Inhalt Read Only! */
Boolean (*SetIsOccupiedFnc)(void),       /* TRUE: SET instr, to be parsed by code generator */
        (*SaveIsOccupiedFnc)(void),      /* ditto for SAVE */
        (*RestoreIsOccupiedFnc)(void);   /* ditto for RESTORE */
Boolean SwitchIsOccupied,                /* TRUE: SWITCH/PAGE/SHIFT ist Prozessorbefehl */
        PageIsOccupied,
        ShiftIsOccupied;
#ifdef __PROTOS__
Boolean (*DecodeAttrPart)(void);         /* dissect attribute of instruction */
void (*MakeCode)(void);                  /* Codeerzeugungsprozedur */
Boolean (*ChkPC)(LargeWord Addr);        /* ueberprueft Codelaengenueberschreitungen */
Boolean (*IsDef)(void);	                 /* ist Label nicht als solches zu werten ? */
void (*SwitchFrom)(void) = NULL;         /* bevor von einer CPU weggeschaltet wird */
void (*InternSymbol)(char *Asc, TempResult *Erg); /* vordefinierte Symbole ? */
#else
Boolean (*DecodeAttrPart)();
void (*MakeCode)();
Boolean (*ChkPC)();
Boolean (*IsDef)();
void (*SwitchFrom)();
void (*InternSymbol)();
#endif
DissectBitProc DissectBit;
DissectRegProc DissectReg;
tQualifyQuoteFnc QualifyQuote;

StringPtr IncludeList;	                /* Suchpfade fuer Includedateien */
Integer IncDepth, NextIncDepth;         /* Verschachtelungstiefe INCLUDEs */
FILE *ErrorFile = NULL;                 /* Fehlerausgabe */
FILE *LstFile = NULL;                   /* Listdatei */
FILE *ShareFile = NULL;                 /* Sharefile */
FILE *MacProFile = NULL;                /* Makroprozessorausgabe */
FILE *MacroFile = NULL;                 /* Ausgabedatei Makroliste */
Boolean InMacroFlag;                    /* momentan wird Makro expandiert */
StringPtr LstName;                      /* Name der Listdatei */
StringPtr MacroName, MacProName;
tLstMacroExp DoLst, NextDoLst;          /* Listing an */
StringPtr ShareName;                    /* Name des Sharefiles */

CPUVar MomCPU, MomVirtCPU;              /* definierter/vorgegaukelter Prozessortyp */
StringPtr MomCPUArgs;                   /* Arguments for Current Processor Type */
char DefCPU[20];                        /* per Kommandozeile vorgegebene CPU */
char MomCPUIdent[20],                   /* dessen Name in ASCII */
     MomFPUIdent[20],                   /* ditto FPU */
     MomPMMUIdent[20];                  /* ditto PMMU */

Boolean FPUAvail,                       /* Koprozessor erlaubt ? */
        PMMUAvail;                      /* MMU-Befehle erlaubt? */
Boolean DoPadding,                      /* auf gerade Byte-Zahl ausrichten ? */
        TargetBigEndian;                /* Datenablage Big Endian? */
Boolean Packing;                        /* gepackte Ablage ? */
Boolean DefSupAllowed, SupAllowed;      /* Supervisormode freigegeben */
Boolean Maximum;                        /* CPU nicht kastriert */
Boolean DoBranchExt;                    /* Spruenge automatisch verlaengern */

int OutRadixBase;                       /* dito fuer Ausgabe */
int ListRadixBase;                      /* ditto for listing */
const char *pCommentLeadIn;             /* list of comment lead-in sequences */

tStrComp *ArgStr;                       /* Komponenten der Zeile */
StringPtr pLOpPart;
tStrComp LabPart, CommPart, ArgPart, OpPart, AttrPart;
char AttrSplit;
int ArgCnt;                             /* Argumentzahl */
int AllocArgCnt;
StringPtr OneLine;                      /* eingelesene Zeile */
#ifdef PROFILE_MEMO
unsigned NumMemo;
unsigned long NumMemoCnt, NumMemoSum;
#endif

Byte LstCounter;                        /* Zeilenzaehler fuer automatischen Umbruch */
Word PageCounter[ChapMax + 1];          /* hierarchische Seitenzaehler */
Byte ChapDepth;                         /* momentane Kapitelverschachtelung */
StringPtr ListLine;                     /* alternative Ausgabe vor Listing fuer EQU */
Byte PageLength, PageWidth;             /* Seitenlaenge/breite in Zeilen/Spalten */
tLstMacroExpMod LstMacroExpModOverride, /* Override macro expansion ? */
                LstMacroExpModDefault;
Boolean DottedStructs;                  /* structure elements with dots */
StringPtr PrtInitString;                /* Druckerinitialisierungsstring */
StringPtr PrtExitString;                /* Druckerdeinitialisierungsstring */
StringPtr PrtTitleString;               /* Titelzeile */

LongInt MomSectionHandle;               /* mom. Namensraum */
PSaveSection SectionStack;              /* gespeicherte Sektionshandles */
tSavePhase *pPhaseStacks[PCMax];	/* saves nested PHASE values */

tSymbolSize AttrPartOpSize;             /* instruction operand size deduced from insn attribute */
LongWord MaxCodeLen = 0;                /* max. length of generated code */
LongInt CodeLen;                        /* Laenge des erzeugten Befehls */
LongWord *DAsmCode;                     /* Zwischenspeicher erzeugter Code */
Word *WAsmCode;
Byte *BAsmCode;

Boolean DontPrint;                      /* Flag:PC veraendert, aber keinen Code erzeugt */
Word ActListGran;                       /* uebersteuerte List-Granularitaet */

Byte StopfZahl;                         /* Anzahl der im 2.Pass festgestellten
                                           ueberfluessigen Worte, die mit NOP ge-
                                           fuellt werden muessen */

Boolean SuppWarns;

PTransTable TransTables,                /* Liste mit Codepages */
            CurrTransTable;             /* aktuelle Codepage */

PDefinement FirstDefine;                /* Liste von Praeprozessor-Defines */

PSaveState FirstSaveState;              /* gesicherte Zustaende */

Boolean MakeDebug;                      /* Debugginghilfe */
FILE *Debug;

Boolean WasIF, WasMACRO;

void AsmDefInit(void)
{
  LongInt z;

  DoLst = eLstMacroExpAll;
  PassNo = 1;
  MaxSymPass = 1;

  LineSum = 0;

  for (z = 0; z <= ChapMax; PageCounter[z++] = 0);
  LstCounter = 0;
  ChapDepth = 0;

  PrtInitString[0] = '\0';
  PrtExitString[0] = '\0';
  PrtTitleString[0] = '\0';

  CurrFileName[0] = '\0';
  MomLineCounter = 0;

  FirstDefine = NULL;
  FirstSaveState = NULL;
}

void NullProc(void)
{
}

void Default_InternSymbol(char *Asc, TempResult *Erg)
{
  UNUSED(Asc);

  Erg->Typ = TempNone;
}

void Default_DissectBit(char *pDest, size_t DestSize, LargeWord BitSpec)
{
  HexString(pDest, DestSize, BitSpec, 0);
}

static char *GetString(void)
{
  return (char*)malloc(STRINGSIZE * sizeof(char));
}

int SetMaxCodeLen(LongWord NewMaxCodeLen)
{
  if (NewMaxCodeLen > MaxCodeLen_Max)
    return ENOMEM;
  if (NewMaxCodeLen > MaxCodeLen)
  {
    void *pNewMem;

    if (!MaxCodeLen)
      pNewMem = (LongWord *) malloc(NewMaxCodeLen);
    else
      pNewMem = (LongWord *) realloc(DAsmCode, NewMaxCodeLen);
    if (!pNewMem)
      return ENOMEM;

    DAsmCode = (LongWord *)pNewMem;
    WAsmCode = (Word *) DAsmCode;
    BAsmCode = (Byte *) DAsmCode;
    MaxCodeLen = NewMaxCodeLen;
  }
  return 0;
}

void IncArgCnt(void)
{
  if (ArgCnt >= ArgCntMax)
    WrXError(ErrNum_InternalError, "MaxArgCnt");
  ++ArgCnt;
  if (ArgCnt >= AllocArgCnt)
  {
    unsigned NewSize = sizeof(*ArgStr) * (ArgCnt + 1); /* one more, [0] is unused */
    int z;

    ArgStr = ArgStr ? (tStrComp*)realloc(ArgStr, NewSize) : (tStrComp*)malloc(NewSize);
    for (z = AllocArgCnt; z <= ArgCnt; z++)
      StrCompAlloc(&ArgStr[z]);
    AllocArgCnt = ArgCnt + 1;
  }
}

Boolean SetIsOccupied(void)
{
  return SetIsOccupiedFnc && SetIsOccupiedFnc();
}

Boolean SaveIsOccupied(void)
{
  return SaveIsOccupiedFnc && SaveIsOccupiedFnc();
}

Boolean RestoreIsOccupied(void)
{
  return RestoreIsOccupiedFnc && RestoreIsOccupiedFnc();
}

void asmdef_init(void)
{
  SwitchFrom = NullProc;
  InternSymbol = Default_InternSymbol;
  DissectBit = Default_DissectBit;
  DissectReg = NULL;
  QualifyQuote = NULL;

  SetMaxCodeLen(MaxCodeLen_Ini);

  /* auf diese Weise wird PCSymbol defaultmaessig nicht erreichbar
     da das schon von den Konstantenparsern im Formelparser abgefangen
     wuerde */

  PCSymbol = "--PC--SYMBOL--";
  *DefCPU = '\0';

  ArgStr = NULL;
  AllocArgCnt = 0;
  SourceFile = GetString();
  CursUp = GetString();
  ErrorPath = GetString();
  ErrorName = GetString();
  OutName = GetString();
  CurrFileName = GetString();
  IncludeList = GetString();
  LstName = GetString();
  MacroName = GetString();
  MacProName = GetString();
  ShareName = GetString();
  StrCompAlloc(&LabPart);
  StrCompAlloc(&OpPart);
  StrCompAlloc(&AttrPart);
  StrCompAlloc(&ArgPart);
  StrCompAlloc(&CommPart);
  pLOpPart = GetString();
  OneLine = GetString();
  ListLine = GetString();
  PrtInitString = GetString();
  PrtExitString = GetString();
  PrtTitleString = GetString();
  MomCPUArgs = GetString();

  SegInits = (LargeWord*)malloc((PCMax + 1) * sizeof(LargeWord));
  SegLimits = (LargeWord*)malloc((PCMax + 1) * sizeof(LargeWord));
  Phases = (LargeWord*)malloc((StructSeg + 1) * sizeof(LargeWord));
  PCs = (LargeWord*)malloc((StructSeg + 1) * sizeof(LargeWord));
}
