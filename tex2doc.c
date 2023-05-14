/* tex2doc.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Konverter TeX-->ASCII-DOC                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "asmitree.h"
#include "chardefs.h"
#include <ctype.h>
#include <string.h>
#include "strutil.h"

#include "findhyphen.h"
#ifndef __MSDOS__
#include "ushyph.h"
#include "grhyph.h"
#endif

#include "chardefs.h"
#include "texutil.h"
#include "texrefs.h"
#include "textoc.h"
#include "texfonts.h"
#include "nls.h"

/*--------------------------------------------------------------------------*/

#define TOKLEN 250

static char *TableName,
            *BiblioName,
            *ContentsName,
#define ErrorEntryCnt 3
            *ErrorEntryNames[ErrorEntryCnt];

typedef enum
{
  EnvNone, EnvDocument, EnvItemize, EnvEnumerate, EnvDescription, EnvTable,
  EnvTabular, EnvRaggedLeft, EnvRaggedRight, EnvCenter, EnvVerbatim,
  EnvQuote, EnvTabbing, EnvBiblio, EnvMarginPar, EnvCaption, EnvHeading, EnvCount
} EnvType;

typedef enum
{
  AlignNone, AlignCenter, AlignLeft, AlignRight
} TAlignment;

typedef struct sEnvSave
{
  struct sEnvSave *Next;
  EnvType SaveEnv;
  int ListDepth, ActLeftMargin, LeftMargin, RightMargin;
  int EnumCounter, FontNest;
  TAlignment Alignment;
} TEnvSave, *PEnvSave;

typedef enum
{
  ColLeft, ColRight, ColCenter, ColBar
} TColumn;

#define MAXCOLS 30
#define MAXROWS 500
typedef char *TableLine[MAXCOLS];
typedef struct
{
  int ColumnCount, TColumnCount;
  TColumn ColTypes[MAXCOLS];
  int ColLens[MAXCOLS];
  int LineCnt;
  TableLine Lines[MAXROWS];
  Boolean LineFlags[MAXROWS];
  Boolean MultiFlags[MAXROWS];
} TTable;

static char *EnvNames[EnvCount] =
{
  "___NONE___", "document", "itemize", "enumerate", "description", "table", "tabular",
  "raggedleft", "raggedright", "center", "verbatim", "quote", "tabbing",
  "thebibliography", "___MARGINPAR___", "___CAPTION___", "___HEADING___"
};

static int IncludeNest;
static FILE *infiles[50], *outfile;
static char TocName[200];
static char SrcDir[TOKLEN + 1], asname[TOKLEN];

#define CHAPMAX 6
static int Chapters[CHAPMAX];
static int TableNum, ErrState, FracState, BibIndent, BibCounter;
#define TABMAX 100
static int TabStops[TABMAX], TabStopCnt, CurrTabStop;
static Boolean InAppendix, InMathMode;
static TTable *pThisTable;
static int CurrRow, CurrCol;
static Boolean GermanMode;

static EnvType CurrEnv;
static int CurrPass;
static int CurrListDepth;
static int EnumCounter;
static int ActLeftMargin, LeftMargin, RightMargin;
static TAlignment Alignment;
static PEnvSave EnvStack;

static PInstTable TeXTable;

static tCodepage Codepage;
static const tNLSCharacterTab *pCharacterTab;
static Boolean enable_hyphenation;

/*--------------------------------------------------------------------------*/

void ChkStack(void)
{
}

static void SetSrcDir(const char *pSrcFile)
{
  const char *pSep;

  pSep = strchr(pSrcFile, PATHSEP);
  if (!pSep)
    pSep = strchr(pSrcFile, '/');

  if (!pSep)
    *SrcDir = '\0';
  else
  {
    size_t l = pSep + 1 - pSrcFile;

    if (l >= sizeof(SrcDir))
    {
      fprintf(stderr, "%s: path too long\n", pSrcFile);
      exit(3);
    }
    memcpy(SrcDir, pSrcFile, l);
    SrcDir[l] = '\0';
  }
}

static void error(char *Msg)
{
  int z;

  fprintf(stderr, "%s:%d.%d: %s\n", pInFileName, CurrLine, CurrColumn, Msg);
  for (z = 0; z < IncludeNest; fclose(infiles[z++]));
  fclose(outfile);
  exit(2);
}

static void SetLang(Boolean IsGerman)
{
  char **pp;

  if (GermanMode == IsGerman)
    return;

  DestroyTree();
  GermanMode = IsGerman;
  if (GermanMode)
  {
    TableName = "Tabelle";
    BiblioName = "Literaturverzeichnis";
    ContentsName = "Inhalt";
    ErrorEntryNames[0] = "Typ";
    ErrorEntryNames[1] = "Ursache";
    ErrorEntryNames[2] = "Argument";
#ifndef __MSDOS__
    BuildTree(GRHyphens);
#endif
  }
  else
  {
    TableName = "Table";
    BiblioName = "Bibliography";
    ContentsName = "Contents";
    ErrorEntryNames[0] = "Type";
    ErrorEntryNames[1] = "Reason";
    ErrorEntryNames[2] = "Argument";
#ifndef __MSDOS__
    BuildTree(USHyphens);
    for (pp = USExceptions; *pp != NULL; pp++)
      AddException(*pp);
#endif
  }
}

/*------------------------------------------------------------------------------*/

static void GetNext(char *Src, char *Dest)
{
  char *c = strchr(Src, ' ');

  if (!c)
  {
    strcpy(Dest, Src);
    *Src = '\0';
  }
  else
  {
    *c = '\0';
    strcpy(Dest, Src);
    for (c++; *c == ' '; c++);
    strmov(Src, c);
  }
}

static void ReadAuxFile(char *Name)
{
  FILE *file = fopen(Name, "r");
  char Line[300], Cmd[300], Nam[300], Val[300];

  if (!file)
    return;

  while (!feof(file))
  {
    if (!fgets(Line, 299, file))
      break;
    if ((*Line) && (Line[strlen(Line) - 1] == '\n'))
      Line[strlen(Line) - 1] = '\0';
    GetNext(Line, Cmd);
    if (!strcmp(Cmd, "Label"))
    {
      GetNext(Line, Nam);
      GetNext(Line, Val);
      AddLabel(Nam, Val);
    }
    else if (!strcmp(Cmd, "Citation"))
    {
      GetNext(Line, Nam);
      GetNext(Line, Val);
      AddCite(Nam, Val);
    }
  }

  fclose(file);
}

/*--------------------------------------------------------------------------*/

static Boolean issep(char inp)
{
  return ((inp == ' ') || (inp == '\t') || (inp == '\n'));
}

static Boolean isalphanum(char inp)
{
  return ((inp >= 'A') && (inp <= 'Z'))
      || ((inp >= 'a') && (inp <= 'z'))
      || ((inp >= '0') && (inp <= '9'))
      || (inp == '.');
}

static char LastChar = '\0';
static char SaveSep = '\0', SepString[TOKLEN] = "";
static Boolean DidEOF;
static char BufferLine[TOKLEN] = "", *BufferPtr = BufferLine;
typedef struct
{
  char Token[TOKLEN], Sep[TOKLEN];
} PushedToken;
static int PushedTokenCnt = 0;
static PushedToken PushedTokens[16];

static int GetChar(void)
{
  Boolean Comment;
  static Boolean DidPar = False;
  char *Result;

  if (*BufferPtr == '\0')
  {
    do
    {
      if (IncludeNest <= 0)
        return EOF;
      do
      {
        Result = fgets(BufferLine, TOKLEN, infiles[IncludeNest - 1]);
        if (Result)
          break;
        fclose(infiles[--IncludeNest]);
        if (IncludeNest <= 0)
          return EOF;
      }
      while (True);
      CurrLine++;
      BufferPtr = BufferLine;
      Comment = (strlen(BufferLine) >= 2) && (!strncmp(BufferLine, "%%", 2));
      if ((*BufferLine == '\0') || (*BufferLine == '\n'))
      {
        if ((CurrEnv == EnvDocument) && (!DidPar))
        {
          strcpy(BufferLine, "\\par\n");
          DidPar = True;
          Comment = False;
        }
      }
      else if (!Comment)
        DidPar = False;
    }
    while (Comment);
  }
  return *(BufferPtr++);
}

static Boolean ReadToken(char *Dest)
{
  int ch, z;
  Boolean Good;
  char *run;

  if (PushedTokenCnt > 0)
  {
    strcpy(Dest, PushedTokens[0].Token);
    strcpy(SepString, PushedTokens[0].Sep);
    for (z = 0; z < PushedTokenCnt - 1; z++)
      PushedTokens[z] = PushedTokens[z + 1];
    PushedTokenCnt--;
    return True;
  }

  if (DidEOF)
    return FALSE;

  CurrColumn = BufferPtr - BufferLine + 1;

  /* falls kein Zeichen gespeichert, fuehrende Blanks ueberspringen */

  *Dest = '\0';
  *SepString = SaveSep;
  run = SepString + ((SaveSep == '\0') ? 0 : 1);
  if (LastChar == '\0')
  {
    do
    {
      ch = GetChar();
      if (ch == '\r')
        ch = GetChar();
      if (issep(ch))
        *(run++) = ' ';
    }
    while ((issep(ch)) && (ch != EOF));
    *run = '\0';
    if (ch == EOF)
    {
      DidEOF = TRUE;
      return FALSE;
    }
  }
  else
  {
    ch = LastChar;
    LastChar = '\0';
  }

  /* jetzt Zeichen kopieren, bis Leerzeichen */

  run = Dest;
  SaveSep = '\0';
  if (isalphanum(*(run++) = ch))
  {
    do
    {
      ch = GetChar();
      Good = (!issep(ch)) && (isalphanum(ch)) && (ch != EOF);
      if (Good)
        *(run++) = ch;
    }
    while (Good);

    /* Dateiende ? */

    if (ch == EOF)
      DidEOF = TRUE;

    /* Zeichen speichern ? */

    else if ((!issep(ch)) && (!isalphanum(ch)))
      LastChar = ch;

    /* Separator speichern ? */

    else if (issep(ch))
      SaveSep = ' ';
  }

  /* Ende */

  *run = '\0';
  return True;
}

static void BackToken(char *Token)
{
  if (PushedTokenCnt >= 16)
    return;
  strcpy(PushedTokens[PushedTokenCnt].Token, Token);
  strcpy(PushedTokens[PushedTokenCnt].Sep, SepString);
  PushedTokenCnt++;
}

/*--------------------------------------------------------------------------*/

static void assert_token(char *ref)
{
  char token[TOKLEN];

  ReadToken(token);
  if (strcmp(ref, token))
  {
    as_snprintf(token, sizeof(token), "\"%s\" expected", ref);
    error(token);
  }
}

static void collect_token(char *dest, char *term)
{
  char Comp[TOKLEN];
  Boolean first = TRUE, done;

  *dest = '\0';
  do
  {
    ReadToken(Comp);
    done = !strcmp(Comp, term);
    if (!done)
    {
      if (!first)
        strcat(dest, SepString);
      strcat(dest, Comp);
    }
    first = False;
  }
  while (!done);
}

/*--------------------------------------------------------------------------*/

static const char CHR_ae[3] = HYPHEN_CHR_ae,
                  CHR_oe[3] = HYPHEN_CHR_oe,
                  CHR_ue[3] = HYPHEN_CHR_ue,
                  CHR_AE[3] = HYPHEN_CHR_AE,
                  CHR_OE[3] = HYPHEN_CHR_OE,
                  CHR_UE[3] = HYPHEN_CHR_UE,
                  CHR_sz[3] = HYPHEN_CHR_sz;

static int visible_clen(char ch)
{
  if (Codepage != eCodepageASCII)
    return 1;
  else if (ch == *CHR_ae)
    return CharTab_GetLength(pCharacterTab, eCH_ae);
  else if (ch == *CHR_oe)
    return CharTab_GetLength(pCharacterTab, eCH_oe);
  else if (ch == *CHR_ue)
    return CharTab_GetLength(pCharacterTab, eCH_ue);
  else if (ch == *CHR_AE)
    return CharTab_GetLength(pCharacterTab, eCH_Ae);
  else if (ch == *CHR_OE)
    return CharTab_GetLength(pCharacterTab, eCH_Oe);
  else if (ch == *CHR_UE)
    return CharTab_GetLength(pCharacterTab, eCH_Ue);
  else if (ch == *CHR_sz)
    return CharTab_GetLength(pCharacterTab, eCH_sz);
  else
    return 1;
}

static int visible_strlen(const char *pStr)
{
  int res = 0;

  while (*pStr)
    res += visible_clen(*pStr++);
  return res;
}

static int visible_strnlen(const char *pStr, int MaxLen)
{
  int res = 0;

  while (*pStr && MaxLen)
  {
    res += visible_clen(*pStr++);
    MaxLen--;
  }
  return res;
}

static void outc(char ch)
{
  char Buf[3];

  if (ch == *CHR_ae)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_ae, Buf), outfile);
  else if (ch == *CHR_oe)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_oe, Buf), outfile);
  else if (ch == *CHR_ue)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_ue, Buf), outfile);
  else if (ch == *CHR_AE)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_Ae, Buf), outfile);
  else if (ch == *CHR_OE)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_Oe, Buf), outfile);
  else if (ch == *CHR_UE)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_Ue, Buf), outfile);
  else if (ch == *CHR_sz)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_sz, Buf), outfile);
  else
    fputc(ch, outfile);
}

static void outs(const char *pStr)
{
  while (*pStr)
    outc(*pStr++);
}

static char OutLineBuffer[TOKLEN] = "", SideMargin[TOKLEN];

static void PutLine(Boolean DoBlock)
{
  int ll = RightMargin - LeftMargin + 1;
  int l, n, ptrcnt, diff, div, mod, divmod;
  char *chz, *ptrs[50];
  Boolean SkipFirst, IsFirst;

  outs(Blanks(LeftMargin - 1));
  if ((Alignment != AlignNone) || (!DoBlock))
  {
    l = visible_strlen(OutLineBuffer);
    diff = ll - l;
    switch (Alignment)
    {
      case AlignRight:
        outs(Blanks(diff));
        l = ll;
        break;
      case AlignCenter:
        outs(Blanks(diff >> 1));
        l += diff >> 1;
        break;
      default:
        break;
    }
    outs(OutLineBuffer);
  }
  else
  {
    SkipFirst = ((CurrEnv == EnvItemize) || (CurrEnv == EnvEnumerate) || (CurrEnv == EnvDescription) || (CurrEnv == EnvBiblio));
    if (LeftMargin == ActLeftMargin)
      SkipFirst = False;
    l = ptrcnt = 0;
    IsFirst = SkipFirst;
    for (chz = OutLineBuffer; *chz != '\0'; chz++)
    {
      if ((chz > OutLineBuffer) && (*(chz - 1) != ' ') && (*chz == ' '))
      {
        if (!IsFirst)
          ptrs[ptrcnt++] = chz;
        IsFirst = False;
      }
      l += visible_clen(*chz);
    }
    (void)ptrs;
    diff = ll + 1 - l;
    div = (ptrcnt > 0) ? diff / ptrcnt : 0;
    mod = diff - (ptrcnt*div);
    divmod = (mod > 0) ? ptrcnt / mod : ptrcnt + 1;
    IsFirst = SkipFirst;
    ptrcnt = 0;
    for (chz = OutLineBuffer; *chz != '\0'; chz++)
    {
      outc(*chz);
      if ((chz > OutLineBuffer) && (*(chz - 1) != ' ') && (*chz == ' '))
      {
        if (!IsFirst)
        {
          n = div;
          if ((mod > 0) && (!(ptrcnt % divmod)))
          {
            mod--;
            n++;
          }
          if (n > 0)
            outs(Blanks(n));
          ptrcnt++;
        }
        IsFirst = False;
      }
    }
    l = RightMargin - LeftMargin + 1;
  }
  if (*SideMargin != '\0')
  {
    outs(Blanks(ll + 3 - l));
    outs(SideMargin);
    *SideMargin = '\0';
  }
  outc('\n');
  LeftMargin = ActLeftMargin;
}

static void AddLine(const char *Part, char *Sep)
{
  int mlen = RightMargin - LeftMargin + 1, *hyppos, hypcnt, z, hlen, vlen;
  char *search, save, *lastalpha;

  if (strlen(Sep) > 1)
    Sep[1] = '\0';
  if (*OutLineBuffer != '\0')
    strcat(OutLineBuffer, Sep);
  strcat(OutLineBuffer, Part);
  vlen = visible_strlen(OutLineBuffer);
  if (vlen >= mlen)
  {
    search = OutLineBuffer + mlen;
    while (search >= OutLineBuffer)
    {
      if (*search == ' ')
        break;
      if (search > OutLineBuffer)
      {
        if (*(search - 1) == '-')
          break;
        else if (*(search - 1) == '/')
          break;
        else if (*(search - 1) == ';')
          break;
        else if (*(search - 1) == ';')
          break;
      }
      search--;
    }
    if (search <= OutLineBuffer)
    {
      PutLine(True);
      *OutLineBuffer = '\0';
    }
    else
    {
      if (*search == ' ')
      {
        for (lastalpha = search + 1; *lastalpha != '\0'; lastalpha++)
          if ((as_tolower(*lastalpha) < 'a') || (as_tolower(*lastalpha) > 'z'))
            break;
        if (lastalpha - search > 3)
        {
          save = (*lastalpha);
          *lastalpha = '\0';
          if (enable_hyphenation)
            DoHyphens(search + 1, &hyppos, &hypcnt);
          else
            hypcnt = 0;
          *lastalpha = save;
          hlen = -1;
          for (z = 0; z < hypcnt; z++)
            if (visible_strnlen(OutLineBuffer, search - OutLineBuffer) + hyppos[z] + 1 < mlen)
              hlen = hyppos[z];
          if (hlen > 0)
          {
            memmove(search + hlen + 2, search + hlen + 1, strlen(search + hlen + 1) + 1);
            search[hlen + 1] = '-';
            search += hlen + 2;
          }
          if (hypcnt > 0)
            free(hyppos);
        }
      }
      save = (*search);
      *search = '\0';
      PutLine(True);
      *search = save;
      for (; *search == ' '; search++);
      strcpy(OutLineBuffer, search);
    }
  }
}

static void AddSideMargin(const char *Part, char *Sep)
{
  if (strlen(Sep) > 1)
    Sep[1] = '\0';
  if (*Sep != '\0')
    if ((*SideMargin != '\0') || (!issep(*Sep)))
      strcat(SideMargin, Sep);
  strcat(SideMargin, Part);
}

static void FlushLine(void)
{
  if (*OutLineBuffer != '\0')
  {
    PutLine(False);
    *OutLineBuffer = '\0';
  }
}

static void ResetLine(void)
{
  *OutLineBuffer = '\0';
}

/*--------------------------------------------------------------------------*/

void PrFontDiff(int OldFlags, int NewFlags)
{
  (void)OldFlags;
  (void)NewFlags;
}

void PrFontSize(tFontSize Type, Boolean On)
{
  (void)Type;
  (void)On;
}

static void SaveEnv(EnvType NewEnv)
{
  PEnvSave NewSave;

  NewSave = (PEnvSave) malloc(sizeof(TEnvSave));
  NewSave->Next = EnvStack;
  NewSave->ListDepth = CurrListDepth;
  NewSave->LeftMargin = LeftMargin;
  NewSave->Alignment = Alignment;
  NewSave->ActLeftMargin = ActLeftMargin;
  NewSave->RightMargin = RightMargin;
  NewSave->EnumCounter = EnumCounter;
  NewSave->SaveEnv = CurrEnv;
  NewSave->FontNest = FontNest;
  EnvStack = NewSave;
  CurrEnv = NewEnv;
  FontNest = 0;
}

static void RestoreEnv(void)
{
  PEnvSave OldSave;

  OldSave = EnvStack;
  EnvStack = OldSave->Next;
  CurrListDepth = OldSave->ListDepth;
  LeftMargin = OldSave->LeftMargin;
  ActLeftMargin = OldSave->ActLeftMargin;
  RightMargin = OldSave->RightMargin;
  Alignment = OldSave->Alignment;
  EnumCounter = OldSave->EnumCounter;
  FontNest = OldSave->FontNest;
  CurrEnv = OldSave->SaveEnv;
  free(OldSave);
}

static void InitTableRow(int Index)
{
  int z;

  for (z = 0; z < pThisTable->TColumnCount; pThisTable->Lines[Index][z++] = NULL);
  pThisTable->MultiFlags[Index] = False;
  pThisTable->LineFlags[Index] = False;
}

static void NextTableColumn(void)
{
  if (CurrEnv != EnvTabular)
    error("table separation char not within tabular environment");

  if ((pThisTable->MultiFlags[CurrRow])
   || (CurrCol >= pThisTable->TColumnCount))
    error("too many columns within row");

  CurrCol++;
}

static void AddTableEntry(const char *Part, char *Sep)
{
  char *Ptr = pThisTable->Lines[CurrRow][CurrCol];
  int nlen = Ptr ? strlen(Ptr) : 0;
  Boolean UseSep = (nlen > 0);

  if (strlen(Sep) > 1)
    Sep[1] = '\0';
  if (UseSep)
    nlen += strlen(Sep);
  nlen += strlen(Part);
  if (!Ptr)
  {
    Ptr = (char *) malloc(nlen + 1);
    *Ptr = '\0';
  }
  else
  {
    char *NewPtr = (char *) realloc(Ptr, nlen + 1);
    if (NewPtr)
      Ptr = NewPtr;
  }
  if (UseSep)
    strcat(Ptr, Sep);
  strcat(Ptr, Part);
  pThisTable->Lines[CurrRow][CurrCol] = Ptr;
}

static void DoPrnt(char *Ptr, TColumn Align, int len)
{
  int l = (!Ptr) ? 0 : visible_strlen(Ptr), diff;

  len -= 2;
  diff = len - l;
  outc(' ');
  switch (Align)
  {
    case ColRight:
      outs(Blanks(diff));
      break;
    case ColCenter:
      outs(Blanks((diff + 1) / 2));
      break;
    default:
      break;
  }
  if (Ptr)
  {
    outs(Ptr);
    free(Ptr);
  }
  switch (Align)
  {
    case ColLeft:
      outs(Blanks(diff));
      break;
    case ColCenter:
      outs(Blanks(diff / 2));
      break;
    default:
      break;
  }
  outc(' ');
}

static void DumpTable(void)
{
  int RowCnt, rowz, colz, colptr, ml, l, diff, sumlen, firsttext, indent;

  /* compute widths of individual rows */
  /* get index of first text column */

  RowCnt = (pThisTable->Lines[CurrRow][0]) ? CurrRow + 1 : CurrRow;
  firsttext = -1;
  for (colz = colptr = 0; colz < pThisTable->ColumnCount; colz++)
    if (pThisTable->ColTypes[colz] == ColBar)
      pThisTable->ColLens[colz] = 1;
    else
    {
      ml = 0;
      for (rowz = 0; rowz < RowCnt; rowz++)
        if ((!pThisTable->LineFlags[rowz]) && (!pThisTable->MultiFlags[rowz]))
        {
          l = (!pThisTable->Lines[rowz][colptr]) ? 0 : visible_strlen(pThisTable->Lines[rowz][colptr]);
          if (ml < l)
            ml = l;
        }
      pThisTable->ColLens[colz] = ml + 2;
      colptr++;
      if (firsttext < 0)
        firsttext = colz;
    }

  /* get total width */

  for (colz = sumlen = 0; colz < pThisTable->ColumnCount; sumlen += pThisTable->ColLens[colz++]);
  indent = (RightMargin - LeftMargin + 1 - sumlen) / 2;
  if (indent < 0)
    indent = 0;

  /* search for multicolumns and extend first field if table is too lean */

  ml = 0;
  for (rowz = 0; rowz < RowCnt; rowz++)
    if ((!pThisTable->LineFlags[rowz]) && (pThisTable->MultiFlags[rowz]))
    {
      l = pThisTable->Lines[rowz][0] ? strlen(pThisTable->Lines[rowz][0]) : 0;
      if (ml < l)
        ml = l;
    }
  if (ml + 4 > sumlen)
  {
    diff = ml + 4 - sumlen;
    pThisTable->ColLens[firsttext] += diff;
  }

  /* print rows */

  for (rowz = 0; rowz < RowCnt; rowz++)
  {
    outs(Blanks(LeftMargin - 1 + indent));
    if (pThisTable->MultiFlags[rowz])
    {
      l = sumlen;
      if (pThisTable->ColTypes[0] == ColBar)
      {
        l--;
        outc('|');
      }
      if (pThisTable->ColTypes[pThisTable->ColumnCount - 1] == ColBar)
        l--;
      for (colz = 0; colz < pThisTable->ColumnCount; colz++)
      {
        if (!colz)
          DoPrnt(pThisTable->Lines[rowz][colz], pThisTable->ColTypes[firsttext], l);
        else if (pThisTable->Lines[rowz][colz])
        {
          free(pThisTable->Lines[rowz][colz]);
          pThisTable->Lines[rowz][colz] = NULL;
        }
        pThisTable->Lines[rowz][0] = NULL;
      }
      if (pThisTable->ColTypes[pThisTable->ColumnCount - 1] == ColBar)
        outc('|');
    }
    else
    {
      for (colz = colptr = 0; colz < pThisTable->ColumnCount; colz++)
        if (pThisTable->LineFlags[rowz])
        {
          if (pThisTable->ColTypes[colz] == ColBar)
            outc('+');
          else
            for (l = 0; l < pThisTable->ColLens[colz]; l++)
              outc('-');
        }
        else
          if (pThisTable->ColTypes[colz] == ColBar)
            outc('|');
          else
          {
            DoPrnt(pThisTable->Lines[rowz][colptr], pThisTable->ColTypes[colz], pThisTable->ColLens[colz]);
            pThisTable->Lines[rowz][colptr] = NULL;
            colptr++;
          }
    }
    outc('\n');
  }
}

static void DoAddNormal(const char *Part, char *Sep)
{
  switch (CurrEnv)
  {
    case EnvMarginPar:
      AddSideMargin(Part, Sep);
      break;
    case EnvTabular:
      AddTableEntry(Part, Sep);
      break;
    default:
      AddLine(Part, Sep);
  }
}

static void GetTableName(char *Dest, size_t DestSize)
{
  int ThisTableNum = (CurrEnv == EnvTabular) ? TableNum + 1 : TableNum;

  if (InAppendix)
    as_snprintf(Dest, DestSize, "%c.%d", Chapters[0] + 'A', ThisTableNum);
  else
    as_snprintf(Dest, DestSize, "%d.%d", Chapters[0], ThisTableNum);
}

static void GetSectionName(char *Dest, size_t DestSize)
{
  int z;

  *Dest = '\0';
  for (z = 0; z <= 2; z++)
  {
    if ((z > 0) && (Chapters[z] == 0))
      break;
    if ((InAppendix) && (z == 0))
      as_snprcatf(Dest, DestSize, "%c.", Chapters[z] + 'A');
    else
      as_snprcatf(Dest, DestSize, "%d.", Chapters[z]);
  }
}

/*--------------------------------------------------------------------------*/

static char BackSepString[TOKLEN];

static void TeXFlushLine(Word Index)
{
  UNUSED(Index);

  if (CurrEnv == EnvTabular)
  {
    for (CurrCol++; CurrCol < pThisTable->TColumnCount; pThisTable->Lines[CurrRow][CurrCol++] = as_strdup(""));
    CurrRow++;
    if (CurrRow == MAXROWS)
      error("too many rows in table");
    InitTableRow(CurrRow);
    CurrCol = 0;
  }
  else
  {
    if (*OutLineBuffer == '\0')
      strcpy(OutLineBuffer, " ");
    FlushLine();
  }
  if (CurrEnv == EnvTabbing)
    CurrTabStop = 0;
}

static void TeXKillLine(Word Index)
{
  UNUSED(Index);

  ResetLine();
}

static void TeXDummy(Word Index)
{
  UNUSED(Index);
}

static void TeXDummyNoBrack(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  ReadToken(Token);
}

static void TeXDummyEqual(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("=");
  ReadToken(Token);
}

static void TeXDummyInCurl(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  ReadToken(Token);
  assert_token("}");
}

static void TeXNewCommand(Word Index)
{
  char token[TOKLEN], command[TOKLEN], sum_token[TOKLEN], arg_cnt[TOKLEN];
  int level;
  UNUSED(Index);

  assert_token("{");
  assert_token("\\");
  ReadToken(command);
  assert_token("}");
  ReadToken(token);

  if (!strcmp(token, "["))
  {
    ReadToken(arg_cnt);
    assert_token("]");
    ReadToken(token);
  }
  if (strcmp(token, "{"))
    error("\"{\" expected");

  level = 1;
  *sum_token = '\0';
  do
  {
    ReadToken(token);
    if (!strcmp(token, "{"))
      level++;
    else if (!strcmp(token, "}"))
      level--;
    if (level != 0)
      strmaxcat(sum_token, token, sizeof(sum_token));
  }
  while (level != 0);
  if (!strcmp(command, "asname"))
    strmaxcpy(asname, sum_token, sizeof(asname));
}

static void TeXDef(Word Index)
{
  char Token[TOKLEN];
  int level;
  UNUSED(Index);

  assert_token("\\");
  ReadToken(Token);
  assert_token("{");
  level = 1;
  do
  {
    ReadToken(Token);
    if (!strcmp(Token, "{"))
      level++;
    else if (!strcmp(Token, "}"))
      level--;
  }
  while (level != 0);
}

static void TeXFont(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("\\");
  ReadToken(Token);
  assert_token("=");
  ReadToken(Token);
  ReadToken(Token);
  assert_token("\\");
  ReadToken(Token);
}

static void TeXAppendix(Word Index)
{
  int z;
  UNUSED(Index);

  InAppendix = True;
  *Chapters = -1;
  for (z = 1; z < CHAPMAX; Chapters[z++] = 0);
}

static int LastLevel;

static void TeXNewSection(Word Level)
{
  int z;

  if (Level >= CHAPMAX)
    return;

  FlushLine();
  outc('\n');

  assert_token("{");
  LastLevel = Level;
  SaveEnv(EnvHeading);
  RightMargin = 200;

  Chapters[Level]++;
  for (z = Level + 1; z < CHAPMAX; Chapters[z++] = 0);
  if (Level == 0)
    TableNum = 0;
}

static void EndSectionHeading(void)
{
  int Level = LastLevel, z;
  char Line[TOKLEN], Title[TOKLEN];

  strcpy(Title, OutLineBuffer);
  *OutLineBuffer = '\0';

  *Line = '\0';
  if (Level < 3)
  {
    GetSectionName(Line, sizeof(Line));
    as_snprcatf(Line, sizeof(Line), " ");
    if ((Level == 2) && (((strlen(Line) + strlen(Title))&1) == 0))
      as_snprcatf(Line, sizeof(Line), " ");
  }
  as_snprcatf(Line, sizeof(Line), "%s", Title);

  outs("        ");
  outs(Line);
  outs("\n        ");
  for (z = 0; z < (int)strlen(Line); z++)
    switch(Level)
    {
      case 0:
        outc('=');
        break;
      case 1:
        outc('-');
        break;
      case 2:
        outc(((z&1) == 0) ? '-' : ' ');
        break;
      case 3:
        outc('.');
        break;
    }
  outc('\n');

  if (Level < 3)
  {
    GetSectionName(Line, sizeof(Line));
    as_snprcatf(Line, sizeof(Line), " %s", Title);
    AddToc(Line, 5 + Level);
  }
}

static EnvType GetEnvType(char *Name)
{
  EnvType z;

  if (!strcmp(Name, "longtable"))
    return EnvTabular;
  for (z = EnvNone + 1; z < EnvCount; z++)
    if (!strcmp(Name, EnvNames[z]))
      return z;

  error("unknown environment");
  return EnvNone;
}

static void TeXBeginEnv(Word Index)
{
  char EnvName[TOKLEN], Add[TOKLEN];
  EnvType NEnv;
  Boolean done;
  TColumn NCol;
  int z;
  UNUSED(Index);

  assert_token("{");
  ReadToken(EnvName);
  if ((NEnv = GetEnvType(EnvName)) == EnvTable)
  {
    ReadToken(Add);
    if (!strcmp(Add, "*"))
      assert_token("}");
    else if (strcmp(Add, "}"))
      error("unknown table environment");
  }
  else
    assert_token("}");

  if (NEnv != EnvVerbatim)
    SaveEnv(NEnv);

  switch (NEnv)
  {
    case EnvItemize:
    case EnvEnumerate:
    case EnvDescription:
      FlushLine();
      if (CurrListDepth == 0)
        outc('\n');
      ++CurrListDepth;
      ActLeftMargin = LeftMargin = (CurrListDepth * 4) + 1;
      RightMargin = 70;
      EnumCounter = 0;
      break;
    case EnvBiblio:
      FlushLine(); outc('\n');
      outs("        ");
      outs(BiblioName);
      outs("\n        ");
      for (z = 0; z < (int)strlen(BiblioName); z++)
        outc('=');
      outc('\n');
      assert_token("{");
      ReadToken(Add);
      assert_token("}");
      ActLeftMargin = LeftMargin = 4 + (BibIndent = strlen(Add));
      break;
    case EnvVerbatim:
      FlushLine();
      if ((*BufferLine != '\0') && (*BufferPtr != '\0'))
      {
        outs(BufferPtr);
        *BufferLine = '\0';
        BufferPtr = BufferLine;
      }
      do
      {
        if (!fgets(Add, TOKLEN-1, infiles[IncludeNest - 1]))
          break;
        CurrLine++;
        done = strstr(Add, "\\end{verbatim}") != NULL;
        if (!done)
          outs(Add);
      }
      while (!done);
      outc('\n');
      break;
    case EnvQuote:
      FlushLine();
      outc('\n');
      ActLeftMargin = LeftMargin = 5;
      RightMargin = 70;
      break;
    case EnvTabbing:
      FlushLine();
      outc('\n');
      TabStopCnt = 0;
      CurrTabStop = 0;
      break;
    case EnvTable:
      ReadToken(Add);
      if (strcmp(Add, "["))
        BackToken(Add);
      else
        do
        {
          ReadToken(Add);
        }
        while (strcmp(Add, "]"));
      FlushLine();
      outc('\n');
      ++TableNum;
      break;
    case EnvCenter:
      FlushLine();
      Alignment = AlignCenter;
      break;
    case EnvRaggedRight:
      FlushLine();
      Alignment = AlignLeft;
      break;
    case EnvRaggedLeft:
      FlushLine();
      Alignment = AlignRight;
      break;
    case EnvTabular:
      FlushLine();
      assert_token("{");
      pThisTable->ColumnCount = pThisTable->TColumnCount = 0;
      do
      {
        ReadToken(Add);
        done = !strcmp(Add, "}");
        if (!done)
        {
          if (pThisTable->ColumnCount >= MAXCOLS)
            error("too many columns in table");
          if (!strcmp(Add, "|"))
            NCol = ColBar;
          else if (!strcmp(Add, "l"))
            NCol = ColLeft;
          else if (!strcmp(Add, "r"))
            NCol = ColRight;
          else if (!strcmp(Add, "c"))
            NCol = ColCenter;
          else
          {
            NCol = ColBar;
            error("unknown table column descriptor");
          }
          if ((pThisTable->ColTypes[pThisTable->ColumnCount++] = NCol) != ColBar)
           pThisTable->TColumnCount++;
        }
      }
      while (!done);
      InitTableRow(CurrRow = 0);
      CurrCol = 0;
      break;
    default:
      break;
  }
}

static void TeXEndEnv(Word Index)
{
  char EnvName[TOKLEN], Add[TOKLEN];
  EnvType NEnv;
  UNUSED(Index);

  assert_token("{");
  ReadToken(EnvName);
  if ((NEnv = GetEnvType(EnvName)) == EnvTable)
  {
    ReadToken(Add);
    if (!strcmp(Add, "*"))
      assert_token("}");
    else if (strcmp(Add, "}"))
      error("unknown table environment");
  }
  else
    assert_token("}");

  if (!EnvStack)
    error("end without begin");
  if (CurrEnv != NEnv)
  {
    char Str[100];

    as_snprintf(Str, sizeof(Str), "begin (%s) and end (%s) of environment do not match",
                EnvNames[CurrEnv], EnvNames[NEnv]);
    error(Str);
  }

  switch (CurrEnv)
  {
    case EnvItemize:
    case EnvEnumerate:
    case EnvDescription:
      FlushLine();
      if (CurrListDepth == 1)
        outc('\n');
      break;
    case EnvBiblio:
    case EnvQuote:
    case EnvTabbing:
      FlushLine();
      outc('\n');
      break;
    case EnvCenter:
    case EnvRaggedRight:
    case EnvRaggedLeft:
      FlushLine();
      break;
    case EnvTabular:
      DumpTable();
      break;
    case EnvTable:
      FlushLine();
      outc('\n');
      break;
    default:
      break;
  }

  RestoreEnv();
}

static void TeXItem(Word Index)
{
  char NumString[20], Token[TOKLEN], Acc[TOKLEN];
  UNUSED(Index);

  FlushLine();
  switch(CurrEnv)
  {
    case EnvItemize:
      LeftMargin = ActLeftMargin - 3;
      AddLine(" - ", "");
      break;
    case EnvEnumerate:
      LeftMargin = ActLeftMargin - 4;
      as_snprintf(NumString, sizeof(NumString), "%3d ", ++EnumCounter);
      AddLine(NumString, "");
      break;
    case EnvDescription:
      ReadToken(Token);
      if (strcmp(Token, "[")) BackToken(Token);
      else
      {
        collect_token(Acc, "]");
        LeftMargin = ActLeftMargin - 4;
        as_snprintf(NumString, sizeof(NumString), "%3s ", Acc);
        AddLine(NumString, "");
      }
      break;
    default:
      error("\\item not in a list environment");
  }
}

static void TeXBibItem(Word Index)
{
  char NumString[20], Token[TOKLEN], Name[TOKLEN], Format[10];
  UNUSED(Index);

  if (CurrEnv != EnvBiblio)
    error("\\bibitem not in bibliography environment");

  assert_token("{");
  collect_token(Name, "}");

  FlushLine();
  outc('\n');
  ++BibCounter;

  LeftMargin = ActLeftMargin - BibIndent - 3;
  as_snprintf(Format, sizeof(Format), "[%%%dd] ", BibIndent);
  as_snprintf(NumString, sizeof(NumString), Format, BibCounter);
  AddLine(NumString, "");
  as_snprintf(NumString, sizeof(NumString), "%d", BibCounter);
  AddCite(Name, NumString);
  ReadToken(Token);
  *SepString = '\0';
  BackToken(Token);
}

static void TeXAddDollar(Word Index)
{
  UNUSED(Index);

  DoAddNormal("$", BackSepString);
}

static void TeXAddUnderbar(Word Index)
{
  UNUSED(Index);

  DoAddNormal("_", BackSepString);
}

#if 0
static void TeXAddPot(Word Index)
{
  UNUSED(Index);

  DoAddNormal("^", BackSepString);
}
#endif

static void TeXAddAmpersand(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&", BackSepString);
}

static void TeXAddAt(Word Index)
{
  UNUSED(Index);

  DoAddNormal("@", BackSepString);
}

static void TeXAddImm(Word Index)
{
  UNUSED(Index);

  DoAddNormal("#", BackSepString);
}

static void TeXAddPercent(Word Index)
{
  UNUSED(Index);

  DoAddNormal("%", BackSepString);
}

static void TeXAddSSharp(Word Index)
{
  UNUSED(Index);

  DoAddNormal(HYPHEN_CHR_sz, BackSepString);
}

static void TeXAddIn(Word Index)
{
  UNUSED(Index);

  DoAddNormal("in", BackSepString);
}

static void TeXAddReal(Word Index)
{
  UNUSED(Index);

  DoAddNormal("R", BackSepString);
}

static void TeXAddGreekMu(Word Index)
{
  char Buf[3];

  UNUSED(Index);

  DoAddNormal(CharTab_GetNULTermString(pCharacterTab, eCH_mu, Buf), BackSepString);
}

static void TeXAddGreekPi(Word Index)
{
  UNUSED(Index);

  DoAddNormal("Pi", BackSepString);
}

static void TeXAddLessEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<=", BackSepString);
}

static void TeXAddGreaterEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal(">=", BackSepString);
}

static void TeXAddNotEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("!=", BackSepString);
}

static void TeXAddLAnd(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&", BackSepString);
}

static void TeXAddLOr(Word Index)
{
  UNUSED(Index);

  DoAddNormal("|", BackSepString);
}

static void TeXAddOPlus(Word Index)
{
  UNUSED(Index);

  DoAddNormal("^", BackSepString);
}

static void TeXAddMid(Word Index)
{
  UNUSED(Index);

  DoAddNormal("|", BackSepString);
}

static void TeXASName(Word Index)
{
  char arg[TOKLEN];

  UNUSED(Index);

  assert_token("{");
  collect_token(arg, "}");
  DoAddNormal(asname, BackSepString);
}

static void TeXAddRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("->", BackSepString);
}

static void TeXAddLongRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("-->", BackSepString);
}

static void TeXAddLongLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<--", BackSepString);
}

static void TeXAddLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<-", BackSepString);
}

static void TeXAddLeftRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<->", BackSepString);
}

static void TeXDoFrac(Word Index)
{
  UNUSED(Index);

  assert_token("{");
  *SepString = '\0';
  BackToken("(");
  FracState = 0;
}

static void NextFracState(void)
{
  if (FracState == 0)
  {
    assert_token("{");
    *SepString = '\0';
    BackToken(")");
    BackToken("/");
    BackToken("(");
  }
  else if (FracState == 1)
  {
    *SepString = '\0';
    BackToken(")");
  }
  if ((++FracState) == 2)
    FracState = -1;
}

static void TeXNewFontType(Word Index)
{
  CurrFontType = (tFontType) Index;
}

static void TeXEnvNewFontType(Word Index)
{
  char NToken[TOKLEN];

  SaveFont();
  CurrFontType = (tFontType) Index;
  assert_token("{");
  ReadToken(NToken);
  strcpy(SepString, BackSepString);
  BackToken(NToken);
}

static void TeXNewFontSize(Word Index)
{
  CurrFontSize = (tFontSize) Index;
}

static void TeXEnvNewFontSize(Word Index)
{
  char NToken[TOKLEN];

  SaveFont();
  CurrFontSize = (tFontSize) Index;
  assert_token("{");
  ReadToken(NToken);
  strcpy(SepString, BackSepString);
  BackToken(NToken);
}

static void TeXAddMarginPar(Word Index)
{
  UNUSED(Index);

  assert_token("{");
  SaveEnv(EnvMarginPar);
}

static void TeXAddCaption(Word Index)
{
  char tmp[100];
  int cnt;
  UNUSED(Index);

  assert_token("{");
  if ((CurrEnv != EnvTable) && (CurrEnv != EnvTabular))
    error("caption outside of a table");
  FlushLine();
  outc('\n');
  GetTableName(tmp, sizeof(tmp));
  SaveEnv(EnvCaption);
  AddLine(TableName, "");
  cnt = strlen(TableName);
  strcat(tmp, ": ");
  AddLine(tmp, " ");
  cnt += 1 + strlen(tmp);
  LeftMargin = 1;
  ActLeftMargin = cnt + 1;
  RightMargin = 70;
}

static void TeXEndHead(Word Index)
{
  UNUSED(Index);
}

static void TeXHorLine(Word Index)
{
  UNUSED(Index);

  if (CurrEnv != EnvTabular)
    error("\\hline outside of a table");

  if (pThisTable->Lines[CurrRow][0])
    InitTableRow(++CurrRow);
  pThisTable->LineFlags[CurrRow] = True;
  InitTableRow(++CurrRow);
}

static void TeXMultiColumn(Word Index)
{
  char Token[TOKLEN], *endptr;
  int cnt;
  UNUSED(Index);

  if (CurrEnv != EnvTabular) error("\\multicolumn outside of a table");
  if (CurrCol != 0) error("\\multicolumn must be in first column");

  assert_token("{");
  ReadToken(Token);
  assert_token("}");
  cnt = strtol(Token, &endptr, 10);
  if (*endptr != '\0')
    error("invalid numeric format to \\multicolumn");
  if (cnt != pThisTable->TColumnCount)
    error("\\multicolumn must span entire table");
  assert_token("{");
  do
  {
    ReadToken(Token);
  }
  while (strcmp(Token, "}"));
  pThisTable->MultiFlags[CurrRow] = True;
}

static void TeXIndex(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  do
  {
    ReadToken(Token);
  }
  while (strcmp(Token, "}"));
}

static int GetDim(Double *Factors)
{
  char Acc[TOKLEN];
  static char *UnitNames[] = {"cm", "mm", ""}, **run, *endptr;
  Double Value;

  assert_token("{");
  collect_token(Acc, "}");
  for (run = UnitNames; **run != '\0'; run++)
    if (!strcmp(*run, Acc + strlen(Acc) - strlen(*run)))
      break;
  if (**run == '\0')
    error("unknown unit for dimension");
  Acc[strlen(Acc) - strlen(*run)] = '\0';
  Value = strtod(Acc, &endptr);
  if (*endptr != '\0')
    error("invalid numeric format for dimension");
  return (int)(Value*Factors[run - UnitNames]);
}

static Double HFactors[] = { 4.666666, 0.4666666, 0 };
static Double VFactors[] = { 3.111111, 0.3111111, 0 };

static void TeXHSpace(Word Index)
{
  UNUSED(Index);

  DoAddNormal(Blanks(GetDim(HFactors)), "");
}

static void TeXVSpace(Word Index)
{
  int z, erg;
  UNUSED(Index);

  erg = GetDim(VFactors);
  FlushLine();
  for (z = 0; z < erg; z++)
    outc('\n');
}

static void TeXRule(Word Index)
{
  int h = GetDim(HFactors), v = GetDim(VFactors);
  char Rule[200];
  UNUSED(Index);

  for (v = 0; v < h; Rule[v++] = '-');
  Rule[v] = '\0';
  DoAddNormal(Rule, BackSepString);
}

static void TeXAddTabStop(Word Index)
{
  int z, n, p;
  UNUSED(Index);

  if (CurrEnv != EnvTabbing)
    error("tab marker outside of tabbing environment");
  if (TabStopCnt >= TABMAX)
    error("too many tab stops");

  n = strlen(OutLineBuffer);
  for (p = 0; p < TabStopCnt; p++)
    if (TabStops[p] > n)
      break;
  for (z = TabStopCnt - 1; z >= p; z--)
    TabStops[z + 1] = TabStops[z];
  TabStops[p] = n;
  TabStopCnt++;
}

static void TeXJmpTabStop(Word Index)
{
  int diff;
  UNUSED(Index);

  if (CurrEnv != EnvTabbing)
    error("tab trigger outside of tabbing environment");
  if (CurrTabStop >= TabStopCnt)
    error("not enough tab stops");

  diff = TabStops[CurrTabStop] - strlen(OutLineBuffer);
  if (diff > 0)
    DoAddNormal(Blanks(diff), "");
  CurrTabStop++;
}

static void TeXDoVerb(Word Index)
{
  char Token[TOKLEN], *pos, Marker;
  UNUSED(Index);

  ReadToken(Token);
  if (*SepString != '\0')
    error("invalid control character for \\verb");
  Marker = (*Token);
  strmov(Token, Token + 1);
  strcpy(SepString, BackSepString);
  do
  {
    DoAddNormal(SepString, "");
    pos = strchr(Token, Marker);
    if (pos)
    {
      *pos = '\0';
      DoAddNormal(Token, "");
      *SepString = '\0';
      BackToken(pos + 1);
      break;
    }
    else
    {
      DoAddNormal(Token, "");
      ReadToken(Token);
    }
  }
  while (True);
}

static void TeXErrEntry(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  ReadToken(Token);
  assert_token("}");
  assert_token("{");
  *SepString = '\0';
  BackToken("\\");
  BackToken("item");
  BackToken("[");
  BackToken(Token);
  BackToken("]");
  ErrState = 0;
}

static void NextErrState(void)
{
  if (ErrState < 3)
    assert_token("{");
  if (ErrState == 0)
  {
    *SepString = '\0';
    BackToken("\\");
    BackToken("begin");
    BackToken("{");
    BackToken("description");
    BackToken("}");
  }
  if ((ErrState >= 0) && (ErrState <= 2))
  {
    *SepString = '\0';
    BackToken("\\");
    BackToken("item");
    BackToken("[");
    BackToken(ErrorEntryNames[ErrState]);
    BackToken(":");
    BackToken("]");
    BackToken("\\");
    BackToken("\\");
  }
  if (ErrState == 3)
  {
    *SepString = '\0';
    BackToken("\\");
    BackToken("\\");
    BackToken(" ");
    BackToken("\\");
    BackToken("end");
    BackToken("{");
    BackToken("description");
    BackToken("}");
    ErrState = -1;
  }
  else
    ErrState++;
}

static void TeXWriteLabel(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Name, "}");

  if ((CurrEnv == EnvCaption) || (CurrEnv == EnvTabular))
    GetTableName(Value, sizeof(Value));
  else
  {
    GetSectionName(Value, sizeof(Value));
    if ((*Value) && (Value[strlen(Value) - 1] == '.'))
      Value[strlen(Value) - 1] = '\0';
  }

  AddLabel(Name, Value);
}

static void TeXWriteRef(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Name, "}");
  GetLabel(Name, Value);
  DoAddNormal(Value, BackSepString);
}

static void TeXWriteCitation(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Name, "}");
  GetCite(Name, Value);
  as_snprintf(Name, sizeof(Name), "[%s]", Value);
  DoAddNormal(Name, BackSepString);
}

static void TeXNewParagraph(Word Index)
{
  UNUSED(Index);

  FlushLine();
  outc('\n');
}

static void TeXContents(Word Index)
{
  FILE *file = fopen(TocName, "r");
  char Line[200];
  UNUSED(Index);

  if (!file)
  {
    Warning("contents file not found.");
    DoRepass = True;
    return;
  }

  FlushLine();
  outs("        ");
  outs(ContentsName);
  outs("\n\n");
  while (!feof(file))
  {
    if (!fgets(Line, 199, file))
      break;
    outs(Line);
  }

  fclose(file);
}

static void TeXParSkip(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  ReadToken(Token);
  do
  {
    ReadToken(Token);
    if ((!strncmp(Token, "plus", 4)) || (!strncmp(Token, "minus", 5)))
    {
    }
    else
    {
      BackToken(Token);
      return;
    }
  }
  while (1);
}

static void TeXNLS(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = NULL;
  UNUSED(Index);

  /* NOTE: For characters relevant to hyphenation, insert the
           (codepage-independent) hyphen characters at this place.
           Transformation to codepage-dependent character takes
           place @ output: */

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = HYPHEN_CHR_ae;
        break;
      case 'e':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ee, Buf);
        break;
      case 'i':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ie, Buf);
        break;
      case 'o':
        Repl = HYPHEN_CHR_oe;
        break;
      case 'u':
        Repl = HYPHEN_CHR_ue;
        break;
      case 'A':
        Repl = HYPHEN_CHR_AE;
        break;
      case 'E':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ee, Buf);
        break;
      case 'I':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ie, Buf);
        break;
      case 'O':
        Repl = HYPHEN_CHR_OE;
        break;
      case 'U':
        Repl = HYPHEN_CHR_UE;
        break;
      case 's':
        Repl = HYPHEN_CHR_sz;
        break;
      default :
        break;
    }

  if (Repl)
  {
    if (strlen(Repl) > 1)
      memmove(Token + strlen(Repl), Token + 1, strlen(Token));
    memcpy(Token, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else
    DoAddNormal("\"", BackSepString);

  BackToken(Token);
}

static void TeXNLSGrave(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = NULL;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_agrave, Buf);
        break;
      case 'A':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Agrave, Buf);
        break;
      case 'e':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_egrave, Buf);
        break;
      case 'E':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Egrave, Buf);
        break;
      case 'i':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_igrave, Buf);
        break;
      case 'I':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Igrave, Buf);
        break;
      case 'o':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ograve, Buf);
        break;
      case 'O':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ograve, Buf);
        break;
      case 'u':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ugrave, Buf);
        break;
      case 'U':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ugrave, Buf);
        break;
      default:
        break;
    }

  if (Repl)
  {
    if (strlen(Repl) > 1)
      memmove(Token + strlen(Repl), Token + 1, strlen(Token));
    memcpy(Token, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else
    DoAddNormal("\"", BackSepString);

  BackToken(Token);
}

static void TeXNLSAcute(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = NULL;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_aacute, Buf);
        break;
      case 'A':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Aacute, Buf);
        break;
      case 'e':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_eacute, Buf);
        break;
      case 'E':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Eacute, Buf);
        break;
      case 'i':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_iacute, Buf);
        break;
      case 'I':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Iacute, Buf);
        break;
      case 'o':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_oacute, Buf);
        break;
      case 'O':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Oacute, Buf);
        break;
      case 'u':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_uacute, Buf);
        break;
      case 'U':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Uacute, Buf);
        break;
      default:
        break;
    }

  if (Repl)
  {
    if (strlen(Repl) > 1)
      memmove(Token + strlen(Repl), Token + 1, strlen(Token));
    memcpy(Token, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else
    DoAddNormal("\"", BackSepString);

  BackToken(Token);
}

static void TeXNLSCirc(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = "";
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_acirc, Buf);
        break;
      case 'A':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Acirc, Buf);
        break;
      case 'e':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ecirc, Buf);
        break;
      case 'E':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ecirc, Buf);
        break;
      case 'i':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_icirc, Buf);
        break;
      case 'I':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Icirc, Buf);
        break;
      case 'o':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ocirc, Buf);
        break;
      case 'O':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ocirc, Buf);
        break;
      case 'u':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ucirc, Buf);
        break;
      case 'U':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ucirc, Buf);
        break;
      default:
        break;
    }

  if (Repl)
  {
    if (strlen(Repl) > 1)
      memmove(Token + strlen(Repl), Token + 1, strlen(Token));
    memcpy(Token, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else
    DoAddNormal("\"", BackSepString);

  BackToken(Token);
}

static void TeXNLSTilde(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = "";
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'n':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_ntilde, Buf);
        break;
      case 'N':
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Ntilde, Buf);
        break;
    }

  if (Repl)
  {
    if (strlen(Repl) > 1)
      memmove(Token + strlen(Repl), Token + 1, strlen(Token));
    memcpy(Token, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else
    DoAddNormal("\"", BackSepString);

  BackToken(Token);
}

static void TeXCedilla(Word Index)
{
  char Token[TOKLEN], Buf[3];
  UNUSED(Index);

  assert_token("{");
  collect_token(Token, "}");
  if (!strcmp(Token, "c"))
    strcpy(Token, CharTab_GetNULTermString(pCharacterTab, eCH_ccedil, Buf));
  if (!strcmp(Token, "C"))
    strcpy(Token, CharTab_GetNULTermString(pCharacterTab, eCH_Ccedil, Buf));

  DoAddNormal(Token, BackSepString);
}

static void TeXAsterisk(Word Index)
{
  (void)Index;
  DoAddNormal("*", BackSepString);
}

static Boolean TeXNLSSpec(char *Line)
{
  Boolean Found = True;
  char Buf[3];
  const char *Repl = NULL;
  int cnt = 0;

  if (*SepString == '\0')
    switch (*Line)
    {
      case 'o':
        cnt = 1;
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_oslash, Buf);
        break;
      case 'O':
        cnt = 1;
        Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Oslash, Buf);
        break;
      case 'a':
        switch (Line[1])
        {
          case 'a':
            cnt = 2;
            Repl = CharTab_GetNULTermString(pCharacterTab, eCH_aring, Buf);
            break;
          case 'e':
            cnt = 2;
            Repl = CharTab_GetNULTermString(pCharacterTab, eCH_aelig, Buf);
            break;
          default:
            Found = False;
        }
        break;
      case 'A':
        switch (Line[1])
        {
          case 'A':
            cnt = 2;
            Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Aring, Buf);
            break;
          case 'E':
            cnt = 2;
            Repl = CharTab_GetNULTermString(pCharacterTab, eCH_Aelig, Buf);
            break;
          default:
            Found = False;
        }
        break;
      default:
        Found = False;
    }

  if (Found)
  {
    if ((int)strlen(Repl) != cnt)
      memmove(Line + strlen(Repl), Line + cnt, strlen(Line) - cnt + 1);
    memcpy(Line, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else
    DoAddNormal("\"", BackSepString);

  BackToken(Line);
  return Found;
}

static void TeXHyphenation(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Token, "}");
  AddException(Token);
}

static void TeXDoPot(void)
{
  char Token[TOKLEN];

  ReadToken(Token);
  if (*Token == '2')
  {
    char Buf[3];
    const char *pRepl = CharTab_GetNULTermString(pCharacterTab, eCH_e2, Buf);

    if (strlen(pRepl) > 1)
      memmove(Token + strlen(pRepl), Token + 1, strlen(Token));
    memcpy(Token, pRepl, strlen(pRepl));
  }
  else
    DoAddNormal("^", BackSepString);

  BackToken(Token);
}

static void TeXDoSpec(void)
{
  strcpy(BackSepString, SepString);
  TeXNLS(0);
}

static void TeXInclude(Word Index)
{
  char Token[2 * TOKLEN + 1], Msg[2 * TOKLEN + 1];
  UNUSED(Index);

  assert_token("{");
  strcpy(Token, SrcDir);
  collect_token(Token + strlen(Token), "}");
  infiles[IncludeNest] = fopen(Token, "r");
  if (!infiles[IncludeNest])
  {
    as_snprintf(Msg, sizeof(Msg), "file %s not found", Token);
    error(Msg);
  }
  else
    IncludeNest++;
}

static void TeXDocumentStyle(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  ReadToken(Token);
  if (!strcmp(Token, "["))
  {
    do
    {
      ReadToken(Token);
      if (!strcmp(Token, "german"))
        SetLang(True);
    }
    while (strcmp(Token, "]"));
    assert_token("{");
    ReadToken(Token);
    if (CurrPass <= 1)
    {
      if (!as_strcasecmp(Token,  "article"))
      {
        AddInstTable(TeXTable, "section", 0, TeXNewSection);
        AddInstTable(TeXTable, "subsection", 1, TeXNewSection);
        AddInstTable(TeXTable, "subsubsection", 3, TeXNewSection);
      }
      else
      {
        AddInstTable(TeXTable, "chapter", 0, TeXNewSection);
        AddInstTable(TeXTable, "section", 1, TeXNewSection);
        AddInstTable(TeXTable, "subsection", 2, TeXNewSection);
        AddInstTable(TeXTable, "subsubsection", 3, TeXNewSection);
      }
    }
    assert_token("}");
  }
}

/*!------------------------------------------------------------------------
 * \fn     TeXUsePackage(Word Index)
 * \brief  parse \usepackage command
 * ------------------------------------------------------------------------ */

static void TeXUsePackage(Word Index)
{
  char Token[TOKLEN], Msg[2 * TOKLEN + 1];
  Boolean read_german_opt = False;

  UNUSED(Index);

  while (True)
  {
    ReadToken(Token);
    if (!strcmp(Token, "["))
    {
      do
      {
        ReadToken(Token);
        if (!strcmp(Token, "german"))
          read_german_opt = True;
      }
      while (strcmp(Token, "]"));
    }
    else if (!strcmp(Token, "{"))
    {
      ReadToken(Token);
      if (!as_strcasecmp(Token, "german"))
        SetLang(True);
      else if (!as_strcasecmp(Token, "babel"))
        SetLang(read_german_opt);
      else if (!as_strcasecmp(Token, "makeidx"));
      else if (!as_strcasecmp(Token, "hyperref"));
      else if (!as_strcasecmp(Token, "longtable"));
      else
      {
        as_snprintf(Msg, sizeof(Msg), "unknown package '%s'", Token);
        error(Msg);
      }
      assert_token("}");
      break;
    }
    else
    {
      as_snprintf(Msg, sizeof(Msg), "expecting [ or { after \\usepackage");
      error(Msg);
    }
  }
}

/*--------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  char Line[TOKLEN], Comp[TOKLEN], *p, AuxFile[200];
  int z, NumPassesLeft;

  if (argc < 3)
  {
    fprintf(stderr, "calling convention: %s <input file> <output file>\n", *argv);
    exit(1);
  }

  nls_init();
  if (!NLS_Initialize(&argc, argv))
    exit(3);

  enable_hyphenation = True;
  if (argc)
  {
    int z, dest;

    z = dest = 1;
    while (z < argc)
      if (!as_strcasecmp(argv[z], "-nohyphen"))
      {
        enable_hyphenation = False;
        z++;
      }
      else
        argv[dest++] = argv[z++];
    argc = dest;
  }

  Codepage = NLS_GetCodepage();
  pCharacterTab = GetCharacterTab(Codepage);
  pThisTable = (TTable*)calloc(1, sizeof(*pThisTable));

  TeXTable = CreateInstTable(301);
  AddInstTable(TeXTable, "\\", 0, TeXFlushLine);
  AddInstTable(TeXTable, "par", 0, TeXNewParagraph);
  AddInstTable(TeXTable, "-", 0, TeXDummy);
  AddInstTable(TeXTable, "hyphenation", 0, TeXHyphenation);
  AddInstTable(TeXTable, "kill", 0, TeXKillLine);
  AddInstTable(TeXTable, "/", 0, TeXDummy);
  AddInstTable(TeXTable, "pagestyle", 0, TeXDummyInCurl);
  AddInstTable(TeXTable, "thispagestyle", 0, TeXDummyInCurl);
  AddInstTable(TeXTable, "sloppy", 0, TeXDummy);
  AddInstTable(TeXTable, "clearpage", 0, TeXDummy);
  AddInstTable(TeXTable, "cleardoublepage", 0, TeXDummy);
  AddInstTable(TeXTable, "topsep", 0, TeXDummyNoBrack);
  AddInstTable(TeXTable, "parskip", 0, TeXParSkip);
  AddInstTable(TeXTable, "parindent", 0, TeXDummyNoBrack);
  AddInstTable(TeXTable, "textwidth", 0, TeXDummyNoBrack);
  AddInstTable(TeXTable, "evensidemargin", 0, TeXDummyNoBrack);
  AddInstTable(TeXTable, "oddsidemargin", 0, TeXDummyNoBrack);
  AddInstTable(TeXTable, "hfuzz", 0, TeXDummyEqual);
  AddInstTable(TeXTable, "newcommand", 0, TeXNewCommand);
  AddInstTable(TeXTable, "def", 0, TeXDef);
  AddInstTable(TeXTable, "font", 0, TeXFont);
  AddInstTable(TeXTable, "documentstyle", 0, TeXDocumentStyle);
  AddInstTable(TeXTable, "documentclass", 0, TeXDocumentStyle);
  AddInstTable(TeXTable, "usepackage", 0, TeXUsePackage);
  AddInstTable(TeXTable, "appendix", 0, TeXAppendix);
  AddInstTable(TeXTable, "makeindex", 0, TeXDummy);
  AddInstTable(TeXTable, "begin", 0, TeXBeginEnv);
  AddInstTable(TeXTable, "end", 0, TeXEndEnv);
  AddInstTable(TeXTable, "item", 0, TeXItem);
  AddInstTable(TeXTable, "bibitem", 0, TeXBibItem);
  AddInstTable(TeXTable, "errentry", 0, TeXErrEntry);
  AddInstTable(TeXTable, "$", 0, TeXAddDollar);
  AddInstTable(TeXTable, "_", 0, TeXAddUnderbar);
  AddInstTable(TeXTable, "&", 0, TeXAddAmpersand);
  AddInstTable(TeXTable, "@", 0, TeXAddAt);
  AddInstTable(TeXTable, "#", 0, TeXAddImm);
  AddInstTable(TeXTable, "%", 0, TeXAddPercent);
  AddInstTable(TeXTable, "ss", 0, TeXAddSSharp);
  AddInstTable(TeXTable, "in", 0, TeXAddIn);
  AddInstTable(TeXTable, "rz", 0, TeXAddReal);
  AddInstTable(TeXTable, "mu", 0, TeXAddGreekMu);
  AddInstTable(TeXTable, "pi", 0, TeXAddGreekPi);
  AddInstTable(TeXTable, "leq", 0, TeXAddLessEq);
  AddInstTable(TeXTable, "geq", 0, TeXAddGreaterEq);
  AddInstTable(TeXTable, "neq", 0, TeXAddNotEq);
  AddInstTable(TeXTable, "land", 0, TeXAddLAnd);
  AddInstTable(TeXTable, "lor", 0, TeXAddLOr);
  AddInstTable(TeXTable, "oplus", 0, TeXAddOPlus);
  AddInstTable(TeXTable, "mid", 0, TeXAddMid);
  AddInstTable(TeXTable, "asname", 0, TeXASName);
  AddInstTable(TeXTable, "frac", 0, TeXDoFrac);
  AddInstTable(TeXTable, "rm", FontStandard, TeXNewFontType);
  AddInstTable(TeXTable, "em", FontEmphasized, TeXNewFontType);
  AddInstTable(TeXTable, "bf", FontBold, TeXNewFontType);
  AddInstTable(TeXTable, "tt", FontTeletype, TeXNewFontType);
  AddInstTable(TeXTable, "it", FontItalic, TeXNewFontType);
  AddInstTable(TeXTable, "bb", FontBold, TeXEnvNewFontType);
  AddInstTable(TeXTable, "tty", FontTeletype, TeXEnvNewFontType);
  AddInstTable(TeXTable, "ii", FontItalic, TeXEnvNewFontType);
  AddInstTable(TeXTable, "tiny", FontTiny, TeXNewFontSize);
  AddInstTable(TeXTable, "small", FontSmall, TeXNewFontSize);
  AddInstTable(TeXTable, "normalsize", FontNormalSize, TeXNewFontSize);
  AddInstTable(TeXTable, "large", FontLarge, TeXNewFontSize);
  AddInstTable(TeXTable, "huge", FontHuge, TeXNewFontSize);
  AddInstTable(TeXTable, "tin", FontTiny, TeXEnvNewFontSize);
  AddInstTable(TeXTable, "rightarrow", 0, TeXAddRightArrow);
  AddInstTable(TeXTable, "longrightarrow", 0, TeXAddLongRightArrow);
  AddInstTable(TeXTable, "leftarrow", 0, TeXAddLeftArrow);
  AddInstTable(TeXTable, "longleftarrow", 0, TeXAddLongLeftArrow);
  AddInstTable(TeXTable, "leftrightarrow", 0, TeXAddLeftRightArrow);
  AddInstTable(TeXTable, "marginpar", 0, TeXAddMarginPar);
  AddInstTable(TeXTable, "caption", 0, TeXAddCaption);
  AddInstTable(TeXTable, "endhead", 0, TeXEndHead);
  AddInstTable(TeXTable, "label", 0, TeXWriteLabel);
  AddInstTable(TeXTable, "ref", 0, TeXWriteRef);
  AddInstTable(TeXTable, "cite", 0, TeXWriteCitation);
  AddInstTable(TeXTable, "hline", 0, TeXHorLine);
  AddInstTable(TeXTable, "multicolumn", 0, TeXMultiColumn);
  AddInstTable(TeXTable, "ttindex", 0, TeXIndex);
  AddInstTable(TeXTable, "hspace", 0, TeXHSpace);
  AddInstTable(TeXTable, "vspace", 0, TeXVSpace);
  AddInstTable(TeXTable, "=", 0, TeXAddTabStop);
  AddInstTable(TeXTable, ">", 0, TeXJmpTabStop);
  AddInstTable(TeXTable, "verb", 0, TeXDoVerb);
  AddInstTable(TeXTable, "printindex", 0, TeXDummy);
  AddInstTable(TeXTable, "tableofcontents", 0, TeXContents);
  AddInstTable(TeXTable, "rule", 0, TeXRule);
  AddInstTable(TeXTable, "\"", 0, TeXNLS);
  AddInstTable(TeXTable, "`", 0, TeXNLSGrave);
  AddInstTable(TeXTable, "'", 0, TeXNLSAcute);
  AddInstTable(TeXTable, "^", 0, TeXNLSCirc);
  AddInstTable(TeXTable, "~", 0, TeXNLSTilde);
  AddInstTable(TeXTable, "c", 0, TeXCedilla);
  AddInstTable(TeXTable, "*", 0, TeXAsterisk);
  AddInstTable(TeXTable, "newif", 0, TeXDummy);
  AddInstTable(TeXTable, "fi", 0, TeXDummy);
  AddInstTable(TeXTable, "ifelektor", 0, TeXDummy);
  AddInstTable(TeXTable, "elektortrue", 0, TeXDummy);
  AddInstTable(TeXTable, "elektorfalse", 0, TeXDummy);
  AddInstTable(TeXTable, "input", 0, TeXInclude);

  CurrPass = 0;
  NumPassesLeft = 3;
  do
  {
    CurrPass++;

    DidEOF = False;
    IncludeNest = 0;
    pInFileName = argv[1];
    *infiles = fopen(pInFileName, "r");
    if (!*infiles)
    {
      perror(pInFileName);
      exit(3);
    }
    else
      IncludeNest++;
    SetSrcDir(pInFileName);
    if (!strcmp(argv[2], "-"))
      outfile = stdout;
    else
    {
      outfile = fopen(argv[2], "w");
      if (!outfile)
      {
        perror(argv[2]);
        exit(3);
      }
    }

    for (z = 0; z < CHAPMAX; Chapters[z++] = 0);
    TableNum = 0;
    TabStopCnt = 0;
    CurrTabStop = 0;
    ErrState = FracState = -1;
    InAppendix = False;
    EnvStack = NULL;
    CurrEnv = EnvNone;
    CurrListDepth = 0;
    ActLeftMargin = LeftMargin = 1;
    RightMargin = 70;
    Alignment = AlignNone;
    EnumCounter = 0;
    InitFont();
    CurrLine = 0;
    InitLabels();
    InitCites();
    InitToc();
    *SideMargin = '\0';
    DoRepass = False;
    BibIndent = BibCounter = 0;
    GermanMode = True;
    SetLang(False);

    strcpy(TocName, pInFileName);
    p = strrchr(TocName, '.');
    if (p)
      *p = '\0';
    strcat(TocName, ".dtoc");

    strcpy(AuxFile, pInFileName);
    p = strrchr(AuxFile, '.');
    if (p)
      *p = '\0';
    strcat(AuxFile, ".daux");
    ReadAuxFile(AuxFile);

    while (1)
    {
      if (!ReadToken(Line))
        break;
      if (!strcmp(Line, "\\"))
      {
        strcpy(BackSepString, SepString);
        if (!ReadToken(Line))
          error("unexpected end of file");
        if (*SepString != '\0')
          BackToken(Line);
        else if (!LookupInstTable(TeXTable, Line))
          if (!TeXNLSSpec(Line))
          {
            as_snprintf(Comp, sizeof(Comp), "unknown TeX command %s", Line);
            Warning(Comp);
          }
      }
      else if (!strcmp(Line, "$"))
      {
        InMathMode = !InMathMode;
        if (InMathMode)
        {
          strcpy(BackSepString, SepString);
          ReadToken(Line);
          strcpy(SepString, BackSepString);
          BackToken(Line);
        }
      }
      else if (!strcmp(Line, "&"))
        NextTableColumn();
      else if ((!strcmp(Line, "^")) && (InMathMode))
        TeXDoPot();
      else if ((!strcmp(Line, "\"")) && (GermanMode))
        TeXDoSpec();
      else if (!strcmp(Line, "{"))
        SaveFont();
      else if (!strcmp(Line, "}"))
      {
        if (FontNest > 0)
          RestoreFont();
        else if (ErrState >= 0)
          NextErrState();
        else if (FracState >= 0)
          NextFracState();
        else switch (CurrEnv)
        {
          case EnvMarginPar:
            RestoreEnv();
            break;
          case EnvCaption:
            FlushLine();
            RestoreEnv();
            break;
          case EnvHeading:
            EndSectionHeading();
            RestoreEnv();
            break;
          default:
            RestoreFont();
        }
      }
      else
        DoAddNormal(Line, SepString);
    }
    FlushLine();

    for (z = 0; z < IncludeNest; fclose(infiles[z++]));
    fclose(outfile);

    unlink(AuxFile);
    PrintLabels(AuxFile);
    PrintCites(AuxFile);
    PrintToc(TocName);

    FreeLabels();
    FreeCites();
    DestroyTree();
    FreeToc();
    FreeFontStack();

    NumPassesLeft--;
    if (DoRepass)
      fprintf(stderr, "additional pass needed\n");
  }
  while (DoRepass && NumPassesLeft);

  DestroyInstTable(TeXTable);

  if (DoRepass)
  {
    fprintf(stderr, "additional passes needed but cowardly not done\n");
    return 3;
  }
  else
  {
    fprintf(stderr, "%d pass(es) needed\n", CurrPass);
    return 0;
  }
}

#ifdef CKMALLOC
#undef malloc
#undef realloc

void *ckmalloc(size_t s)
{
  void *tmp = malloc(s);
  if (!tmp)
  {
    fprintf(stderr, "allocation error(malloc): out of memory");
    exit(255);
  }
  return tmp;
}

void *ckrealloc(void *p, size_t s)
{
  void *tmp = realloc(p, s);
  if (!tmp)
  {
    fprintf(stderr, "allocation error(realloc): out of memory");
    exit(255);
  }
  return tmp;
}
#endif

