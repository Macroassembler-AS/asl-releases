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

#define DBG_BIT_FIELD 0

#define CodeBufferSize 512

static Word LenSoFar;
static LongInt RecPos, LenPos;
static Boolean ThisRel;

static Word CodeBufferFill;
static Byte *CodeBuffer;
static unsigned basmcode_partially_used;
static Boolean basmcode_partially_used_be;
Boolean last_basmcode_bit_field_set_be;

PPatchEntry PatchList, PatchLast;
PExportEntry ExportList, ExportLast;
LongInt SectSymbolCounter;
String SectSymbolName;

static LongWord code_len_guessed = 0, max_code_len_guessed = 0;
static Byte *basmcode_guessed;
static Word *wasmcode_guessed;
static LongWord *dasmcode_guessed;

#if DBG_BIT_FIELD
#define dbg_bit_field_printf(p_fmt, ...) printf(p_fmt, __VA_ARGS__)
#else
static int dbg_bit_field_printf(const char *p_fmt, ...) { (void)p_fmt; return 0; }
#endif

static void FlushBuffer(void)
{
  if (CodeBufferFill > 0)
  {
    if (fwrite(CodeBuffer, 1, CodeBufferFill, PrgFile) != CodeBufferFill)
      ChkIO(ErrNum_FileWriteError);
    CodeBufferFill = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_code_swap_bytes(void)
 * \brief  16/32 bit byte swap if target and host endianess of D|WAsmCode do
           not match
 * ------------------------------------------------------------------------ */

void as_code_swap_bytes(void)
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
  switch (Grans[ActPC])
  {
    case 1:
      b = grans_bits_unused[ActPC] ? (256 - (8 - grans_bits_unused[ActPC])) : 1;
      break;
    default:
      b = Grans[ActPC];
  }
  if (fwrite(&b, 1, 1, PrgFile) != 1) ChkIO(ErrNum_FileWriteError);
  fflush(PrgFile);
}

void NewRecord(LargeWord NStart)
{
  LongInt h;
  LongWord PC;
  Byte Header;

  /* flush remaining code in buffer */

  flush_bytes();
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

  flush_bytes();

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

void as_code_write_bytes(LongInt this_code_len)
{
  Word ErgLen;
  Boolean bitwise_segment = (Granularity() == 1) && (gran_bits_unused() == 7);

  if (this_code_len == 0)
    return;

  if (bitwise_segment)
    ErgLen = (basmcode_partially_used + this_code_len) / 8;
  else
    ErgLen = this_code_len * Granularity();
  if (((TurnWords != 0) != (HostBigEndian != 0)) && !bitwise_segment)
    as_code_swap_bytes();
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
  if (((TurnWords != 0) != (HostBigEndian != 0)) && !bitwise_segment)
    as_code_swap_bytes();
}

/*!------------------------------------------------------------------------
 * \fn     transfer_partial_byte(void)
 * \brief  after code was written/listed, transfer the not written partial byte
           of bitwise segment to the next instruction
 * ------------------------------------------------------------------------ */

void transfer_partial_byte(void)
{
  if ((Granularity() == 1) && (gran_bits_unused() == 7))
  {
    unsigned tot_bits = basmcode_partially_used + CodeLen,
             num_bytes = tot_bits / 8,
             next_basmcode_partially_used = tot_bits % 8;

    /* No need to transfer a partial byte if the current instruction only
       reserved space, and all bits that would be transferred would be from it: */

    if (DontPrint && (CodeLen >= (LongInt)next_basmcode_partially_used))
      basmcode_partially_used = 0;
    else
    {
      if ((num_bytes > 0) && next_basmcode_partially_used)
        BAsmCode[0] = BAsmCode[num_bytes];
      basmcode_partially_used = next_basmcode_partially_used;
      basmcode_partially_used_be = last_basmcode_bit_field_set_be;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     flush_bytes(void)
 * \brief  flush bytes in output partially filled with bits
 * ------------------------------------------------------------------------ */

void flush_bytes(void)
{
  unsigned partial = basmcode_partially_used & 7;

  if (partial)
  {
    unsigned pad_len = 8 - partial;
    set_basmcode_bit_field_ve(CodeLen, 0, pad_len, basmcode_partially_used_be);
    as_code_write_bytes(pad_len);
    transfer_partial_byte();
  }
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

/*!------------------------------------------------------------------------
 * \fn     compute_bitfield_fragment(bitfield_fragment_ctx_t *p_ctx, unsigned bitfield_start, unsigned field_length, Boolean big_endian)
 * \brief  For a bit field with given start pos (LSB in LE mode, MSB in BE mode),
           compute the byte offset in BAsmCode[Guessed] where the bit field starts,
           and the maximum # of bits in that byte that may belong to the bit field.
           Take into account that some bits in the very first byte may be occupied
           as 'leftovers' from the previous code bits, which may be written in
           different endianess.
 * \param  p_ctx returns bit/byte positions and count
 * \param  bitfield_start start position of bit field
 * \param  field_length length of bit field in bits
 * \param  big_endian current bit field in big endian order?
 * ------------------------------------------------------------------------ */

typedef struct
{
  unsigned byte_offs, bit_offs, bit_count;
} bitfield_fragment_ctx_t;

static void compute_bitfield_fragment(bitfield_fragment_ctx_t *p_ctx, unsigned bitfield_start, unsigned field_length, Boolean big_endian)
{
  unsigned avl_0 = basmcode_partially_used ? 8 - basmcode_partially_used : 0;
  unsigned byte_avl_start, byte_avl_end;

  /* Does the bit field fragment start in the first byte that contains bits from previous code? */
  if (bitfield_start < avl_0)
  {
    p_ctx->byte_offs = 0;
    /* Previous code written in big endian -> bits [0,basmcode_partially_used-1] are available in first byte */
    if (basmcode_partially_used_be)
    {
      byte_avl_start = 0;
      byte_avl_end = avl_0;
    }
    /* Previous code written in little endian -> bits [basmcode_partially_used,7] are available in first byte */
    else /* bits basmcode_partially_used..7 available in first byte */
    {
      byte_avl_start = basmcode_partially_used;
      byte_avl_end = 8;
    }
  }
  /* Field does not start in this byte -> subtract # of available bits in it from offset, and compute
     offsets as usual: */
  else
  {
    bitfield_start -= avl_0;
    p_ctx->byte_offs = bitfield_start / 8;
    byte_avl_start = 0; byte_avl_end = 8;
    bitfield_start -= (p_ctx->byte_offs * 8);
    p_ctx->byte_offs += !!basmcode_partially_used;
  }

  /* Now shrink the bit field due to start within (partial) byte: */

  if (big_endian)
    byte_avl_end -= bitfield_start;
  else
    byte_avl_start += bitfield_start;
  p_ctx->bit_offs = byte_avl_start;
  p_ctx->bit_count = byte_avl_end - byte_avl_start;

  /* ...and shrink again if the given bit field is even smaller.
     Depending on endianess, this sub-field is left- or right-aligned
     in the field deduced so far: */

  if (p_ctx->bit_count > field_length)
  {
    if (big_endian)
      p_ctx->bit_offs += (p_ctx->bit_count - field_length);
    p_ctx->bit_count = field_length;
  }
}

static void set_byte_bit_field_le_core(Byte *p_dest, unsigned start, LongWord field_value, unsigned field_length)
{
  while (field_length > 0)
  {
    bitfield_fragment_ctx_t ctx;
    unsigned mask;

    compute_bitfield_fragment(&ctx, start, field_length, False);
    mask = ((1 << ctx.bit_count) - 1) << ctx.bit_offs;

    p_dest[ctx.byte_offs] = (p_dest[ctx.byte_offs] & ~mask) | ((field_value << ctx.bit_offs) & mask);
    start += ctx.bit_count;
    field_value >>= ctx.bit_count;
    field_length -= ctx.bit_count;
  }
}

static LongWord get_byte_bit_field_le_core(Byte *p_src, unsigned start, unsigned field_length)
{
  LongWord ret = 0;
  unsigned part_shift = 0;

  while (field_length > 0)
  {
    bitfield_fragment_ctx_t ctx;
    unsigned mask;
    LongWord part;

    compute_bitfield_fragment(&ctx, start, field_length, False);
    mask = ((1 << ctx.bit_count) - 1);
    part = (p_src[ctx.byte_offs] >> ctx.bit_offs) & mask;
    ret |= part << part_shift;
    start += ctx.bit_count;
    part_shift += ctx.bit_count;
    field_length -= ctx.bit_count;
  }
  return ret;
}

static void set_byte_bit_field_be_core(Byte *p_dest, unsigned start, LongWord field_value, unsigned field_length)
{
  if (field_length < 32)
    field_value <<= (32 - field_length);
  while (field_length > 0)
  {
    bitfield_fragment_ctx_t ctx;
    unsigned in_word_shift, mask;

    compute_bitfield_fragment(&ctx, start, field_length, True);
    in_word_shift = 32 - ctx.bit_count;
    mask = ((1 << ctx.bit_count) - 1) << ctx.bit_offs;

    p_dest[ctx.byte_offs] = (p_dest[ctx.byte_offs] & ~mask) | (((field_value >> in_word_shift) << ctx.bit_offs) & mask);
    start += ctx.bit_count;
    field_value <<= ctx.bit_count;
    field_length -= ctx.bit_count;
  }
}

static LongWord get_byte_bit_field_be_core(Byte *p_dest, unsigned start, unsigned field_length)
{
  LongWord ret = 0;

  while (field_length > 0)
  {
    bitfield_fragment_ctx_t ctx;
    unsigned mask;
    LongWord part;

    compute_bitfield_fragment(&ctx, start, field_length, True);
    mask = (1 << ctx.bit_count) - 1;
    part = (p_dest[ctx.byte_offs] >> ctx.bit_offs) & mask;
    ret = (ret << ctx.bit_count) | part;
    start += ctx.bit_count;
    field_length -= ctx.bit_count;
  }
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     set_basmcode_bit_field_le(unsigned start, LongWord field_value, unsigned field_length)
 * \brief  write bit field to code bytes, little endian
 * \param  start start position of bit field
 * \param  field_value bit field's value
 * \param  field_length bit field's length
 * ------------------------------------------------------------------------ */

void set_basmcode_bit_field_le(unsigned start, LongWord field_value, unsigned field_length)
{
  dbg_bit_field_printf("set_basmcode_bit_field_le(start=%u, field_value=0x%x, field_length=%u)\n",
                       start, field_value, field_length);

  /* actual bit offset into BAsmCode must include bits not yet written from previous instruction: */

  SetMaxCodeLen((basmcode_partially_used + start + field_length + 7) >> 3);
  set_byte_bit_field_le_core(BAsmCode, start, field_value, field_length);
  last_basmcode_bit_field_set_be = False;
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_bit_field_le(unsigned start, unsigned field_length)
 * \brief  read bit field from code bytes, little endian
 * \param  start start position of bit field
 * \param  field_length bit field's length
 * \return bit field's value
 * ------------------------------------------------------------------------ */

LongWord get_basmcode_bit_field_le(unsigned start, unsigned field_length)
{
  LongWord ret;

  dbg_bit_field_printf("get_basmcode_bit_field_le(start=%u, field_length=%u)",
                       start, field_length);

  /* actual bit offset into BAsmCode must include bits not yet written from previous instruction: */

  SetMaxCodeLen((basmcode_partially_used + start + field_length + 7) >> 3);
  ret = get_byte_bit_field_le_core(BAsmCode, start, field_length);
  dbg_bit_field_printf("=0x%x\n", ret);
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     set_basmcode_bit_field_be(unsigned start, LongWord field_value, unsigned field_length)
 * \brief  write bit field to code bytes, big endian
 * \param  start start position of bit field
 * \param  field_value bit field's value
 * \param  field_length bit field's length
 * ------------------------------------------------------------------------ */

void set_basmcode_bit_field_be(unsigned start, LongWord field_value, unsigned field_length)
{
  /* actual bit offset into BAsmCode must include bits not yet written from previous instruction: */

  SetMaxCodeLen((basmcode_partially_used + start + field_length + 7) >> 3);
  set_byte_bit_field_be_core(BAsmCode, start, field_value, field_length);
  last_basmcode_bit_field_set_be = True;
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_bit_field_be(unsigned start, unsigned field_length)
 * \brief  read bit field from code bytes, big endian
 * \param  start start position of bit field
 * \param  field_length bit field's length
 * \return bit field's value
 * ------------------------------------------------------------------------ */

LongWord get_basmcode_bit_field_be(unsigned start, unsigned field_length)
{
  LongWord ret;

  dbg_bit_field_printf("get_basmcode_bit_field_be(start=%u, field_length=%u)",
                       start, field_length);

  /* actual bit offset into BAsmCode must include bits not yet written from previous instruction: */

  SetMaxCodeLen((basmcode_partially_used + start + field_length + 7) >> 3);
  ret = get_byte_bit_field_be_core(BAsmCode, start, field_length);
  dbg_bit_field_printf("=0x%x\n", ret);
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     set_basmcode_bit_field_ve(unsigned start, LongWord field_value, unsigned field_length, Boolean big_endian)
 * \brief  write bit field to code bytes, variable endianess
 * \param  start start position of bit field
 * \param  field_value bit field's value
 * \param  field_length bit field's length
 * \param  big_endian big endian order?
 * ------------------------------------------------------------------------ */

void set_basmcode_bit_field_ve(unsigned start, LongWord field_value, unsigned field_length, Boolean big_endian)
{
  if (big_endian)
    set_basmcode_bit_field_be(start, field_value, field_length);
  else
    set_basmcode_bit_field_le(start, field_value, field_length);
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_bit_field_ve(unsigned start, unsigned field_length, Boolean big_endian)
 * \brief  read bit field from code bytes, variable endianess
 * \param  start start position of bit field
 * \param  field_length bit field's length
 * \param  big_endian big endian order?
 * \return bit field's value
 * ------------------------------------------------------------------------ */

LongWord get_basmcode_bit_field_ve(unsigned start, unsigned field_length, Boolean big_endian)
{
  return big_endian
        ? get_basmcode_bit_field_be(start, field_length)
        : get_basmcode_bit_field_le(start, field_length);
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
 * \fn     set_basmcode_guessed_bit_field_le(unsigned start, unsigned field_length)
 * \brief  set guessed mask of bit field in bytes, little endian version
 * \param  start start index of bit field
 * \param  count # of bits to set
 * ------------------------------------------------------------------------ */

void set_basmcode_guessed_bit_field_le(unsigned start, unsigned field_length)
{
  extend_zero_basmcode_guessed_len((basmcode_partially_used + start + field_length + 7) >> 3);
  set_byte_bit_field_le_core(basmcode_guessed, start, (1ul << field_length) - 1, field_length);
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_guessed_bit_field_le(unsigned start, unsigned field_length)
 * \brief  get guessed mask of bit field in bytes, little endian version
 * \param  start start index of bit field
 * \param  count # of bits to get
 * \return guess bits in bit field
 * ------------------------------------------------------------------------ */

LongWord get_basmcode_guessed_bit_field_le(unsigned start, unsigned field_length)
{
  extend_zero_basmcode_guessed_len((basmcode_partially_used + start + field_length + 7) >> 3);
  return get_byte_bit_field_le_core(basmcode_guessed, start, field_length);
}

/*!------------------------------------------------------------------------
 * \fn     set_basmcode_guessed_bit_field_be(unsigned start, unsigned field_length)
 * \brief  set guessed mask of bit field in bytes, big endian version
 * \param  start start index of bit field
 * \param  count # of bits to set
 * ------------------------------------------------------------------------ */

void set_basmcode_guessed_bit_field_be(unsigned start, unsigned field_length)
{
  extend_zero_basmcode_guessed_len((basmcode_partially_used + start + field_length + 7) >> 3);
  set_byte_bit_field_be_core(basmcode_guessed, start, (1ul << field_length) - 1, field_length);
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_guessed_bit_field_be(unsigned start, unsigned field_length)
 * \brief  get guessed mask of bit field in bytes, big endian version
 * \param  start start index of bit field
 * \param  count # of bits to get
 * \return guess bits in bit field
 * ------------------------------------------------------------------------ */

LongWord get_basmcode_guessed_bit_field_be(unsigned start, unsigned field_length)
{
  extend_zero_basmcode_guessed_len((basmcode_partially_used + start + field_length + 7) >> 3);
  return get_byte_bit_field_be_core(basmcode_guessed, start, field_length);
}

/*!------------------------------------------------------------------------
 * \fn     set_basmcode_guessed_bit_field_ve(unsigned start, unsigned field_length, Boolean big_endian)
 * \brief  set guessed mask of bit field in bytes, variable endianess version
 * \param  start start index of bit field
 * \param  count # of bits to set
 * \param  big_endian big endian order?
 * ------------------------------------------------------------------------ */

void set_basmcode_guessed_bit_field_ve(unsigned start, unsigned field_length, Boolean big_endian)
{
  if (big_endian)
    set_basmcode_guessed_bit_field_be(start, field_length);
  else
    set_basmcode_guessed_bit_field_le(start, field_length);
}

/*!------------------------------------------------------------------------
 * \fn     get_basmcode_guessed_bit_field_ve(unsigned start, unsigned field_length, Boolean big_endian)
 * \brief  get guessed mask of bit field in bytes, variable endianess version
 * \param  start start index of bit field
 * \param  count # of bits to get
 * \param  big_endian big endian order?
 * \return guess bits in bit field
 * ------------------------------------------------------------------------ */

LongWord get_basmcode_guessed_bit_field_ve(unsigned start, unsigned field_length, Boolean big_endian)
{
  return big_endian
       ? get_basmcode_guessed_bit_field_be(start, field_length)
       : get_basmcode_guessed_bit_field_le(start, field_length);
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

/*!------------------------------------------------------------------------
 * \fn     asmcode_init(void)
 * \brief  module initialization
 * ------------------------------------------------------------------------ */

void asmcode_init(void)
{
  PatchList = PatchLast = NULL;
  ExportList = ExportLast = NULL;
  CodeBuffer = (Byte*) malloc(sizeof(Byte) * (CodeBufferSize + 1));
  basmcode_partially_used = 0;
}
