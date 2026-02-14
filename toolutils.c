/* toolutils.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Unterroutinen fuer die AS-Tools                                           */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "be_le.h"
#include <string.h>
#include <stdarg.h>

#include "strutil.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "stdhandl.h"
#include "ioerrs.h"
#include "headids.h"

#include "nls.h"
#include "nlmessages.h"
#include "tools.rsc"
#ifdef _USE_MSH
# include "tools.msh"
#endif

#include "toolutils.h"

#include "version.h"

/****************************************************************************/

static Boolean DoFilter;
static int FilterCnt;
static Byte FilterBytes[100];

Word FileID = 0x1489;       /* Dateiheader Eingabedateien */
const char *OutName = "STDOUT";   /* Pseudoname Output */

String target_name;

static TMsgCat MsgCat;

Boolean QuietMode, Verbose;

/****************************************************************************/

void WrCopyRight(const char *Msg)
{
  printf("%s\n%s\n", Msg, InfoMessCopyright);
}

void DelSuffix(char *Name)
{
  char *p,*z,*Part;

  p = NULL;
  for (z = Name; *z != '\0'; z++)
    if (*z == '\\')
      p = z;
  Part = (p != NULL) ? p : Name;
  Part = strchr(Part, '.');
  if (Part != NULL)
    *Part = '\0';
}

void AddSuffix(char *pName, unsigned NameSize, const char *Suff)
{
  char *p, *z, *Part;

  p = NULL;
  for (z = pName; *z != '\0'; z++)
    if (*z == '\\')
      p = z;
  Part = (p != NULL) ? p : pName;
  if (strchr(Part, '.') == NULL)
    strmaxcat(pName, Suff, NameSize);
}

void FormatError(const char *Name, const char *Detail)
{
  fprintf(stderr, "%s%s%s (%s)\n",
          catgetmessage(&MsgCat, Num_FormatErr1aMsg),
          Name,
          catgetmessage(&MsgCat, Num_FormatErr1bMsg),
          Detail);
  fprintf(stderr, "%s\n",
          catgetmessage(&MsgCat, Num_FormatErr2Msg));
  exit(3);
}

static void wr_io_str(const char *p_name, const char *p_error_msg)
{
  fprintf(stderr, "%s%s%s\n",
          catgetmessage(&MsgCat, Num_IOErrAHeaderMsg),
          p_name,
          catgetmessage(&MsgCat, Num_IOErrBHeaderMsg));

  fprintf(stderr, "%s.\n", p_error_msg);

  fprintf(stderr, "%s\n",
          catgetmessage(&MsgCat, Num_ErrMsgTerminating));

  exit(2);
}

void ChkIO(const char *Name)
{
  if (errno)
    wr_io_str(Name, GetErrorMsg(errno));
}

void chk_wr_read_error(const char *p_name)
{
  wr_io_str(p_name, errno ? GetErrorMsg(errno) : GetReadErrorMsg());
}

int chkio_fprintf(FILE *p_file, const char *p_name, const char *p_fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, p_fmt);
  ret = vfprintf(p_file, p_fmt, ap);
  va_end(ap);
  if (ret < 0)
    ChkIO(p_name);
  return ret;
}

int chkio_printf(const char *p_name, const char *p_fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, p_fmt);
  ret = vprintf(p_fmt, ap);
  va_end(ap);
  if (ret < 0)
    ChkIO(p_name);
  return ret;
}

void ReadRecordHeader(Byte *Header, Byte *CPU, Byte* Segment,
                      Byte *Gran, const char *Name, FILE *f)
{
#ifdef _WIN32
   /* CygWin B20 seems to mix up the file pointer under certain
      conditions. Difficult to reproduce, so we reposition it. */

  long pos;

  pos = ftell(f);
  fflush(f);
  rewind(f);
  fseek(f, pos, SEEK_SET);
#endif

  if (fread(Header, 1, 1, f) != 1)
    chk_wr_read_error(Name);
  if ((*Header != FileHeaderEnd) && (*Header != FileHeaderStartAdr))
  {
    if ((*Header == FileHeaderDataRec) || (*Header == FileHeaderRDataRec) ||
        (*Header == FileHeaderRelocRec) || (*Header == FileHeaderRRelocRec))
    {
      if (fread(CPU, 1, 1, f) != 1)
        chk_wr_read_error(Name);
      if (fread(Segment, 1, 1, f) != 1)
        chk_wr_read_error(Name);
      if (fread(Gran, 1, 1, f) != 1)
        chk_wr_read_error(Name);
    }
    else if (*Header <= 0x7f)
    {
      const TFamilyDescr *p_descr;

      *CPU = *Header;
      p_descr = FindFamilyById(*CPU);
      *Header = FileHeaderDataRec;
      *Segment = SegCode;
      *Gran = p_descr->get_granularity((as_addrspace_t)*Segment);
    }
  }
}

void WriteRecordHeader(Byte *Header, Byte *CPU, Byte *Segment,
                       Byte *Gran, const char *Name, FILE *f)
{
  if ((*Header == FileHeaderEnd) || (*Header == FileHeaderStartAdr))
  {
    if (fwrite(Header, 1, 1, f) != 1)
      ChkIO(Name);
  }
  else if ((*Header == FileHeaderDataRec) || (*Header == FileHeaderRDataRec))
  {
    const TFamilyDescr *p_descr = FindFamilyById(*CPU);
    Word def_granularity = p_descr ? p_descr->get_granularity((as_addrspace_t)*Segment) : 1;

    if ((*Segment != SegCode) || (*Gran != def_granularity) || (*CPU >= 0x80))
    {
      if (fwrite(Header, 1, 1, f))
        ChkIO(Name);
      if (fwrite(CPU, 1, 1, f))
        ChkIO(Name);
      if (fwrite(Segment, 1, 1, f))
        ChkIO(Name);
      if (fwrite(Gran, 1, 1, f))
        ChkIO(Name);
    }
    else
    {
      if (fwrite(CPU, 1, 1, f))
        ChkIO(Name);
    }
  }
  else
  {
    if (fwrite(CPU, 1, 1, f))
      ChkIO(Name);
  }
}

Byte record_gran_bits(Byte gran_encoded)
{
  return (gran_encoded >= 0xf8)
       ? (256 - gran_encoded)
       : gran_encoded * 8;
}

LongWord record_byte_address(LongWord target_word_address, Byte gran_encoded)
{
  return (gran_encoded > 0xf8)
       ? (target_word_address / (8 / (256 - gran_encoded)))
       : (target_word_address * gran_encoded);
}

LongWord record_target_word_last_address(LongWord target_word_start_address, LongWord length_bytes, Byte gran_encoded)
{
  return (gran_encoded > 0xf8)
       ? (target_word_start_address + (((LongWord)length_bytes * 8) / (256 - gran_encoded)) - 1)
       : (target_word_start_address + (length_bytes / gran_encoded) - 1);
}

LongWord record_target_word_length(LongWord byte_length, Byte gran_encoded)
{
  return (gran_encoded > 0xf8)
       ? (byte_length * 8 / record_gran_bits(gran_encoded))
       : (byte_length / gran_encoded);
}

LongWord record_byte_length(LongWord target_word_first_address, LongWord target_word_last_address, Byte gran_encoded)
{
  return (gran_encoded > 0xf8)
       ? ((target_word_last_address - target_word_first_address + 1) / (8 / (256 - gran_encoded)))
       : ((target_word_last_address - target_word_first_address + 1) * gran_encoded);
}

LongWord record_byte_offset(LongWord target_word_act_address, LongWord target_word_start_address, Byte gran_encoded)
{
  return (gran_encoded > 0xf8)
       ? ((target_word_act_address - target_word_start_address) / (8 / (256 - gran_encoded)))
       : ((target_word_act_address - target_word_start_address) * gran_encoded);
}

void SkipRecord(Byte Header, const char *Name, FILE *f)
{
  int Length;
  LongWord Addr, RelocCount, ExportCount, StringLen;
  Word Len;

  switch (Header)
  {
    case FileHeaderStartAdr:
      Length = 4;
      break;
    case FileHeaderEnd:
      Length = 0;
      break;
    case FileHeaderRelocInfo:
      if (!Read4(f, &RelocCount))
        ChkIO(Name);
      if (!Read4(f, &ExportCount))
        ChkIO(Name);
      if (!Read4(f, &StringLen))
        ChkIO(Name);
      Length = (16 * RelocCount) + (16 * ExportCount) + StringLen;
      break;
    default:
      if (!Read4(f, &Addr))
        ChkIO(Name);
      if (!Read2(f, &Len))
        ChkIO(Name);
      Length = Len;
      break;
  }

  if (fseek(f, Length, SEEK_CUR) != 0) ChkIO(Name);
}

PRelocInfo ReadRelocInfo(FILE *f)
{
  PRelocInfo PInfo;
  PRelocEntry PEntry;
  PExportEntry PExp;
  Boolean OK = FALSE;
  LongWord StringLen, StringPos;
  LongInt z;

  /* get memory for structure */

  PInfo = (PRelocInfo) malloc(sizeof(TRelocInfo));
  if (PInfo != NULL)
  {
    PInfo->RelocEntries = NULL;
    PInfo->ExportEntries = NULL;
    PInfo->Strings = NULL;

    /* read global numbers */

    if ((Read4(f, &PInfo->RelocCount))
     && (Read4(f, &PInfo->ExportCount))
     && (Read4(f, &StringLen)))
    {
      /* allocate memory */

      PInfo->RelocEntries = (PRelocEntry) malloc(sizeof(TRelocEntry) * PInfo->RelocCount);
      if ((PInfo->RelocCount == 0) || (PInfo->RelocEntries != NULL))
      {
        PInfo->ExportEntries = (PExportEntry) malloc(sizeof(TExportEntry) * PInfo->ExportCount);
        if ((PInfo->ExportCount == 0) || (PInfo->ExportEntries != NULL))
        {
          PInfo->Strings = (char*) malloc(sizeof(char) * StringLen);
          if ((StringLen == 0) || (PInfo->Strings != NULL))
          {
            /* read relocation entries */

            for (z = 0, PEntry = PInfo->RelocEntries; z < PInfo->RelocCount; z++, PEntry++)
            {
              if (!Read8(f, &PEntry->Addr))
                break;
              if (!Read4(f, &StringPos))
                break;
              PEntry->Name = PInfo->Strings + StringPos;
              if (!Read4(f, &PEntry->Type))
                break;
            }

            /* read export entries */

            for (z = 0, PExp = PInfo->ExportEntries; z < PInfo->ExportCount; z++, PExp++)
            {
              if (!Read4(f, &StringPos))
                break;
              PExp->Name = PInfo->Strings + StringPos;
              if (!Read4(f, &PExp->Flags))
                break;
              if (!Read8(f, &PExp->Value))
                break;
            }

            /* read strings */

            if (z == PInfo->ExportCount)
              OK = ((fread(PInfo->Strings, 1, StringLen, f)) == StringLen);
          }
        }
      }
    }
  }

  if (!OK)
  {
    if (PInfo != NULL)
    {
      DestroyRelocInfo(PInfo);
      PInfo = NULL;
    }
  }

  return PInfo;
}

void DestroyRelocInfo(PRelocInfo PInfo)
{
  if (PInfo->Strings != NULL)
  {
    free(PInfo->Strings);
    PInfo->Strings = NULL;
  }
  if ((PInfo->ExportCount > 0) && (PInfo->RelocEntries != NULL))
  {
    free(PInfo->RelocEntries);
    PInfo->RelocEntries = NULL;
  }
  if ((PInfo->RelocCount > 0) && (PInfo->ExportEntries != NULL))
  {
    free(PInfo->ExportEntries);
    PInfo->ExportEntries = NULL;
  }
  free (PInfo);
}

as_cmd_result_t CMD_FilterList(Boolean Negate, const char *Arg)
{
  Byte FTemp;
  const char *p_end;
  char *p;
  int Search;
  String Copy;

  if (*Arg == '\0')
    return e_cmd_err;
  strmaxcpy(Copy, Arg, STRINGSIZE);

  do
  {
    p = strchr(Copy,',');
    if (p != NULL)
      *p = '\0';
    FTemp = as_cmd_strtol(Copy, &p_end);
    if (*p_end)
      return e_cmd_err;

    for (Search = 0; Search < FilterCnt; Search++)
      if (FilterBytes[Search] == FTemp)
        break;

    if ((Negate) && (Search < FilterCnt))
      FilterBytes[Search] = FilterBytes[--FilterCnt];

    else if ((!Negate) && (Search >= FilterCnt))
      FilterBytes[FilterCnt++] = FTemp;

    if (p != NULL)
      strmov(Copy, p + 1);
  }
  while (p != NULL);

  DoFilter = (FilterCnt != 0);

  return e_cmd_arg;
}

as_cmd_result_t CMD_Range(LongWord *pStartAddr, LongWord *pStopAddr,
                          Boolean *pStartAuto, Boolean *pStopAuto,
                          const char *Arg)
{
  const char *p, *p_end;
  String StartStr;

  p = strchr(Arg, '-');
  if (!p) return e_cmd_err;

  strmemcpy(StartStr, sizeof(StartStr), Arg, p - Arg);
  *pStartAuto = AddressWildcard(StartStr);
  if (!*pStartAuto)
  {
    *pStartAddr = as_cmd_strtol(StartStr, &p_end);
    if (*p_end)
      return e_cmd_err;
  }

  *pStopAuto = AddressWildcard(p + 1);
  if (!*pStopAuto)
  {
    *pStopAddr = as_cmd_strtol(p + 1, &p_end);
    if (*p_end)
      return e_cmd_err;
  }

  if (!*pStartAuto && !*pStopAuto && (*pStartAddr > *pStopAddr))
    return e_cmd_err;

  return e_cmd_arg;
}

as_cmd_result_t CMD_QuietMode(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  QuietMode = !Negate;
  return e_cmd_ok;
}

as_cmd_result_t CMD_Verbose(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  Verbose = !Negate;
  return e_cmd_ok;
}

as_cmd_result_t cmd_target_name(Boolean negate, const char *p_arg)
{
  if (negate)
  {
    *target_name = '\0';
    return e_cmd_ok;
  }
  else if (!p_arg || !*p_arg)
    return e_cmd_err;
  else
  {
    strmaxcpy(target_name, p_arg, sizeof target_name);
    return e_cmd_arg;
  }
}

Boolean FilterOK(Byte Header)
{
  int z;

  if (DoFilter)
  {
    for (z = 0; z < FilterCnt; z++)
     if (Header == FilterBytes[z])
       return True;
    return False;
  }
  else
    return True;
}

Boolean RemoveOffset(char *Name, LongWord *Offset)
{
  int z, Nest;

  *Offset = 0;
  if ((*Name) && (Name[strlen(Name)-1] == ')'))
  {
    z = strlen(Name) - 2;
    Nest = 0;
    while ((z >= 0) && (Nest >= 0))
    {
      switch (Name[z])
      {
        case '(': Nest--; break;
        case ')': Nest++; break;
      }
      if (Nest != -1)
        z--;
    }
    if (Nest != -1)
      return False;
    else
    {
      const char *p_end;

      Name[strlen(Name) - 1] = '\0';
      *Offset = as_cmd_strtol(Name + z + 1, &p_end);
      Name[z] = '\0';
      return !*p_end;
    }
  }
  else
    return True;
}

void EraseFile(const char *FileName, LongWord Offset)
{
  UNUSED(Offset);

  if (unlink(FileName) == -1)
    ChkIO(FileName);
}

void toolutils_init(const char *ProgPath)
{
  version_init();

#ifdef _USE_MSH
  msg_catalog_open_buffer(&MsgCat, tools_msh_data, sizeof(tools_msh_data), MsgId1, MsgId2);
  UNUSED(ProgPath);
#else
  msg_catalog_open_file(&MsgCat, "tools.msg", ProgPath, MsgId1, MsgId2);
#endif

  FilterCnt = 0;
  DoFilter = False;
}

Boolean AddressWildcard(const char *addr)
{
  return ((strcmp(addr, "$") == 0) || (as_strcasecmp(addr, "0x") == 0));
}

#ifdef CKMALLOC
#undef malloc
#undef realloc

void *ckmalloc(size_t s)
{
  void *tmp = malloc(s);
  if (tmp == NULL)
  {
    fprintf(stderr,"allocation error(malloc): out of memory");
    exit(255);
  }
  return tmp;
}

void *ckrealloc(void *p, size_t s)
{
  void *tmp = realloc(p,s);
  if (tmp == NULL)
  {
    fprintf(stderr,"allocation error(realloc): out of memory");
    exit(255);
  }
  return tmp;
}
#endif

void WrError(Word Num)
{
  UNUSED(Num);
}
