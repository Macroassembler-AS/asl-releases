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
#include "toolutils.h"
#include "nls.h"
#include "nlmessages.h"
#include "pbind.rsc"
#include "ioerrs.h"

#define BufferSize 8192

static const char *Creator = "BIND/C 1.42";

static as_cmd_processed_t ParUnprocessed;

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
    ChkIO(FileName);
  if (TestID != FileMagic)
    FormatError(FileName, getmessage(Num_FormatInvHeaderMsg));

  if (!QuietMode)
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
        ChkIO(FileName);
      WriteRecordHeader(&InpHeader, &InpCPU, &InpSegment, &InpGran, TargName, TargFile);
      if (!Write4(TargFile, &InpStart))
        ChkIO(TargName);
    }

    else if (InpHeader == FileHeaderDataRec)
    {
      if (!Read4(SrcFile, &InpStart))
        ChkIO(FileName);
      if (!Read2(SrcFile, &InpLen))
        ChkIO(FileName);

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
            ChkIO(FileName);
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

  if (!QuietMode)
  {
    errno = 0; printf("  ("); ChkIO(OutName);
    errno = 0; printf(Integ32Format, SumLen); ChkIO(OutName);
    errno = 0; printf(" %s)\n", getmessage((SumLen == 1) ? Num_Byte : Num_Bytes)); ChkIO(OutName);
  }

  if (fclose(SrcFile) == EOF)
    ChkIO(FileName);
}

static void ParamError(Boolean InEnv, char *Arg)
{
  fprintf(stderr, "%s%s\n", getmessage(InEnv ? Num_ErrMsgInvEnvParam : Num_ErrMsgInvParam), Arg);
  fprintf(stderr, "%s\n", getmessage(Num_ErrMsgProgTerm));
  exit(1);
}

static const as_cmd_rec_t BINDParams[] =
{
  { "f"        , CMD_FilterList},
  { "q"        , CMD_QuietMode },
  { "QUIET"    , CMD_QuietMode }
};

int main(int argc, char **argv)
{
  int z;
  char *ph1, *ph2;
  String Ver;

  if (!NLS_Initialize(&argc, argv))
    exit(4);
  endian_init();

  stdhandl_init();
  as_cmdarg_init(*argv);
  toolutils_init(*argv);
  nls_init();
  nlmessages_init("pbind.msg", *argv, MsgId1, MsgId2);
  ioerrs_init(*argv);

  Buffer = (Byte*) malloc(sizeof(Byte) * BufferSize);

  if (argc <= 1)
  {
    errno = 0; printf("%s%s%s\n", getmessage(Num_InfoMessHead1), GetEXEName(argv[0]), getmessage(Num_InfoMessHead2)); ChkIO(OutName);
    for (ph1 = getmessage(Num_InfoMessHelp), ph2 = strchr(ph1, '\n'); ph2; ph1 = ph2+1, ph2 = strchr(ph1, '\n'))
    {
      *ph2 = '\0';
      printf("%s\n", ph1);
      *ph2 = '\n';
    }
    exit(1);
  }

  as_cmd_process(argc, argv, BINDParams, as_array_size(BINDParams), ParUnprocessed, "BINDCMD", ParamError);

  if (!QuietMode)
  {
    as_snprintf(Ver, sizeof(Ver), "BIND/C V%s", Version);
    WrCopyRight(Ver);
  }

  z = argc - 1;
  while ((z > 0) && (!ParUnprocessed[z]))
    z--;
  if (z == 0)
  {
    errno = 0; fprintf(stderr, "%s\n", getmessage(Num_ErrMsgTargetMissing)); ChkIO(OutName);
    exit(1);
  }
  else
  {
    strmaxcpy(TargName, argv[z], STRINGSIZE); ParUnprocessed[z] = False;
    AddSuffix(TargName, STRINGSIZE, getmessage(Num_Suffix));
  }

  OpenTarget();

  for (z = 1; z < argc; z++)
   if (ParUnprocessed[z])
     DirScan(argv[z], ProcessFile);

  CloseTarget();

  return 0;
}
