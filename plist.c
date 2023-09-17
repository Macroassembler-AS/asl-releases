/* plist.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Anzeige des Inhalts einer Code-Datei                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "version.h"
#include "endian.h"
#include "bpemu.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "msg_level.h"
#include "nls.h"
#include "nlmessages.h"
#include "plist.rsc"
#ifdef _USE_MSH
# include "plist.msh"
#endif
#include "ioerrs.h"
#include "strutil.h"
#include "toolutils.h"
#include "headids.h"

/* --------------------------------------------------------------- */

static unsigned num_files;
LongWord Sums[SegCount];

/* --------------------------------------------------------------- */

static void ProcessSingle(const char *pFileName)
{
  FILE *ProgFile;
  Byte Header, Segment, Gran, CPU;
  const TFamilyDescr *FoundId;
  int Ch;
  Word Len, ID;
  LongWord StartAdr;
  Boolean HeadFnd;

  ProgFile = fopen(pFileName, OPENRDMODE);
  if (!ProgFile)
    ChkIO(pFileName);

  if (!Read2(ProgFile, &ID))
    chk_wr_read_error(pFileName);
  if (ID != FileMagic)
    FormatError(pFileName, getmessage(Num_FormatInvHeaderMsg));

  if (num_files > 1)
    printf("%s\n", pFileName);

  do
  {
    ReadRecordHeader(&Header, &CPU, &Segment, &Gran, pFileName, ProgFile);

    HeadFnd = False;

    if (num_files > 1)
      printf("%s", Blanks(strlen(getmessage(Num_MessHeaderLine1F))));
    if (Header == FileHeaderEnd)
    {
      errno = 0; fputs(getmessage(Num_MessGenerator), stdout); ChkIO(OutName);
      do
      {
        errno = 0; Ch = fgetc(ProgFile); ChkIO(pFileName);
        if (Ch != EOF)
        {
          errno = 0; putchar(Ch); ChkIO(OutName);
        }
      }
      while (Ch != EOF);
      errno = 0; printf("\n"); ChkIO(OutName);
      HeadFnd = True;
    }

    else if (Header == FileHeaderStartAdr)
    {
      if (!Read4(ProgFile, &StartAdr))
        chk_wr_read_error(pFileName);
      errno = 0;
      printf("%s%08lX\n", getmessage(Num_MessEntryPoint), LoDWord(StartAdr));
      ChkIO(OutName);
    }

    else if (Header == FileHeaderRelocInfo)
    {
      PRelocInfo RelocInfo;
      PRelocEntry PEntry;
      PExportEntry PExp;
      int z;

      RelocInfo = ReadRelocInfo(ProgFile);
      for (z = 0,  PEntry = RelocInfo->RelocEntries; z < RelocInfo->RelocCount; z++, PEntry++)
        printf("%s  %08lX        %3d:%d(%c)     %c%s\n",
               getmessage(Num_MessRelocInfo),
               LoDWord(PEntry->Addr), RelocBitCnt(PEntry->Type) >> 3,
               RelocBitCnt(PEntry->Type) & 7,
               (PEntry->Type & RelocFlagBig) ? 'B' : 'L',
               (PEntry->Type & RelocFlagSUB) ? '-' : '+', PEntry->Name);

      for (z = 0,  PExp = RelocInfo->ExportEntries; z < RelocInfo->ExportCount; z++, PExp++)
        printf("%s  %08lX          %c          %s\n",
               getmessage(Num_MessExportInfo),
               LoDWord(PExp->Value),
               (PExp->Flags & RelFlag_Relative) ? 'R' : ' ',
               PExp->Name);

      DestroyRelocInfo(RelocInfo);
    }

    else if ((Header == FileHeaderDataRec) || (Header == FileHeaderRDataRec)
          || (Header == FileHeaderRelocRec) || (Header == FileHeaderRRelocRec))
    {
      errno = 0;
      if (Magic != 0)
        FoundId = NULL;
      else
        FoundId = FindFamilyById(CPU);
      if (!FoundId)
        printf("\?\?\?=%02x        ", Header);
      else
        printf("%-13s ", FoundId->Name);
      ChkIO(OutName);

      errno = 0; printf("%-7s   ", SegNames[Segment]); ChkIO(OutName);

      if (!Read4(ProgFile, &StartAdr))
        chk_wr_read_error(pFileName);
      errno = 0; printf("%08lX          ", LoDWord(StartAdr)); ChkIO(OutName);

      if (!Read2(ProgFile, &Len))
        chk_wr_read_error(pFileName);
      errno = 0; printf("%04X       ", LoWord(Len));  ChkIO(OutName);

      if (Len != 0)
        StartAdr += (Len / Gran) - 1;
      else
        StartAdr--;
      errno = 0; printf("%08lX\n", LoDWord(StartAdr));  ChkIO(OutName);

      Sums[Segment] += Len;

      if (ftell(ProgFile) + Len >= FileSize(ProgFile))
        FormatError(pFileName, getmessage(Num_FormatInvRecordLenMsg));
      else if (fseek(ProgFile, Len, SEEK_CUR) != 0)
        ChkIO(pFileName);
    }
    else
     SkipRecord(Header, pFileName, ProgFile);
  }
  while (Header != 0);

  errno = 0; fclose(ProgFile); ChkIO(pFileName);

  (void)HeadFnd;
}

int main(int argc, char **argv)
{
  Word z;
  Boolean FirstSeg;
  as_cmd_results_t cmd_results;
  char *p_file_name;

  nls_init();
  if (!NLS_Initialize(&argc, argv))
    exit(4);

  endian_init();
  bpemu_init();
  strutil_init();
#ifdef _USE_MSH
  nlmessages_init_buffer(plist_msh_data, sizeof(plist_msh_data), MsgId1, MsgId2);
#else
  nlmessages_init_file("plist.msg", *argv, MsgId1, MsgId2); ioerrs_init(*argv);
#endif
  as_cmdarg_init(*argv);
  msg_level_init();
  toolutils_init(*argv);

  if (e_cmd_err == as_cmd_process(argc, argv, "PLISTCMD", &cmd_results))
  {
    fprintf(stderr, "%s%s\n", getmessage(cmd_results.error_arg_in_env ? Num_ErrMsgInvEnvParam : Num_ErrMsgInvParam), cmd_results.error_arg);
    fprintf(stderr, "%s\n", getmessage(Num_ErrMsgProgTerm));
    exit(1);
  }

  if ((msg_level >= e_msg_level_verbose) || cmd_results.write_version_exit)
  {
    String Ver;

    as_snprintf(Ver, sizeof(Ver), "PLIST V%s", Version);
    WrCopyRight(Ver);
    errno = 0; printf("\n"); ChkIO(OutName);
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

  num_files = StringListCount(cmd_results.file_arg_list);
  if (!num_files)
  {
    fprintf(stderr, "%s: %s\n", as_cmdarg_get_executable_name(), getmessage(Num_ErrMessNoInputFiles));
    exit(1);
  }

  errno = 0; printf("%s%s\n", (num_files > 1) ? getmessage(Num_MessHeaderLine1F) : "", getmessage(Num_MessHeaderLine1)); ChkIO(OutName);
  errno = 0; printf("%s%s\n", (num_files > 1) ? getmessage(Num_MessHeaderLine2F) : "", getmessage(Num_MessHeaderLine2)); ChkIO(OutName);

  for (z = 0; z < SegCount; Sums[z++] = 0);

  while (True)
  {
    p_file_name = MoveAndCutStringListFirst(&cmd_results.file_arg_list);
    if (!p_file_name)
      break;
    if (*p_file_name)
    {
      String exp_file_name;

      strmaxcpy(exp_file_name, p_file_name, sizeof(exp_file_name));
      AddSuffix(exp_file_name, sizeof(exp_file_name), getmessage(Num_Suffix));
      ProcessSingle(exp_file_name);
    }
    free(p_file_name);
  }

  errno = 0; printf("\n"); ChkIO(OutName);
  FirstSeg = True;
  for (z = 0; z < SegCount; z++)
    if ((z == SegCode) || Sums[z])
    {
      errno = 0;
      printf("%s", FirstSeg ? getmessage(Num_MessSum1) : Blanks(strlen(getmessage(Num_MessSum1))));
      printf(LongIntFormat, Sums[z]);
      printf("%s%s\n",
             getmessage((Sums[z] == 1) ? Num_MessSumSing : Num_MessSumPlur),
             SegNames[z]);
      FirstSeg = False;
    }

  return 0;
}
