/* p2bin.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Umwandlung von AS-Codefiles in Binaerfiles                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "version.h"
#include "endian.h"
#include "bpemu.h"
#include "strutil.h"
#include "nls.h"
#include "nlmessages.h"
#include "p2bin.rsc"
#ifdef _USE_MSH
# include "p2bin.msh"
#endif
#include "ioerrs.h"
#include "chunks.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "msg_level.h"
#include "toolutils.h"

#define BinSuffix ".bin"


typedef void (*ProcessProc)(
#ifdef __PROTOS__
const char *FileName, LongWord Offset
#endif
);


static FILE *TargFile;
static String TargName;

static LongWord StartAdr, StopAdr, EntryAdr, RealFileLen;
static LongWord MaxGran, Dummy;
static Boolean StartAuto, StopAuto, AutoErase, EntryAdrPresent;

static Byte FillVal, ValidSegment;
static Boolean DoCheckSum;

static Byte SizeDiv;
static LongWord ANDMask, ANDEq;
static ShortInt StartHeader;

static ChunkList UsedList;


#ifdef DEBUG
#define ChkIO(s) ChkIO_L(s, __LINE__)

static void ChkIO_L(char *s, int line)
{
  if (errno != 0)
  {
    fprintf(stderr, "%s %d\n", s, line);
    exit(3);
  }
}
#endif

static void ParamError(Boolean InEnv, char *Arg)
{
  fprintf(stderr, "%s%s\n%s\n",
          getmessage(InEnv ? Num_ErrMsgInvEnvParam:Num_ErrMsgInvParam),
          Arg, getmessage(Num_ErrMsgProgTerm));
}

#define BufferSize 4096
static Byte Buffer[BufferSize];

static void OpenTarget(void)
{
  LongWord Rest, Trans, AHeader;

  TargFile = fopen(TargName, OPENWRMODE);
  if (!TargFile)
    ChkIO(TargName);
  RealFileLen = ((StopAdr - StartAdr + 1) * MaxGran) / SizeDiv;

  AHeader = abs(StartHeader);
  if (StartHeader != 0)
  {
    memset(Buffer, 0, AHeader);
    if (fwrite(Buffer, 1, abs(StartHeader), TargFile) != AHeader)
      ChkIO(TargName);
  }

  memset(Buffer, FillVal, BufferSize);

  Rest = RealFileLen;
  while (Rest != 0)
  {
    Trans = min(Rest, BufferSize);
    if (fwrite(Buffer, 1, Trans, TargFile) != Trans)
      ChkIO(TargName);
    Rest -= Trans;
  }
}

static void CloseTarget(void)
{
  LongWord z, AHeader;

  AHeader = abs(StartHeader);

  /* write entry address to file? */

  if ((EntryAdrPresent) && (StartHeader != 0))
  {
    LongWord bpos;

    rewind(TargFile);
    bpos = ((StartHeader > 0) ? 0 : -1 - StartHeader) << 3;
    for (z = 0; z < AHeader; z++)
    {
      Buffer[z] = (EntryAdr >> bpos) & 0xff;
      bpos += (StartHeader > 0) ? 8 : -8;
    }
    if (fwrite(Buffer, 1, AHeader, TargFile) != AHeader)
      ChkIO(TargName);
  }

  if (EOF == fclose(TargFile))
    ChkIO(TargName);

  /* compute checksum over file? */

  if (DoCheckSum)
  {
    LongWord Sum, Size, Rest, Trans, Read;

    TargFile = fopen(TargName, OPENUPMODE);
    if (!TargFile)
      ChkIO(TargName);
    if (fseek(TargFile, AHeader, SEEK_SET) == -1)
      ChkIO(TargName);
    Size = Rest = FileSize(TargFile) - AHeader - 1;

    Sum = 0;
    while (Rest > 0)
    {
      Trans = min(Rest, BufferSize);
      Rest -= Trans;
      Read = fread(Buffer, 1, Trans, TargFile);
      if (Read != Trans)
        chk_wr_read_error(TargName);
      for (z = 0; z < Trans; Sum += Buffer[z++]);
    }
    errno = 0;
    printf("%s%08lX\n", getmessage(Num_InfoMessChecksum), LoDWord(Sum));
    Buffer[0] = 0x100 - (Sum & 0xff);

    /* Some systems require fflush() between read & write operations.  And
       some other systems again garble the file pointer upon an fflush(): */

    fflush(TargFile);
    if (fseek(TargFile, AHeader + Size, SEEK_SET) == -1)
      ChkIO(TargName);
    if (fwrite(Buffer, 1, 1, TargFile) != 1)
      ChkIO(TargName);
    fflush(TargFile);

    if (fclose(TargFile) == EOF)
      ChkIO(TargName);
  }

  if (Magic != 0)
    unlink(TargName);
}

static void ProcessFile(const char *FileName, LongWord Offset)
{
  FILE *SrcFile;
  Word TestID;
  Byte InpHeader, InpCPU, InpSegment;
  LongWord InpStart, SumLen;
  Word InpLen, TransLen, ResLen;
  Boolean doit;
  LongWord ErgStart, ErgStop;
  LongInt NextPos;
  Word ErgLen = 0;
  Byte Gran;

  SrcFile = fopen(FileName, OPENRDMODE);
  if (!SrcFile)
    ChkIO(FileName);

  if (!Read2(SrcFile, &TestID))
    chk_wr_read_error(FileName);
  if (TestID != FileID)
    FormatError(FileName, getmessage(Num_FormatInvHeaderMsg));

  errno = 0;
  if (msg_level >= e_msg_level_normal)
    printf("%s==>>%s", FileName, TargName);
  ChkIO(OutName);

  SumLen = 0;

  do
  {
    ReadRecordHeader(&InpHeader, &InpCPU, &InpSegment, &Gran, FileName, SrcFile);

    if (InpHeader == FileHeaderStartAdr)
    {
      if (!Read4(SrcFile, &ErgStart))
        chk_wr_read_error(FileName);
      if (!EntryAdrPresent)
      {
        EntryAdr = ErgStart;
        EntryAdrPresent = True;
      }
    }

    else if (InpHeader == FileHeaderDataRec)
    {
      if (!Read4(SrcFile, &InpStart))
        chk_wr_read_error(FileName);
      if (!Read2(SrcFile, &InpLen))
        chk_wr_read_error(FileName);

      NextPos = ftell(SrcFile) + InpLen;
      if (NextPos >= FileSize(SrcFile) - 1)
        FormatError(FileName, getmessage(Num_FormatInvRecordLenMsg));

      doit = (FilterOK(InpHeader) && (InpSegment == ValidSegment));

      if (doit)
      {
        InpStart += Offset;
        ErgStart = max(StartAdr, InpStart);
        ErgStop = min(StopAdr, InpStart + (InpLen/Gran) - 1);
        doit = (ErgStop >= ErgStart);
        if (doit)
        {
          ErgLen = (ErgStop + 1 - ErgStart) * Gran;
          if (AddChunk(&UsedList, ErgStart, ErgStop - ErgStart + 1, True))
          {
            errno = 0;
            fprintf(stderr, " %s\n", getmessage(Num_ErrMsgOverlap));
            ChkIO(OutName);
          }
        }
      }

      if (doit)
      {
        /* an Anfang interessierender Daten */

        if (fseek(SrcFile, (ErgStart - InpStart) * Gran, SEEK_CUR) == -1)
          ChkIO(FileName);

        /* in Zieldatei an passende Stelle */

        if (fseek(TargFile, (((ErgStart - StartAdr) * Gran)/SizeDiv) + abs(StartHeader), SEEK_SET) == -1)
          ChkIO(TargName);

        /* umkopieren */

        while (ErgLen > 0)
        {
          TransLen = min(BufferSize, ErgLen);
          if (fread(Buffer, 1, TransLen, SrcFile) != TransLen)
            chk_wr_read_error(FileName);
          if (SizeDiv == 1) ResLen = TransLen;
          else
          {
            LongWord Addr;

            ResLen = 0;
            for (Addr = 0; Addr < (LongWord)TransLen; Addr++)
              if (((ErgStart * Gran + Addr) & ANDMask) == ANDEq)
                Buffer[ResLen++] = Buffer[Addr];
          }
          if (fwrite(Buffer, 1, ResLen, TargFile) != ResLen)
            ChkIO(TargName);
          ErgLen -= TransLen;
          ErgStart += TransLen;
          SumLen += ResLen;
        }
      }
      if (fseek(SrcFile, NextPos, SEEK_SET) == -1)
        ChkIO(FileName);
    }
    else
      SkipRecord(InpHeader, FileName, SrcFile);
  }
  while (InpHeader != 0);

  if (msg_level >= e_msg_level_normal)
  {
    errno = 0; printf("  ("); ChkIO(OutName);
    errno = 0; printf(Integ32Format, SumLen); ChkIO(OutName);
    errno = 0; printf(" %s)\n", getmessage((SumLen == 1) ? Num_Byte : Num_Bytes)); ChkIO(OutName);
  }
  if (!SumLen)
  {
    errno = 0;
    fputs(getmessage(Num_WarnEmptyFile), stdout);
    ChkIO(OutName);
  }

  if (fclose(SrcFile) == EOF)
    ChkIO(FileName);
}

static ProcessProc CurrProcessor;
static LongWord CurrOffset;

static void Callback(char *Name)
{
  CurrProcessor(Name, CurrOffset);
}

static void ProcessGroup(const char *GroupName_O, ProcessProc Processor)
{
  String Ext, GroupName;

  CurrProcessor = Processor;
  strmaxcpy(GroupName, GroupName_O, STRINGSIZE);
  strmaxcpy(Ext, GroupName, STRINGSIZE);
  if (!RemoveOffset(GroupName, &CurrOffset))
  {
    ParamError(False, Ext);
    exit(1);
  }
  AddSuffix(GroupName, STRINGSIZE, getmessage(Num_Suffix));

  if (!DirScan(GroupName, Callback))
    fprintf(stderr, "%s%s%s\n", getmessage(Num_ErrMsgNullMaskA), GroupName, getmessage(Num_ErrMsgNullMaskB));
}

static void MeasureFile(const char *FileName, LongWord Offset)
{
  FILE *f;
  Byte Header, CPU, Gran, Segment;
  Word Length, TestID;
  LongWord Adr, EndAdr;
  LongInt NextPos;

  f = fopen(FileName, OPENRDMODE);
  if (!f)
    ChkIO(FileName);

  if (!Read2(f, &TestID))
    chk_wr_read_error(FileName);
  if (TestID != FileMagic)
    FormatError(FileName, getmessage(Num_FormatInvHeaderMsg));

  do
  {
    ReadRecordHeader(&Header, &CPU, &Segment, &Gran, FileName, f);

    if (Header == FileHeaderDataRec)
    {
      if (!Read4(f, &Adr))
        chk_wr_read_error(FileName);
      if (!Read2(f, &Length))
        chk_wr_read_error(FileName);
      NextPos = ftell(f) + Length;
      if (NextPos > FileSize(f))
        FormatError(FileName, getmessage(Num_FormatInvRecordLenMsg));

      if (FilterOK(Header) && (Segment == ValidSegment))
      {
        Adr += Offset;
        EndAdr = Adr + (Length/Gran)-1;
        if (Gran > MaxGran)
          MaxGran = Gran;
        if (StartAuto)
          if (StartAdr > Adr)
            StartAdr = Adr;
        if (StopAuto)
          if (EndAdr > StopAdr)
            StopAdr = EndAdr;
      }

      fseek(f, NextPos, SEEK_SET);
    }
    else
      SkipRecord(Header, FileName, f);
  }
  while (Header != 0);

  if (fclose(f) == EOF)
    ChkIO(FileName);
}

/* --------------------------------------------- */

static as_cmd_result_t CMD_AdrRange(Boolean Negate, const char *Arg)
{
  if (Negate)
  {
    StartAdr = 0; StopAdr = 0x7fff;
    return e_cmd_ok;
  }
  else
    return CMD_Range(&StartAdr, &StopAdr,
                     &StartAuto, &StopAuto, Arg);
}

static as_cmd_result_t CMD_ByteMode(Boolean Negate, const char *pArg)
{
#define ByteModeCnt 9
  static const char *ByteModeStrings[ByteModeCnt] =
  {
    "ALL", "EVEN", "ODD", "BYTE0", "BYTE1", "BYTE2", "BYTE3", "WORD0", "WORD1"
  };
  static Byte ByteModeDivs[ByteModeCnt] =
  {
    1, 2, 2, 4, 4, 4, 4, 2, 2
  };
  static Byte ByteModeMasks[ByteModeCnt] =
  {
    0, 1, 1, 3, 3, 3, 3, 2, 2
  };
  static Byte ByteModeEqs[ByteModeCnt] =
  {
    0, 0, 1, 0, 1, 2, 3, 0, 2
  };

  int z;
  UNUSED(Negate);

  if (*pArg == '\0')
  {
    SizeDiv = 1;
    ANDEq = 0;
    ANDMask = 0;
    return e_cmd_ok;
  }
  else
  {
    String Arg;

    strmaxcpy(Arg, pArg, STRINGSIZE);
    NLS_UpString(Arg);
    ANDEq = 0xff;
    for (z = 0; z < ByteModeCnt; z++)
      if (strcmp(Arg, ByteModeStrings[z]) == 0)
      {
        SizeDiv = ByteModeDivs[z];
        ANDMask = ByteModeMasks[z];
        ANDEq   = ByteModeEqs[z];
      }
    if (ANDEq == 0xff)
      return e_cmd_err;
    else
      return e_cmd_arg;
  }
}

static as_cmd_result_t CMD_StartHeader(Boolean Negate, const char *Arg)
{
  Boolean err;
  ShortInt Sgn;

  if (Negate)
  {
    StartHeader = 0;
    return e_cmd_ok;
  }
  else
  {
    Sgn = 1;
    if (*Arg == '\0')
      return e_cmd_err;
    switch (as_toupper(*Arg))
    {
      case 'B':
        Sgn = -1;
        /* fall-through */
      case 'L':
        Arg++;
    }
    StartHeader = ConstLongInt(Arg, &err, 10);
    if ((!err) || (StartHeader > 4))
      return e_cmd_err;
    StartHeader *= Sgn;
    return e_cmd_arg;
  }
}	

static as_cmd_result_t CMD_EntryAdr(Boolean Negate, const char *Arg)
{
  Boolean err;

  if (Negate)
  {
    EntryAdrPresent = False;
    return e_cmd_ok;
  }
  else
  {
    EntryAdr = ConstLongInt(Arg, &err, 10);
    if (err)
      EntryAdrPresent = True;
    return (err) ? e_cmd_arg : e_cmd_err;
  }
}

static as_cmd_result_t CMD_FillVal(Boolean Negate, const char *Arg)
{
  Boolean err;
  UNUSED(Negate);

  FillVal = ConstLongInt(Arg, &err, 10);
  return err ? e_cmd_arg : e_cmd_err;
}

static as_cmd_result_t CMD_CheckSum(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  DoCheckSum = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_AutoErase(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  AutoErase = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_ForceSegment(Boolean Negate,  const char *Arg)
{
  int z = addrspace_lookup(Arg);

  if (z >= SegCount)
    return e_cmd_err;

  if (!Negate)
    ValidSegment = z;
  else if (ValidSegment == z)
    ValidSegment = SegCode;

  return e_cmd_arg;
}

static as_cmd_rec_t P2BINParams[] =
{
  { "f"        , CMD_FilterList },
  { "r"        , CMD_AdrRange },
  { "s"        , CMD_CheckSum },
  { "m"        , CMD_ByteMode },
  { "l"        , CMD_FillVal },
  { "e"        , CMD_EntryAdr },
  { "S"        , CMD_StartHeader },
  { "k"        , CMD_AutoErase },
  { "SEGMENT"  , CMD_ForceSegment }
};

int main(int argc, char **argv)	
{
  as_cmd_results_t cmd_results;
  char *p_target_name;
  const char *p_src_name;
  StringRecPtr p_src_run;

  nls_init();
  if (!NLS_Initialize(&argc, argv))
    exit(4);

  endian_init();
  strutil_init();
  bpemu_init();
#ifdef _USE_MSH
  nlmessages_init_buffer(p2bin_msh_data, sizeof(p2bin_msh_data), MsgId1, MsgId2);
#else
  nlmessages_init_file("p2bin.msg", *argv, MsgId1, MsgId2);
#endif
  ioerrs_init(*argv);
  chunks_init();
  as_cmdarg_init(*argv);
  msg_level_init();
  toolutils_init(*argv);

  InitChunk(&UsedList);

  StartAdr = 0;
  StopAdr = 0x7fff;
  StartAuto = True;
  StopAuto = True;
  FillVal = 0xff;
  DoCheckSum = False;
  SizeDiv = 1;
  ANDEq = 0;
  EntryAdr = -1;
  EntryAdrPresent = False;
  AutoErase = False;
  StartHeader = 0;
  ValidSegment = SegCode;

  as_cmd_register(P2BINParams, as_array_size(P2BINParams));
  if (e_cmd_err == as_cmd_process(argc, argv, "P2BINCMD", &cmd_results))
  {
    ParamError(cmd_results.error_arg_in_env, cmd_results.error_arg);
    exit(1);
  }

  if ((msg_level >= e_msg_level_verbose) || cmd_results.write_version_exit)
  {
    String Ver;

    as_snprintf(Ver, sizeof(Ver), "P2BIN V%s", Version);
    WrCopyRight(Ver);
  }

  if (cmd_results.write_help_exit)
  {
    char *ph1, *ph2;

    errno = 0;
    printf("%s%s%s\n", getmessage(Num_InfoMessHead1), as_cmdarg_get_executable_name(), getmessage(Num_InfoMessHead2));
    ChkIO(OutName);
    for (ph1 = getmessage(Num_InfoMessHelp), ph2 = strchr(ph1, '\n'); ph2; ph1 = ph2 + 1, ph2 = strchr(ph1, '\n'))
    {
      *ph2 = '\0';
      printf("%s\n", ph1);
      *ph2 = '\n';
    }
  }

  if (cmd_results.write_version_exit || cmd_results.write_help_exit)
    exit(0);

  if (StringListEmpty(cmd_results.file_arg_list))
  {
    fprintf(stderr, "%s: %s\n", as_cmdarg_get_executable_name(), getmessage(Num_ErrMessNoInputFiles));
    exit(1);
  }

  p_target_name = MoveAndCutStringListLast(&cmd_results.file_arg_list);
  if (!p_target_name || !*p_target_name)
  {
    if (p_target_name) free(p_target_name);
    p_target_name = NULL;
    errno = 0;
    fprintf(stderr, "%s\n", getmessage(Num_ErrMsgTargMissing));
    ChkIO(OutName);
    exit(1);
  }

  strmaxcpy(TargName, p_target_name, STRINGSIZE);
  if (!RemoveOffset(TargName, &Dummy))
  {
    strmaxcpy(TargName, p_target_name, STRINGSIZE);
    free(p_target_name); p_target_name = NULL;
    ParamError(False, TargName);
  }

  /* special case: only one argument <name> treated like <name>.p -> <name).bin */

  if (StringListEmpty(cmd_results.file_arg_list))
  {
    AddStringListLast(&cmd_results.file_arg_list, p_target_name);
    DelSuffix(TargName);
  }
  AddSuffix(TargName, STRINGSIZE, BinSuffix);
  free(p_target_name); p_target_name = NULL;

  MaxGran = 1;
  if ((StartAuto) || (StopAuto))
  {
    if (StartAuto)
      StartAdr = 0xfffffffful;
    if (StopAuto)
      StopAdr = 0;
    for (p_src_name = GetStringListFirst(cmd_results.file_arg_list, &p_src_run);
         p_src_name; p_src_name = GetStringListNext(&p_src_run))
      if (*p_src_name)
        ProcessGroup(p_src_name, MeasureFile);
    if (StartAdr > StopAdr)
    {
      errno = 0;
      fprintf(stderr, "%s\n", getmessage(Num_ErrMsgAutoFailed));
      ChkIO(OutName);
      exit(1);
    }
    if (msg_level >= e_msg_level_normal)
    {
      printf("%s: 0x%08lX-", getmessage(Num_InfoMessDeducedRange), LoDWord(StartAdr));
      printf("0x%08lX\n", LoDWord(StopAdr));
    }
  }

  OpenTarget();

  for (p_src_name = GetStringListFirst(cmd_results.file_arg_list, &p_src_run);
       p_src_name; p_src_name = GetStringListNext(&p_src_run))
    if (*p_src_name)
      ProcessGroup(p_src_name, ProcessFile);

  CloseTarget();

  if (AutoErase)
    for (p_src_name = GetStringListFirst(cmd_results.file_arg_list, &p_src_run);
         p_src_name; p_src_name = GetStringListNext(&p_src_run))
      if (*p_src_name)
        ProcessGroup(p_src_name, EraseFile);

  ClearStringList(&cmd_results.file_arg_list);

  return 0;
}
