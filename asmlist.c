/* asmlist.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS Port                                                                   */
/*                                                                           */
/* Generate Listing                                                          */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <assert.h>
#include "be_le.h"
#include "cmdarg.h"
#include "strutil.h"
#include "dynstr.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmif.h"
#include "asmcode.h"
#include "asmlist.h"

/* NOTE: Keep this to a value of the form 8*n, so the overall 'prefix'
         in front of the source line in the listing is a multiple of 8.
         This way, Tabs in the source do not break up: */

#define LISTLINE_PREFIX_TOTAL 40

static unsigned SystemListLen4, SystemListLen8, SystemListLen12, SystemListLen16, SystemListLen32;

static as_dynstr_t list_buf;

static int max_pc_len;

static char *p_listline_prefix_format = NULL;
static const char default_listline_prefix_format[] = "%i%n/%a";
static Boolean list_unknown_values = True;

/*!------------------------------------------------------------------------
 * \fn     as_list_set_max_pc(LargeWord max_pc)
 * \brief  compute maximum width of PC field in listing
 * ------------------------------------------------------------------------ */

void as_list_set_max_pc(LargeWord max_pc)
{
  String tmp;

  switch (ListGran())
  {
    case 1:
    case 2:
      if (gran_bits_unused() && (gran_bits_unused() != 4))
        goto warn;
      break;
    default:
      if (gran_bits_unused())
        goto warn;
      break;
    warn:
      fprintf(stderr, "define SystemListLen for %u bits\n",
              (unsigned)((ListGran() * 8) - gran_bits_unused()));
  }

  as_snprintf(tmp, sizeof(tmp), "%1.*lllu", ListRadixBase, max_pc);
  max_pc_len = strlen(tmp);
  if (max_pc_len > 8)
    max_pc_len = 8;
}

/*!------------------------------------------------------------------------
 * \fn     MakeList()
 * \brief  generate listing for one line, including generated code
 * ------------------------------------------------------------------------ */

static void list_format_error(tErrorNum num, char fmt)
{
  char err_str[10];

  as_snprintf(err_str, sizeof(err_str), "%%c", fmt);
  WrXError(num, err_str);
}

void MakeList(const char *pSrcLine)
{
  Word EffLen, Gran = Granularity();
  Boolean ThisDoLst;

  EffLen = CodeLen * Gran;

#if 0
  fprintf(stderr, "[%s] WasIF %u WasMACRO %u DoLst %u\n", OpPart.Str, WasIF, WasMACRO, DoLst);
#endif
  if (WasIF)
    ThisDoLst = !!(DoLst & eLstMacroExpIf);
  else if (WasMACRO)
    ThisDoLst = !!(DoLst & eLstMacroExpMacro);
  else
  {
    if (!IfAsm && (!(DoLst & eLstMacroExpIf)))
      ThisDoLst = False;
    else
      ThisDoLst = !!(DoLst & eLstMacroExpRest);
  }

  if (!ListToNull && ThisDoLst && (ListMask & 1) && !IFListMask())
  {
    LargeWord ListPC = EProgCounter() - CodeLen;
    size_t sum_len;
    int inc_column_len[3] = { -1, -1, -1 },
        lnum_column_len[3] = { -1, -1, -1 };
    Word Index = 0, CurrListGran, SystemListLen;
    Boolean First = True;

    /* Not enough code to display even on 16/32 bit word?
       Then start dumping bytes right away: */

    if (EffLen < ActListGran)
    {
      CurrListGran = 1;
      SystemListLen = act_list_gran_bits_unused ? SystemListLen4 : SystemListLen8;
    }
    else
    {
      CurrListGran = ActListGran;
      switch (CurrListGran)
      {
        case 4:
          SystemListLen = SystemListLen32;
          break;
        case 2:
          SystemListLen = act_list_gran_bits_unused ? SystemListLen12 : SystemListLen16;
          break;
        default:
          SystemListLen = act_list_gran_bits_unused ? SystemListLen4 : SystemListLen8;
      }
    }

    if (TurnWords && (Gran != ActListGran) && (1 == ActListGran))
      DreheCodes();

    do
    {
      /* Print list line header: First, the part configurable via format string: */

      const char *p_format;
      as_format_ctx_t format_context;
      Boolean replace_space = False;
      size_t num_inc_column = 0, num_lnum_column = 0;

      if (list_buf.capacity > 0)
        list_buf.p_str[0] = '\0';
      as_format_context_reset(&format_context);
      for (p_format = p_listline_prefix_format ? p_listline_prefix_format : default_listline_prefix_format;
           *p_format; p_format++)
      {
        Boolean next_replace_space = False;

        if (as_format_context_consume(&format_context, *p_format));
        else if (format_context.in_format)
        {
          char tmp_buf[40];
          int pad = 0;

          switch (*p_format)
          {
            /* Include level is printed only on the first line: */

            case 'i':
              if (num_inc_column >= as_array_size(inc_column_len))
                list_format_error(ErrNum_ListHeadFormatElemTooOften, *p_format);
              if (First)
              {
                if ((MaxIncDepth > 0) || (format_context.arg[0] > 0))
                {
                  int inc_max_digits = (format_context.arg[0] > 0)
                                     ? format_context.arg[0]
                                     : ((MaxIncDepth > 99) ? 3 : ((MaxIncDepth > 9) ? 2 : 1));

                  inc_column_len[num_inc_column] =
                    (IncDepth > 0) ?
                    as_snprintf(tmp_buf, sizeof(tmp_buf),
                                format_context.lead_zero ? "(%0*u)" : "(%*u)",
                                inc_max_digits, (unsigned)IncDepth) :
                    as_snprintf(tmp_buf, sizeof(tmp_buf), "%*s", 2 + inc_max_digits, "");
                  as_dynstr_append_c_str(&list_buf, tmp_buf);
                }
              }
              else if (inc_column_len[num_inc_column] > 0)
                as_dynstr_append_c_str(&list_buf, Blanks(inc_column_len[num_inc_column]));
              num_inc_column++;
              break;

            /* Source line number is printed only on the first line: */

            case 'n':
              if (num_lnum_column >= as_array_size(lnum_column_len))
                list_format_error(ErrNum_ListHeadFormatElemTooOften, *p_format);
              if (First)
              {
                int line_max_digits = (format_context.arg[0] > 0) ? format_context.arg[0] : 5;

                lnum_column_len[num_lnum_column] =
                   DecString(tmp_buf, sizeof(tmp_buf), CurrLine,
                             format_context.lead_zero ? line_max_digits : 0);
                if (lnum_column_len[num_lnum_column] < line_max_digits)
                  as_dynstr_append_c_str(&list_buf, Blanks(pad = line_max_digits - lnum_column_len[num_lnum_column]));
                as_dynstr_append_c_str(&list_buf, tmp_buf);
                lnum_column_len[num_lnum_column] += pad;
              }
              else if (lnum_column_len[num_lnum_column] > 0)
              {
                as_dynstr_append_c_str(&list_buf, Blanks(lnum_column_len[num_lnum_column]));
                next_replace_space = True;
              }
              num_lnum_column++;
              break;

            /* Memory address is printed on all lines: */

            case 'a':
            {
              int pc_max_digits = (format_context.arg[0] > 0) ? format_context.arg[0] : max_pc_len;
              char pc_buf[40];
              int pc_len;

              pc_len = SysString(pc_buf, sizeof(pc_buf), ListPC, ListRadixBase,
                                 (format_context.lead_zero || ListPCZeroPad) ? pc_max_digits : 0,
                                 False, HexStartCharacter, SplitByteCharacter);
              if (pc_len < pc_max_digits)
                as_dynstr_append_c_str(&list_buf, Blanks(pad = pc_max_digits - pc_len));
              as_dynstr_append_c_str(&list_buf, pc_buf);
              break;
            }
            default:
              list_format_error(ErrNum_InvListHeadFormat, *p_format);
          }
          as_format_context_reset(&format_context);
        }
        else /* !format_context.in_format */
          as_dynstr_append(&list_buf, replace_space ? " " : p_format, 1);

        replace_space = next_replace_space;
      }

      /* Separator resp. indicator for retracted code */

      as_sdprcatf(&list_buf, Retracted ? " R" : " :");
      sum_len = strlen(list_buf.p_str);
      assert(sum_len + 2 < LISTLINE_PREFIX_TOTAL);

      /* Extrawurst in Listing ? */

      if (First && *ListLine)
      {
        size_t rem_space = LISTLINE_PREFIX_TOTAL - sum_len - 2;
        int num_pad = rem_space - strlen(ListLine);

        /* If too long, truncate and add ..
           If shorter than space, pad with spaces: */

        if (num_pad < 0)
          ListLine[rem_space - 2] = '\0';
        sum_len += as_sdprcatf(&list_buf, " %s%s", ListLine, (num_pad >= 0) ? Blanks(num_pad) : "..");
      }

      /* Actual code: */

      else do
      {
        /* We checked initially there is at least one full word,
           and we check after every word whether there is another
           full one: */

        if ((Index >= EffLen) || DontPrint)
          as_sdprcatf(&list_buf, "%*s", (int)(1 + SystemListLen), "");
        else
        {
          LargeWord ThisWord, ThisWordGuessed;

          switch (CurrListGran)
          {
            case 4:
              ThisWord = DAsmCode[Index >> 2];
              ThisWordGuessed = get_dasmcode_guessed(Index >> 2);
              break;
            case 2:
              ThisWord = WAsmCode[Index >> 1];
              ThisWordGuessed = get_wasmcode_guessed(Index >> 1);;
              break;
            default:
              ThisWord = BAsmCode[Index];
              ThisWordGuessed = get_basmcode_guessed(Index);
          }
          as_sdprcatf(&list_buf, " %0*.*lllu", (int)SystemListLen, (int)ListRadixBase, ThisWord);
          if (list_unknown_values && (ThisWordGuessed != 0))
          {
            char mask_buf[100];
            char *p_val_end = list_buf.p_str + strlen(list_buf.p_str),
                 *p_mask_end = mask_buf + as_snprintf(mask_buf, sizeof(mask_buf), "%0*.*lllu", (int)SystemListLen, (int)ListRadixBase, ThisWordGuessed);
            while (p_mask_end > mask_buf)
            {
              p_val_end--;
              if (*--p_mask_end != '0')
                *p_val_end = '?';
            }
          }
        }

        /* advance pointers & keep track of # of characters printed */

        ListPC += (Gran == CurrListGran) ? 1 : CurrListGran;
        Index += CurrListGran;
        sum_len += 1 + SystemListLen;

        /* Less than one full word remaining? Then switch to dumping bytes. */

        if (Index + CurrListGran > EffLen)
        {
          CurrListGran = 1;
          SystemListLen = SystemListLen8;
        }
      }
      while (sum_len + 1 + SystemListLen < LISTLINE_PREFIX_TOTAL);

      /* If first line, pad to max length and append source line */

      if (First)
        as_sdprcatf(&list_buf, "%*s%s", (int)(LISTLINE_PREFIX_TOTAL - sum_len), "", pSrcLine);
      WrLstLine(list_buf.p_str);
      if (First && *ListLine)
      {
        *ListLine = '\0';
        break;
      }
      First = False;
    }
    while ((Index < EffLen) && !DontPrint);

    if (TurnWords && (Gran != ActListGran) && (1 == ActListGran))
      DreheCodes();
  } /* if (!ListToNull... */
}

/*!------------------------------------------------------------------------
 *
 * ------------------------------------------------------------------------ */

static as_cmd_result_t cmd_listline_prefix(Boolean negate, const char *p_arg)
{
  if (p_listline_prefix_format)
    free(p_listline_prefix_format);
  p_listline_prefix_format = negate ? NULL : as_strdup(p_arg);
  return negate ? e_cmd_ok : e_cmd_arg;
}

static as_cmd_result_t cmd_list_unknown_values(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);
  if (negate)
    return e_cmd_err;
  list_unknown_values = True;
  return e_cmd_ok;
}

static as_cmd_result_t cmd_no_list_unknown_values(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);
  if (negate)
    return e_cmd_err;
  list_unknown_values = False;
  return e_cmd_ok;
}

static const as_cmd_rec_t list_params[] =
{
  { "listline-prefix", cmd_listline_prefix },
  { "list-unknown-values", cmd_list_unknown_values },
  { "no-list-unknown-values", cmd_no_list_unknown_values }
};

/*!------------------------------------------------------------------------
 * \fn     asmlist_setup(void)
 * \brief  setup stuff after command line args have been read
 * ------------------------------------------------------------------------ */

void asmlist_setup(void)
{
  String Dummy;

  SysString(Dummy, sizeof(Dummy), 0xf, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen4 = strlen(Dummy);
  SysString(Dummy, sizeof(Dummy), 0xff, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen8 = strlen(Dummy);
  SysString(Dummy, sizeof(Dummy), 0xfff, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen12 = strlen(Dummy);
  SysString(Dummy, sizeof(Dummy), 0xffffu, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen16 = strlen(Dummy);
  SysString(Dummy, sizeof(Dummy), 0xfffffffful, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen32 = strlen(Dummy);
}

/*!------------------------------------------------------------------------
 * \fn     asmlist_init(void)
 * \brief  global setup at program start
 * ------------------------------------------------------------------------ */

void asmlist_init(void)
{
  as_dynstr_ini(&list_buf, STRINGSIZE);

  as_cmd_register(list_params, as_array_size(list_params));
}
