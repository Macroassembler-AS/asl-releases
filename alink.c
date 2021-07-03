/* alink.c */
/****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                    */
/*                                                                          */
/* AS-Portierung                                                            */
/*                                                                          */
/* Linking of AS Code Files                                                 */
/*                                                                          */
/* History:  2. 7.2000 begun                                                */
/*           4. 7.2000 read symbols                                         */
/*           6. 7.2000 start real relocation                                */
/*           7. 7.2000 simple relocations                                   */
/*          30.10.2000 added 1-byte relocations, verbosity levels           */
/*           14. 1.2001 silenced warnings about unused parameters           */
/*                                                                          */
/****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "version.h"

#include "endian.h"
#include "bpemu.h"
#include "strutil.h"
#include "nls.h"
#include "nlmessages.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "toolutils.h"

#include "ioerrs.h"

#include "alink.rsc"

/****************************************************************************/
/* Macros */

/****************************************************************************/
/* Types */

typedef struct sPart
{
  struct sPart *Next;
  int FileNum, RecNum;
  LargeWord CodeStart, CodeLen;
  Byte Gran, Segment;
  Boolean MustReloc;
  PRelocInfo RelocInfo;
} TPart, *PPart;

/****************************************************************************/
/* Variables */

static Boolean DoubleErr;
static int Verbose;
static LongWord UndefErr;

static String TargName;

static CMDProcessed ParUnprocessed;

static PPart PartList, PartLast;

static FILE *TargFile;

static Byte *Buffer;
LongInt BufferSize;

static LargeWord SegStarts[SegCount];

/****************************************************************************/
/* increase buffer if necessary */

static void BumpBuffer(LongInt MinSize)
{
  if (MinSize > BufferSize)
  {
    Buffer = (Byte*) ((BufferSize == 0) ? malloc(sizeof(Byte) * MinSize)
                                        : realloc(Buffer, sizeof(Byte) * MinSize));
    BufferSize = MinSize;
  }
}

/****************************************************************************/
/* reading/patching in buffer */

static LargeWord GetValue(LongInt Type, LargeWord Offset)
{
  switch (Type & ~(RelocFlagSUB | RelocFlagPage))
  {
    case RelocTypeL8:
      return MRead1L(Buffer + Offset);
    case RelocTypeL16:
      return MRead2L(Buffer + Offset);
    case RelocTypeB16:
      return MRead2B(Buffer + Offset);
    case RelocTypeL32:
      return MRead4L(Buffer + Offset);
    case RelocTypeB32:
      return MRead4B(Buffer + Offset);
#ifdef HAS64
    case RelocTypeL64:
      return MRead8L(Buffer + Offset);
    case RelocTypeB64:
      return MRead8B(Buffer + Offset);
#endif
  }
  fprintf(stderr, "unknown relocation type: 0x");
  fprintf(stderr, LongIntFormat, Type);
  fprintf(stderr, "\n");
  exit(3);
}

static void PutValue(LargeWord Value, LongInt Type, LargeWord Offset)
{
  switch (Type & ~(RelocFlagSUB | RelocFlagPage))
  {
    case RelocTypeL8:
      MWrite1L(Buffer + Offset, Value);
      break;
    case RelocTypeL16:
      MWrite2L(Buffer + Offset, Value);
      break;
    case RelocTypeB16:
      MWrite2B(Buffer + Offset, Value);
      break;
    case RelocTypeL32:
      MWrite4L(Buffer + Offset, Value);
      break;
    case RelocTypeB32:
      MWrite4B(Buffer + Offset, Value);
      break;
#ifdef HAS64
    case RelocTypeL64:
      MWrite8L(Buffer + Offset, Value);
      break;
    case RelocTypeB64:
      MWrite8B(Buffer + Offset, Value);
      break;
#endif
    default:
      fprintf(stderr, "unknown relocation type: 0x");
      fprintf(stderr, LongIntFormat, Type);
      fprintf(stderr, "\n");
      exit(3);
  }
}

/****************************************************************************/
/* get the value of an exported symbol */

static Boolean GetExport(char *Name, LargeWord *Result)
{
  PPart PartRun;
  LongInt z;

  for (PartRun = PartList; PartRun; PartRun = PartRun->Next)
   for (z = 0; z < PartRun->RelocInfo->ExportCount; z++)
    if (!strcmp(Name, PartRun->RelocInfo->ExportEntries[z].Name))
    {
      *Result = PartRun->RelocInfo->ExportEntries[z].Value;
      return True;
    }

  return False;
}

/****************************************************************************/
/* read the symbol exports/relocations from a file */

static void ReadSymbols(const char *pSrcName, int Index)
{
  FILE *f;
  String SrcName;
  Byte Header, CPU, Gran, Segment;
  PPart PNew;
  LongWord Addr;
  Word Len;
  int cnt;

  /* open this file - we're only reading */

  strmaxcpy(SrcName, pSrcName, STRINGSIZE);
  DelSuffix(SrcName); AddSuffix(SrcName, STRINGSIZE, getmessage(Num_Suffix));
  if (Verbose >= 2)
    printf("%s '%s'...\n", getmessage(Num_InfoMsgGetSyms), SrcName);
  f = fopen(SrcName, OPENRDMODE);
  if (!f)
    ChkIO(SrcName);

  /* check magic */

  if (!Read2(f, &Len)) ChkIO(SrcName);
  if (Len != FileMagic)
    FormatError(SrcName, getmessage(Num_FormatInvHeaderMsg));

  /* step through records */

  cnt = 0;
  while (!feof(f))
  {
    /* read a record's header */

    ReadRecordHeader(&Header, &CPU, &Segment, &Gran, SrcName, f);

    /* for absolute records, only store address position */
    /* for relocatable records, also read following relocation record */

    if ((Header == FileHeaderDataRec) || (Header == FileHeaderRDataRec)
     || (Header == FileHeaderRelocRec) || (Header == FileHeaderRRelocRec))
    {
      /* build up record */

      PNew = (PPart) malloc(sizeof(TPart));
      PNew->Next = NULL;
      PNew->FileNum = Index;
      PNew->RecNum = cnt++;
      PNew->Gran = Gran;
      PNew->Segment = Segment;
      if (!Read4(f, &Addr))
        ChkIO(SrcName);
      PNew->CodeStart = Addr;
      if (!Read2(f, &Len))
        ChkIO(SrcName);
      PNew->CodeLen = Len;
      PNew->MustReloc = ((Header == FileHeaderRelocRec)
                      || (Header == FileHeaderRRelocRec));

      /* skip code */

      if (fseek(f, Len, SEEK_CUR) != 0)
        ChkIO(SrcName);

      /* relocatable record must be followed by relocation data */

      if ((Header == FileHeaderRDataRec) || (Header == FileHeaderRRelocRec))
      {
        LongInt z;
        LargeWord Dummy;

        ReadRecordHeader(&Header, &CPU, &Segment, &Gran, SrcName, f);
        if (Header != FileHeaderRelocInfo)
          FormatError(SrcName, getmessage(Num_FormatRelocInfoMissing));
        PNew->RelocInfo = ReadRelocInfo(f);
        if (!PNew->RelocInfo)
          ChkIO(SrcName);

        /* check for double-defined symbols */

        for (z = 0; z < PNew->RelocInfo->ExportCount; z++)
          if (GetExport(PNew->RelocInfo->ExportEntries[z].Name, &Dummy))
          {
            fprintf(stderr, "%s: %s '%s'\n",
                    SrcName, getmessage(Num_DoubleDefSymbol),
                    PNew->RelocInfo->ExportEntries[z].Name);
            DoubleErr = True;
          }
      }
      else
        PNew->RelocInfo = NULL;

      /* put into list */

      if (!PartList)
        PartList = PNew;
      else
        PartLast->Next = PNew;
      PartLast = PNew;
    }

    /* end of file ? */

    else if (Header == FileHeaderEnd)
      break;

    /* skip everything else */

    else
      SkipRecord(Header, SrcName, f);
  }

  /* close again */

  fclose(f);
}

/****************************************************************************/
/* do the relocation */

static void ProcessFile(const char *pSrcName, int Index)
{
  FILE *f;
  String SrcName;
  PPart PartRun;
  Byte Header, CPU, Gran, Segment;
  LongInt Addr, z;
  LargeWord Value, RelocVal, NRelocVal;
  Word Len, Magic;
  LongWord SumLen;
  PRelocEntry PReloc;
  Boolean UndefFlag, Found;

  /* open this file - we're only reading */

  strmaxcpy(SrcName, pSrcName, STRINGSIZE);
  DelSuffix(SrcName); AddSuffix(SrcName, STRINGSIZE, getmessage(Num_Suffix));
  if (Verbose >= 2)
    printf("%s '%s'...", getmessage(Num_InfoMsgOpenSrc), SrcName);
  else if (Verbose >= 1)
    printf("%s", SrcName);
  f = fopen(SrcName, OPENRDMODE);
  if (!f)
    ChkIO(SrcName);

  /* check magic */

  if (!Read2(f, &Magic))
    ChkIO(SrcName);
  if (Magic != FileMagic)
    FormatError(SrcName, getmessage(Num_FormatInvHeaderMsg));

  /* due to the way we built the part list, all parts of a file are
     sequentially in the list.  Therefore we only have to look for
     the first part of this file in the list and know the remainders
     will follow sequentially. */

  for (PartRun = PartList; PartRun; PartRun = PartRun->Next)
    if (PartRun->FileNum >= Index)
      break;

  /* now step through the records */

  SumLen = 0;
  while (!feof(f))
  {
    /* get header */

    ReadRecordHeader(&Header, &CPU, &Segment, &Gran, SrcName, f);

    /* records without relocation info do not need any processing - just
       pass them through */

    if ((Header == FileHeaderDataRec) || (Header == FileHeaderRelocRec))
    {
      if (!Read4(f, &Addr))
        ChkIO(SrcName);
      if (!Read2(f, &Len))
        ChkIO(SrcName);
      BumpBuffer(Len);
      if (fread(Buffer, 1, Len, f) != Len)
        ChkIO(SrcName);
      WriteRecordHeader(&Header, &CPU, &Segment, &Gran, TargName, TargFile);
      if (!Write4(TargFile, &Addr))
        ChkIO(TargName);
      if (!Write2(TargFile, &Len))
        ChkIO(TargName);
      if (fwrite(Buffer, 1, Len, TargFile) != Len)
        ChkIO(TargName);
      if (PartRun)
        PartRun = PartRun->Next;
      SumLen += Len;
    }

    /* records with relocation: basically the same, plus the real work...
       the appended relocation info will be skipped in the next loop run. */

    else if ((Header == FileHeaderRDataRec) || (Header == FileHeaderRRelocRec))
    {
      if (!Read4(f, &Addr))
        ChkIO(SrcName);
      if (!Read2(f, &Len))
        ChkIO(SrcName);
      BumpBuffer(Len);
      if (fread(Buffer, 1, Len, f) != Len)
        ChkIO(SrcName);

      UndefFlag = False;
      for (z = 0; z < PartRun->RelocInfo->RelocCount; z++)
      {
        PReloc = PartRun->RelocInfo->RelocEntries + z;
        Found = True;
        if (!strcmp(PReloc->Name, RelName_SegStart))
          Value = PartRun->CodeStart;
        else
          Found = GetExport(PReloc->Name, &Value);
        if (Found)
        {
          if (Verbose >= 2)
            printf("%s 0x%x...", getmessage(Num_InfoMsgReading), (int)PReloc->Addr);
          RelocVal = GetValue(PReloc->Type, PReloc->Addr - PartRun->CodeStart);
          NRelocVal = (PReloc->Type & RelocFlagSUB) ? RelocVal - Value : RelocVal + Value;
          PutValue(NRelocVal, PReloc->Type, PReloc->Addr - PartRun->CodeStart);
        }
        else
        {
          fprintf(stderr, "%s: %s(%s)\n", getmessage(Num_UndefSymbol),
                  PReloc->Name, SrcName);
          UndefFlag = True;
          UndefErr++;
        }
      }

      if (!UndefFlag)
      {
        Header = FileHeaderDataRec;
        WriteRecordHeader(&Header, &CPU, &Segment, &Gran, TargName, TargFile);
        Addr = PartRun->CodeStart;
        if (!Write4(TargFile, &Addr))
          ChkIO(TargName);
        if (!Write2(TargFile, &Len))
          ChkIO(TargName);
        if (fwrite(Buffer, 1, Len, TargFile) != Len)
          ChkIO(TargName);
        if (PartRun)
          PartRun = PartRun->Next;
      }
      SumLen += Len;
    }

    /* all done? */

    else if (Header == FileHeaderEnd)
      break;

    else
      SkipRecord(Header, SrcName, f);
  }

  if (Verbose >= 1)
  {
    printf("(");
    printf(Integ32Format, SumLen);
    printf(" %s)\n", getmessage((SumLen == 1) ? Num_Byte : Num_Bytes));
  }
}

/****************************************************************************/
/* command line processing */

static CMDResult CMD_Verbose(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  if (Negate)
  {
    if (Verbose)
      Verbose--;
  }
  else
    Verbose++;

  return CMDOK;
}

static void ParamError(Boolean InEnv, char *Arg)
{
  printf("%s%s\n%s\n",
         getmessage((InEnv) ? Num_ErrMsgInvEnvParam : Num_ErrMsgInvParam),
         Arg, getmessage(Num_ErrMsgProgTerm));
  exit(1);
}

#define ALINKParamCnt (sizeof(ALINKParams) / sizeof(*ALINKParams))

static CMDRec ALINKParams[] =
{
  {"v", CMD_Verbose}
};

/****************************************************************************/

int main(int argc, char **argv)
{
  String Ver;
  int z;
  Word LMagic;
  Byte LHeader;
  PPart PartRun;
  LargeInt Diff;

  /* the initialization orgy... */

  nls_init();
  if (!NLS_Initialize(&argc, argv))
    exit(4);

  endian_init();
  bpemu_init();
  strutil_init();

  Buffer = NULL;
  BufferSize = 0;

  /* open message catalog */

  nlmessages_init("alink.msg", *argv, MsgId1, MsgId2); ioerrs_init(*argv);
  cmdarg_init(*argv);
  toolutils_init(*argv);

  as_snprintf(Ver, sizeof(Ver), "ALINK/C V%s", Version);
  WrCopyRight(Ver);

  /* no commandline arguments -->print help */

  if (argc <= 1)
  {
    char *ph1, *ph2;

    errno = 0;
    printf("%s%s%s\n", getmessage(Num_InfoMessHead1), GetEXEName(argv[0]),
           getmessage(Num_InfoMessHead2));
    ChkIO(OutName);
    for (ph1 = getmessage(Num_InfoMessHelp), ph2 = strchr(ph1,'\n'); ph2; ph1 = ph2+1, ph2 = strchr(ph1,'\n'))
    {
      *ph2 = '\0';
      printf("%s\n", ph1);
      *ph2 = '\n';
    }
    exit(1);
  }

  /* preinit commandline variables */

  Verbose = 0;

  /* process arguments */

  ProcessCMD(argc, argv, ALINKParams, ALINKParamCnt, ParUnprocessed, "ALINKCMD", ParamError);

  if ((Verbose >= 1) && (argc > 1))
   printf("\n");

  /* extract target file */

  if (ProcessedEmpty(ParUnprocessed))
  {
    errno = 0;
    printf("%s\n", getmessage(Num_ErrMsgTargMissing));
    ChkIO(OutName);
    exit(1);
  }
  for (z = argc - 1; z > 0; z--)
    if (ParUnprocessed[z])
      break;
  strmaxcpy(TargName, argv[z], STRINGSIZE);
  DelSuffix(TargName);
  AddSuffix(TargName, STRINGSIZE, getmessage(Num_Suffix));
  ParUnprocessed[z] = False;

  /* walk over source file(s): */

  if (ProcessedEmpty(ParUnprocessed))
  {
    errno = 0;
    printf("%s\n", getmessage(Num_ErrMsgSrcMissing));
    ChkIO(OutName);
    exit(1);
  }

  /* read symbol info from all files */

  DoubleErr = False;
  PartList = NULL;
  for (z = 1; z < argc; z++)
    if (ParUnprocessed[z])
      ReadSymbols(argv[z], z);

  /* double-defined symbols? */

  if (DoubleErr)
    return 1;

  /* arrange relocatable segments in memory, relocate global symbols */

  for (PartRun = PartList; PartRun; PartRun = PartRun->Next)
    if (PartRun->MustReloc)
    {
      Diff = SegStarts[PartRun->Segment] - PartRun->CodeStart;
      PartRun->CodeStart += Diff;
      if (Verbose >= 2)
        printf("%s 0x%x\n", getmessage(Num_InfoMsgLocating), (int)PartRun->CodeStart);
      if (PartRun->RelocInfo)
      {
        PExportEntry ExpRun, ExpEnd;
        PRelocEntry RelRun, RelEnd;

        ExpRun = PartRun->RelocInfo->ExportEntries;
        ExpEnd = ExpRun + PartRun->RelocInfo->ExportCount;
        for (; ExpRun < ExpEnd; ExpRun++)
          if (ExpRun->Flags & RelFlag_Relative)
            ExpRun->Value += Diff;
        RelRun = PartRun->RelocInfo->RelocEntries;
        RelEnd = RelRun + PartRun->RelocInfo->RelocCount;
        for (; RelRun < RelEnd; RelRun++)
          RelRun->Addr += Diff;
      }
      SegStarts[PartRun->Segment] += PartRun->CodeLen / PartRun->Gran;
    }

  /* open target file */

  TargFile = fopen(TargName, OPENWRMODE);
  if (!TargFile)
    ChkIO(TargName);
  LMagic = FileMagic;
  if (!Write2(TargFile, &LMagic))
    ChkIO(TargName);

  /* do relocations, underwhile write target file */

  UndefErr = 0;
  for (z = 1; z < argc; z++)
    if (ParUnprocessed[z])
      ProcessFile(argv[z], z);

  /* write final creator record */

  LHeader = FileHeaderEnd;
  if (fwrite(&LHeader, 1, 1, TargFile) != 1)
    ChkIO(TargName);
  as_snprintf( Ver, sizeof(Ver), "ALINK %s/%s-%s", Version, ARCHPRNAME, ARCHSYSNAME);
  if (fwrite(Ver, 1, strlen(Ver), TargFile) != strlen(Ver))
    ChkIO(TargName);

  /* close target file and erase if undefined symbols */

  fclose(TargFile);
  if ((UndefErr > 0) || (Magic != 0))
    unlink(TargName);
  if (UndefErr > 0)
  {
    fprintf(stderr, "\n");
    fprintf(stderr, LongIntFormat, UndefErr);
    fprintf(stderr, " %s\n", getmessage((UndefErr == 1) ? Num_SumUndefSymbol : Num_SumUndefSymbols));
    return 1;
  }

  return 0;
}
