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
#include "msg_level.h"
#include "toolutils.h"

#include "ioerrs.h"

#include "alink.rsc"
#ifdef _USE_MSH
# include "alink.msh"
#endif

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
static LongWord UndefErr;

static String TargName;

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

static int GetValue(LongInt Type, LargeWord Offset, LargeWord *p_result)
{
  switch (Type & ~(RelocFlagSUB | RelocFlagPage))
  {
    case RelocTypeL8:
      *p_result = MRead1L(Buffer + Offset);
      return 0;
    case RelocTypeL16:
      *p_result = MRead2L(Buffer + Offset);
      return 0;
    case RelocTypeB16:
      *p_result = MRead2B(Buffer + Offset);
      return 0;
    case RelocTypeL32:
      *p_result = MRead4L(Buffer + Offset);
      return 0;
    case RelocTypeB32:
      *p_result = MRead4B(Buffer + Offset);
      return 0;
#ifdef HAS64
    case RelocTypeL64:
      *p_result = MRead8L(Buffer + Offset);
      return 0;
    case RelocTypeB64:
      *p_result = MRead8B(Buffer + Offset);
      return 0;
#endif
    default:
      fprintf(stderr, "unknown relocation type: 0x");
      fprintf(stderr, LongIntFormat, Type);
      fprintf(stderr, "\n");
      *p_result = 0;
      return -1;
  }
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
  if (msg_level >= 3)
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
        chk_wr_read_error(SrcName);
      PNew->CodeStart = Addr;
      if (!Read2(f, &Len))
        chk_wr_read_error(SrcName);
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
  if (msg_level >= 3)
    printf("%s '%s'...", getmessage(Num_InfoMsgOpenSrc), SrcName);
  else if (msg_level >= e_msg_level_verbose)
    printf("%s", SrcName);
  f = fopen(SrcName, OPENRDMODE);
  if (!f)
    ChkIO(SrcName);

  /* check magic */

  if (!Read2(f, &Magic))
    chk_wr_read_error(SrcName);
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
        chk_wr_read_error(SrcName);
      if (!Read2(f, &Len))
        chk_wr_read_error(SrcName);
      BumpBuffer(Len);
      if (fread(Buffer, 1, Len, f) != Len)
        chk_wr_read_error(SrcName);
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
        chk_wr_read_error(SrcName);
      if (!Read2(f, &Len))
        chk_wr_read_error(SrcName);
      BumpBuffer(Len);
      if (fread(Buffer, 1, Len, f) != Len)
        chk_wr_read_error(SrcName);

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
          if (msg_level >= 3)
            printf("%s 0x%x...", getmessage(Num_InfoMsgReading), (int)PReloc->Addr);
          if (GetValue(PReloc->Type, PReloc->Addr - PartRun->CodeStart, &RelocVal))
            UndefErr++;
          else
          {
            NRelocVal = (PReloc->Type & RelocFlagSUB) ? RelocVal - Value : RelocVal + Value;
            PutValue(NRelocVal, PReloc->Type, PReloc->Addr - PartRun->CodeStart);
          }
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

  if (msg_level >= e_msg_level_verbose)
  {
    printf("(");
    printf(Integ32Format, SumLen);
    printf(" %s)\n", getmessage((SumLen == 1) ? Num_Byte : Num_Bytes));
  }
}

/****************************************************************************/
/* command line processing */

int main(int argc, char **argv)
{
  String Ver;
  int file_index;
  Word LMagic;
  Byte LHeader;
  PPart PartRun;
  LargeInt Diff;
  as_cmd_results_t cmd_results;
  char *p_target_name;
  const char *p_src_name;
  StringRecPtr p_src_run;

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

#ifdef _USE_MSH
  nlmessages_init_buffer(alink_msh_data, sizeof(alink_msh_data), MsgId1, MsgId2);
#else
  nlmessages_init_file("alink.msg", *argv, MsgId1, MsgId2);
#endif
  ioerrs_init(*argv);
  as_cmdarg_init(*argv);
  toolutils_init(*argv);

  /* process arguments */

  if (e_cmd_err == as_cmd_process(argc, argv, "ALINKCMD", &cmd_results))
  {
    printf("%s%s\n%s\n",
           getmessage(cmd_results.error_arg_in_env ? Num_ErrMsgInvEnvParam : Num_ErrMsgInvParam),
           cmd_results.error_arg, getmessage(Num_ErrMsgProgTerm));
    exit(1);
  }

  if ((msg_level >= e_msg_level_verbose) && (argc > 1))
   printf("\n");

  if ((msg_level >= e_msg_level_verbose) || cmd_results.write_version_exit)
  {
    String Ver;

    as_snprintf(Ver, sizeof(Ver), "ALINK V%s", Version);
    WrCopyRight(Ver);
  }

  if (cmd_results.write_help_exit)
  {
    char *ph1, *ph2;

    errno = 0;
    printf("%s%s%s\n", getmessage(Num_InfoMessHead1), as_cmdarg_get_executable_name(),
           getmessage(Num_InfoMessHead2));
    ChkIO(OutName);
    for (ph1 = getmessage(Num_InfoMessHelp), ph2 = strchr(ph1,'\n'); ph2; ph1 = ph2+1, ph2 = strchr(ph1,'\n'))
    {
      *ph2 = '\0';
      printf("%s\n", ph1);
      *ph2 = '\n';
    }
  }

  if (cmd_results.write_version_exit || cmd_results.write_help_exit)
    exit(0);

  /* no command line arguments? */

  if (StringListEmpty(cmd_results.file_arg_list))
  {
    fprintf(stderr, "%s: %s\n", as_cmdarg_get_executable_name(), getmessage(Num_ErrMessNoInputFiles));
    exit(1);
  }

  /* extract target file */

  p_target_name = MoveAndCutStringListLast(&cmd_results.file_arg_list);
  strmaxcpy(TargName, p_target_name ? p_target_name : "", STRINGSIZE);
  free(p_target_name); p_target_name = NULL;
  if (!*TargName)
  {
    errno = 0;
    printf("%s\n", getmessage(Num_ErrMsgTargMissing));
    ChkIO(OutName);
    exit(1);
  }
  DelSuffix(TargName);
  AddSuffix(TargName, STRINGSIZE, getmessage(Num_Suffix));

  /* walk over source file(s): */

  if (StringListEmpty(cmd_results.file_arg_list))
  {
    errno = 0;
    printf("%s\n", getmessage(Num_ErrMsgSrcMissing));
    ChkIO(OutName);
    exit(1);
  }

  /* read symbol info from all files */

  DoubleErr = False;
  PartList = NULL;
  for (p_src_name = GetStringListFirst(cmd_results.file_arg_list, &p_src_run), file_index = 1;
       p_src_name; p_src_name = GetStringListNext(&p_src_run), file_index++)
    if (*p_src_name)
      ReadSymbols(p_src_name, file_index);

  /* double-defined symbols? */

  if (DoubleErr)
    return 1;

  /* arrange relocatable segments in memory, relocate global symbols */

  for (PartRun = PartList; PartRun; PartRun = PartRun->Next)
    if (PartRun->MustReloc)
    {
      Diff = SegStarts[PartRun->Segment] - PartRun->CodeStart;
      PartRun->CodeStart += Diff;
      if (msg_level >= e_msg_level_verbose)
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
  for (p_src_name = GetStringListFirst(cmd_results.file_arg_list, &p_src_run), file_index = 1;
       p_src_name; p_src_name = GetStringListNext(&p_src_run), file_index++)
    if (*p_src_name)
      ProcessFile(p_src_name, file_index);

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

  ClearStringList(&cmd_results.file_arg_list);

  return 0;
}
