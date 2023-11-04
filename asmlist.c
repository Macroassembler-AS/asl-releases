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

static unsigned SystemListLen8, SystemListLen16, SystemListLen32;

static as_dynstr_t list_buf;

static int max_pc_len;

/*!------------------------------------------------------------------------
 * \fn     as_list_set_max_pc(LargeWord max_pc)
 * \brief  compute maximum width of PC field in listing
 * ------------------------------------------------------------------------ */

void as_list_set_max_pc(LargeWord max_pc)
{
  String tmp;

  as_snprintf(tmp, sizeof(tmp), "%1.*lllu", ListRadixBase, max_pc);
  max_pc_len = strlen(tmp);
  if (max_pc_len > 8)
    max_pc_len = 8;
}

/*!------------------------------------------------------------------------
 * \fn     MakeList()
 * \brief  generate listing for one line, including generated code
 * ------------------------------------------------------------------------ */

void MakeList(const char *pSrcLine)
{
  String h2, Tmp;
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
    LargeWord ListPC;
    size_t sum_len;
    int inc_and_line_length;
    const char *p_pc_and_colon_format = ListPCZeroPad ? "%0*.*lllu %c" : "%*.*lllu %c";

    /* Zeilennummer / Programmzaehleradresse: */

    as_sdprintf(&list_buf, "");
    if (IncDepth != 0)
    {
      as_snprintf(Tmp, sizeof(Tmp), IntegerFormat, IncDepth);
      as_sdprcatf(&list_buf, "(%s)", Tmp);
    }
    if (ListMask & ListMask_LineNums)
    {
      DecString(h2, sizeof(h2), CurrLine, 0);
      as_sdprcatf(&list_buf, "%5s/", h2);
    }
    inc_and_line_length = strlen(list_buf.p_str);
    ListPC = EProgCounter() - CodeLen;
    as_sdprcatf(&list_buf, p_pc_and_colon_format,
                max_pc_len, ListRadixBase, ListPC,
                Retracted? 'R' : ':');
    sum_len = strlen(list_buf.p_str);
    assert(sum_len + 2 < LISTLINE_PREFIX_TOTAL);

    /* Extrawurst in Listing ? */

    if (*ListLine)
    {
      size_t rem_space = LISTLINE_PREFIX_TOTAL - sum_len - 2;
      int num_pad = rem_space - strlen(ListLine);

      /* If too long, truncate and add ..
         If shorter than space, pad with spaces: */

      if (num_pad < 0)
        ListLine[rem_space - 2] = '\0';
      as_sdprcatf(&list_buf, " %s%s %s", ListLine,
                  (num_pad >= 0) ? Blanks(num_pad) : "..", pSrcLine);
      WrLstLine(list_buf.p_str);
      *ListLine = '\0';
    }

    /* Code ausgeben */

    else
    {
      Word Index = 0, CurrListGran, SystemListLen;
      Boolean First = True;
      LargeInt ThisWord;

      /* Not enough code to display even on 16/32 bit word?
         Then start dumping bytes right away: */

      if (EffLen < ActListGran)
      {
        CurrListGran = 1;
        SystemListLen = SystemListLen8;
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
            SystemListLen = SystemListLen16;
            break;
          default:
            SystemListLen = SystemListLen8;
        }
      }

      if (TurnWords && (Gran != ActListGran) && (1 == ActListGran))
        DreheCodes();

      do
      {
        /* If not the first code line, prepend blanks to fill up space below line number: */

        if (!First)
        {
          if (inc_and_line_length > 0)
            as_sdprintf(&list_buf, "%*s", inc_and_line_length, "");
          else
            as_sdprintf(&list_buf, "");
          as_sdprcatf(&list_buf, p_pc_and_colon_format,
                      max_pc_len, ListRadixBase, ListPC,
                      Retracted? 'R' : ':');
          sum_len = strlen(list_buf.p_str);
        }
        do
        {
          /* We checked initially there is at least one full word,
             and we check after every word whether there is another
             full one: */

          if ((Index < EffLen) && !DontPrint)
          {
            switch (CurrListGran)
            {
              case 4:
                ThisWord = DAsmCode[Index >> 2];
                break;
              case 2:
                ThisWord = WAsmCode[Index >> 1];
                break;
              default:
                ThisWord = BAsmCode[Index];
            }
            as_sdprcatf(&list_buf, " %0*.*lllu", (int)SystemListLen, (int)ListRadixBase, ThisWord);
          }
          else
            as_sdprcatf(&list_buf, "%*s", (int)(1 + SystemListLen), "");

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
        First = False;
      }
      while ((Index < EffLen) && !DontPrint);

      if (TurnWords && (Gran != ActListGran) && (1 == ActListGran))
        DreheCodes();
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     asmlist_init(void)
 * \brief  setup stuff at program startup
 * ------------------------------------------------------------------------ */

void asmlist_init(void)
{
  String Dummy;

  as_dynstr_ini(&list_buf, STRINGSIZE);

  SysString(Dummy, sizeof(Dummy), 0xff, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen8 = strlen(Dummy);
  SysString(Dummy, sizeof(Dummy), 0xffffu, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen16 = strlen(Dummy);
  SysString(Dummy, sizeof(Dummy), 0xfffffffful, ListRadixBase, 0, False, HexStartCharacter, SplitByteCharacter);
  SystemListLen32 = strlen(Dummy);
}
