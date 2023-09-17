/* pbind.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Bearbeitung von AS-P-Dateien                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "version.h"
#include "endian.h"
#include "stdhandl.h"
#include "bpemu.h"
#include "strutil.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "msg_level.h"
#include "toolutils.h"
#include "nls.h"
#include "nlmessages.h"
#include "pbind.rsc"
#ifdef _USE_MSH
# include "pbind.msh"
#endif
#include "ioerrs.h"

#define BufferSize 8192

static const char *Creator = "BIND 1.42";

static FILE *TargFile;
static String TargName;
static Byte *Buffer;

static void OpenTarget(void)
{
  TargFile = fopen(TargName, OPENWRMODE);
  if (!TargFile) ChkIO(TargName);
  if (!Write2(TargFile, &FileID)) ChkIO(TargName);
}

static void CloseTarget(void)
{
  Byte EndHeader = FileHeaderEnd;

  if (fwrite(&EndHeader, 1, 1, TargFile) != 1)
    ChkIO(TargName);
  if (fwrite(Creator, 1, strlen(Creator), TargFile) != strlen(Creator))
    ChkIO(TargName);
  if (fclose(TargFile) == EOF)
    ChkIO(TargName);
  if (Magic != 0)
    unlink(TargName);
}

static void ProcessFile(char *FileName)
{
  FILE *SrcFile;
  Word TestID;
  Byte InpHeader, InpCPU, InpSegment, InpGran;
  LongInt InpStart, SumLen;
  Word InpLen, TransLen;
  Boolean doit;

  SrcFile = fopen(FileName, OPENRDMODE);
  if (!SrcFile)
    ChkIO(FileName);

  if (!Read2(SrcFile, &TestID))
    chk_wr_read_error(FileName);
  if (TestID != FileMagic)
    FormatError(FileName, getmessage(Num_FormatInvHeaderMsg));

  if (msg_level >= e_msg_level_normal)
  {
    errno = 0; printf("%s==>>%s", FileName, TargName); ChkIO(OutName);
  }

  SumLen = 0;

  do
  {
    ReadRecordHeader(&InpHeader, &InpCPU, &InpSegment, &InpGran, FileName, SrcFile);

    if (InpHeader == FileHeaderStartAdr)
    {
      if (!Read4(SrcFile, &InpStart))
        chk_wr_read_error(FileName);
      WriteRecordHeader(&InpHeader, &InpCPU, &InpSegment, &InpGran, TargName, TargFile);
      if (!Write4(TargFile, &InpStart))
        ChkIO(TargName);
    }

    else if (InpHeader == FileHeaderDataRec)
    {
      if (!Read4(SrcFile, &InpStart))
        chk_wr_read_error(FileName);
      if (!Read2(SrcFile, &InpLen))
        chk_wr_read_error(FileName);

      if (ftell(SrcFile)+InpLen >= FileSize(SrcFile) - 1)
        FormatError(FileName, getmessage(Num_FormatInvRecordLenMsg));

      doit = FilterOK(InpCPU);

      if (doit)
      {
        SumLen += InpLen;
        WriteRecordHeader(&InpHeader, &InpCPU, &InpSegment, &InpGran, TargName, TargFile);
        if (!Write4(TargFile, &InpStart))
          ChkIO(TargName);
        if (!Write2(TargFile, &InpLen))
          ChkIO(TargName);
        while (InpLen > 0)
        {
          TransLen = min(BufferSize, InpLen);
          if (fread(Buffer, 1, TransLen, SrcFile) != TransLen)
            chk_wr_read_error(FileName);
          if (fwrite(Buffer, 1, TransLen, TargFile) != TransLen)
            ChkIO(TargName);
          InpLen -= TransLen;
        }
      }
      else
      {
        if (fseek(SrcFile, InpLen, SEEK_CUR) == -1)
          ChkIO(FileName);
      }
    }
    else
     SkipRecord(InpHeader, FileName, SrcFile);
  }
  while (InpHeader != FileHeaderEnd);

  if (msg_level >= e_msg_level_normal)
  {
    errno = 0; printf("  ("); ChkIO(OutName);
    errno = 0; printf(Integ32Format, SumLen); ChkIO(OutName);
    errno = 0; printf(" %s)\n", getmessage((SumLen == 1) ? Num_Byte : Num_Bytes)); ChkIO(OutName);
  }

  if (fclose(SrcFile) == EOF)
    ChkIO(FileName);
}

/* ------------------------------ */

static const as_cmd_rec_t BINDParams[] =
{
  { "f"        , CMD_FilterList      }
};

int main(int argc, char **argv)
{
  char *p_target_name;
  as_cmd_results_t cmd_results;

  if (!NLS_Initialize(&argc, argv))
    exit(4);
  endian_init();

  stdhandl_init();
  as_cmdarg_init(*argv);
  msg_level_init();
  toolutils_init(*argv);
  nls_init();
#ifdef _USE_MSH
  nlmessages_init_buffer(pbind_msh_data, sizeof(pbind_msh_data), MsgId1, MsgId2);
#else
  nlmessages_init_file("pbind.msg", *argv, MsgId1, MsgId2);
#endif
  ioerrs_init(*argv);

  Buffer = (Byte*) malloc(sizeof(Byte) * BufferSize);

  as_cmd_register(BINDParams, as_array_size(BINDParams));
  if (e_cmd_err == as_cmd_process(argc, argv, "BINDCMD", &cmd_results))
  {
    fprintf(stderr, "%s%s\n", getmessage(cmd_results.error_arg_in_env ? Num_ErrMsgInvEnvParam : Num_ErrMsgInvParam), cmd_results.error_arg);
    fprintf(stderr, "%s\n", getmessage(Num_ErrMsgProgTerm));
    exit(1);
  }

  if ((msg_level >= e_msg_level_verbose) || cmd_results.write_version_exit)
  {
    String Ver;
    as_snprintf(Ver, sizeof(Ver), "PBIND V%s", Version);
    WrCopyRight(Ver);
  }

  if (cmd_results.write_help_exit)
  {
    char *ph1, *ph2;

    errno = 0; printf("%s%s%s\n", getmessage(Num_InfoMessHead1), as_cmdarg_get_executable_name(), getmessage(Num_InfoMessHead2)); ChkIO(OutName);
    for (ph1 = getmessage(Num_InfoMessHelp), ph2 = strchr(ph1, '\n'); ph2; ph1 = ph2+1, ph2 = strchr(ph1, '\n'))
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
  strmaxcpy(TargName, p_target_name ? p_target_name : "", STRINGSIZE);
  free(p_target_name); p_target_name = NULL;
  if (!*TargName)
  {
    errno = 0; fprintf(stderr, "%s\n", getmessage(Num_ErrMsgTargetMissing)); ChkIO(OutName);
    exit(1);
  }
  AddSuffix(TargName, STRINGSIZE, getmessage(Num_Suffix));

  OpenTarget();

  while (True)
  {
    char *p_src_name = MoveAndCutStringListFirst(&cmd_results.file_arg_list);

    if (!p_src_name)
      break;
    if (*p_src_name)
      DirScan(p_src_name, ProcessFile);
    free(p_src_name);
  }

  CloseTarget();

  return 0;
}
