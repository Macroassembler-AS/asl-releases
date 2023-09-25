/* asmsub.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Unterfunktionen, vermischtes                                              */
/*                                                                           */
/*****************************************************************************/


#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "version.h"
#include "endian.h"
#include "stdhandl.h"
#include "console.h"
#include "nls.h"
#include "chardefs.h"
#include "nlmessages.h"
#include "as.rsc"
#include "strutil.h"
#include "stringlists.h"
#include "chunks.h"
#include "ioerrs.h"
#include "intformat.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmdebug.h"
#include "asmlist.h"
#include "as.h"

#include "asmsub.h"


#ifdef __TURBOC__
#ifdef __DPMI16__
#define STKSIZE 32768
#else
#define STKSIZE 49152
#endif
#endif

#define VALID_S1 1
#define VALID_SN 2
#define VALID_M1 4
#define VALID_MN 8

static StringList CopyrightList, OutList, ShareOutList, ListOutList;

static LongWord StartStack, MinStack, LowStack;

static unsigned ValidSymCharLen;
static Byte *ValidSymChar;

/****************************************************************************/
/* Modulinitialisierung */

void AsmSubPassInit(void)
{
  PageLength = 60;
  PageWidth = 0;
}

/****************************************************************************/
/* Copyrightlistenverwaltung */

void AddCopyright(const char *NewLine)
{
  AddStringListLast(&CopyrightList, NewLine);
}

void WriteCopyrights(void(*PrintProc)(const char *))
{
  StringRecPtr Lauf;
  const char *p_line;

  for (p_line = GetStringListFirst(CopyrightList, &Lauf);
       p_line; p_line = GetStringListNext(&Lauf))
    PrintProc(p_line);
}

/*--------------------------------------------------------------------------*/
/* ermittelt das erste/letzte Auftauchen eines Zeichens ausserhalb */
/* "geschuetzten" Bereichen */

static char *QuotPosCore(const char *s, int (*SearchFnc)(const char*, const char*), const char *pSearch, tQualifyQuoteFnc QualifyQuoteFnc)
{
  register ShortInt Brack = 0, AngBrack = 0;
  register const char *i;
  Boolean InSglQuot = False, InDblQuot = False, ThisEscaped = False, NextEscaped = False;

  for (i = s; *i; i++, ThisEscaped = NextEscaped)
  {
    NextEscaped = False;
    if (!SearchFnc(i, pSearch))
    {
      if (!AngBrack && !Brack && !InSglQuot && !InDblQuot)
        return (char*)i;
    }
    switch (*i)
    {
      case '"':
        if (!InSglQuot && !ThisEscaped)
          InDblQuot = !InDblQuot;
        break;
      case '\'':
        if (!InDblQuot && !ThisEscaped)
        {
          if (InSglQuot || !QualifyQuoteFnc || QualifyQuoteFnc(s, i))
            InSglQuot = !InSglQuot;
        }
        break;
      case '\\':
        if ((InSglQuot || InDblQuot) && !ThisEscaped)
          NextEscaped = True;
        break;
      case '(':
        if (!AngBrack && !InDblQuot && !InSglQuot)
          Brack++;
        break;
      case ')':
        if (!AngBrack && !InDblQuot && !InSglQuot)
          Brack--;
        break;
      case '[':
        if (!Brack && !InDblQuot && !InSglQuot)
          AngBrack++;
        break;
      case ']':
        if (!Brack && !InDblQuot && !InSglQuot)
          AngBrack--;
        break;
    }
  }

  return NULL;
}

static int SearchSingleChar(const char *pPos, const char *pSearch)
{
  return ((int)*pSearch) - ((int)*pPos);
}

static int SearchMultChar(const char *pPos, const char *pSearch)
{
  return !strchr(pSearch, *pPos);
}

static int SearchMultString(const char *pPos, const char *pSearch)
{
  int len;

  while (True)
  {
    if (!(len = strlen(pSearch)))
      return 1;
    if (!strncmp(pPos, pSearch, len))
      return 0;
    pSearch += len + 1;
  }
}

char *QuotMultPosQualify(const char *s, const char *pSearch, tQualifyQuoteFnc QualifyQuoteFnc)
{
  return QuotPosCore(s, SearchMultChar, pSearch, QualifyQuoteFnc);
}

char *QuotPosQualify(const char *s, char Zeichen, tQualifyQuoteFnc QualifyQuoteFnc)
{
  return QuotPosCore(s, SearchSingleChar, &Zeichen, QualifyQuoteFnc);
}

char *QuotSMultPosQualify(const char *s, const char *pStrs, tQualifyQuoteFnc QualifyQuoteFnc)
{
  return QuotPosCore(s, SearchMultString, pStrs, QualifyQuoteFnc);
}

char *RQuotPos(char *s, char Zeichen)
{
  ShortInt Brack = 0, AngBrack = 0;
  char *i;
  Boolean Quot = False, Paren = False;

  for (i = s + strlen(s) - 1; i >= s; i--)
    if (*i == Zeichen)
    {
      if ((!AngBrack) && (!Brack) && (!Paren) && (!Quot))
        return i;
    }
    else switch (*i)
    {
      case '"':
        if ((!Brack) && (!AngBrack) && (!Quot))
          Paren = !Paren;
        break;
      case '\'':
        if ((!Brack) && (!AngBrack) && (!Paren))
          Quot = !Quot;
        break;
      case ')':
        if ((!AngBrack) && (!Paren) && (!Quot))
          Brack++;
        break;
      case '(':
        if ((!AngBrack) && (!Paren) && (!Quot))
          Brack--;
        break;
      case ']':
        if ((!Brack) && (!Paren) && (!Quot))
          AngBrack++;
        break;
      case '[':
        if ((!Brack) && (!Paren) && (!Quot))
          AngBrack--;
        break;
    }

  return NULL;
}

/*--------------------------------------------------------------------------*/
/* ermittelt das erste (nicht-) Leerzeichen in einem String */

char *FirstBlank(const char *s)
{
  const char *h, *Min = NULL;

  h = strchr(s, ' ');
  if (h)
    if ((!Min) || (h < Min))
      Min = h;
  h = strchr(s, Char_HT);
  if (h)
    if ((!Min) || (h < Min))
      Min = h;
  return (char*)Min;
}

/*--------------------------------------------------------------------------*/
/* einen String in zwei Teile zerlegen */

void SplitString(char *Source, char *Left, char *Right, char *Trenner)
{
  char Save;
  LongInt slen = strlen(Source);

  if ((!Trenner) || (Trenner >= Source + slen))
    Trenner = Source + slen;
  Save = (*Trenner);
  *Trenner = '\0';
  strmov(Left, Source);
  *Trenner = Save;
  if (Trenner >= Source + slen)
    *Right = '\0';
  else
    strmov(Right, Trenner + 1);
}

/*--------------------------------------------------------------------------*/
/* verbesserte Grossbuchstabenfunktion */

/* einen String in Grossbuchstaben umwandeln.  Dabei Stringkonstanten in Ruhe */
/* lassen */

void UpString(char *s)
{
  char *z;
  int hypquot = 0;
  Boolean LastBk = FALSE, ThisBk;

  for (z = s; *z != '\0'; z++)
  {
    ThisBk = FALSE;
    switch (*z)
    {
      case '\\':
        ThisBk = TRUE;
        break;
      case '\'':
        if ((!(hypquot & 2)) && (!LastBk))
          hypquot ^= 1;
        break;
      case '"':
        if ((!(hypquot & 1)) && (!LastBk))
          hypquot ^= 2;
        break;
      default:
        if (!hypquot)
          *z = UpCaseTable[(int)*z];
    }
    LastBk = ThisBk;
  }
}

/*!------------------------------------------------------------------------
 * \fn     MatchChars(const char *pStr, const char *pPattern, ...)
 * \brief  see if beginning of string matches given pattern
 * \param  pStr string to check
 * \param  pPattern expected pattern
 * \return * to character following match or NULL if no match
 * ------------------------------------------------------------------------ */

char *MatchChars(const char *pStr, const char *pPattern, ...)
{
  va_list ap;
  char *pResult = NULL;

  va_start(ap, pPattern);
  for (; *pPattern; pPattern++)
    switch (*pPattern)
    {
      /* single space in pattern matches arbitrary # of spaces in string */
      case ' ':
        for (; as_isspace(*pStr); pStr++);
        break;
      case '?':
      {
        const char *pPatternStr = va_arg(ap, const char*);
        char *pSave = va_arg(ap, char*);

        if (!strchr(pPatternStr, as_toupper(*pStr)))
          goto func_exit;
        if (pSave)
          *pSave = *pStr;
        pStr++;
        break;
      }
      default:
        if (as_toupper(*pStr) != as_toupper(*pPattern))
          goto func_exit;
        pStr++;
    }
  pResult = (char*)pStr;
func_exit:
  va_end(ap);
  return pResult;
}

/*!------------------------------------------------------------------------
 * \fn     MatchCharsRev(const char *pStr, const char *pPattern, ...)
 * \brief  see if end of string matches given pattern
 * \param  pStr string to check
 * \param  pPattern expected pattern
 * \return * to trailing string matching pattern or NULL if no match
 * ------------------------------------------------------------------------ */

char *MatchCharsRev(const char *pStr, const char *pPattern, ...)
{
  va_list ap;
  char *pResult = NULL;
  const char *pPatternRun = pPattern + strlen(pPattern) - 1,
             *pStrRun = pStr + strlen(pStr) - 1;

  va_start(ap, pPattern);
  for (; pPatternRun >= pPattern; pPatternRun--)
    switch (*pPatternRun)
    {
      /* single space in pattern matches arbitrary # of spaces in string */
      case ' ':
        for (; (pStrRun >= pStr) && as_isspace(*pStrRun); pStrRun--);
        break;
      case '?':
      {
        const char *pPatternStr = va_arg(ap, const char*);
        char *pSave = va_arg(ap, char*);

        if (!strchr(pPatternStr, as_toupper(*pStrRun)))
          goto func_exit;
        if (pSave)
          *pSave = *pStrRun;
        pStrRun--;
        break;
      }
      default:
        if ((pStrRun < pStr) || (as_toupper(*pStrRun) != as_toupper(*pPatternRun)))
          goto func_exit;
        pStrRun--;
    }
  pResult = (char*)(pStrRun + 1);
func_exit:
  va_end(ap);
  return pResult;
}

/*!------------------------------------------------------------------------
 * \fn     FindClosingParenthese(const char *pStr)
 * \brief  find matching closing parenthese
 * \param  pStr * to string right after opening parenthese
 * \return * to closing parenthese or NULL
 * ------------------------------------------------------------------------ */

char *FindClosingParenthese(const char *pStr)
{
  int Nest = 1;
  Boolean InSgl = False, InDbl = False;

  for (; *pStr; pStr++)
  {
    switch (*pStr)
    {
      case '\'':
        if (!InDbl) InSgl = !InSgl;
        break;
      case '"':
        if (!InSgl) InDbl = !InDbl;
        break;
      case '(':
        if (!InSgl && !InDbl) Nest++;
        break;
      case ')':
        if (!InSgl && !InDbl) Nest--;
        if (!Nest)
          return (char*)pStr;
        break;
      default:
        break;
    }
  }
  return NULL;
}

/*!------------------------------------------------------------------------
 * \fn     FindOpeningParenthese(const char *pStrBegin, const char *pStrEnd, const char Bracks[2])
 * \brief  find matching opening parenthese in string
 * \param  pStrBegin start of string
 * \param  pStrEnd end of string, preceding closing parenthese in question
 * \param  Bracks opening & closing parenthese
 * \return * to opening parenthese or NULL if not found
 * ------------------------------------------------------------------------ */

char *FindOpeningParenthese(const char *pStrBegin, const char *pStrEnd, const char Bracks[2])
{
  int Nest = 1;
  Boolean InSgl = False, InDbl = False;

  for (; pStrEnd >= pStrBegin; pStrEnd--)
  {
    if (*pStrEnd == Bracks[1])
    {
      if (!InSgl && !InDbl) Nest++;
    }
    else if (*pStrEnd == Bracks[0])
    {
      if (!InSgl && !InDbl) Nest--;
      if (!Nest)
        return (char*)pStrEnd;
    }
    else switch (*pStrEnd)
    {
      case '\'':
        if (!InDbl) InSgl = !InSgl;
        break;
      case '"':
        if (!InSgl) InDbl = !InDbl;
        break;
      default:
        break;
    }
  }
  return NULL;
}

/****************************************************************************/

ShortInt StrCaseCmp(const char *s1, const char *s2, LongInt Hand1, LongInt Hand2)
{
  int tmp;

  tmp = as_toupper(*s1) - as_toupper(*s2);
  if (!tmp)
    tmp = as_strcasecmp(s1, s2);
  if (!tmp)
    tmp = Hand1 - Hand2;
  if (tmp < 0)
    return -1;
  if (tmp > 0)
    return 1;
  return 0;
}

/****************************************************************************/
/* an einen Dateinamen eine Endung anhaengen */

void AddSuffix(char *s, const char *Suff)
{
  char *p, *z, *Part;

  p = NULL;
  for (z = s; *z != '\0'; z++)
    if (*z == PATHSEP)
      p = z;
  Part = p ? p : s;
  if (!strchr(Part, '.'))
    strmaxcat(s, Suff, STRINGSIZE);
}


/*--------------------------------------------------------------------------*/
/* von einem Dateinamen die Endung loeschen */

void KillSuffix(char *s)
{
  char *p, *z, *Part;

  p = NULL;
  for (z = s; *z != '\0'; z++)
    if (*z == PATHSEP)
      p = z;
  Part = p ? p : s;
  Part = strchr(Part, '.');
  if (Part)
    *Part = '\0';
}

/*--------------------------------------------------------------------------*/
/* Pfadanteil (Laufwerk+Verzeichnis) von einem Dateinamen abspalten */

char *PathPart(char *Name)
{
  static String s;
  char *p;

  strmaxcpy(s, Name, STRINGSIZE);

  p = strrchr(Name, PATHSEP);
#ifdef DRSEP
  if (!p)
    p = strrchr(Name, DRSEP);
#endif

  if (!p)
    *s = '\0';
  else
    s[1] = '\0';

  return s;
}

/*--------------------------------------------------------------------------*/
/* Namensanteil von einem Dateinamen abspalten */

const char *NamePart(const char *Name)
{
  const char *p = strrchr(Name, PATHSEP);

#ifdef DRSEP
  if (!p)
    p = strrchr(Name, DRSEP);
#endif

  return p ? p + 1 : Name;
}

/****************************************************************************/
/* eine Gleitkommazahl in einen String umwandeln */

void FloatString(char *pDest, size_t DestSize, Double f)
{
#define MaxLen 18
  char *p, *d, ExpChar = HexStartCharacter + ('E' - 'A');
  sint n, ExpVal, nzeroes;
  Boolean WithE, OK;

  /* 1. mit Maximallaenge wandeln, fuehrendes Vorzeichen weg */

  (void)DestSize;
  as_snprintf(pDest, DestSize, "%27.15e", f);
  for (p = pDest; (*p == ' ') || (*p == '+'); p++);
  if (p != pDest)
    strmov(pDest, p);

  /* 2. Exponenten soweit als moeglich kuerzen, evtl. ganz streichen */

  p = strchr(pDest, ExpChar);
  if (!p)
    return;
  switch (*(++p))
  {
    case '+':
      strmov(p, p + 1);
      break;
    case '-':
      p++;
      break;
  }

  while (*p == '0')
    strmov(p, p + 1);
  WithE = (*p != '\0');
  if (!WithE)
    pDest[strlen(pDest) - 1] = '\0';

  /* 3. Nullen am Ende der Mantisse entfernen, Komma bleibt noch */

  p = WithE ? strchr(pDest, ExpChar) : pDest + strlen(pDest);
  p--;
  while (*p == '0')
  {
    strmov(p, p + 1);
    p--;
  }

  /* 4. auf die gewuenschte Maximalstellenzahl begrenzen */

  p = WithE ? strchr(pDest, ExpChar) : pDest + strlen(pDest);
  d = strchr(pDest, '.');
  n = p - d - 1;

  /* 5. Maximallaenge ueberschritten ? */

  if (strlen(pDest) > MaxLen)
    strmov(d + (n - (strlen(pDest) - MaxLen)), d + n);

  /* 6. Exponentenwert berechnen */

  if (WithE)
  {
    p = strchr(pDest, ExpChar);
    ExpVal = ConstLongInt(p + 1, &OK, 10);
  }
  else
  {
    p = pDest + strlen(pDest);
    ExpVal = 0;
  }

  /* 7. soviel Platz, dass wir den Exponenten weglassen und evtl. Nullen
       anhaengen koennen ? */

  if (ExpVal > 0)
  {
    nzeroes = ExpVal - (p - strchr(pDest, '.') - 1); /* = Zahl von Nullen, die anzuhaengen waere */

    /* 7a. nur Kommaverschiebung erforderlich. Exponenten loeschen und
          evtl. auch Komma */

    if (nzeroes <= 0)
    {
      *p = '\0';
      d = strchr(pDest, '.');
      strmov(d, d + 1);
      if (nzeroes != 0)
      {
        memmove(pDest + strlen(pDest) + nzeroes + 1, pDest + strlen(pDest) + nzeroes, -nzeroes);
        pDest[strlen(pDest) - 1 + nzeroes] = '.';
      }
    }

    /* 7b. Es muessen Nullen angehaengt werden. Schauen, ob nach Loeschen von
          Punkt und E-Teil genuegend Platz ist */

    else
    {
      n = strlen(p) + 1 + (MaxLen - strlen(pDest)); /* = Anzahl freizubekommender Zeichen+Gutschrift */
      if (n >= nzeroes)
      {
        *p = '\0';
        d = strchr(pDest, '.');
        strmov(d, d + 1);
        d = pDest + strlen(pDest);
        for (n = 0; n < nzeroes; n++)
          *(d++) = '0';
        *d = '\0';
      }
    }
  }

  /* 8. soviel Platz, dass Exponent wegkann und die Zahl mit vielen Nullen
       vorne geschrieben werden kann ? */

  else if (ExpVal < 0)
  {
    n = (-ExpVal) - (strlen(p)); /* = Verlaengerung nach Operation */
    if (strlen(pDest) + n <= MaxLen)
    {
      *p = '\0';
      d = strchr(pDest, '.');
      strmov(d, d + 1);
      d = (pDest[0] == '-') ? pDest + 1 : pDest;
      memmove(d - ExpVal + 1, d, strlen(pDest) + 1);
      *(d++) = '0';
      *(d++) = '.';
      for (n = 0; n < -ExpVal - 1; n++)
        *(d++) = '0';
    }
  }


  /* 9. Ueberfluessiges Komma entfernen */

  if (WithE)
    p = strchr(pDest, ExpChar);
  else
    p = pDest + strlen(pDest);
  if (p && (*(p - 1) == '.'))
    strmov(p - 1, p);
}

/****************************************************************************/
/* Symbol in String wandeln */

void StrSym(const TempResult *t, Boolean WithSystem, as_dynstr_t *p_dest, unsigned Radix)
{
  LargeInt IntVal;

  if (p_dest->capacity)
    p_dest->p_str[0] = '\0';
  switch (t->Typ)
  {
    case TempInt:
      IntVal = t->Contents.Int;
    IsInt:
    {
      String Buf;

      if (WithSystem)
      {
        switch (IntConstMode)
        {
          case eIntConstModeMoto:
            as_sdprcatf(p_dest, "%s", GetIntConstMotoPrefix(Radix));
            break;
          case eIntConstModeC:
            as_sdprcatf(p_dest, "%s", GetIntConstCPrefix(Radix));
            break;
          case eIntConstModeIBM:
            as_sdprcatf(p_dest, "%s", GetIntConstIBMPrefix(Radix));
            break;
          default:
            break;
        }
      }
      SysString(Buf, sizeof(Buf), IntVal, Radix,
                1, (16 == Radix) && (IntConstMode == eIntConstModeIntel),
                HexStartCharacter, SplitByteCharacter);
      as_sdprcatf(p_dest, "%s", Buf);
      if (WithSystem)
      {
        switch (IntConstMode)
        {
          case eIntConstModeIntel:
            as_sdprcatf(p_dest, GetIntConstIntelSuffix(Radix));
            break;
          case eIntConstModeIBM:
            as_sdprcatf(p_dest, GetIntConstIBMSuffix(Radix));
            break;
          default:
            break;
        }
      }
      break;
    }
    case TempFloat:
      FloatString(p_dest->p_str, p_dest->capacity, t->Contents.Float);
      break;
    case TempString:
      as_tempres_append_dynstr(p_dest, t);
      break;
    case TempReg:
      if (t->Contents.RegDescr.Dissect)
        t->Contents.RegDescr.Dissect(p_dest->p_str, p_dest->capacity, t->Contents.RegDescr.Reg, t->DataSize);
      else
      {
        IntVal = t->Contents.RegDescr.Reg;
        goto IsInt;
      }
      break;
    default:
      as_sdprintf(p_dest, "???");
  }
}

/****************************************************************************/
/* Listingzaehler zuruecksetzen */

void ResetPageCounter(void)
{
  int z;

  for (z = 0; z <= ChapMax; z++)
    PageCounter[z] = 0;
  LstCounter = 0;
  ChapDepth = 0;
}

/*--------------------------------------------------------------------------*/
/* eine neue Seite im Listing beginnen */

void NewPage(ShortInt Level, Boolean WithFF)
{
  ShortInt z;
  String Header, s;
  char Save;

  if (ListOn == 0)
    return;

  LstCounter = 0;

  if (ChapDepth < (Byte) Level)
  {
    memmove(PageCounter + (Level - ChapDepth), PageCounter, (ChapDepth + 1) * sizeof(Word));
    for (z = 0; z <= Level - ChapDepth; PageCounter[z++] = 1);
    ChapDepth = Level;
  }
  for (z = 0; z <= Level - 1; PageCounter[z++] = 1);
  PageCounter[Level]++;

  if ((WithFF) && (!ListToNull))
  {
    errno = 0;
    fprintf(LstFile, "%c", Char_FF);
    ChkIO(ErrNum_ListWrError);
  }

  as_snprintf(Header, sizeof(Header), " AS V%s%s%s",
              Version,
              getmessage(Num_HeadingFileNameLab),
              NamePart(SourceFile));
  if (strcmp(CurrFileName, "INTERNAL")
   && *CurrFileName
   && strcmp(NamePart(CurrFileName), NamePart(SourceFile)))
  {
    strmaxcat(Header, "(", STRINGSIZE);
    strmaxcat(Header, NamePart(CurrFileName), STRINGSIZE);
    strmaxcat(Header, ")", STRINGSIZE);
  }
  strmaxcat(Header, getmessage(Num_HeadingPageLab), STRINGSIZE);

  for (z = ChapDepth; z >= 0; z--)
  {
    as_snprintf(s, sizeof(s), IntegerFormat, PageCounter[z]);
    strmaxcat(Header, s, STRINGSIZE);
    if (z != 0)
      strmaxcat(Header, ".", STRINGSIZE);
  }

  strmaxcat(Header, " - ", STRINGSIZE);
  NLS_CurrDateString(s, sizeof(s));
  strmaxcat(Header, s, STRINGSIZE);
  strmaxcat(Header, " ", STRINGSIZE);
  NLS_CurrTimeString(False, s, sizeof(s));
  strmaxcat(Header, s, STRINGSIZE);

  if (PageWidth != 0)
    while (strlen(Header) > PageWidth)
    {
      Save = Header[PageWidth];
      Header[PageWidth] = '\0';
      if (!ListToNull)
      {
        errno = 0;
        fprintf(LstFile, "%s\n", Header);
        ChkIO(ErrNum_ListWrError);
      }
      Header[PageWidth] = Save;
      strmov(Header, Header + PageWidth);
    }

  if (!ListToNull)
  {
    errno = 0;
    fprintf(LstFile, "%s\n", Header);
    ChkIO(ErrNum_ListWrError);

    if (PrtTitleString[0])
    {
      errno = 0;
      fprintf(LstFile, "%s\n", PrtTitleString);
      ChkIO(ErrNum_ListWrError);
    }

    errno = 0;
    fprintf(LstFile, "\n\n");
    ChkIO(ErrNum_ListWrError);
  }
}

/*--------------------------------------------------------------------------*/
/* eine Zeile ins Listing schieben */

void WrLstLine(const char *Line)
{
  int LLength;
  char bbuf[2500];
  String LLine;
  int blen = 0, hlen, z, Start;

  if ((ListOn == 0) || (ListToNull))
    return;

  if (PageLength == 0)
  {
    errno = 0;
    fprintf(LstFile, "%s\n", Line);
    ChkIO(ErrNum_ListWrError);
  }
  else
  {
    if ((PageWidth == 0) || ((strlen(Line) << 3) < PageWidth))
      LLength = 1;
    else
    {
      blen = 0;
      for (z = 0; z < (int)strlen(Line);  z++)
        if (Line[z] == Char_HT)
        {
          memset(bbuf + blen, ' ', 8 - (blen & 7));
          blen += 8 - (blen&7);
        }
        else
          bbuf[blen++] = Line[z];
      LLength = blen / PageWidth;
      if (blen % PageWidth)
        LLength++;
    }
    if (LLength == 1)
    {
      errno = 0;
      fprintf(LstFile, "%s\n", Line);
      ChkIO(ErrNum_ListWrError);
      if ((++LstCounter) == PageLength)
        NewPage(0, True);
    }
    else
    {
      Start = 0;
      for (z = 1; z <= LLength; z++)
      {
        hlen = PageWidth;
        if (blen - Start < hlen)
          hlen = blen - Start;
        memcpy(LLine, bbuf + Start, hlen);
        LLine[hlen] = '\0';
        errno = 0;
        fprintf(LstFile, "%s\n", LLine);
        if ((++LstCounter) == PageLength)
          NewPage(0, True);
        Start += hlen;
      }
    }
  }
}

/*****************************************************************************/
/* Ausdruck in Spalte vor Listing */

void SetListLineVal(TempResult *t)
{
  as_dynstr_t str;

  as_dynstr_ini(&str, STRINGSIZE);
  StrSym(t, True, &str, ListRadixBase);
  as_snprintf(ListLine, STRINGSIZE, "=%s", str.p_str);
  as_dynstr_free(&str);
}

/*!------------------------------------------------------------------------
 * \fn     PrintOneLineMuted(FILE *pFile, const char *pLine,
                             const struct sLineComp *pMuteComponent,
                             const struct sLineComp *pMuteComponent2)
 * \brief  print a line, with a certain component muted out (i.e. replaced by spaces)
 * \param  pFile where to write
 * \param  pLine line to print
 * \param  pMuteComponent component to mute in printout
 * ------------------------------------------------------------------------ */

static Boolean CompMatch(int Col, const struct sLineComp *pComp)
{
  return (pComp
       && (pComp->StartCol >= 0)
       && (Col >= pComp->StartCol)
       && (Col < pComp->StartCol + (int)pComp->Len));
}

void PrintOneLineMuted(FILE *pFile, const char *pLine,
                       const struct sLineComp *pMuteComponent,
                       const struct sLineComp *pMuteComponent2)
{
  int z, Len = strlen(pLine);
  Boolean Match;

  errno = 0;
  for (z = 0; z < Len; z++)
  {
    Match = CompMatch(z, pMuteComponent) || CompMatch(z, pMuteComponent2);
    fputc(Match ? ' ' : pLine[z], pFile);
  }
  fputc('\n', pFile);
  ChkIO(ErrNum_ListWrError);
}

/*!------------------------------------------------------------------------
 * \fn     PrLineMarker(FILE *pFile, const char *pLine, const char *pPrefix, const char *pTrailer,
                        char Marker, const struct sLineComp *pLineComp)
 * \brief  print a line, optionally with a marking of a component below
 * \param  pFile where to write
 * \param  pLine line to print/underline
 * \param  pPrefix what to print before (under)line
 * \param  pTrailer what to print after (under)line
 * \param  Marker character to use for marking
 * \param  pLineComp position and length of optional marker
 * ------------------------------------------------------------------------ */

void PrLineMarker(FILE *pFile, const char *pLine, const char *pPrefix, const char *pTrailer,
                  char Marker, const struct sLineComp *pLineComp)
{
  const char *pRun;
  int z;

  fputs(pPrefix, pFile);
  for (pRun = pLine; *pRun; pRun++)
    fputc(TabCompressed(*pRun), pFile);
  fprintf(pFile, "%s\n", pTrailer);

  if (pLineComp && (pLineComp->StartCol >= 0) && (pLineComp->Len > 0))
  {
    fputs(pPrefix, pFile);
    if (pLineComp->StartCol > 0)
      fprintf(pFile, "%*s", pLineComp->StartCol, "");
    for (z = 0; z < (int)pLineComp->Len; z++)
      fputc(Marker, pFile);
    fprintf(pFile, "%s\n", pTrailer);
  }
}

/****************************************************************************/
/* einen Symbolnamen auf Gueltigkeit ueberpruefen */

static Byte GetValidSymChar(unsigned Ch)
{
  return (Ch < ValidSymCharLen) ? ValidSymChar[Ch] : 0;
}

static char *ChkNameUpTo(const char *pSym, const char *pUpTo, Byte _Mask)
{
  Byte Mask = _Mask;
  unsigned Ch;
  const char *pPrev;

  if (!*pSym)
    return (char*)pSym;

  while (*pSym && (pSym != pUpTo))
  {
    pPrev = pSym;
    if (ValidSymCharLen > 256)
      Ch = UTF8ToUnicode(&pSym);
    else
      Ch = ((unsigned int)*pSym++) & 0xff;

    if (!(GetValidSymChar(Ch) & Mask))
      return (char*)pPrev;
    Mask = _Mask << 1;
  }
  return (char*)pSym;
}

char *ChkSymbNameUpTo(const char *pSym, const char *pUpTo)
{
  char *pResult = ChkNameUpTo(pSym, pUpTo, VALID_S1);

  /* If NULL as UpTo was given, and all is fine up to end of string,
     also return NULL as result.  So Equation 'Result==UpTo' is fulfilled: */

  if (!pUpTo && !*pResult)
     pResult= NULL;
  return pResult;
}

Boolean ChkSymbName(const char *pSym)
{
  const char *pEnd = ChkSymbNameUpTo(pSym, NULL);
  return *pSym && !pEnd;
}

char *ChkMacSymbNameUpTo(const char *pSym, const char *pUpTo)
{
  char *pResult = ChkNameUpTo(pSym, pUpTo, VALID_M1);

  /* If NULL as UpTo was given, and all is fine up to end of string,
     also return NULL as result.  So Equation 'Result==UpTo' is fulfilled: */

  if (!pUpTo && !*pResult)
     pResult= NULL;
  return pResult;
}

Boolean ChkMacSymbName(const char *pSym)
{
  const char *pEnd = ChkMacSymbNameUpTo(pSym, NULL);
  return *pSym && !pEnd;
}

/*!------------------------------------------------------------------------
 * \fn     visible_strlen(const char *pSym)
 * \brief  retrieve 'visible' length of string, regarding multi-byte
           sequences for UTF-8
 * \param  pSym symbol name
 * \return visible length in characters
 * ------------------------------------------------------------------------ */

unsigned visible_strlen(const char *pSym)
{
  if (ValidSymCharLen > 256)
  {
    unsigned Result = 0;

    while (*pSym)
      Result += as_wcwidth(UTF8ToUnicode(&pSym));
    return Result;
  }
  else
    return strlen(pSym);
}

/****************************************************************************/

LargeWord ProgCounter(void)
{
  return PCs[ActPC];
}

/*--------------------------------------------------------------------------*/
/* aktuellen Programmzaehler mit Phasenverschiebung holen */

LargeWord EProgCounter(void)
{
  return PCs[ActPC] + Phases[ActPC];
}

/*--------------------------------------------------------------------------*/
/* Granularitaet des aktuellen Segments holen */

Word Granularity(void)
{
  return Grans[ActPC];
}

/*--------------------------------------------------------------------------*/
/* Linstingbreite des aktuellen Segments holen */

Word ListGran(void)
{
  return ListGrans[ActPC];
}

/*--------------------------------------------------------------------------*/
/* pruefen, ob alle Symbole einer Formel im korrekten Adressraum lagen */

void ChkSpace(Byte AddrSpace, unsigned AddrSpaceMask)
{
  AddrSpaceMask &= ~(1 << AddrSpace);

  if (AddrSpaceMask) WrError(ErrNum_WrongSegment);
}

/****************************************************************************/
/* eine Chunkliste im Listing ausgeben & Speicher loeschen */

void PrintChunk(ChunkList *NChunk, DissectBitProc Dissect, int ItemsPerLine)
{
  LargeWord NewMin, FMin;
  Boolean Found;
  Word p = 0, z;
  int BufferZ;
  String BufferS;
  int MaxItemLen = 79 / ItemsPerLine;

  NewMin = 0;
  BufferZ = 0;
  *BufferS = '\0';

  do
  {
    /* niedrigsten Start finden, der ueberhalb des letzten Endes liegt */

    Found = False;
    FMin = IntTypeDefs[LargeUIntType].Max;
    for (z = 0; z < NChunk->RealLen; z++)
      if (NChunk->Chunks[z].Start >= NewMin)
        if (FMin > NChunk->Chunks[z].Start)
        {
          Found = True;
          FMin = NChunk->Chunks[z].Start;
          p = z;
        }

    if (Found)
    {
      char Num[30];

      Dissect(Num, sizeof(Num), NChunk->Chunks[p].Start);
      strmaxcat(BufferS, Num, STRINGSIZE);
      if (NChunk->Chunks[p].Length != 1)
      {
        strmaxcat(BufferS, "-", STRINGSIZE);
        Dissect(Num, sizeof(Num), NChunk->Chunks[p].Start + NChunk->Chunks[p].Length - 1);
        strmaxcat(BufferS, Num, STRINGSIZE);
      }
      strmaxcat(BufferS, Blanks(MaxItemLen - strlen(BufferS) % MaxItemLen), STRINGSIZE);
      if (++BufferZ == ItemsPerLine)
      {
        WrLstLine(BufferS);
        *BufferS = '\0';
        BufferZ = 0;
      }
      NewMin = NChunk->Chunks[p].Start + NChunk->Chunks[p].Length;
    }
  }
  while (Found);

  if (BufferZ != 0)
    WrLstLine(BufferS);
}

/*--------------------------------------------------------------------------*/
/* Listen ausgeben */

void PrintUseList(void)
{
  int z, z2, l;
  String s;

  for (z = 1; z < SegCount; z++)
    if (SegChunks[z].Chunks)
    {
      as_snprintf(s, sizeof(s), "  %s%s%s",
                  getmessage(Num_ListSegListHead1), SegNames[z],
                  getmessage(Num_ListSegListHead2));
      WrLstLine(s);
      strcpy(s, "  ");
      l = strlen(SegNames[z]) + strlen(getmessage(Num_ListSegListHead1)) + strlen(getmessage(Num_ListSegListHead2));
      for (z2 = 0; z2 < l; z2++)
        strmaxcat(s, "-", STRINGSIZE);
      WrLstLine(s);
      WrLstLine("");
      PrintChunk(SegChunks + z,
                 (z == SegBData) ? DissectBit : Default_DissectBit,
                 (z == SegBData) ? 3 : 4);
      WrLstLine("");
    }
}

void ClearUseList(void)
{
  int z;

  for (z = 1; z < SegCount; z++)
    ClearChunk(SegChunks + z);
}

/****************************************************************************/
/* Include-Pfadlistenverarbeitung */

static char *GetPath(char *Acc)
{
  char *p;
  static String tmp;

  p = strchr(Acc, DIRSEP);
  if (!p)
  {
    strmaxcpy(tmp, Acc, STRINGSIZE);
    Acc[0] = '\0';
  }
  else
  {
    *p = '\0';
    strmaxcpy(tmp, Acc, STRINGSIZE);
    strmov(Acc, p + 1);
  }
  return tmp;
}

void AddIncludeList(char *NewPath)
{
  String Test;

  strmaxcpy(Test, IncludeList, STRINGSIZE);
  while (*Test != '\0')
    if (!strcmp(GetPath(Test), NewPath))
      return;
  if (*IncludeList != '\0')
    strmaxprep(IncludeList, SDIRSEP, STRINGSIZE);
  strmaxprep(IncludeList, NewPath, STRINGSIZE);
}


void RemoveIncludeList(char *RemPath)
{
  String Save;
  char *Part;

  strmaxcpy(Save, IncludeList, STRINGSIZE);
  IncludeList[0] = '\0';
  while (Save[0] != '\0')
  {
    Part = GetPath(Save);
    if (strcmp(Part, RemPath))
    {
      if (IncludeList[0] != '\0')
        strmaxcat(IncludeList, SDIRSEP, STRINGSIZE);
      strmaxcat(IncludeList, Part, STRINGSIZE);
    }
  }
}

/****************************************************************************/
/* Listen mit Ausgabedateien */

void ClearOutList(void)
{
  ClearStringList(&OutList);
}

void AddToOutList(const char *NewName)
{
  AddStringListLast(&OutList, NewName);
}

void RemoveFromOutList(const char *OldName)
{
  RemoveStringList(&OutList, OldName);
}

char *MoveFromOutListFirst(void)
{
  return MoveAndCutStringListFirst(&OutList);
}

void ClearShareOutList(void)
{
  ClearStringList(&ShareOutList);
}

void AddToShareOutList(const char *NewName)
{
  AddStringListLast(&ShareOutList, NewName);
}

void RemoveFromShareOutList(const char *OldName)
{
  RemoveStringList(&ShareOutList, OldName);
}

char *MoveFromShareOutListFirst(void)
{
  return MoveAndCutStringListFirst(&ShareOutList);
}

void ClearListOutList(void)
{
  ClearStringList(&ListOutList);
}

void AddToListOutList(const char *NewName)
{
  AddStringListLast(&ListOutList, NewName);
}

void RemoveFromListOutList(const char *OldName)
{
  RemoveStringList(&ListOutList, OldName);
}

char *MoveFromListOutListFirst(void)
{
  return MoveAndCutStringListFirst(&ListOutList);
}

/****************************************************************************/
/* Tokenverarbeitung */

static Boolean CompressLine_NErl(char ch)
{
  return (((ch >= 'A') && (ch <= 'Z'))
       || ((ch >= 'a') && (ch <= 'z'))
       || ((ch >= '0') && (ch <= '9')));
}

typedef int (*tCompareFnc)(const char *s1, const char *s2, size_t n);

int ReplaceLine(as_dynstr_t *p_str, const char *pSearch, const char *pReplace, Boolean CaseSensitive)
{
  int SearchLen = strlen(pSearch), ReplaceLen = strlen(pReplace), StrLen = strlen(p_str->p_str), DeltaLen = ReplaceLen - SearchLen;
  int NumReplace = 0, Pos, End, CmpRes, Avail, nCopy, nMove;
  tCompareFnc Compare = CaseSensitive ? strncmp : as_strncasecmp;

  Pos = 0;
  while (Pos <= StrLen - SearchLen)
  {
    End = Pos + SearchLen;
    CmpRes = Compare(&p_str->p_str[Pos], pSearch, SearchLen);
    if ((!CmpRes)
     && ((Pos == 0) || (!CompressLine_NErl(p_str->p_str[Pos - 1])))
     && ((End >= StrLen) || (!CompressLine_NErl(p_str->p_str[End]))))
    {
      if (StrLen + DeltaLen + 1 > (int)p_str->capacity)
        as_dynstr_realloc(p_str, as_dynstr_roundup_len(p_str->capacity + DeltaLen));
      Avail = p_str->capacity - 1 - Pos;
      nCopy = ReplaceLen; if (nCopy > Avail) nCopy = Avail;
      Avail -= nCopy;
      nMove = StrLen - (Pos + SearchLen); if (nMove > Avail) nMove = Avail;
      memmove(&p_str->p_str[Pos + nCopy], &p_str->p_str[Pos + SearchLen], nMove);
      memcpy(&p_str->p_str[Pos], pReplace, nCopy);
      p_str->p_str[Pos + nCopy + nMove] = '\0';
      Pos += nCopy;
      StrLen += DeltaLen;
      NumReplace++;
    }
    else
      Pos++;
  }
  return NumReplace;
}

static void SetToken(char *Token, unsigned TokenNum)
{
  Token[0] = (TokenNum >> 4) + 1;
  Token[1] = (TokenNum & 15) + 1;
  Token[2] = 0;
}

/*!------------------------------------------------------------------------
 * \fn     CompressLine(const char *TokNam, unsigned TokenNum, as_dynstr_t *p_str, Boolean ThisCaseSensitive)
 * \brief  compress tokens in line
 * \param  TokNam name to compress into token
 * \param  TokenNum token #
 * \param  p_str string to work on
 * \param  ThisCaseSensitive operate case sensitive?
 * ------------------------------------------------------------------------ */

int CompressLine(const char *TokNam, unsigned TokenNum, as_dynstr_t *p_str, Boolean ThisCaseSensitive)
{
  char Token[3];
  SetToken(Token, TokenNum);
  return ReplaceLine(p_str, TokNam, Token, ThisCaseSensitive);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandLine(const char *TokNam, unsigned TokenNum, as_dynstr_t *p_str)
 * \brief  expand tokens in line
 * \param  TokNam name to expand token to
 * \param  TokenNum token #
 * \param  p_str string to work on
 * ------------------------------------------------------------------------ */

void ExpandLine(const char *TokNam, unsigned TokenNum, as_dynstr_t *p_str)
{
  char Token[3];
  SetToken(Token, TokenNum);
  (void)ReplaceLine(p_str, Token, TokNam, True);
}

void KillCtrl(char *Line)
{
  char *z;

  if (*(z = Line) == '\0')
    return;
  do
  {
    if (*z == '\0');
    else if (*z == Char_HT)
    {
      strmov(z, z + 1);
      strprep(z, Blanks(8 - ((z - Line) % 8)));
    }
    else if ((*z & 0xe0) == 0)
      *z = ' ';
    z++;
  }
  while (*z != '\0');
}

/****************************************************************************/
/* Buchhaltung */

void BookKeeping(void)
{
  if (MakeUseList)
    if (AddChunk(SegChunks + ActPC, ProgCounter(), CodeLen, ActPC == SegCode))
      WrError(ErrNum_Overlap);
  if (DebugMode != DebugNone)
  {
    AddSectionUsage(ProgCounter(), CodeLen);
    AddLineInfo(InMacroFlag, CurrLine, CurrFileName, ActPC, PCs[ActPC], CodeLen);
  }
}

/****************************************************************************/
/* Differenz zwischen zwei Zeiten mit Tagesueberlauf berechnen */

long DTime(long t1, long t2)
{
  LongInt d;

  d = t2 - t1;
  if (d < 0) d += (24*360000);
  return (d > 0) ? d : -d;
}

/*--------------------------------------------------------------------------*/
/* Init/Deinit passes */

typedef struct sProcStore
{
  struct sProcStore *pNext;
  SimpProc Proc;
} tProcStore;

static tProcStore *pInitPassProcStore = NULL,
                  *pClearUpProcStore = NULL;

void InitPass(void)
{
  tProcStore *pStore;

  for (pStore = pInitPassProcStore; pStore; pStore = pStore->pNext)
    pStore->Proc();
}

void ClearUp(void)
{
  tProcStore *pStore;

  for (pStore = pClearUpProcStore; pStore; pStore = pStore->pNext)
    pStore->Proc();
}

void AddInitPassProc(SimpProc NewProc)
{
  tProcStore *pNewStore = (tProcStore*)calloc(1, sizeof(*pNewStore));

  pNewStore->pNext = pInitPassProcStore;
  pNewStore->Proc = NewProc;
  pInitPassProcStore = pNewStore;
}

void AddClearUpProc(SimpProc NewProc)
{
  tProcStore *pNewStore = (tProcStore*)calloc(1, sizeof(*pNewStore));

  pNewStore->pNext = pClearUpProcStore;
  pNewStore->Proc = NewProc;
  pClearUpProcStore = pNewStore;
}

/*--------------------------------------------------------------------------*/
/* Zeit holen */

#ifdef __MSDOS__

#include <dos.h>

long GTime(void)
{
  struct time tbuf;
  long result;

  gettime(&tbuf);
  result = tbuf.ti_hour;
  result = (result * 60) + tbuf.ti_min;
  result = (result * 60) + tbuf.ti_sec;
  result = (result * 100) + tbuf.ti_hund;
  return result;
}

#elif __IBMC__

#include <time.h>
#define INCL_DOSDATETIME
#include <os2.h>

long GTime(void)
{
  DATETIME dt;
  struct tm ts;
  DosGetDateTime(&dt);
  memset(&ts, 0, sizeof(ts));
  ts.tm_year = dt.year - 1900;
  ts.tm_mon  = dt.month - 1;
  ts.tm_mday = dt.day;
  ts.tm_hour = dt.hours;
  ts.tm_min  = dt.minutes;
  ts.tm_sec  = dt.seconds;
  return (mktime(&ts) * 100) + (dt.hundredths);
}

#elif __MINGW32__

/* distribution by Gunnar Wallmann */

#include <sys/time.h>
#include "math64.h"

/*time from 1 Jan 1601 to 1 Jan 1970 in 100ns units */

typedef struct _FILETIME
{
  unsigned long dwLowDateTime;
  unsigned long dwHighDateTime;
} FILETIME;

void __stdcall GetSystemTimeAsFileTime(FILETIME*);

long GTime(void)
{
  union
  {
#ifndef NOLONGLONG
    long long ns100; /*time since 1 Jan 1601 in 100ns units */
#endif
    FILETIME ft;
  } _now;

  GetSystemTimeAsFileTime(&(_now.ft));
#ifdef NOLONGLONG
  {
    static const t64 offs = { 0xd53e8000, 0x019db1de },
                     div = { 100000, 0 };
    t64 acc;

    acc.low = _now.ft.dwLowDateTime;
    acc.high = _now.ft.dwHighDateTime;
    sub64(&acc, &acc, &offs);
    div64(&acc, &acc, &div);
    return acc.low;
  }
#else
# define _W32_FT_OFFSET (116444736000000000LL)
  return (_now.ns100 - _W32_FT_OFFSET) / 100000LL;
#endif
}

#else

#include <sys/time.h>

long GTime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (tv.tv_sec*100) + (tv.tv_usec/10000);
}

#endif

/*-------------------------------------------------------------------------*/
/* Stackfehler abfangen - bis auf DOS nur Dummies */

#ifdef __TURBOC__

#ifdef __DPMI16__
#else
unsigned _stklen = STKSIZE;
unsigned _ovrbuffer = 64*48;
#endif
#include <malloc.h>

void ChkStack(void)
{
  LongWord avail = stackavail();
  if (avail < MinStack)
    WrError(ErrNum_StackOvfl);
  if (avail < LowStack)
    LowStack = avail;
}

void ResetStack(void)
{
  LowStack = stackavail();
}

LongWord StackRes(void)
{
  return LowStack - MinStack;
}
#endif /* __TURBOC__ */

#ifdef CKMALLOC
#undef malloc
#undef realloc

void *ckmalloc(size_t s)
{
  void *tmp;

#ifdef __TURBOC__
  if (coreleft() < HEAPRESERVE + s)
    WrError(ErrNum_HeapOvfl);
#endif

  tmp = malloc(s);
  if (!tmp && (s > 0))
    WrError(ErrNum_HeapOvfl);
  return tmp;
}

void *ckrealloc(void *p, size_t s)
{
  void *tmp;

#ifdef __TURBOC__
  if (coreleft() < HEAPRESERVE + s)
    WrError(ErrNum_HeapOvfl);
#endif

  tmp = realloc(p, s);
  if (!tmp)
    WrError(ErrNum_HeapOvfl);
  return tmp;
}
#endif

static void SetValidSymChar(unsigned Ch, Byte Value)
{
  ValidSymChar[Ch] = Value;
}

static void SetValidSymChars(unsigned Start, unsigned Stop, Byte Value)
{
  for (; Start <= Stop; Start++)
    SetValidSymChar(Start, Value);
}

void asmsub_init(void)
{
#ifdef __TURBOC__
#ifdef __MSDOS__
#ifdef __DPMI16__
  char *MemFlag, *p;
  String MemVal, TempName;
  unsigned long FileLen;
#else
  char *envval;
  int ovrerg;
#endif
#endif
#endif

  InitStringList(&CopyrightList);
  InitStringList(&OutList);
  InitStringList(&ShareOutList);
  InitStringList(&ListOutList);

#ifdef __TURBOC__
#ifdef __MSDOS__
#ifdef __DPMI16__
  /* Fuer DPMI evtl. Swapfile anlegen */

  MemFlag = getenv("ASXSWAP");
  if (MemFlag)
  {
    strmaxcpy(MemVal, MemFlag, STRINGSIZE);
    p = strchr(MemVal, ',');
    if (!p)
      strcpy(TempName, "ASX.TMP");
    else
    {
      *p = NULL;
      strcpy(TempName, MemVal);
      strmov(MemVal, p + 1);
    };
    KillBlanks(TempName);
    KillBlanks(MemVal);
    FileLen = strtol(MemFlag, &p, 0);
    if (*p != '\0')
    {
      fputs(getmessage(Num_ErrMsgInvSwapSize), stderr);
      exit(4);
    }
    if (MEMinitSwapFile(TempName, FileLen << 20) != RTM_OK)
    {
      fputs(getmessage(Num_ErrMsgSwapTooBig), stderr);
      exit(4);
    }
  }
#else
  /* Bei DOS Auslagerung Overlays in XMS/EMS versuchen */

  envval = getenv("USEXMS");
  if ((envval) && (as_toupper(*envval) == 'N'))
    ovrerg = -1;
  else
    ovrerg = _OvrInitExt(0, 0);
  if (ovrerg != 0)
  {
    envval = getenv("USEEMS");
    if ((!envval) || (as_toupper(*envval) != 'N'))
      _OvrInitEms(0, 0, 0);
  }
#endif
#endif
#endif

#ifdef __TURBOC__
  StartStack = stackavail();
  LowStack = stackavail();
  MinStack = StartStack - STKSIZE + 0x800;
#else
  StartStack = LowStack = MinStack = 0;
#endif

  /* initialize array of valid characters */

  ValidSymCharLen = (NLS_GetCodepage() == eCodepageUTF8) ? 1280 : 256;
  ValidSymChar = (Byte*) calloc(ValidSymCharLen, sizeof(Byte));

  /* The basic ASCII stuff: letters, dot and underbar are allowed
     anwhere, numbers not at beginning: */

  SetValidSymChars('a', 'z', VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
  SetValidSymChars('A', 'Z', VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
  SetValidSymChars('0', '9',            VALID_SN |            VALID_MN);
  SetValidSymChar ('.'     , VALID_S1 | VALID_SN                      );
  SetValidSymChar ('_'     , VALID_S1 | VALID_SN                      );

  /* Extensions, depending on character set: */

  switch (NLS_GetCodepage())
  {
    case eCodepage1251:
      SetValidSymChar (0xa3      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xb3      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xa8      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xb8      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xaa      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xba      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xaf      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xbf      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xbd      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xbe      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      goto iso8859_1;
    case eCodepage1252:
      SetValidSymChar (0x8a      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0x9a      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0x8c      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0x9c      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0x8e      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0x9e      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0x9f      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      goto iso8859_1;
    case eCodepage850:
      SetValidSymChars(0xb5, 0xb7, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0xc6, 0xc7, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0xd0, 0xd9, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xde      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0xe0, 0xed, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      /* fall-through */
    case eCodepage437:
      SetValidSymChars(128, 165, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (225     , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      break;
    case eCodepage866:
      SetValidSymChars(0x80, 0xaf, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0xe0, 0xf7, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      break;
    case eCodepageISO8859_15:
      SetValidSymChar (0xa6      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xa8      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xb4      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xb8      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xbc      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xbd      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xbe      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      /* fall-through */
    case eCodepageISO8859_1:
    iso8859_1:
      SetValidSymChar (0xa1      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xa2      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0xc0, 0xff, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      break;
    case eCodepageKOI8_R:
      SetValidSymChar (0xa3      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (0xb3      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0xc0, 0xff, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      break;
    case eCodepageUTF8:
    {
      const tNLSCharacterTab *pTab = GetCharacterTab(eCodepageUTF8);
      tNLSCharacter ch;
      unsigned Unicode;
      const char *pCh;

      for (ch = (tNLSCharacter)0; ch < eCH_cnt; ch++)
      {
        if ((ch == eCH_e2) || (ch == eCH_mu) || (ch == eCH_iquest) || (ch == eCH_iexcl))
          continue;
        pCh = &((*pTab)[ch][0]);
        Unicode = UTF8ToUnicode(&pCh);
        if (Unicode < ValidSymCharLen)
          SetValidSymChar (Unicode, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      }

      /* Greek */

      SetValidSymChar ( 895      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar ( 902      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (1011      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (1016      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (1018      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChar (1019      , VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars( 904,  974, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars( 984, 1007, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);

      /* Cyrillic */

      SetValidSymChars(0x400, 0x481, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
      SetValidSymChars(0x48a, 0x4ff, VALID_S1 | VALID_SN | VALID_M1 | VALID_MN);
    }
    default:
      break;
  }

#if 0
  for (z = 0; z < ValidSymCharLen; z++)
  {
    if (!(z & 15))
      fprintf(stderr, "%02x:", z);
    fprintf(stderr, " %x", ValidSymChar[z]);
    if ((z & 15) == 15)
      fprintf(stderr, "\n");
  }
#endif

  version_init();
}
