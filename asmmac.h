#ifndef _ASMMAC_H
#define _ASMMAC_H
/* asmmac.h  */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Unterroutinen des Makroprozessors                                         */
/*                                                                           */
/* Historie: 16. 5.1996 Grundsteinlegung                                     */
/*           2001-12-31 added DoIntLabel flag                                */
/*           2002-03-03 added FromFile flag, LineRun pointer to input tag    */
/*                                                                           */
/*****************************************************************************/

#include "errmsg.h"
#include "strcomp.h"

typedef struct _MacroRec
{
  char *Name;            /* Name des Makros */
  Byte ParamCount;       /* Anzahl Parameter */
  StringList FirstLine;  /* Zeiger auf erste Zeile */
  StringList ParamNames; /* parameter names, needed for named parameters */
  StringList ParamDefVals; /* default values */
  LongInt UseCounter;    /* to limit recursive calls */
  tLstMacroExpMod LstMacroExpMod; /* modify macro expansion list subset */
  Boolean LocIntLabel;   /* Label used internally instead at beginning */
  Boolean GlobalSymbols; /* labels not local to macro */
  Boolean UsesNumArgs,   /* NUMARGS referenced in macro body */
          UsesAllArgs;   /* ALLARGS referenced in macro body */
} MacroRec, *PMacroRec;

#define BufferArraySize 1024

struct as_dynstr;

typedef struct _TInputTag
{
  struct _TInputTag *Next;
  Boolean IsMacro;
  Integer IfLevel, IncludeLevel;
  Boolean First;
  Boolean GlobalSymbols;
  tLstMacroExp OrigDoLst;
  LongInt StartLine;
  Boolean (*Processor)(
#ifdef __PROTOS__
                       struct _TInputTag *P, struct as_dynstr *p_dest
#endif
                                                      );
  LongInt ParCnt,ParZ;
  StringList Params;
  LongInt LineCnt, ContLineCnt, LineZ;
  StringRecPtr Lines, LineRun;
  String SpecNameStr, SaveAttr, SaveLabel, AllArgs;
  tStrComp SpecName;
  ShortString NumArgs;
  Boolean IsEmpty, FromFile, UsesAllArgs, UsesNumArgs;
  FILE *Datei;
  void *Buffer;
  void (*Cleanup)(
#ifdef __PROTOS__
                  struct _TInputTag *P
#endif
                                      );
  void (*Restorer)(
#ifdef __PROTOS__
                   struct _TInputTag *P
#endif
                                       );
  Boolean (*GetPos)(
#ifdef __PROTOS__
                    struct _TInputTag *P, char *Dest, size_t DestSize, Boolean ActGNUErrors
#endif
                                                    );
  PMacroRec Macro;
} TInputTag, *PInputTag;

typedef struct _TOutputTag
{
  struct _TOutputTag *Next;
  void (*Processor)(
#ifdef __PROTOS__
                    void
#endif
                        );
  Integer NestLevel;
  PInputTag Tag;
  PMacroRec Mac;
  StringList ParamNames, ParamDefVals;
  LongInt PubSect,GlobSect;
  Boolean DoExport, DoGlobCopy, UsesNumArgs, UsesAllArgs;
  String GName;
  tErrorNum OpenErrMsg;
} TOutputTag, *POutputTag;


extern PInputTag FirstInputTag;
extern POutputTag FirstOutputTag;

struct sStrComp;

extern void Preprocess(void);


extern void AddMacro(PMacroRec Neu, LongInt DefSect, Boolean Protest);

extern Boolean FoundMacro(PMacroRec *Erg, const struct sStrComp *p_name);

extern void ClearMacroList(void);

extern void ResetMacroDefines(void);

extern void ClearMacroRec(PMacroRec *Alt, Boolean Complete);

extern void PrintMacroList(void);


extern void PrintDefineList(void);

extern void ClearDefineList(void);

extern void ExpandDefines(char *Line);


extern void asmmac_init(void);
#endif /* _ASMMAC_H */
