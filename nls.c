/* nls.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Abhandlung landesspezifischer Unterschiede                                */
/*                                                                           */
/*****************************************************************************/

#undef DEBUG_NLS

#include "stdinc.h"
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "strutil.h"

#include "nls.h"

/*-------------------------------------------------------------------------------*/

typedef struct
{
  Word Country;        /* = internationale Vorwahl */
  tCodepage Codepage; /* mom. gewaehlter Zeichensatz */
  void (*DateString)(Word Year, Word Month, Word Day, char *Dest, size_t DestSize);
  void (*TimeString)(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize);
#if (defined OS2_NLS) || (defined DOS_NLS)
  DateFormat DateFmt;  /* Datumsreihenfolge */
  const char *DateSep; /* Trennzeichen zwischen Datumskomponenten */
  TimeFormat TimeFmt;  /* 12/24-Stundenanzeige */
  const char *TimeSep; /* Trennzeichen zwischen Zeitkomponenten */
#elif defined LOCALE_NLS
  const char *DateFmtStr;
  const char *TimeFmtStr;
#endif
  const char *Currency;      /* Waehrungsname */
  CurrFormat CurrFmt;  /* Anzeigeformat Waehrung */
  Byte CurrDecimals;   /* Nachkommastellen Waehrungsbetraege */
  const char *ThouSep; /* Trennzeichen fuer Tausenderbloecke */
  const char *DecSep;  /* Trennzeichen fuer Nachkommastellen */
  const char *DataSep; /* ??? */
  Boolean Initialized;
} NLS_CountryInfo;

/*-------------------------------------------------------------------------------*/

CharTable UpCaseTable;               /* Umsetzungstabellen */
CharTable LowCaseTable;

static NLS_CountryInfo NLSInfo;
static CharTable CollateTable;
static tCodepage OverrideCodepage = eCodepageCnt;

static const char *CodepageNames[eCodepageCnt] =
{
  "ascii",
  "iso8859-1",
  "iso8859-15",
  "koi8-r",
  "437",
  "850",
  "866",
  "1251",
  "1252",
  "utf-8",
};

/*-------------------------------------------------------------------------------*/

static void DumpNLSInfo(void)
{
#ifdef DEBUG_NLS
  unsigned z, z2;

  printf("Country      = %d\n", NLSInfo.Country);
  printf("Codepage     = %s\n", CodepageNames[NLSInfo.Codepage]);
#if (defined OS2_NLS) || (defined DOS_NLS)
  printf("DateFmt      = ");
  switch(NLSInfo.DateFmt)
  {
    case DateFormatMTY:
      printf("MTY\n");
      break;
    case DateFormatTMY:
      printf("TMY\n");
      break;
    case DateFormatYMT:
      printf("YMT\n");
      break;
    default:
      printf("???\n");
  }
  printf("DateSep      = %s\n", NLSInfo.DateSep);
  printf("TimeFmt      = ");
  switch(NLSInfo.TimeFmt)
  {
    case TimeFormatUSA:
      printf("USA\n");
      break;
    case TimeFormatEurope:
      printf("Europe\n");
      break;
    case TimeFormatJapan:
      printf("Japan\n");
      break;
    default:
      printf("???\n");
  }
  printf("TimeSep      = %s\n", NLSInfo.TimeSep);
#elif defined LOCALE_NLS
  printf("DateFmtStr   = %s\n", NLSInfo.DateFmtStr);
  printf("TimeFmtStr   = %s\n", NLSInfo.TimeFmtStr);
#endif
  printf("Currency     = %s\n", NLSInfo.Currency);
  printf("CurrFmt      = ");
  switch (NLSInfo.CurrFmt)
  {
    case CurrFormatPreNoBlank:
      printf("PreNoBlank\n");
      break;
    case CurrFormatPostNoBlank:
      printf("PostNoBlank\n");
      break;
    case CurrFormatPreBlank:
      printf("PreBlank\n");
      break;
    case CurrFormatPostBlank:
      printf("PostBlank\n");
      break;
    default:
      printf("???\n");
  }
  printf("CurrDecimals = %d\n", NLSInfo.CurrDecimals);
  printf("ThouSep      = %s\n", NLSInfo.ThouSep);
  printf("DecSep       = %s\n", NLSInfo.DecSep);
  printf("DataSep      = %s\n", NLSInfo.DataSep);

  printf("\nUpcaseTable:\n");
  for (z = 0; z < 256; z++)
  {
    z2 = (unsigned char)UpCaseTable[z];
    if (z2 != z)
      printf("0x%02x -> %02x\n", z, z2);
  }

  printf("\nLowcaseTable:\n");
  for (z = 0; z < 256; z++)
  {
    z2 = (unsigned char)LowCaseTable[z];
    if (z2 != z)
      printf("0x%02x -> %02x\n", z, z2);
  }

  printf("\nCollateTable:\n");
  for (z = 0; z < 4; z++)
  {
    for (z2 = 0; z2 < 63; z2++)
      if (z * 64 + z2 > 32)
        putchar(z * 64 + z2);
    putchar ('\n');
    for (z2 = 0; z2 < 63; z2++)
      if (z * 64 + z2 > 32)
        putchar(CollateTable[z * 64 + z2]);
    putchar ('\n');
    putchar('\n');
  }
#endif /* DEBUG_NLS */
}

/*-------------------------------------------------------------------------------*/

static void SetLoUp(Byte Lo, Byte Up)
{
  UpCaseTable[Lo] = Up;
  LowCaseTable[Up] = Lo;
}

void UpCaseFromCodeTable(void)
{
  int z;

  for (z = 0; z < 128; z++)
  {
    UpCaseTable[z] = toupper(z);
    LowCaseTable[z] = tolower(z);
  }
  for (; z < 256; z++)
  {
    UpCaseTable[z] = z;
    LowCaseTable[z] = z;
  }

  switch (NLSInfo.Codepage)
  {
    case eCodepage866:
      for (z = 0x80; z <= 0x8f; z++)
        SetLoUp(z + 0x20, z);
      for (z = 0x90; z <= 0x9f; z++)
        SetLoUp(z + 0x50, z);
      for (z = 0xf0; z <= 0xf6; z += 2)
        SetLoUp(z + 1, z);
      break;
    case eCodepage850:
      SetLoUp(0xa0, 0xb5); /* &aacute; */
      SetLoUp(0xa1, 0xd6); /* &iacute; */
      SetLoUp(0xa2, 0xe0); /* &oacute; */
      SetLoUp(0xa3, 0xe9); /* &uacute; */
      SetLoUp(0x85, 0xb7); /* &agrave; */
      SetLoUp(0x8a, 0xd4); /* &egrave; */
      SetLoUp(0x8d, 0xde); /* &igrave; */
      SetLoUp(0x95, 0xe3); /* &ograve; */
      SetLoUp(0x97, 0xeb); /* &ugrave; */
      SetLoUp(0x83, 0xb6); /* &acirc; */
      SetLoUp(0x88, 0xd2); /* &ecirc; */
      SetLoUp(0x8c, 0xd6); /* &icirc; */
      SetLoUp(0x93, 0xe2); /* &ocirc; */
      SetLoUp(0x96, 0xea); /* &ucirc; */
      SetLoUp(0xec, 0xed); /* &yacute; */
      SetLoUp(0x9b, 0x9d); /* &oslash; */
      SetLoUp(0xe8, 0xe7); /* &thorn; */
      /* fall-through */
    case eCodepage437:
      SetLoUp(0x80, 0x87); /* &ccedil; */
      SetLoUp(0x84, 0x8e); /* &auml; */
      SetLoUp(0x94, 0x99); /* &ouml; */
      SetLoUp(0x81, 0x9a); /* &uuml; */
      SetLoUp(0x82, 0x90); /* &eacute; */
      SetLoUp(0xa4, 0xa5); /* &ntilde; */
      SetLoUp(0x86, 0x8f); /* &aring; */
      SetLoUp(0x91, 0x92); /* &aelig; */
      break;
    case eCodepage1251:
      SetLoUp(0x90, 0x80);
      SetLoUp(0x9a, 0x8a);
      SetLoUp(0x9c, 0x8c);
      SetLoUp(0x9d, 0x8d);
      SetLoUp(0x9e, 0x8e);
      SetLoUp(0x9f, 0x8f);
      SetLoUp(0xa2, 0xa1);
      SetLoUp(0xb3, 0xb2);
      SetLoUp(0xb4, 0xa5);
      SetLoUp(0xb8, 0xa8);
      SetLoUp(0xba, 0xaa);
      SetLoUp(0xbe, 0xbd);
      SetLoUp(0xbf, 0xaf);
      for (z = 0xc0; z <= 0xdf; z++)
        SetLoUp(z + 0x20, z);
      break;
    case eCodepage1252:
      SetLoUp(0x9a, 0x8a); /* &scaron */
      SetLoUp(0x9e, 0x8e); /* &zcaron; */
      SetLoUp(0x9c, 0x8c); /* &oelog; */
      goto iso8859_1;
    case eCodepageISO8859_15:
      SetLoUp(0xa8, 0xa6); /* &scaron */
      SetLoUp(0xb8, 0xb4); /* &zcaron; */
      SetLoUp(0xbd, 0xbc); /* &oelog; */
      /* fall-through */
    case eCodepageISO8859_1:
    case eCodepageUTF8:
      iso8859_1:
      SetLoUp(0xe0, 0xc0); /* &agrave; */
      SetLoUp(0xe1, 0xc1); /* &aacute; */
      SetLoUp(0xe2, 0xc2); /* &acirc; */
      SetLoUp(0xe3, 0xc3); /* &atilde; */
      SetLoUp(0xe4, 0xc4); /* &auml; */
      SetLoUp(0xe5, 0xc5); /* &aring; */
      SetLoUp(0xe6, 0xc6); /* &aelig; */
      SetLoUp(0xe7, 0xc7); /* &ccedil; */
      SetLoUp(0xe8, 0xc8); /* &egrave; */
      SetLoUp(0xe9, 0xc9); /* &eacute; */
      SetLoUp(0xea, 0xca); /* &ecirc; */
      SetLoUp(0xeb, 0xcb); /* &euml; */
      SetLoUp(0xec, 0xcc); /* &igrave; */
      SetLoUp(0xed, 0xcd); /* &iacute; */
      SetLoUp(0xee, 0xce); /* &icirc; */
      SetLoUp(0xef, 0xcf); /* &iuml; */
      SetLoUp(0xf1, 0xd1); /* &ntilde; */
      SetLoUp(0xf2, 0xd2); /* &ograve; */
      SetLoUp(0xf3, 0xd3); /* &oacute; */
      SetLoUp(0xf4, 0xd4); /* &ocirc; */
      SetLoUp(0xf5, 0xd5); /* &otilde; */
      SetLoUp(0xf6, 0xd6); /* &ouml; */
      SetLoUp(0xf8, 0xd8); /* &oslash; */
      SetLoUp(0xf9, 0xd9); /* &ugrave; */
      SetLoUp(0xfa, 0xda); /* &uacute; */
      SetLoUp(0xfb, 0xdb); /* &ucirc; */
      SetLoUp(0xfc, 0xdc); /* &uuml; */
      SetLoUp(0xfd, 0xdd); /* &yacute; */
      SetLoUp(0xfe, 0xde); /* &thorn; */
      break;
    case eCodepageKOI8_R:
      SetLoUp(0xa3, 0xb3);
      for (z = 0xc0; z <= 0xdf; z++)
        SetLoUp(z, z + 0x20);
    default:
      break;
  }
}

#if (defined OS2_NLS) || (defined DOS_NLS)

static void DOS_OS2_DateString(Word Year, Word Month, Word Day, char *Dest, size_t DestSize)
{
  switch (NLSInfo.DateFmt)
  {
    case DateFormatMTY:
      as_snprintf(Dest, DestSize, "%d%s%d%s%d", Month, NLSInfo.DateSep, Day, NLSInfo.DateSep, Year);
      break;
    case DateFormatTMY:
      as_snprintf(Dest, DestSize, "%d%s%d%s%d", Day, NLSInfo.DateSep, Month, NLSInfo.DateSep, Year);
      break;
    case DateFormatYMT:
      as_snprintf(Dest, DestSize, "%d%s%d%s%d", Year, NLSInfo.DateSep, Month, NLSInfo.DateSep, Day);
      break;
  }
}

static void DOS_OS2_TimeString(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize)
{
  Word OriHour;

  OriHour = Hour;
  if (NLSInfo.TimeFmt == TimeFormatUSA)
  {
    Hour %= 12;
    if (Hour == 0)
      Hour = 12;
  }
  as_snprintf(Dest, DestSize, "%d%s%02d%s%02d", Hour, NLSInfo.TimeSep, Minute, NLSInfo.TimeSep, Second);
  if (Sec100 < 100)
    as_snprcatf(Dest, DestSize, "%s%02d", NLSInfo.DecSep, Sec100);
  if (NLSInfo.TimeFmt == TimeFormatUSA)
    as_snprcatf(Dest, DestSize, "%c", (OriHour > 12) ? 'p' : 'a');
}
#endif /* OS2_NLS || DOS_NLS */

/*-------------------------------------------------------------------------------*/

#if defined OS2_NLS

#define INCL_DOSNLS
#include <os2.h>

static void QueryInfo(void)
{
  COUNTRYCODE ccode;
  COUNTRYINFO cinfo;
  ULONG erglen;
  int z;

  ccode.country = 0;
  ccode.codepage = 0;
  DosQueryCtryInfo(sizeof(cinfo), &ccode, &cinfo, &erglen);

  NLSInfo.Country = cinfo.country;
  switch (cinfo.codepage)
  {
    case 437: NLSInfo.Codepage = eCodepage437; break;
    case 850: NLSInfo.Codepage = eCodepage850; break;
    case 866: NLSInfo.Codepage = eCodepage866; break;
    case 1251: NLSInfo.Codepage = eCodepage1251; break;
    case 1252: NLSInfo.Codepage = eCodepage1252; break;
    case 20866: NLSInfo.Codepage = eCodepageKOI8_R; break;
    case 28591: NLSInfo.Codepage = eCodepageISO8859_1; break;
    case 28605: NLSInfo.Codepage = eCodepageISO8859_15; break;
    default: NLSInfo.Codepage = eCodepageASCII;
  }
  NLSInfo.DecSep = as_strdup(cinfo.szDecimal);
  NLSInfo.DataSep = as_strdup(cinfo.szDataSeparator);
  NLSInfo.ThouSep = as_strdup(cinfo.szThousandsSeparator);
  NLSInfo.Currency = as_strdup(cinfo.szCurrency);
  NLSInfo.CurrDecimals = cinfo.cDecimalPlace;
  NLSInfo.CurrFmt = (CurrFormat) cinfo.fsCurrencyFmt;
  NLSInfo.DateFmt = (DateFormat) cinfo.fsDateFmt;
  NLSInfo.DateSep = as_strdup(cinfo.szDateSeparator);
  NLSInfo.DateString = DOS_OS2_DateString;
  NLSInfo.TimeFmt = (TimeFormat) cinfo.fsTimeFmt;
  NLSInfo.TimeSep = as_strdup(cinfo.szTimeSeparator);
  NLSInfo.TimeString = DOS_OS2_TimeString;
  for (z = 0; z < 256; z++)
    UpCaseTable[z] = (char) z;
  for (z = 'a'; z <= 'z'; z++)
    UpCaseTable[z] -= 'a' - 'A';
  DosMapCase(sizeof(UpCaseTable), &ccode, UpCaseTable);
  for (z = 0; z < 256; z++)
    LowCaseTable[z] = (char) z;
  for (z = 255; z >= 0; z--)
    if (UpCaseTable[z] != (char) z)
      LowCaseTable[((unsigned int) UpCaseTable[z])&0xff] = (char) z;
  for (z = 0; z < 256; z++)
    CollateTable[z] = (char) z;
  DosQueryCollate(sizeof(CollateTable), &ccode, CollateTable, &erglen);
}

#elif defined W32_NLS

static void Default_DateString(Word Year, Word Month, Word Day, char *Dest, size_t DestSize)
{
  as_snprintf(Dest, DestSize, "%u/%u/%u", Month, Day, Year);
}

static void Default_TimeString(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize)
{
  (void)Sec100;
  as_snprintf(Dest, DestSize, "%u:%u:%u", Hour, Minute, Second);
}

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

static void QueryInfo(void)
{
  int z;

  NLSInfo.DecSep = ".";
  NLSInfo.DataSep = ",";
  NLSInfo.ThouSep = ",";
  NLSInfo.Currency = "$";
  NLSInfo.CurrDecimals = 2;
  NLSInfo.CurrFmt = CurrFormatPreNoBlank;
  NLSInfo.DateString = Default_DateString;
  NLSInfo.TimeString = Default_TimeString;

  switch (GetACP())
  {
    case 437: NLSInfo.Codepage = eCodepage437; break;
    case 850: NLSInfo.Codepage = eCodepage850; break;
    case 866: NLSInfo.Codepage = eCodepage866; break;
    case 1251: NLSInfo.Codepage = eCodepage1251; break;
    case 1252: NLSInfo.Codepage = eCodepage1252; break;
    case 20866: NLSInfo.Codepage = eCodepageKOI8_R; break;
    case 28591: NLSInfo.Codepage = eCodepageISO8859_1; break;
    case 28605: NLSInfo.Codepage = eCodepageISO8859_15; break;
    default: NLSInfo.Codepage = eCodepageASCII;
  }

  UpCaseFromCodeTable();

  for (z = 0; z < 256; z++)
    CollateTable[z] = z;
  for (z = 'a'; z <= 'z'; z++)
    CollateTable[z] = toupper(z);
}

#elif defined DOS_NLS

#include <dos.h>

typedef struct
{
  Byte SubFuncNo;
  char *Result;
} DosTableRec;

char *DosCopy(char *Src, int Len)
{
  char *res = malloc(sizeof(char)*(Len + 1));
  memcpy(res, Src, Len);
  res[Len] = '\0';
  return res;
}

static void QueryInfo(void)
{
  union REGS Regs;
  struct SREGS SRegs;
  struct COUNTRY country_info;
  DosTableRec DOSTablePtr;
  int z;
  Word CodePage;
  Boolean GotTable = False;

  if (_osmajor < 3)
  {
    fprintf(stderr, "requires DOS 3.x or above\n");
    exit(255);
  }

  if (_osminor < 30)
    CodePage = 437;
  else
  {
    Regs.x.ax = 0x6601;
    int86(0x21, &Regs, &Regs);
    CodePage = Regs.x.bx;
  }
  switch (CodePage)
  {
    case 437: NLSInfo.Codepage = eCodepage437; break;
    case 850: NLSInfo.Codepage = eCodepage850; break;
    case 866: NLSInfo.Codepage = eCodepage866; break;
    case 1251: NLSInfo.Codepage = eCodepage1251; break;
    case 1252: NLSInfo.Codepage = eCodepage1252; break;
    case 20866: NLSInfo.Codepage = eCodepageKOI8_R; break;
    case 28591: NLSInfo.Codepage = eCodepageISO8859_1; break;
    case 28605: NLSInfo.Codepage = eCodepageISO8859_15; break;
    default: NLSInfo.Codepage = eCodepageASCII;
  }

  country(0x0000, &country_info);

  NLSInfo.DecSep = DosCopy(country_info.co_desep, 2);
  NLSInfo.DataSep = DosCopy(country_info.co_dtsep, 2);
  NLSInfo.ThouSep = DosCopy(country_info.co_thsep, 2);
  NLSInfo.Currency = DosCopy(country_info.co_curr, 5);
  NLSInfo.CurrDecimals = country_info.co_digits;
  NLSInfo.CurrFmt = (CurrFormat) country_info.co_currstyle;

  NLSInfo.Country = strcmp(NLSInfo.Currency, "DM") ? 1 : 49;

  NLSInfo.DateFmt = (DateFormat) country_info.co_date;
  NLSInfo.DateSep = DosCopy(country_info.co_dtsep, 2);
  NLSInfo.DateString = DOS_OS2_DateString;

  NLSInfo.TimeFmt = (TimeFormat) country_info.co_time;
  NLSInfo.TimeSep = DosCopy(country_info.co_tmsep, 2);
  NLSInfo.TimeString = DOS_OS2_TimeString;

#ifndef __DPMI16__
  for (z = 0; z < 256; z++)
    UpCaseTable[z] = (char) z;
  for (z = 'a'; z <= 'z'; z++)
    UpCaseTable[z] -= 'a' - 'A';
  if ((((Word)_osmajor) * 100) + _osminor >= 330)
  {
    Regs.x.ax = 0x6502;
    Regs.x.bx = CodePage;
    Regs.x.dx = NLSInfo.Country;
    Regs.x.cx = sizeof(DOSTablePtr);
    info = &DOSTablePtr;
    SRegs.es = FP_SEG(info);
    Regs.x.di = FP_OFF(info);
    int86x(0x21, &Regs, &Regs, &SRegs);
    if (Regs.x.cx == sizeof(DOSTablePtr))
    {
      DOSTablePtr.Result += sizeof(Word);
      memcpy(UpCaseTable + 128, DOSTablePtr.Result, 128);
      GotTable = True;
    }
  }
#endif /* __DPMI16__ */

  if (GotTable)
  {
    for (z = 0; z < 256; z++)
      LowCaseTable[z] = (char) z;
    for (z = 255; z >= 0; z--)
      if (UpCaseTable[z] != (char) z)
        LowCaseTable[((unsigned int) UpCaseTable[z])&0xff] = (char) z;
  }
  else
    UpCaseFromCodeTable();

  for (z = 0; z < 256; z++)
    CollateTable[z] = (char) z;
  for (z = 'a'; z <= 'z'; z++)
    CollateTable[z] = (char) (z - ('a' - 'A'));
#ifndef __DPMI16__
  if ((((Word)_osmajor)*100) + _osminor >= 330)
  {
    Regs.x.ax = 0x6506;
    Regs.x.bx = CodePage;
    Regs.x.dx = NLSInfo.Country;
    Regs.x.cx = sizeof(DOSTablePtr);
    info = &DOSTablePtr;
    SRegs.es = FP_SEG(info);
    Regs.x.di = FP_OFF(info);
    int86x(0x21, &Regs, &Regs, &SRegs);
    if (Regs.x.cx == sizeof(DOSTablePtr))
    {
      DOSTablePtr.Result += sizeof(Word);
      memcpy(CollateTable, DOSTablePtr.Result, 256);
    }
  }
#endif /* __DPMI16__ */
}

#elif defined LOCALE_NLS

#include <locale.h>
#include <langinfo.h>

static void Locale_DateString(Word Year, Word Month, Word Day, char *Dest, size_t DestSize)
{
  struct tm tm;

  tm.tm_year = Year - 1900;
  tm.tm_mon = Month - 1;
  tm.tm_mday = Day;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  strftime(Dest, DestSize, NLSInfo.DateFmtStr, &tm);
}

static void Locale_TimeString(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize)
{
  struct tm tm;

  (void)Sec100;
  tm.tm_year = 0;
  tm.tm_mon = 0;
  tm.tm_mday = 1;
  tm.tm_hour = Hour;
  tm.tm_min = Minute;
  tm.tm_sec = Second;
  strftime(Dest, DestSize, NLSInfo.TimeFmtStr, &tm);
}

static void QueryInfo(void)
{
  struct lconv *lc;
  int z;
  const char *pCodepage;

  lc = localeconv();

  NLSInfo.DecSep = lc->mon_decimal_point ? lc->decimal_point : ".";

  NLSInfo.ThouSep = lc->mon_thousands_sep ? lc->mon_thousands_sep : ",";

  NLSInfo.DataSep = ",";

  NLSInfo.Currency = lc->currency_symbol ? lc->currency_symbol : "$";

  NLSInfo.CurrDecimals = lc->int_frac_digits;
  if (NLSInfo.CurrDecimals > 4)
    NLSInfo.CurrDecimals = 2;

  if (lc->p_cs_precedes)
    NLSInfo.CurrFmt = lc->p_sep_by_space ? CurrFormatPreBlank : CurrFormatPreNoBlank;
  else
    NLSInfo.CurrFmt = lc->p_sep_by_space ? CurrFormatPostBlank : CurrFormatPostNoBlank;

  NLSInfo.DateFmtStr = nl_langinfo(D_FMT);
  if (!NLSInfo.DateFmtStr || !*NLSInfo.DateFmtStr)
    NLSInfo.DateFmtStr = "%m/%d/%y";
  NLSInfo.DateString = Locale_DateString;

  NLSInfo.TimeFmtStr = nl_langinfo(T_FMT);
  if (!NLSInfo.TimeFmtStr || !*NLSInfo.TimeFmtStr)
    NLSInfo.TimeFmtStr = "%H:%M:%S";
  NLSInfo.TimeString = Locale_TimeString;

  NLSInfo.Codepage = eCodepageASCII;
  pCodepage = getenv("LC_CTYPE");
  if (!pCodepage)
    pCodepage = getenv("LC_ALL");
  if (!pCodepage)
    pCodepage = getenv("LANG");
  if (pCodepage)
  {
    pCodepage = strchr(pCodepage, '.');
    if (pCodepage)
    {
      pCodepage++;
      if (!as_strcasecmp(pCodepage, "ISO-8859-1"))
        NLSInfo.Codepage = eCodepageISO8859_1;
      else if (!as_strcasecmp(pCodepage, "ISO-8859-15"))
        NLSInfo.Codepage = eCodepageISO8859_15;
      else if (!as_strcasecmp(pCodepage, "UTF-8") || !as_strcasecmp(pCodepage, "UTF8"))
        NLSInfo.Codepage = eCodepageUTF8;
      else if (!as_strcasecmp(pCodepage, "KOI8-R"))
        NLSInfo.Codepage = eCodepageKOI8_R;
    }
  }

  UpCaseFromCodeTable();

  for (z = 0; z < 256; z++)
    CollateTable[z] = z;
  for (z = 'a'; z <= 'z'; z++)
    CollateTable[z] = toupper(z);
}

#else /* NO_NLS */

static void Default_DateString(Word Year, Word Month, Word Day, char *Dest, size_t DestSize)
{
  as_snprintf(Dest, DestSize, "%u/%u/%u", Month, Day, Year);
}

static void Default_TimeString(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize)
{
  (void)Sec100;
  as_snprintf(Dest, DestSize, "%u:%u:%u", Hour, Minute, Second);
}

static void QueryInfo(void)
{
  int z;

  NLSInfo.Codepage = eCodepageASCII;
  NLSInfo.DecSep = ".";
  NLSInfo.DataSep = ",";
  NLSInfo.ThouSep = ",";
  NLSInfo.Currency = "$";
  NLSInfo.CurrDecimals = 2;
  NLSInfo.CurrFmt = CurrFormatPreNoBlank;
  NLSInfo.DateString = Default_DateString;
  NLSInfo.TimeString = Default_TimeString;

  UpCaseFromCodeTable();

  for (z = 0; z < 256; z++)
    CollateTable[z] = z;
  for (z = 'a'; z <= 'z'; z++)
    CollateTable[z] = toupper(z);
}

#endif /* xxx_NLS */

/*-------------------------------------------------------------------------------*/
/* Da es moeglich ist, die aktuelle Codeseite im Programmlauf zu wechseln,
   ist die Initialisierung in einer getrennten Routine untergebracht.  Nach
   einem Wechsel stellt ein erneuter Aufruf wieder korrekte Verhaeltnisse
   her.  Wen das stoert, der schreibe einfach einen Aufruf in den Initiali-
   sierungsteil der Unit hinein. */

Boolean NLS_Initialize(int *argc, char **argv)
{
  QueryInfo();

  /* query overrides */

  if (argc)
  {
    int z, dest;

    z = dest = 1;
    while (z < *argc)
      if ((z < *argc - 1) && !as_strcasecmp(argv[z], "-codepage"))
      {
        for (OverrideCodepage = (tCodepage)0; OverrideCodepage < eCodepageCnt; OverrideCodepage++)
          if (!as_strcasecmp(argv[z + 1], CodepageNames[OverrideCodepage]))
            break;
        if (OverrideCodepage >= eCodepageCnt)
        {
          fprintf(stderr, "unknown codepage: %s\n", argv[z + 1]);
          return False;
        }
        z += 2;
      }
      else
        argv[dest++] = argv[z++];
    *argc = dest;
  }
  if (OverrideCodepage < eCodepageCnt)
    NLSInfo.Codepage = OverrideCodepage;

  DumpNLSInfo();

  NLSInfo.Initialized = True;
  return True;
}

void NLS_DateString(Word Year, Word Month, Word Day, char *Dest, size_t DestSize)
{
  if (!NLSInfo.DateString)
  {
    fprintf(stderr, "NLS not yet initialized\n");
    exit(255);
  }
  NLSInfo.DateString(Year, Month, Day, Dest, DestSize);
}

void NLS_CurrDateString(char *Dest, size_t DestSize)
{
  time_t timep;
  struct tm *trec;

  time(&timep);
  trec = localtime(&timep);
  NLS_DateString(trec->tm_year + 1900, trec->tm_mon + 1, trec->tm_mday, Dest, DestSize);
}

void NLS_TimeString(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize)
{
  if (!NLSInfo.TimeString)
  {
    fprintf(stderr, "NLS not yet initialized\n");
    exit(255);
  }
  NLSInfo.TimeString(Hour, Minute, Second, Sec100, Dest, DestSize);
}

void NLS_CurrTimeString(Boolean Use100, char *Dest, size_t DestSize)
{
  time_t timep;
  struct tm *trec;

  time(&timep); trec = localtime(&timep);
  NLS_TimeString(trec->tm_hour, trec->tm_min, trec->tm_sec, Use100 ? 0 : 100, Dest, DestSize);
}

void NLS_CurrencyString(double inp, char *erg, size_t DestSize)
{
  char s[1024];
  char *p, *z;

  /* Schritt 1: mit passender Nachkommastellenzahl wandeln */

  as_snprintf(s, sizeof(s), "%0.*f", (int)NLSInfo.CurrDecimals, inp);

  /* Schritt 2: vorne den Punkt suchen */

  p = (NLSInfo.CurrDecimals == 0) ? s + strlen(s) : strchr(s, '.');

  /* Schritt 3: Tausenderstellen einfuegen */

  z = p;
  while (z - s > 3)
  {
    strins(s, NLSInfo.ThouSep, z - s - 3);
    z -= 3;
    p += strlen(NLSInfo.ThouSep);
  };

  /* Schritt 4: Komma anpassen */

  strmov(p, p + 1);
  strins(s, NLSInfo.DecSep, p - s);

  /* Schritt 5: Einheit anbauen */

  switch (NLSInfo.CurrFmt)
  {
    case CurrFormatPreNoBlank:
      as_snprintf(erg, DestSize, "%s%s", NLSInfo.Currency, s);
      break;
    case CurrFormatPreBlank:
      as_snprintf(erg, DestSize, "%s %s", NLSInfo.Currency, s);
      break;
    case CurrFormatPostNoBlank:
      as_snprintf(erg, DestSize, "%s%s", s, NLSInfo.Currency);
      break;
    case CurrFormatPostBlank:
      as_snprintf(erg, DestSize, "%s %s", s, NLSInfo.Currency);
      break;
    default:
      *p = '\0';
      as_snprintf(erg, DestSize, "%s%s%s", s, NLSInfo.Currency, p + strlen(NLSInfo.DecSep));
  }
}

char Upcase(char inp)
{
  return UpCaseTable[((unsigned int) inp) & 0xff];
}

void NLS_UpString(char *pStr)
{
  unsigned Unicode;
  const char *pSrc = pStr;

  /* TODO: assure pSrc & pStr remain in-sync for UTF-8 */

  while (*pStr)
  {
    if (eCodepageUTF8 == NLSInfo.Codepage)
      Unicode = UTF8ToUnicode(&pSrc);
    else
      Unicode = ((unsigned int)(*pSrc++)) & 0xff;
    Unicode = (Unicode < 256) ? (unsigned int)(UpCaseTable[Unicode] & 0xff) : Unicode;
    if (eCodepageUTF8 == NLSInfo.Codepage)
      UnicodeToUTF8(&pStr, Unicode);
    else
      *pStr++ = Unicode;
  }
}

/*!------------------------------------------------------------------------
 * \fn     NLS_UpString2(char *p_dest, size_t dest_size, const char *p_src, size_t src_len)
 * \brief  comvert string to upper case
 * \param  p_dest destination
 * \param  dest_size capacity of destination
 * \param  p_src source
 * \param  src_len length of source (need not be NUL-terminated)
 * ------------------------------------------------------------------------ */

void NLS_UpString2(char *p_dest, size_t dest_size, const char *p_src, size_t src_len)
{
  unsigned unicode;
  char *p_dest_end;
  const char *p_src_end;

  if (dest_size < 1)
    return;
  p_src_end = p_src + src_len;
  p_dest_end = p_dest + dest_size - 1;

  while (p_src < p_src_end)
  {
    if (eCodepageUTF8 == NLSInfo.Codepage)
      unicode = UTF8ToUnicode(&p_src);
    else
      unicode = ((unsigned int)(*p_src++)) & 0xff;
    unicode = (unicode < 256) ? (unsigned int)(UpCaseTable[unicode] & 0xff) : unicode;
    if (eCodepageUTF8 == NLSInfo.Codepage)
    {
      if (p_dest < p_dest_end - 2)
        UnicodeToUTF8(&p_dest, unicode);
    }
    else
    {
      if (p_dest < p_dest_end)
        *p_dest++ = unicode;
    }
  }
  *p_dest = '\0';
}

void NLS_LowString(char *pStr)
{
  unsigned Unicode;
  const char *pSrc = pStr;

  /* TODO: assure pSrc & pStr remain in-sync for UTF-8 */

  while (*pStr)
  {
    if (eCodepageUTF8 == NLSInfo.Codepage)
      Unicode = UTF8ToUnicode(&pSrc);
    else
      Unicode = ((unsigned int)(*pSrc++)) & 0xff;
    Unicode = (Unicode < 256) ? (unsigned int)(LowCaseTable[Unicode] & 0xff) : Unicode;
    if (eCodepageUTF8 == NLSInfo.Codepage)
      UnicodeToUTF8(&pStr, Unicode);
    else
      *pStr++ = Unicode;
  }
}

int NLS_StrCmp(const char *s1, const char *s2)
{
  while (CollateTable[((unsigned int)*s1) & 0xff] == CollateTable[((unsigned int)*s2) & 0xff])
  {
    if ((!*s1) && (!*s2))
      return 0;
    s1++;
    s2++;
  }
  return ((int) CollateTable[((unsigned int)*s1) & 0xff] - CollateTable[((unsigned int)*s2) & 0xff]);
}

Word NLS_GetCountryCode(void)
{
  if (!NLSInfo.Initialized)
  {
    fprintf(stderr, "NLS_GetCountryCode() called before initialization\n");
    exit(255);
  }
  return NLSInfo.Country;
}

tCodepage NLS_GetCodepage(void)
{
  if (!NLSInfo.Initialized)
  {
    fprintf(stderr, "NLS_GetCodepage() called before initialization\n");
    exit(255);
  }
  return NLSInfo.Codepage;
}

void nls_init(void)
{
#ifdef LOCALE_NLS
  (void) setlocale(LC_TIME, "");
  (void) setlocale(LC_MONETARY, "");
#endif
}
