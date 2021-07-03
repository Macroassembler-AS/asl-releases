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
#include "endian.h"
#include "strutil.h"
#include "dynstr.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmif.h"
#include "asmcode.h"
#include "asmlist.h"

static unsigned SystemListLen8, SystemListLen16, SystemListLen32;

static as_dynstr_t list_buf;

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

  if ((!ListToNull) && (ThisDoLst) && ((ListMask & 1) != 0) && (!IFListMask()))
  {
    LargeWord ListPC;

    /* Zeilennummer / Programmzaehleradresse: */

    if (IncDepth == 0)
      as_sdprintf(&list_buf, "   ");
    else
    {
      as_snprintf(Tmp, sizeof(Tmp), IntegerFormat, IncDepth);
      as_sdprintf(&list_buf, "(%s)", Tmp);
    }
    if (ListMask & ListMask_LineNums)
    {
      DecString(h2, sizeof(h2), CurrLine, 0);
      as_sdprcatf(&list_buf, "%5s/", h2);
    }
    ListPC = EProgCounter() - CodeLen;
    as_sdprcatf(&list_buf, "%8.*lllu %c ",
                ListRadixBase, ListPC, Retracted? 'R' : ':');

    /* Extrawurst in Listing ? */

    if (*ListLine)
    {
      as_sdprcatf(&list_buf, "%s %s%s", ListLine, Blanks(LISTLINESPACE - strlen(ListLine)), pSrcLine);
      WrLstLine(list_buf.p_str);
      *ListLine = '\0';
    }

    /* Code ausgeben */

    else
    {
      Word Index = 0, CurrListGran, SystemListLen;
      Boolean First = True;
      LargeInt ThisWord;
      int SumLen;

      /* Not enough code to display even on 16/32 bit word?
         Then start rightaway dumping bytes */

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
          as_sdprintf(&list_buf, "%*s%8.*lllu %c ",
                      (ListMask & ListMask_LineNums) ? 9 : 3, "",
                      ListRadixBase, ListPC, Retracted? 'R' : ':');

        SumLen = 0;
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
            as_sdprcatf(&list_buf, "%0*.*lllu ", (int)SystemListLen, (int)ListRadixBase, ThisWord);
          }
          else
            as_sdprcatf(&list_buf, "%*s", SystemListLen + 1, "");

          /* advance pointers & keep track of # of characters printed */

          ListPC += (Gran == CurrListGran) ? 1 : CurrListGran;
          Index += CurrListGran;
          SumLen += SystemListLen + 1;

          /* Less than one full word remaining? Then switch to dumping bytes. */

          if (Index + CurrListGran > EffLen)
          {
            CurrListGran = 1;
            SystemListLen = SystemListLen8;
          }
        }
        while (SumLen + SystemListLen + 1 < LISTLINESPACE);

        /* If first line, pad to max length and append source line */

        if (First)
          as_sdprcatf(&list_buf, "%*s%s", LISTLINESPACE - SumLen, "", pSrcLine);
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
