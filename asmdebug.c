/* asmdebug.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung der Debug-Informationen zur Assemblierzeit                     */
/*                                                                           */
/*****************************************************************************/


#include "stdinc.h"
#include <string.h>
#include "strutil.h"

#include "endian.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmfnums.h"
#include "errmsg.h"

#include "asmdebug.h"


typedef struct
{
  Boolean InMacro;
  LongInt LineNum;
  Integer FileName;
  ShortInt Space;
  LargeInt Address;
  Word Code;
} TLineInfo;

typedef struct sLineInfoList
{
  struct sLineInfoList *Next;
  TLineInfo Contents;
} TLineInfoList, *PLineInfoList;

String TempFileName;
FILE *TempFile;
PLineInfoList LineInfoRoot;


void AddLineInfo(Boolean InMacro, LongInt LineNum, char *FileName,
                 ShortInt Space, LargeInt Address, LargeInt Len)
{
  PLineInfoList PNeu, PFirst, PLast, Run, Link;
  int RecCnt, z;
  Integer FNum;

  /* do not accept line infor for pseudo segments */

  if (Space >= SegCount)
    return;

  /* wieviele Records schreiben ? */

  RecCnt = ((DebugMode == DebugAtmel) && (CodeLen > 1)) ? CodeLen : 1;

  FNum = GetFileNum(FileName);

  /* Einfuegepunkt in Liste finden */

  Run = LineInfoRoot;
  if (!Run)
    Link = NULL;
  else
  {
    while ((Run->Next) && (Run->Next->Contents.Space < Space))
      Run = Run->Next;
    while ((Run->Next) && (Run->Next->Contents.FileName < FNum))
      Run = Run->Next;
    while ((Run->Next) && (Run->Next->Contents.Address < Address))
      Run = Run->Next;
    Link = Run->Next;
  }

  /* neue Teilliste bilden */

  PLast = PFirst = NULL;
  for (z = 0; z < RecCnt; z++)
  {
    PNeu = (PLineInfoList) malloc(sizeof(TLineInfoList));
    PNeu->Contents.InMacro = InMacro;
    PNeu->Contents.LineNum = LineNum;
    PNeu->Contents.FileName = FNum;
    PNeu->Contents.Space = Space;
    PNeu->Contents.Address = Address + z;
    PNeu->Contents.Code = ((CodeLen < z + 1) || (DontPrint)) ? 0 : WAsmCode[z];
    if (z == 0)
      PFirst = PNeu;
    if (PLast != NULL)
      PLast->Next = PNeu;
    PLast = PNeu;
  }

  /* Teilliste einhaengen */

  if (!Run)
    LineInfoRoot = PFirst;
  else
    Run->Next = PFirst;
  PLast->Next = Link;

  if (Space == SegCode)
    AddAddressRange(FNum, Address, Len);
}


void InitLineInfo(void)
{
  TempFileName[0] = '\0';
  LineInfoRoot = NULL;
}


void ClearLineInfo(void)
{
  PLineInfoList Run;

  if (TempFileName[0] != '\0')
  {
    fclose(TempFile);
    unlink(TempFileName);
  }

  while (LineInfoRoot)
  {
    Run = LineInfoRoot;
    LineInfoRoot = LineInfoRoot->Next;
    free(Run);
  }

  InitLineInfo();
}


static void DumpDebugInfo_MAP(void)
{
  PLineInfoList Run;
  Integer ActFile;
  int ModZ;
  ShortInt ActSeg;
  FILE *MAPFile;
  String MAPName;
  char Tmp[30], Tmp2[30];

  strmaxcpy(MAPName, SourceFile, STRINGSIZE);
  KillSuffix(MAPName);
  AddSuffix(MAPName, MapSuffix);
  MAPFile = fopen(MAPName, "w");
  if (!MAPFile)
    ChkIO(ErrNum_OpeningFile);

  Run = LineInfoRoot;
  ActSeg = -1;
  ActFile = -1;
  ModZ = 0;
  while (Run)
  {
    if (Run->Contents.Space != ActSeg)
    {
      ActSeg = Run->Contents.Space;
      if (ModZ != 0)
      {
        errno = 0; fprintf(MAPFile, "\n"); ChkIO(ErrNum_FileWriteError);
      }
      ModZ = 0;
      errno = 0; fprintf(MAPFile, "Segment %s\n", SegNames[ActSeg]); ChkIO(ErrNum_FileWriteError);
      ActFile = -1;
    }
    if (Run->Contents.FileName != ActFile)
    {
      ActFile = Run->Contents.FileName;
      if (ModZ != 0)
      {
        errno = 0; fprintf(MAPFile, "\n"); ChkIO(ErrNum_FileWriteError);
      }
      ModZ = 0;
      errno = 0; fprintf(MAPFile, "File %s\n", GetFileName(Run->Contents.FileName)); ChkIO(ErrNum_FileWriteError);
    };
    errno = 0;
    as_snprintf(Tmp, sizeof(Tmp), LongIntFormat, Run->Contents.LineNum);
    HexString(Tmp2, sizeof(Tmp2), Run->Contents.Address, 8);
    fprintf(MAPFile, "%5s:%s ", Tmp, Tmp2);
    ChkIO(ErrNum_FileWriteError);
    if (++ModZ == 5)
    {
      errno = 0; fprintf(MAPFile, "\n"); ChkIO(ErrNum_FileWriteError); ModZ = 0;
    }
    Run = Run->Next;
  }
  if (ModZ != 0)
  {
    errno = 0; fprintf(MAPFile, "\n"); ChkIO(ErrNum_FileWriteError);
  }

  PrintDebSymbols(MAPFile);

  PrintDebSections(MAPFile);

  fclose(MAPFile);
}

static void DumpDebugInfo_Atmel(void)
{
  static const char *OBJString = "AVR Object File";
  PLineInfoList Run;
  LongInt FNamePos, RecPos;
  FILE *OBJFile;
  String OBJName;
  const char *FName;
  Byte TByte, TNum, NameCnt;
  int z;
  LongInt LTurn;
  Word WTurn;

  strmaxcpy(OBJName, SourceFile, STRINGSIZE);
  KillSuffix(OBJName);
  AddSuffix(OBJName, OBJSuffix);
  OBJFile = fopen(OBJName, OPENWRMODE);
  if (!OBJFile)
    ChkIO(ErrNum_OpeningFile);

  /* initialer Kopf, Positionen noch unbekannt */

  FNamePos = 0;
  RecPos = 0;
  if (!Write4(OBJFile, &FNamePos)) ChkIO(ErrNum_FileWriteError);
  if (!Write4(OBJFile, &RecPos)) ChkIO(ErrNum_FileWriteError);
  TByte = 9;
  if (fwrite(&TByte, 1, 1, OBJFile) != 1) ChkIO(ErrNum_FileWriteError);
  NameCnt = GetFileCount() - 1;
  if (fwrite(&NameCnt, 1, 1, OBJFile) != 1) ChkIO(ErrNum_FileWriteError);
  if ((int)fwrite(OBJString, 1, strlen(OBJString) + 1, OBJFile) != (int)strlen(OBJString) + 1) ChkIO(ErrNum_FileWriteError);

  /* Objekt-Records */

  RecPos = ftell(OBJFile);
  for (Run = LineInfoRoot; Run; Run = Run->Next)
    if (Run->Contents.Space == SegCode)
    {
      LTurn = Run->Contents.Address;
      if (!HostBigEndian)
        DSwap(&LTurn, 4);
      if (fwrite(((Byte *) &LTurn)+1, 1, 3, OBJFile) != 3) ChkIO(ErrNum_FileWriteError);
      WTurn = Run->Contents.Code;
      if (!HostBigEndian)
        WSwap(&WTurn, 2);
      if (fwrite(&WTurn, 1, 2, OBJFile) != 2) ChkIO(ErrNum_FileWriteError);
      TNum = Run->Contents.FileName - 1;
      if (fwrite(&TNum, 1, 1, OBJFile) != 1) ChkIO(ErrNum_FileWriteError);
      WTurn = Run->Contents.LineNum;
      if (!HostBigEndian)
        WSwap(&WTurn, 2);
      if (fwrite(&WTurn, 1, 2, OBJFile) != 2) ChkIO(ErrNum_FileWriteError);
      TNum = Ord(Run->Contents.InMacro);
      if (fwrite(&TNum, 1, 1, OBJFile) != 1) ChkIO(ErrNum_FileWriteError);
    }

  /* Dateinamen */

  FNamePos = ftell(OBJFile);
  for (z = 1; z <= NameCnt; z++)
  {
    FName = NamePart(GetFileName(z));
    if ((int)fwrite(FName, 1, strlen(FName) + 1, OBJFile) != (int)strlen(FName) + 1) ChkIO(ErrNum_FileWriteError);
  }
  TByte = 0;
  if (fwrite(&TByte, 1, 1, OBJFile) != 1) ChkIO(ErrNum_FileWriteError);

  /* korrekte Positionen in Kopf schreiben */

  rewind(OBJFile);
  if (!HostBigEndian) DSwap(&FNamePos, 4);
  if (fwrite(&FNamePos, 1, 4, OBJFile) != 4) ChkIO(ErrNum_FileWriteError);
  if (!HostBigEndian) DSwap(&RecPos, 4);
  if (fwrite(&RecPos, 1, 4, OBJFile) != 4) ChkIO(ErrNum_FileWriteError);

  fclose(OBJFile);
}

static void DumpDebugInfo_NOICE(void)
{
  PLineInfoList Run;
  Integer ActFile;
  FILE *MAPFile;
  String MAPName, Tmp1, Tmp2;
  LargeWord Start, End;
  Boolean HadLines;

  strmaxcpy(MAPName, SourceFile, STRINGSIZE);
  KillSuffix(MAPName);
  AddSuffix(MAPName, ".noi");
  MAPFile = fopen(MAPName, "w");
  if (!MAPFile)
    ChkIO(ErrNum_OpeningFile);

  fprintf(MAPFile, "CASE %d\n", (CaseSensitive) ? 1 : 0);

  PrintNoISymbols(MAPFile);

  for (ActFile = 0; ActFile < GetFileCount(); ActFile++)
  {
    HadLines = FALSE;
    Run = LineInfoRoot;
    while (Run)
    {
      if ((Run->Contents.Space == SegCode) && (Run->Contents.FileName == ActFile))
      {
        if (!HadLines)
        {
          GetAddressRange(ActFile, &Start, &End);
          as_snprintf(Tmp1, sizeof(Tmp1), "%X", Start);
          errno = 0;
          fprintf(MAPFile, "FILE %s 0x%s\n", GetFileName(Run->Contents.FileName), Tmp1);
          ChkIO(ErrNum_FileWriteError);
        }
        errno = 0;
        as_snprintf(Tmp1, sizeof(Tmp1), LongIntFormat, Run->Contents.LineNum);
        as_snprintf(Tmp2, sizeof(Tmp2), "%X", Run->Contents.Address - Start);
        fprintf(MAPFile, "LINE %s 0x%s\n", Tmp1, Tmp2);
        ChkIO(ErrNum_FileWriteError);
        HadLines = TRUE;
      }
      Run = Run->Next;
    }
    if (HadLines)
    {
      as_snprintf(Tmp1, sizeof(Tmp1), "%X", End);
      errno = 0; fprintf(MAPFile, "ENDFILE 0x%s\n", Tmp1); ChkIO(ErrNum_FileWriteError);
    }
  }

  fclose(MAPFile);
}


void DumpDebugInfo(void)
{
  switch (DebugMode)
  {
    case DebugMAP:
      DumpDebugInfo_MAP();
      break;
    case DebugAtmel:
      DumpDebugInfo_Atmel();
      break;
    case DebugNoICE:
      DumpDebugInfo_NOICE();
      break;
    default:
      break;
  }
}


void asmdebug_init(void)
{
}
