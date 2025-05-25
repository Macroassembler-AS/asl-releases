/* asmcode.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung der Code-Datei                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "version.h"
#include "be_le.h"
#include "chunks.h"
#include "as.h"
#include "asmdef.h"
#include "errmsg.h"
#include "strutil.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmrelocs.h"
#include "asmlist.h"
#include "asmlabel.h"

#include "asmcode.h"

#define CodeBufferSize 512

static Word LenSoFar;
static LongInt RecPos, LenPos;
static Boolean ThisRel;

static Word CodeBufferFill;
static Byte *CodeBuffer;

PPatchEntry PatchList, PatchLast;
PExportEntry ExportList, ExportLast;
LongInt SectSymbolCounter;
String SectSymbolName;

static LongWord code_len_guessed = 0, max_code_len_guessed = 0;
static Byte *basmcode_guessed;
static Word *wasmcode_guessed;
static LongWord *dasmcode_guessed;

static void FlushBuffer(void)
{
  if (CodeBufferFill > 0)
  {
    if (fwrite(CodeBuffer, 1, CodeBufferFill, PrgFile) != CodeBufferFill)
      ChkIO(ErrNum_FileWriteError);
    CodeBufferFill = 0;
  }
}

void DreheCodes(void)
{
  int z;
  LongInt l = CodeLen * Granularity();

  switch (ActListGran)
  {
    case 2:
      for (z = 0; z < l >> 1; z++)
        WAsmCode[z] = ((WAsmCode[z] & 0xff) << 8) + ((WAsmCode[z] & 0xff00) >> 8);
      break;
    case 4:
      for (z = 0; z < l >> 2; z++)
      {
        LongWord Dest;
        int z2;

        for (z2 = 0, Dest = 0; z2 < 4; z2++)
        {
          Dest = (Dest << 8) | (DAsmCode[z] & 0xff);
          DAsmCode[z] >>= 8;
        }
        DAsmCode[z] = Dest;
      }
      break;
  }
}

static void WrPatches(void)
{
  LongWord Cnt, ExportCnt, StrLen;
  Byte T8;

  if (PatchList || ExportList)
  {
    /* find out length of string field */

    Cnt = StrLen = 0;
    for (PatchLast = PatchList; PatchLast; PatchLast = PatchLast->Next)
    {
      Cnt++;
      StrLen += (PatchLast->len = strlen(PatchLast->Ref) + 1);
    }
    ExportCnt = 0;
    for (ExportLast = ExportList; ExportLast; ExportLast = ExportLast->Next)
    {
      ExportCnt++;
      StrLen += (ExportLast->len = strlen(ExportLast->Name) + 1);
    }

    /* write header */

    T8 = FileHeaderRelocInfo;
    if (fwrite(&T8, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
    if (!Write4(PrgFile, &Cnt)) ChkIO(ErrNum_FileWriteError);
    if (!Write4(PrgFile, &ExportCnt)) ChkIO(ErrNum_FileWriteError);
    if (!Write4(PrgFile, &StrLen)) ChkIO(ErrNum_FileWriteError);

    /* write patch entries */

    StrLen = 0;
    for (PatchLast = PatchList; PatchLast; PatchLast = PatchLast->Next)
    {
      if (!Write8(PrgFile, &(PatchLast->Address))) ChkIO(ErrNum_FileWriteError);
      if (!Write4(PrgFile, &StrLen)) ChkIO(ErrNum_FileWriteError);
      if (!Write4(PrgFile, &(PatchLast->RelocType))) ChkIO(ErrNum_FileWriteError);
      StrLen += PatchLast->len;
    }

    /* write export entries */

    for (ExportLast = ExportList; ExportLast; ExportLast = ExportLast->Next)
    {
      if (!Write4(PrgFile, &StrLen)) ChkIO(ErrNum_FileWriteError);
      if (!Write4(PrgFile, &(ExportLast->Flags))) ChkIO(ErrNum_FileWriteError);
      if (!Write8(PrgFile, &(ExportLast->Value))) ChkIO(ErrNum_FileWriteError);
      StrLen += ExportLast->len;
    }

    /* write string table, free structures */

    while (PatchList)
    {
      PatchLast = PatchList;
      if (fwrite(PatchLast->Ref, 1, PatchLast->len, PrgFile) != PatchLast->len) ChkIO(ErrNum_FileWriteError);
      free(PatchLast->Ref);
      PatchList = PatchLast->Next;
      free(PatchLast);
    }
    PatchLast = NULL;

    while (ExportList)
    {
      ExportLast = ExportList;
      if (fwrite(ExportLast->Name, 1, ExportLast->len, PrgFile) != ExportLast->len) ChkIO(ErrNum_FileWriteError);
      free(ExportLast->Name);
      ExportList = ExportLast->Next;
      free(ExportLast);
    }
    ExportLast = NULL;
  }
}

/*--- neuen Record in Codedatei anlegen.  War der bisherige leer, so wird ---
 ---- dieser ueberschrieben. ------------------------------------------------*/

static void WrRecHeader(void)
{
  Byte b;

  /* assume simple record without relocation info */

  ThisRel = RelSegs;
  b = ThisRel ? FileHeaderRelocRec : FileHeaderDataRec;
  if (fwrite(&b, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
  if (fwrite(&HeaderID, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
  b = ActPC; if (fwrite(&b, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
  b = Grans[ActPC]; if (fwrite(&b, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
  fflush(PrgFile);
}

void NewRecord(LargeWord NStart)
{
  LongInt h;
  LongWord PC;
  Byte Header;

  /* flush remaining code in buffer */

  FlushBuffer();

  /* zero length record which may be deleted ? */
  /* do not write out patches at this place - they
     will be merged with the next record. */

  if (LenSoFar == 0)
  {
    if (fseek(PrgFile, RecPos, SEEK_SET) != 0) ChkIO(ErrNum_FileReadError);
    WrRecHeader();
    h = NStart;
    if (!Write4(PrgFile, &h)) ChkIO(ErrNum_FileWriteError);
    LenPos = ftell(PrgFile);
    if (!Write2(PrgFile, &LenSoFar)) ChkIO(ErrNum_FileWriteError);
  }

  /* otherwise full record */

  else
  {
    /* store current position (=end of file) */

    h = ftell(PrgFile);

    /* do we have reloc. info? - then change record type */

    if (PatchList || ExportList)
    {
      fflush(PrgFile);
      if (fseek(PrgFile, RecPos, SEEK_SET) != 0) ChkIO(ErrNum_FileReadError);
      Header = ThisRel ? FileHeaderRRelocRec : FileHeaderRDataRec;
      if (fwrite(&Header, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
    }

    /* fill in length of record */

    fflush(PrgFile);
    if (fseek(PrgFile, LenPos, SEEK_SET) != 0) ChkIO(ErrNum_FileReadError);
    if (!Write2(PrgFile, &LenSoFar)) ChkIO(ErrNum_FileWriteError);

    /* go back to end of file */

    if (fseek(PrgFile, h, SEEK_SET) != 0) ChkIO(ErrNum_FileReadError);

    /* write out reloc info */

    WrPatches();

    /* store begin of new code record */

    RecPos = ftell(PrgFile);

    LenSoFar = 0;
    WrRecHeader();
    ThisRel = RelSegs;
    PC = NStart;
    if (!Write4(PrgFile, &PC)) ChkIO(ErrNum_FileWriteError);
    LenPos = ftell(PrgFile);
    if (!Write2(PrgFile, &LenSoFar)) ChkIO(ErrNum_FileWriteError);
  }
#if 0
  /* put in the hidden symbol for the relocatable segment ? */

  if ((RelSegs) && (strcmp(CurrFileName, "INTERNAL")))
  {
    as_snprintf(SectSymbolName, sizeof(SectSymbolName), "__%s_%d",
                NamePart(CurrFileName), (int)(SectSymbolCounter++));
    AddExport(SectSymbolName, ProgCounter());
  }
#endif
}

/*--- Codedatei eroeffnen --------------------------------------------------*/

void OpenFile(void)
{
  Word h;

  errno = 0;
  PrgFile = fopen(OutName, OPENWRMODE);
  if (!PrgFile)
    ChkXIO(ErrNum_OpeningFile, OutName);

  errno = 0;
  h = FileMagic;
  if (!Write2(PrgFile,&h)) ChkIO(ErrNum_FileWriteError);

  CodeBufferFill = 0;
  RecPos = ftell(PrgFile);
  LenSoFar = 0;
  NewRecord(PCs[ActPC]);
}

/*---- Codedatei schliessen -------------------------------------------------*/

void CloseFile(void)
{
  Byte Head;
  String h;
  LongWord Adr;

  as_snprintf(h, sizeof(h), "AS %s/%s-%s", Version, ARCHPRNAME, ARCHSYSNAME);

  NewRecord(PCs[ActPC]);
  fseek(PrgFile, RecPos, SEEK_SET);

  if (StartAdrPresent)
  {
    Head = FileHeaderStartAdr;
    if (fwrite(&Head,sizeof(Head), 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
    Adr = StartAdr;
    if (!Write4(PrgFile,&Adr)) ChkIO(ErrNum_FileWriteError);
  }

  Head = FileHeaderEnd;
  if (fwrite(&Head,sizeof(Head), 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
  if (fwrite(h, 1, strlen(h), PrgFile) != strlen(h)) ChkIO(ErrNum_FileWriteError);
  fclose(PrgFile);
  if (Magic)
    unlink(OutName);
}

/*--- erzeugten Code einer Zeile in Datei ablegen ---------------------------*/

void WriteBytes(void)
{
  Word ErgLen;

  if (CodeLen == 0)
    return;
  ErgLen = CodeLen * Granularity();
  if ((TurnWords != 0) != (HostBigEndian != 0))
    DreheCodes();
  if (((LongInt)LenSoFar) + ((LongInt)ErgLen) > 0xffff)
    NewRecord(PCs[ActPC]);
  if (CodeBufferFill + ErgLen < CodeBufferSize)
  {
    memcpy(CodeBuffer + CodeBufferFill, BAsmCode, ErgLen);
    CodeBufferFill += ErgLen;
  }
  else
  {
    FlushBuffer();
    if (ErgLen < CodeBufferSize)
    {
      memcpy(CodeBuffer, BAsmCode, ErgLen);
      CodeBufferFill = ErgLen;
    }
    else if (fwrite(BAsmCode, 1, ErgLen, PrgFile) != ErgLen)
      ChkIO(ErrNum_FileWriteError);
  }
  LenSoFar += ErgLen;
  if ((TurnWords != 0) != (HostBigEndian != 0))
    DreheCodes();
}

void RetractWords(Word Cnt)
{
  Word ErgLen;

  ErgLen = Cnt * Granularity();
  if (LenSoFar < ErgLen)
  {
    WrError(ErrNum_ParNotPossible);
    return;
  }

  if (MakeUseList)
    DeleteChunk(SegChunks + ActPC, ProgCounter() - Cnt, Cnt);

  PCs[ActPC] -= Cnt;

  if (CodeBufferFill >= ErgLen)
    CodeBufferFill -= ErgLen;
  else
  {
    if (fseek(PrgFile, -(ErgLen - CodeBufferFill), SEEK_CUR) == -1)
      ChkIO(ErrNum_FileWriteError);
    CodeBufferFill = 0;
  }

  LenSoFar -= ErgLen;

  Retracted = True;
}

/*!------------------------------------------------------------------------
 * \fn     InsertPadding(unsigned NumBytes, Boolean OnlyReserve)
 * \brief  insert padding bytes into code
 * \param  NumBytes # of bytes to add
 * \param  OnlyReserve write code or only reserve?
 * ------------------------------------------------------------------------ */

void InsertPadding(unsigned NumBytes, Boolean OnlyReserve)
{
  Boolean SaveDontPrint = DontPrint;
  LargeWord OldValue = EProgCounter();

  /* write/reserve code */

  SetMaxCodeLen(NumBytes);
  DontPrint = OnlyReserve;
  memset(BAsmCode, 0, CodeLen = NumBytes);
  WriteCode();
  MakeList("<padding>");

  /* fix up possible label value so it points to the actual code */

  LabelModify(OldValue, EProgCounter());

  CodeLen = 0;
  DontPrint = SaveDontPrint;
}

void code_len_reset(void)
{
  CodeLen = code_len_guessed = 0;
  DontPrint = False;
}

static void assure_max_code_len_guessed(LongWord new_max_len)
{
  if (new_max_len > max_code_len_guessed)
  {
    LongWord *p_new_code_len_guessed = max_code_len_guessed ?
             (LongWord*)realloc(dasmcode_guessed, new_max_len) : (LongWord*)malloc(new_max_len);
    if (!p_new_code_len_guessed)
      return;
    basmcode_guessed = (Byte *)p_new_code_len_guessed;
    wasmcode_guessed = (Word *) p_new_code_len_guessed;
    dasmcode_guessed = (LongWord *)p_new_code_len_guessed;
    memset(&basmcode_guessed[max_code_len_guessed], 0x00, new_max_len - max_code_len_guessed);
    max_code_len_guessed = new_max_len;
  }
}

static void extend_zero_basmcode_guessed_len(LongWord new_len)
{
  assure_max_code_len_guessed(new_len);
  if (new_len > code_len_guessed)
  {
    memset(&basmcode_guessed[code_len_guessed], 0x00, new_len - code_len_guessed);
    code_len_guessed = new_len;
  }
}

static void extend_zero_wasmcode_guessed_len(LongWord new_len)
{
  /* If actual granularity is smaller than 16 bits, code_len_guessed
     must be counted in the smaller unit: */

  if (Granularity() < 2)
    extend_zero_basmcode_guessed_len(new_len * 2);
  else
  {
    assure_max_code_len_guessed(new_len * 2);
    if (new_len > code_len_guessed)
    {
      memset(&wasmcode_guessed[code_len_guessed], 0x00, (new_len - code_len_guessed) * 2);
      code_len_guessed = new_len;
    }
  }
}

static void extend_zero_dasmcode_guessed_len(LongWord new_len)
{
  /* If actual granularity is smaller than 32 bits, code_len_guessed
     must be counted in the smaller unit: */

  if (Granularity() < 4)
    extend_zero_wasmcode_guessed_len(new_len * 2);
  else
  {
    assure_max_code_len_guessed(new_len * 4);
    if (new_len > code_len_guessed)
    {
      memset(&dasmcode_guessed[code_len_guessed], 0x00, (new_len - code_len_guessed) * 4);
      code_len_guessed = new_len;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     set_basmcode_guessed(LongWord start, LongWord count, Byte value)
 * \brief  set guessed mask of bytes
 * \param  start start index of bytes to set
 * \param  count # of bytes to set
 * \param  value mask to set on bytes
 * ------------------------------------------------------------------------ */

void set_basmcode_guessed(LongWord start, LongWord count, Byte value)
{
  LongWord end = start + count;

  if (end < start)
    return;
  extend_zero_basmcode_guessed_len(end);
  memset(&basmcode_guessed[start], value, count);
}

/*!------------------------------------------------------------------------
 * \fn     copy_basmcode_guessed(LongWord dest, LongWord src, size_t count)
 * \brief  replicate or move guessed mask of bytes
 * \param  dest where to replicate to
 * \param  src where to replicate from
 * \param  count # of bytes to replicate
 * ------------------------------------------------------------------------ */

void copy_basmcode_guessed(LongWord dest, LongWord src, size_t count)
{
  LongWord dest_end = dest + count,
           src_end = src + count;

  if ((dest_end < dest) || (src_end < src))
    return;
  extend_zero_basmcode_guessed_len((dest_end > src_end) ? dest_end : src_end);
  memmove(&basmcode_guessed[dest], &basmcode_guessed[src], count);
}

/*!------------------------------------------------------------------------
 * \fn     or_basmcode_guessed(LongWord start, LongWord count, Byte value)
 * \brief  augment guessed mask of bytes
 * \param  start start index of bytes to set
 * \param  count # of bytes to set
 * \param  value mask of bits to set on ytes
 * ------------------------------------------------------------------------ */

void or_basmcode_guessed(LongWord start, LongWord count, Byte value)
{
  LongWord end = start + count;

  if (end < start)
    return;
  extend_zero_basmcode_guessed_len(end);
  for (; start < end; start++)
    basmcode_guessed[start] |= value;
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_guessed(LongWord index)
 * \brief  retrieve guessed mask of byte #n
 * \param  index n-th DWord
 * \return mask or zero if beyond code_len_guessed
 * ------------------------------------------------------------------------ */

Byte get_basmcode_guessed(LongWord index)
{
  return (index < code_len_guessed) ? basmcode_guessed[index] : 0;
}

/*!------------------------------------------------------------------------
 * \fn     set_wasmcode_guessed(LongWord start, LongWord count, Word value)
 * \brief  set guessed mask of 16-bit words
 * \param  start start index of words to set
 * \param  count # of words to set
 * \param  value mask to set on words
 * ------------------------------------------------------------------------ */

void set_wasmcode_guessed(LongWord start, LongWord count, Word value)
{
  LongWord end = start + count;

  if (end < start)
    return;
  extend_zero_wasmcode_guessed_len(end);
  for (; start < end; start++)
    wasmcode_guessed[start] = value;
}

/*!------------------------------------------------------------------------
 * \fn     copy_wasmcode_guessed(LongWord dest, LongWord src, size_t count)
 * \brief  replicate/move guessed mask of 16-bit words
 * \param  dest where to replicate to
 * \param  src where to replicate from
 * \param  count # of words to replicate
 * ------------------------------------------------------------------------ */

void copy_wasmcode_guessed(LongWord dest, LongWord src, size_t count)
{
  LongWord dest_end = dest + count,
           src_end = src + count;

  if ((dest_end < dest) || (src_end < src))
    return;
  extend_zero_wasmcode_guessed_len((dest_end > src_end) ? dest_end : src_end);
  memmove(&wasmcode_guessed[dest], &wasmcode_guessed[src], count * 2);
}

/*!------------------------------------------------------------------------
 * \fn     or_wasmcode_guessed(LongWord start, LongWord count, Word value)
 * \brief  augment guessed mask of 16 bit words
 * \param  start start index of words to set
 * \param  count # of words to set
 * \param  value mask of bits to set on words
 * ------------------------------------------------------------------------ */

void or_wasmcode_guessed(LongWord start, LongWord count, Word value)
{
  LongWord end = start + count;

  if (end < start)
    return;
  extend_zero_wasmcode_guessed_len(end);
  for (; start < end; start++)
    wasmcode_guessed[start] |= value;
}

/*!------------------------------------------------------------------------
 * \fn     get_wasmcode_guessed(LongWord index)
 * \brief  retrieve guessed mask of 16-bit word #n
 * \param  index n-th DWord
 * \return mask or zero if beyond code_len_guessed
 * ------------------------------------------------------------------------ */

Word get_wasmcode_guessed(LongWord index)
{
  return (index < code_len_guessed) ? wasmcode_guessed[index] : 0;
}

/*!------------------------------------------------------------------------
 * \fn     dump_wasmcode_guessed(const char *p_title)
 * \brief  dump guessed masks as 16-bit values
 * \param  p_titleoptional dump title
 * ------------------------------------------------------------------------ */

void dump_wasmcode_guessed(const char *p_title)
{
  int z;

  if (p_title) printf("%s:", p_title);
  if (Granularity() < 2)
  {
    for (z = 0; z < CodeLen >> 1; z++) printf(" %04x", get_wasmcode_guessed(z));
    if (CodeLen & 1) printf(" %02x", get_basmcode_guessed(CodeLen - 1));
  }
  else
  {
    for (z = 0; z < CodeLen; z++) printf(" %04x", get_wasmcode_guessed(z));
  }
  printf("\n");
}

/*!------------------------------------------------------------------------
 * \fn     set_dasmcode_guessed(LongWord start, LongWord count, LongWord value)
 * \brief  set guessed mask of 32 bit words
 * \param  start start index of dwords to set
 * \param  count # of dwords to set
 * \param  value mask to set on dwords
 * ------------------------------------------------------------------------ */

void set_dasmcode_guessed(LongWord start, LongWord count, LongWord value)
{
  LongWord end = start + count;

  if (end < start)
    return;
  extend_zero_dasmcode_guessed_len(end);
  for (; start < end; start++)
    dasmcode_guessed[start] = value;
}

/*!------------------------------------------------------------------------
 * \fn     copy_dasmcode_guessed(LongWord dest, LongWord src, size_t count)
 * \brief  replicate/move guessed mask of 32-bit words
 * \param  dest where to replicate to
 * \param  src where to replicate from
 * \param  count # of dwords to replicate
 * ------------------------------------------------------------------------ */

void copy_dasmcode_guessed(LongWord dest, LongWord src, size_t count)
{
  LongWord dest_end = dest + count,
           src_end = src + count;

  if ((dest_end < dest) || (src_end < src))
    return;
  extend_zero_dasmcode_guessed_len((dest_end > src_end) ? dest_end : src_end);
  memmove(&dasmcode_guessed[dest], &dasmcode_guessed[src], count * 4);
}

/*!------------------------------------------------------------------------
 * \fn     or_dasmcode_guessed(LongWord start, LongWord count, LongWord value)
 * \brief  augment guessed mask of 32 bit words
 * \param  start start index of dwords to set
 * \param  count # of dwords to set
 * \param  value mask of bits to set on dwords
 * ------------------------------------------------------------------------ */

void or_dasmcode_guessed(LongWord start, LongWord count, LongWord value)
{
  LongWord end = start + count;

  if (end < start)
    return;
  extend_zero_dasmcode_guessed_len(end);
  for (; start < end; start++)
    dasmcode_guessed[start] |= value;
}

/*!------------------------------------------------------------------------
 * \fn     get_dasmcode_guessed(LongWord index)
 * \brief  retrieve guessed mask of 32-bit word #n
 * \param  index n-th DWord
 * \return mask or zero if beyond code_len_guessed
 * ------------------------------------------------------------------------ */

LongWord get_dasmcode_guessed(LongWord index)
{
  return (index < code_len_guessed) ? dasmcode_guessed[index] : 0;
}

void asmcode_init(void)
{
  PatchList = PatchLast = NULL;
  ExportList = ExportLast = NULL;
  CodeBuffer = (Byte*) malloc(sizeof(Byte) * (CodeBufferSize + 1));
}
