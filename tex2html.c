/* tex2html.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Konverter TeX-->HTML                                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "asmitree.h"
#include "chardefs.h"
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "texrefs.h"
#include "texutil.h"
#include "textoc.h"
#include "texfonts.h"
#include "strutil.h"

#ifdef __MSDOS__
# include <dir.h>
#endif

/*--------------------------------------------------------------------------*/

#define TOKLEN 350

static char *TableName,
            *BiblioName,
            *ContentsName,
            *IndexName,
#define ErrorEntryCnt 3
            *ErrorEntryNames[ErrorEntryCnt];

typedef enum
{
  EnvNone, EnvDocument, EnvItemize, EnvEnumerate, EnvDescription, EnvTable,
  EnvTabular, EnvRaggedLeft, EnvRaggedRight, EnvCenter, EnvVerbatim,
  EnvQuote, EnvTabbing, EnvBiblio, EnvMarginPar, EnvCaption, EnvHeading, EnvCount
} EnvType;

static char *FontNames[FontCnt] =
{
  "", "EM", "B", "TT", "I", "SUP"
};

typedef struct sEnvSave
{
  struct sEnvSave *Next;
  EnvType SaveEnv;
  int ListDepth, ActLeftMargin, LeftMargin, RightMargin;
  int EnumCounter, FontNest;
  Boolean InListItem;
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

typedef struct sIndexSave
{
  struct sIndexSave *Next;
  char *Name;
  int RefCnt;
} TIndexSave, *PIndexSave;

static char *EnvNames[EnvCount] =
{
  "___NONE___", "document", "itemize", "enumerate", "description", "table", "tabular",
  "raggedleft", "raggedright", "center", "verbatim", "quote", "tabbing",
  "thebibliography", "___MARGINPAR___", "___CAPTION___", "___HEADING___"
};

static int IncludeNest;
static FILE *infiles[50], *outfile;
static char *outfilename;
static char TocName[200];

#define CHAPMAX 6
static int Chapters[CHAPMAX];
static int TableNum, ErrState, FracState, BibIndent, BibCounter;
#define TABMAX 100
static int TabStops[TABMAX], TabStopCnt, CurrTabStop;
static Boolean InAppendix, InMathMode, InListItem;
static TTable *pThisTable;
static int CurrRow, CurrCol;
static char SrcDir[TOKLEN + 1], asname[TOKLEN];
static Boolean GermanMode;

static int Structured;

static EnvType CurrEnv;
static int CurrListDepth;
static int EnumCounter;
static int ActLeftMargin, LeftMargin, RightMargin;
static int CurrPass;
static PEnvSave EnvStack;
static PIndexSave FirstIndex;

static PInstTable TeXTable;

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
  if (GermanMode == IsGerman)
    return;

  GermanMode = IsGerman;
  if (GermanMode)
  {
    TableName = "Tabelle";
    BiblioName = "Literaturverzeichnis";
    ContentsName = "Inhalt";
    IndexName = "Index";
    ErrorEntryNames[0] = "Typ";
    ErrorEntryNames[1] = "Ursache";
    ErrorEntryNames[2] = "Argument";
  }
  else
  {
    TableName = "Table";
    BiblioName = "Bibliography";
    ContentsName = "Contents";
    IndexName = "Index";
    ErrorEntryNames[0] = "Type";
    ErrorEntryNames[1] = "Reason";
    ErrorEntryNames[2] = "Argument";
  }
}

/*------------------------------------------------------------------------------*/

static void GetNext(char *Src, char *Dest)
{
  char *c = strchr(Src,' ');

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
      GetNext(Line, Nam); GetNext(Line, Val);
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
      else if (Comment)
      {
        if ((*BufferLine) && (BufferLine[strlen(BufferLine) - 1] == '\n'))
          BufferLine[strlen(BufferLine) - 1] = '\0';
        if (!strncmp(BufferLine + 2, "TITLE ", 6))
          fprintf(outfile, "<TITLE>%s</TITLE>\n", BufferLine + 8);
      }
      else
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
    done = (!strcmp(Comp, term));
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

static char OutLineBuffer[TOKLEN] = "", SideMargin[TOKLEN];

static void PutLine(Boolean DoBlock)
{
  int l, n, ptrcnt, diff, div, mod, divmod;
  char *chz, *ptrs[50];
  Boolean SkipFirst, IsFirst;

  fputs(Blanks(LeftMargin - 1), outfile);
  if ((CurrEnv == EnvRaggedRight) || (!DoBlock))
  {
    fprintf(outfile, "%s", OutLineBuffer);
    l = strlen(OutLineBuffer);
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
      l++;
    }
    (void)ptrs;
    diff = RightMargin - LeftMargin + 1 - l;
    div = (ptrcnt > 0) ? diff / ptrcnt : 0;
    mod = diff - (ptrcnt * div);
    divmod = (mod > 0) ? ptrcnt / mod : ptrcnt + 1;
    IsFirst = SkipFirst;
    ptrcnt = 0;
    for (chz = OutLineBuffer; *chz != '\0'; chz++)
    {
      fputc(*chz, outfile);
      if ((chz > OutLineBuffer) && (*(chz - 1) != ' ') && (*chz == ' '))
      {
        if (!IsFirst)
        {
          n = div;
          if ((mod > 0) && ((ptrcnt % divmod) == 0))
          {
            mod--;
            n++;
          }
          if (n > 0)
            fputs(Blanks(n), outfile);
          ptrcnt++;
        }
        IsFirst = False;
      }
    }
    l = RightMargin - LeftMargin + 1;
  }
  if (*SideMargin != '\0')
  {
    fputs(Blanks(RightMargin - LeftMargin + 4 - l), outfile);
#if 0
    fprintf(outfile, "%s", SideMargin);
#endif
    *SideMargin = '\0';
  }
  fputc('\n', outfile);
  LeftMargin = ActLeftMargin;
}

static void AddLine(const char *Part, char *Sep)
{
  int mlen = RightMargin - LeftMargin + 1;
  char *search, save;

  if (strlen(Sep) > 1)
    Sep[1] = '\0';
  if (*OutLineBuffer != '\0')
    strcat(OutLineBuffer, Sep);
  strcat(OutLineBuffer, Part);
  if ((int)strlen(OutLineBuffer) >= mlen)
  {
    search = OutLineBuffer + mlen;
    while (search >= OutLineBuffer)
    {
      if (*search == ' ')
        break;
      search--;
    }
    if (search <= OutLineBuffer)
    {
      PutLine(False);
      *OutLineBuffer = '\0';
    }
    else
    {
      save = (*search);
      *search = '\0';
      PutLine(False);
      *search = save;
      for (; *search == ' '; search++);
      strmov(OutLineBuffer, search);
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

static void AddTableEntry(const char *Part, char *Sep)
{
  char *Ptr = pThisTable->Lines[CurrRow][CurrCol];
  int nlen = (!Ptr) ? 0 : strlen(Ptr);
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

static void DoAddNormal(const char *Part, char *Sep)
{
  if (!strcmp(Part, "<"))
    Part = "&lt;";
  else if (!strcmp(Part, ">"))
    Part = "&gt;";
  else if (!strcmp(Part, "&"))
    Part = "&amp;";

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

/*--------------------------------------------------------------------------*/

void PrFontDiff(int OldFlags, int NewFlags)
{
  tFontType z;
  int Mask;
  char erg[10];

  for (z = FontStandard + 1, Mask = 2; z < FontCnt; z++, Mask = Mask << 1)
   if ((OldFlags^NewFlags) & Mask)
   {
     as_snprintf(erg, sizeof(erg), "<%s%s>", (NewFlags & Mask)?"":"/", FontNames[z]);
     DoAddNormal(erg, "");
   }
}

void PrFontSize(tFontSize Type, Boolean On)
{
  char erg[10];

  strcpy(erg, "<");
  if (FontNormalSize == Type)
    return;

  if (!On)
    strcat(erg, "/");
  switch (Type)
  {
    case FontTiny:
    case FontSmall:
      strcat(erg, "SMALL");
      break;
    case FontLarge:
    case FontHuge:
      strcat(erg, "BIG");
      break;
    default:
      break;
  }
  strcat (erg, ">");
  DoAddNormal(erg, "");
  if ((FontTiny == Type) || (FontHuge == Type))
    DoAddNormal(erg, "");
}

static void SaveEnv(EnvType NewEnv)
{
  PEnvSave NewSave;

  NewSave = (PEnvSave) malloc(sizeof(TEnvSave));
  NewSave->Next = EnvStack;
  NewSave->ListDepth = CurrListDepth;
  NewSave->LeftMargin = LeftMargin;
  NewSave->ActLeftMargin = ActLeftMargin;
  NewSave->RightMargin = RightMargin;
  NewSave->EnumCounter = EnumCounter;
  NewSave->SaveEnv = CurrEnv;
  NewSave->FontNest = FontNest;
  NewSave->InListItem = InListItem;
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
  EnumCounter = OldSave->EnumCounter;
  FontNest = OldSave->FontNest;
  InListItem = OldSave->InListItem;
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

static void DumpTable(void)
{
  int TextCnt, RowCnt, rowz, rowz2, rowz3, colz, colptr, ml, l, diff, sumlen, firsttext, indent;
  char *ColTag;

  /* compute widths of individual rows */
  /* get index of first text column */

  RowCnt = pThisTable->Lines[CurrRow][0] ? CurrRow + 1 : CurrRow;
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
          l = (!pThisTable->Lines[rowz][colptr]) ? 0 : strlen(pThisTable->Lines[rowz][colptr]);
          if (ml < l)
            ml = l;
        }
      pThisTable->ColLens[colz] = ml + 2;
      colptr++;
      if (firsttext < 0) firsttext = colz;
    }

  /* count number of text columns */

  for (colz = TextCnt = 0; colz < pThisTable->ColumnCount; colz++)
    if (pThisTable->ColTypes[colz] != ColBar)
      TextCnt++;

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
      l = (!pThisTable->Lines[rowz][0]) ? 0 : strlen(pThisTable->Lines[rowz][0]);
      if (ml < l)
        ml = l;
    }
  if (ml + 4 > sumlen)
  {
    diff = ml + 4 - sumlen;
    pThisTable->ColLens[firsttext] += diff;
  }

  /* tell browser to switch to table mode */

  fprintf(outfile, "<P><CENTER><TABLE SUMMARY=\"No Summary\" BORDER=1 CELLPADDING=5>\n");

  /* print rows */

  rowz = 0;
  while (rowz < RowCnt)
  {
    /* find first text line */

    for (; rowz < RowCnt; rowz++)
      if (!pThisTable->LineFlags[rowz])
        break;

    /* find last text line */

    for (rowz2 = rowz; rowz2 < RowCnt; rowz2++)
      if (pThisTable->LineFlags[rowz2])
        break;
    rowz2--;

    if (rowz < RowCnt)
    {
      /* if more than one line follows, take this as header line(s) */

      if ((rowz2 <= RowCnt - 3) && (pThisTable->LineFlags[rowz2 + 1]) && (pThisTable->LineFlags[rowz2 + 2]))
        ColTag = "TH";
      else
        ColTag = "TD";

      /* start a row */

      fprintf(outfile, "<TR ALIGN=LEFT>\n");

      /* over all columns... */

      colptr = 0;
      for (colz = 0; colz < ((pThisTable->MultiFlags[rowz])?firsttext + 1:pThisTable->ColumnCount); colz++)
        if (pThisTable->ColTypes[colz] != ColBar)
        {
          /* start a column */

          fprintf(outfile, "<%s VALIGN=TOP NOWRAP", ColTag);
          if (pThisTable->MultiFlags[rowz])
            fprintf(outfile, " COLSPAN=%d", TextCnt);
          switch (pThisTable->ColTypes[colz])
          {
            case ColLeft:
              fputs(" ALIGN=LEFT>", outfile);
              break;
            case ColCenter:
              fputs(" ALIGN=CENTER>", outfile);
              break;
            case ColRight:
              fputs(" ALIGN=RIGHT>", outfile);
              break;
            default:
              break;
          }

          /* write items */

          for (rowz3 = rowz; rowz3 <= rowz2; rowz3++)
          {
            if (pThisTable->Lines[rowz3][colptr])
              fputs(pThisTable->Lines[rowz3][colptr], outfile);
            if (rowz3 != rowz2)
              fputs("<BR>\n", outfile);
          }

          /* end column */

          fprintf(outfile, "</%s>\n", ColTag);

          colptr++;
        }

      /* end row */

      fprintf(outfile, "</TR>\n");

      for (rowz3 = rowz; rowz3 <= rowz2; rowz3++)
        for (colz = 0; colz < pThisTable->ColumnCount; colz++)
          if (pThisTable->Lines[rowz3][colz])
          {
            free(pThisTable->Lines[rowz3][colz]);
            pThisTable->Lines[rowz3][colz] = NULL;
          }

      rowz = rowz2 + 1;
    }
  }

  /* end table mode */

  fprintf(outfile, "</TABLE></CENTER>\n");
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
  else if (CurrEnv == EnvTabbing)
  {
    CurrTabStop = 0;
    PrFontDiff(CurrFontFlags, 0);
    AddLine("</TD></TR>", "");
    FlushLine();
    AddLine("<TR><TD NOWRAP>", "");
    PrFontDiff(0, CurrFontFlags);
  }
  else
  {
    if (*OutLineBuffer == '\0')
      strcpy(OutLineBuffer, " ");
    AddLine("<BR>", "");
    FlushLine();
  }
}

static void TeXKillLine(Word Index)
{
  UNUSED(Index);

  ResetLine();
  if (CurrEnv == EnvTabbing)
  {
    AddLine("<TR><TD NOWRAP>", "");
    PrFontDiff(0, CurrFontFlags);
  }
}

static void TeXDummy(Word Index)
{
  UNUSED(Index);
}

static void TeXDummyEqual(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("=");
  ReadToken(Token);
}

static void TeXDummyNoBrack(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

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
  fputc('\n', outfile);

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
  int Level = LastLevel;
  char Line[TOKLEN], Title[TOKLEN], *rep;

  strcpy(Title, OutLineBuffer);
  *OutLineBuffer = '\0';

  fprintf(outfile, "<H%d>", Level + 1);

  *Line = '\0';
  if (Level < 3)
  {
    GetSectionName(Line, sizeof(Line));
    fprintf(outfile, "<A NAME=\"sect_");
    for (rep = Line; *rep; rep++)
      fputc((*rep == '.') ? '_' : *rep, outfile);
    fprintf(outfile, "\">");
    as_snprcatf(Line, sizeof(Line), " ");
  }
  as_snprcatf(Line, sizeof(Line), "%s", Title);

  fprintf(outfile, "%s", Line);

  if (Level < 3)
  {
    fputs("</A>", outfile);
    GetSectionName(Line, sizeof(Line));
    as_snprcatf(Line, sizeof(Line), " %s", Title);
    AddToc(Line, 0);
  }

  fprintf(outfile, "</H%d>\n", Level + 1);
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
  char EnvName[TOKLEN], Add[TOKLEN], *p;
  EnvType NEnv;
  Boolean done;
  TColumn NCol;
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
    case EnvDocument:
      fputs("</HEAD>\n", outfile);
      fputs("<BODY>\n", outfile);
      break;
    case EnvItemize:
      FlushLine();
      fprintf(outfile, "<UL>\n");
      ++CurrListDepth;
      ActLeftMargin = LeftMargin = (CurrListDepth * 4) + 1;
      RightMargin = 70;
      EnumCounter = 0;
      InListItem = False;
      break;
    case EnvDescription:
      FlushLine();
      fprintf(outfile, "<DL COMPACT>\n");
      ++CurrListDepth;
      ActLeftMargin = LeftMargin = (CurrListDepth * 4) + 1;
      RightMargin = 70;
      EnumCounter = 0;
      InListItem = False;
      break;
    case EnvEnumerate:
      FlushLine();
      fprintf(outfile, "<OL>\n");
      ++CurrListDepth;
      ActLeftMargin = LeftMargin = (CurrListDepth * 4) + 1;
      RightMargin = 70;
      EnumCounter = 0;
      InListItem = False;
      break;
    case EnvBiblio:
      FlushLine();
      fprintf(outfile, "<P>\n");
      fprintf(outfile, "<H1><A NAME=\"sect_bib\">%s</A></H1>\n<DL COMPACT>\n", BiblioName);
      assert_token("{");
      ReadToken(Add);
      assert_token("}");
      ActLeftMargin = LeftMargin = 4 + (BibIndent = strlen(Add));
      AddToc(BiblioName, 0);
      break;
    case EnvVerbatim:
      FlushLine();
      fprintf(outfile, "<PRE>\n");
      if ((*BufferLine != '\0') && (*BufferPtr != '\0'))
      {
        fprintf(outfile, "%s", BufferPtr);
        *BufferLine = '\0';
        BufferPtr = BufferLine;
      }
      do
      {
        if (!fgets(Add, TOKLEN - 1, infiles[IncludeNest - 1]))
          break;
        CurrLine++;
        done = strstr(Add, "\\end{verbatim}") != NULL;
        if (!done)
        {
          for (p = Add; *p != '\0';)
            if (*p == '<')
            {
              memmove(p + 3, p, strlen(p) + 1);
              memcpy(p, "&lt;", 4);
              p += 4;
            }
            else if (*p == '>')
            {
              memmove(p + 3, p, strlen(p) + 1);
              memcpy(p, "&gt;", 4);
              p += 4;
            }
            else
              p++;
          fprintf(outfile, "%s", Add);
        }
      }
      while (!done);
      fprintf(outfile, "\n</PRE>\n");
      break;
    case EnvQuote:
      FlushLine();
      fprintf(outfile, "<BLOCKQUOTE>\n");
      ActLeftMargin = LeftMargin = 5;
      RightMargin = 70;
      break;
    case EnvTabbing:
      FlushLine();
      fputs("<TABLE SUMMARY=\"No Summary\" CELLPADDING=2>\n", outfile);
      TabStopCnt = 0;
      CurrTabStop = 0;
      RightMargin = TOKLEN - 1;
      AddLine("<TR><TD NOWRAP>", "");
      PrFontDiff(0, CurrFontFlags);
      break;
    case EnvTable:
      ReadToken(Add);
      if (strcmp(Add, "["))
        BackToken(Add);
      else
      {
        do
        {
          ReadToken(Add);
        }
        while (strcmp(Add, "]"));
      }
      FlushLine();
      fputc('\n', outfile);
      ++TableNum;
      break;
    case EnvCenter:
      FlushLine();
      fputs("<CENTER>\n", outfile);
      break;
    case EnvRaggedRight:
      FlushLine();
      fputs("<DIV ALIGN=LEFT>\n", outfile);
      break;
    case EnvRaggedLeft:
      FlushLine();
      fputs("<DIV ALIGN=RIGHT>\n", outfile);
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
          NCol = ColLeft;
          if (!strcmp(Add, "|"))
            NCol = ColBar;
          else if (!strcmp(Add, "l"))
            NCol = ColLeft;
          else if (!strcmp(Add, "r"))
            NCol = ColRight;
          else if (!strcmp(Add, "c"))
            NCol = ColCenter;
          else
            error("unknown table column descriptor");
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
    error("begin and end of environment do not match");

  switch (CurrEnv)
  {
    case EnvDocument:
      FlushLine();
      fputs("</BODY>\n", outfile);
      break;
    case EnvItemize:
      if (InListItem)
        AddLine("</LI>", "");
      FlushLine();
      fprintf(outfile, "</UL>\n");
      break;
    case EnvDescription:
      if (InListItem)
        AddLine("</DD>", "");
      FlushLine();
      fprintf(outfile, "</DL>\n");
      break;
    case EnvEnumerate:
      if (InListItem)
        AddLine("</LI>", "");
      FlushLine();
      fprintf(outfile, "</OL>\n");
      break;
    case EnvQuote:
      FlushLine();
      fprintf(outfile, "</BLOCKQUOTE>\n");
      break;
    case EnvBiblio:
      FlushLine();
      fprintf(outfile, "</DL>\n");
      break;
    case EnvTabbing:
      PrFontDiff(CurrFontFlags, 0);
      AddLine("</TD></TR>", "");
      FlushLine();
      fputs("</TABLE>", outfile);
      break;
    case EnvCenter:
      FlushLine();
      fputs("</CENTER>\n", outfile);
      break;
    case EnvRaggedRight:
    case EnvRaggedLeft:
      FlushLine();
      fputs("</DIV>\n", outfile);
      break;
    case EnvTabular:
      DumpTable();
      break;
    case EnvTable:
      FlushLine(); fputc('\n', outfile);
      break;
    default:
      break;
  }

  RestoreEnv();
}

static void TeXItem(Word Index)
{
  char Token[TOKLEN], Acc[TOKLEN];
  UNUSED(Index);

  if (InListItem)
    AddLine((CurrEnv == EnvDescription) ? "</DD>" : "</LI>", "");
  FlushLine();
  InListItem = True;
  switch(CurrEnv)
  {
    case EnvItemize:
      fprintf(outfile, "<LI>");
      LeftMargin = ActLeftMargin - 3;
      break;
    case EnvEnumerate:
      fprintf(outfile, "<LI>");
      LeftMargin = ActLeftMargin - 4;
      break;
    case EnvDescription:
      ReadToken(Token);
      if (strcmp(Token, "["))
        BackToken(Token);
      else
      {
        collect_token(Acc, "]");
        LeftMargin = ActLeftMargin - 4;
        fprintf(outfile, "<DT>%s", Acc);
      }
      fprintf(outfile, "<DD>");
      break;
    default:
      error("\\item not in a list environment");
  }
}

static void TeXBibItem(Word Index)
{
  char NumString[20], Token[TOKLEN], Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  if (CurrEnv != EnvBiblio)
    error("\\bibitem not in bibliography environment");

  assert_token("{");
  collect_token(Name, "}");

  FlushLine();
  AddLine("<DT>", "");
  ++BibCounter;

  LeftMargin = ActLeftMargin - BibIndent - 3;
  as_snprintf(Value, sizeof(Value), "<A NAME=\"cite_%s\">", Name);
  DoAddNormal(Value, "");
  as_snprintf(NumString, sizeof(NumString), "[%*d] </A><DD>", BibIndent, BibCounter);
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

static void TeXAddAsterisk(Word Index)
{
  UNUSED(Index);

  DoAddNormal("*", BackSepString);
}

static void TeXAddSSharp(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&szlig;", BackSepString);
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
  UNUSED(Index);

  DoAddNormal("&micro;", BackSepString);
}

static void TeXAddGreekPi(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&pi;", BackSepString);
}

static void TeXAddLessEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&le;", BackSepString);
}

static void TeXAddGreaterEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&ge;", BackSepString);
}

static void TeXAddNotEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&ne;", BackSepString);
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

static void TeXAddLAnd(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&and;", BackSepString);
}

static void TeXAddLOr(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&or;", BackSepString);
}

static void TeXAddOPlus(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&veebar;", BackSepString);
}

static void TeXAddRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&RightArrow;", BackSepString);
}

static void TeXAddLongRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&LongRightArrow;", BackSepString);
}

static void TeXAddLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&LeftArrow;", BackSepString);
}

static void TeXAddGets(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&LeftArrow;", BackSepString);
}

static void TeXAddLongLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&LongLeftArrow;", BackSepString);
}

static void TeXAddLeftRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&LeftRightArrow;", BackSepString);
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
  int NewFontFlags;

  NewFontFlags = (Index == FontStandard) ? 0 : CurrFontFlags | (1 << Index);
  PrFontDiff(CurrFontFlags, NewFontFlags);
  CurrFontFlags = NewFontFlags;
}

static void TeXEnvNewFontType(Word Index)
{
  char NToken[TOKLEN];

  SaveFont();
  TeXNewFontType(Index);
  assert_token("{");
  ReadToken(NToken);
  strcpy(SepString, BackSepString);
  BackToken(NToken);
}

static void TeXNewFontSize(Word Index)
{
  PrFontSize(CurrFontSize = (tFontSize) Index, True);
}

static void TeXEnvNewFontSize(Word Index)
{
  char NToken[TOKLEN];

  SaveFont();
  TeXNewFontSize(Index);
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

static void TeXEndHead(Word Index)
{
  UNUSED(Index);
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
  fputs("<P><CENTER>", outfile);
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

  if (CurrEnv != EnvTabular)
    error("\\hline outside of a table");
  if (CurrCol != 0)
    error("\\multicolumn must be in first column");

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
  char Token[TOKLEN], Erg[TOKLEN];
  PIndexSave run, prev, neu;
  UNUSED(Index);

  assert_token("{");
  collect_token(Token, "}");
  run = FirstIndex;
  prev = NULL;
  while ((run) && (strcmp(Token, run->Name) > 0))
  {
    prev = run;
    run = run->Next;
  }
  if ((!run) || (strcmp(Token, run->Name) < 0))
  {
    neu = (PIndexSave) malloc(sizeof(TIndexSave));
    neu->Next = run;
    neu->RefCnt = 1;
    neu->Name = as_strdup(Token);
    if (!prev)
      FirstIndex = neu;
    else
      prev->Next = neu;
    run = neu;
  }
  else
    run->RefCnt++;
  as_snprintf(Erg, sizeof(Erg), "<A NAME=\"index_%s_%d\"></A>", Token, run->RefCnt);
  DoAddNormal(Erg, "");
}

static void FreeIndex(void)
{
  while (FirstIndex)
  {
    PIndexSave Old = FirstIndex;
    FirstIndex = Old->Next;
    if (Old->Name)
      free(Old->Name);
    free(Old);
  }
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
  return (int)(Value * Factors[run - UnitNames]);
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
    fputc('\n', outfile);
}

static void TeXRule(Word Index)
{
  int h = GetDim(HFactors);
  char Rule[200];
  UNUSED(Index);

  GetDim(VFactors);
  as_snprintf(Rule, sizeof(Rule), "<HR WIDTH=\"%d%%\" ALIGN=LEFT>", (h * 100) / 70);
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

  PrFontDiff(CurrFontFlags, 0);
  DoAddNormal("</TD><TD NOWRAP>", "");
  PrFontDiff(0, CurrFontFlags);
}

static void TeXJmpTabStop(Word Index)
{
  UNUSED(Index);

  if (CurrEnv != EnvTabbing)
    error("tab trigger outside of tabbing environment");
  if (CurrTabStop >= TabStopCnt)
    error("not enough tab stops");

  PrFontDiff(CurrFontFlags, 0);
  DoAddNormal("</TD><TD NOWRAP>", "");
  PrFontDiff(0, CurrFontFlags);
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
  as_snprintf(Value, sizeof(Value), "<A NAME=\"ref_%s\"></A>", Name);
  DoAddNormal(Value, "");
}

static void TeXWriteRef(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN], HRef[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Name, "}");
  GetLabel(Name, Value);
  as_snprintf(HRef, sizeof(HRef), "<A HREF=\"#ref_%s\">", Name);
  DoAddNormal(HRef, BackSepString);
  DoAddNormal(Value, "");
  DoAddNormal("</A>", "");
}

static void TeXWriteCitation(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN], HRef[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Name, "}");
  GetCite(Name, Value);
  as_snprintf(HRef, sizeof(HRef), "<A HREF=\"#cite_%s\">", Name);
  DoAddNormal(HRef, BackSepString);
  as_snprintf(Name, sizeof(Name), "[%s]", Value);
  DoAddNormal(Name, "");
  DoAddNormal("</A>", "");
}

static void TeXNewParagraph(Word Index)
{
  UNUSED(Index);

  FlushLine();
  fprintf(outfile, "<P>\n");
}

static void TeXContents(Word Index)
{
  FILE *file = fopen(TocName, "r");
  char Line[200], Ref[50], *ptr, *run;
  int Level;
  UNUSED(Index);

  if (!file)
  {
    Warning("contents file not found.");
    DoRepass = True;
    return;
  }

  FlushLine();
  fprintf(outfile, "<P>\n<H1>%s</H1><P>\n", ContentsName);
  while (!feof(file))
  {
    if (!fgets(Line, 199, file))
      break;
    if ((*Line != '\0') && (*Line != '\n'))
    {
      if (!strncmp(Line, BiblioName, strlen(BiblioName)))
      {
        strcpy(Ref, "bib");
        Level = 1;
      }
      else if (!strncmp(Line, IndexName, strlen(IndexName)))
      {
        strcpy(Ref, "index");
        Level = 1;
      }
      else
      {
        ptr = Ref;
        Level = 1;
        if ((*Line) && (Line[strlen(Line) - 1] == '\n'))
          Line[strlen(Line) - 1] = '\0';
        for (run = Line; *run != '\0'; run++)
          if (*run != ' ')
            break;
        for (; *run != '\0'; run++)
          if (*run == ' ')
            break;
          else if (*run == '.')
          {
            *(ptr++) = '_';
            Level++;
          }
          else if ((*run >= '0') && (*run <= '9'))
            *(ptr++) = (*run);
          else if ((*run >= 'A') && (*run <= 'Z'))
            *(ptr++) = (*run);
        *ptr = '\0';
      }
      fprintf(outfile, "<P><H%d>", Level);
      if (*Ref != '\0')
        fprintf(outfile, "<A HREF=\"#sect_%s\">", Ref);
      fputs(Line, outfile);
      if (*Ref != '\0')
        fprintf(outfile, "</A></H%d>", Level);
      fputc('\n', outfile);
    }
  }

  fclose(file);
}

static void TeXPrintIndex(Word Index)
{
  PIndexSave run;
  int i, rz;
  UNUSED(Index);

  FlushLine();
  fprintf(outfile, "<H1><A NAME=\"sect_index\">%s</A></H1>\n", IndexName);
  AddToc(IndexName, 0);

  fputs("<TABLE SUMMARY=\"Index\" BORDER=0 CELLPADDING=5>\n", outfile);
  rz = 0;
  for (run = FirstIndex; run; run = run->Next)
  {
    if ((rz % 5) == 0)
      fputs("<TR ALIGN=LEFT>\n", outfile);
    fputs("<TD VALIGN=TOP NOWRAP>", outfile);
    fputs(run->Name, outfile);
    for (i = 0; i < run->RefCnt; i++)
      fprintf(outfile, " <A HREF=\"#index_%s_%d\">%d</A>", run->Name, i + 1, i + 1);
    fputs("</TD>\n", outfile);
    if ((rz % 5) == 4)
      fputs("</TR>\n", outfile);
    rz++;
  }
  if ((rz % 5) != 0)
    fputs("</TR>\n", outfile);
  fputs("</TABLE>\n", outfile);
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
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = "&auml;";
        break;
      case 'e':
        Repl = "&euml;";
        break;
      case 'i':
        Repl = "&iuml;";
        break;
      case 'o':
        Repl = "&ouml;";
        break;
      case 'u':
        Repl = "&uuml;";
        break;
      case 'A':
        Repl = "&Auml;";
        break;
      case 'E':
        Repl = "&Euml;";
        break;
      case 'I':
        Repl = "&Iuml;";
        break;
      case 'O':
        Repl = "&Ouml;";
        break;
      case 'U':
        Repl = "&Uuml;";
        break;
      case 's':
        Repl = "&szlig;";
        break;
      default :
        Found = False;
    }
  else
    Found = False;

  if (Found)
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
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = "&agrave;";
        break;
      case 'e':
        Repl = "&egrave;";
        break;
      case 'i':
        Repl = "&igrave;";
        break;
      case 'o':
        Repl = "&ograve;";
        break;
      case 'u':
        Repl = "&ugrave;";
        break;
      case 'A':
        Repl = "&Agrave;";
        break;
      case 'E':
        Repl = "&Egrave;";
        break;
      case 'I':
        Repl = "&Igrave;";
        break;
      case 'O':
        Repl = "&Ograve;";
        break;
      case 'U':
        Repl = "&Ugrave;";
        break;
      default:
        Found = False;
    }
  else
    Found = False;

  if (Found)
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
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = "&aacute;";
        break;
      case 'e':
        Repl = "&eacute;";
        break;
      case 'i':
        Repl = "&iacute;";
        break;
      case 'o':
        Repl = "&oacute;";
        break;
      case 'u':
        Repl = "&uacute;";
        break;
      case 'A':
        Repl = "&Aacute;";
        break;
      case 'E':
        Repl = "&Eacute;";
        break;
      case 'I':
        Repl = "&Iacute;";
        break;
      case 'O':
        Repl = "&Oacute;";
        break;
      case 'U':
        Repl = "&Uacute;";
        break;
      default:
        Found = False;
    }
  else
    Found = False;

  if (Found)
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
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'a':
        Repl = "&acirc;";
        break;
      case 'e':
        Repl = "&ecirc;";
        break;
      case 'i':
        Repl = "&icirc;";
        break;
      case 'o':
        Repl = "&ocirc;";
        break;
      case 'u':
        Repl = "&ucirc;";
        break;
      case 'A':
        Repl = "&Acirc;";
        break;
      case 'E':
        Repl = "&Ecirc;";
        break;
      case 'I':
        Repl = "&Icirc;";
        break;
      case 'O':
        Repl = "&Ocirc;";
        break;
      case 'U':
        Repl = "&Ucirc;";
        break;
      default:
        Found = False;
    }
  else
    Found = False;

  if (Found)
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
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  ReadToken(Token);
  if (*SepString == '\0')
    switch (*Token)
    {
      case 'n':
        Repl = "&ntilde;";
        break;
      case 'N':
        Repl = "&Ntilde;";
        break;
      default:
        Found = False;
    }
  else
    Found = False;

  if (Found)
  {
    if (strlen(Repl) > 1)
      memmove(Token + strlen(Repl), Token + 1, strlen(Token));
    memcpy(Token, Repl, strlen(Repl));
    strcpy(SepString, BackSepString);
  }
  else DoAddNormal("\"", BackSepString);

  BackToken(Token);
}

static void TeXCedilla(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  assert_token("{");
  collect_token(Token, "}");
  if (!strcmp(Token, "c"))
    strcpy(Token, "&ccedil;");
  if (!strcmp(Token, "C"))
    strcpy(Token, "&Ccedil;");

  DoAddNormal(Token, BackSepString);
}

static Boolean TeXNLSSpec(char *Line)
{
  Boolean Found = True;
  char *Repl = NULL;
  int cnt = 0;

  if (*SepString == '\0')
    switch (*Line)
    {
      case 'o':
        cnt = 1;
        Repl = "&oslash;";
        break;
      case 'O':
        cnt = 1;
        Repl = "&Oslash;";
        break;
      case 'a':
        switch (Line[1])
        {
          case 'a':
            cnt = 2;
            Repl = "&aring;";
            break;
          case 'e':
            cnt = 2;
            Repl = "&aelig;";
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
            Repl = "&Aring;";
            break;
          case 'E':
            cnt = 2;
            Repl = "&AElig;";
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
}

static void TeXDoPot(void)
{
  char Token[TOKLEN];

  ReadToken(Token);
  if (!strcmp(Token, "1"))
    DoAddNormal("&sup1;", BackSepString);
  else if (!strcmp(Token, "2"))
    DoAddNormal("&sup2;", BackSepString);
  else if (!strcmp(Token, "3"))
    DoAddNormal("&sup3;", BackSepString);
  else if (!strcmp(Token, "{"))
  {
    SaveFont();
    TeXNewFontType(FontSuper);
    ReadToken(Token);
    strcpy(SepString, BackSepString);
    BackToken(Token);
  }
  else
  {
    DoAddNormal("^", BackSepString);
    AddLine(Token, "");
  }
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

static void StartFile(char *Name)
{
  char comp[TOKLEN];
  struct stat st;

  /* create name ? */

  if (Structured)
  {
    as_snprintf(comp, sizeof(comp), "%s.dir/%s", outfilename, Name);
    Name = comp;
  }

  /* open file */

  if ((outfile = fopen(Name, "w")) == NULL)
  {
    perror(Name);
    exit(3);
  }

  /* write head */

  fputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n", outfile);
  fputs("<HTML>\n", outfile);
  fputs("<HEAD>\n", outfile);
  fprintf(outfile, "<META NAME=\"Author\" CONTENT=\"automatically generated by tex2html from %s\">\n", pInFileName);
  if (stat(pInFileName, &st))
    stat(Name, &st);
  strncpy(comp, ctime(&st.st_mtime), TOKLEN - 1);
  if ((*comp) && (comp[strlen(comp) - 1] == '\n'))
    comp[strlen(comp) - 1] = '\0';
  fprintf(outfile, "<META NAME=\"Last-modified\" CONTENT=\"%s\">\n", comp);
}

/*--------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  char Line[TOKLEN], Comp[TOKLEN], *p, AuxFile[200];
  int z, ergc, NumPassesLeft;

  /* assume defaults for flags */

  Structured = False;

  /* extract switches */

  ergc = 1;
  for (z = 1; z < argc; z++)
  {
    if (!strcmp(argv[z], "-w"))
      Structured = True;
    else
      argv[ergc++] = argv[z];
  }
  argc = ergc;

  /* do we want that ? */

  if (argc < 3)
  {
    fprintf(stderr, "calling convention: %s [switches] <input file> <output file>\n"
                    "switches: -w --> create structured document\n", *argv);
    exit(1);
  }

  pThisTable = (TTable*)calloc(1, sizeof(*pThisTable));

  /* save file names */

  pInFileName = argv[1];
  outfilename = argv[2];

  /* set up hash table */

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
  AddInstTable(TeXTable, "*", 0, TeXAddAsterisk);
  AddInstTable(TeXTable, "ss", 0, TeXAddSSharp);
  AddInstTable(TeXTable, "in", 0, TeXAddIn);
  AddInstTable(TeXTable, "rz", 0, TeXAddReal);
  AddInstTable(TeXTable, "mu", 0, TeXAddGreekMu);
  AddInstTable(TeXTable, "pi", 0, TeXAddGreekPi);
  AddInstTable(TeXTable, "leq", 0, TeXAddLessEq);
  AddInstTable(TeXTable, "geq", 0, TeXAddGreaterEq);
  AddInstTable(TeXTable, "neq", 0, TeXAddNotEq);
  AddInstTable(TeXTable, "mid", 0, TeXAddMid);
  AddInstTable(TeXTable, "asname", 0, TeXASName);
  AddInstTable(TeXTable, "land", 0, TeXAddLAnd);
  AddInstTable(TeXTable, "lor", 0, TeXAddLOr);
  AddInstTable(TeXTable, "oplus", 0, TeXAddOPlus);
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
  AddInstTable(TeXTable, "Huge", FontHuge, TeXNewFontSize);
  AddInstTable(TeXTable, "tin", FontTiny, TeXEnvNewFontSize);
  AddInstTable(TeXTable, "rightarrow", 0, TeXAddRightArrow);
  AddInstTable(TeXTable, "longrightarrow", 0, TeXAddLongRightArrow);
  AddInstTable(TeXTable, "leftarrow", 0, TeXAddLeftArrow);
  AddInstTable(TeXTable, "longleftarrow", 0, TeXAddLongLeftArrow);
  AddInstTable(TeXTable, "leftrightarrow", 0, TeXAddLeftRightArrow);
  AddInstTable(TeXTable, "gets", 0, TeXAddGets);
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
  AddInstTable(TeXTable, "printindex", 0, TeXPrintIndex);
  AddInstTable(TeXTable, "tableofcontents", 0, TeXContents);
  AddInstTable(TeXTable, "rule", 0, TeXRule);
  AddInstTable(TeXTable, "\"", 0, TeXNLS);
  AddInstTable(TeXTable, "`", 0, TeXNLSGrave);
  AddInstTable(TeXTable, "'", 0, TeXNLSAcute);
  AddInstTable(TeXTable, "^", 0, TeXNLSCirc);
  AddInstTable(TeXTable, "~", 0, TeXNLSTilde);
  AddInstTable(TeXTable, "c", 0, TeXCedilla);
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

    /* set up inclusion stack */

    DidEOF = False;
    IncludeNest = 0;
    *infiles = fopen(pInFileName, "r");
    if (!*infiles)
    {
      perror(pInFileName);
      exit(3);
    }
    else
      IncludeNest++;
    SetSrcDir(pInFileName);

    /* preset state variables */

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
    EnumCounter = 0;
    InListItem = False;
    InitFont();
    CurrLine = 0;
    InitLabels();
    InitCites();
    InitToc();
    FirstIndex = NULL;
    *SideMargin = '\0';
    DoRepass = False;
    BibIndent = BibCounter = 0;
    GermanMode = True;
    SetLang(False);

    /* open help files */

    strmaxcpy(TocName, pInFileName, sizeof(TocName));
    p = strrchr(TocName, '.');
    if (p)
      *p = '\0';
    strcat(TocName, ".htoc");

    strmaxcpy(AuxFile, pInFileName, sizeof(AuxFile));
    p = strrchr(AuxFile, '.');
    if (p)
      *p = '\0';
    strcat(AuxFile, ".haux");
    ReadAuxFile(AuxFile);

    if (!strcmp(outfilename, "-"))
    {
      if (Structured)
      {
        printf("%s: structured write must be to a directory\n", *argv);
        exit(1);
      }
      else
        outfile = stdout;
    }

    /* do we need to make a directory ? */

    else if (Structured)
    {
      as_snprintf(Line, sizeof(Line), "%s.dir", outfilename);
#if (defined _WIN32) && (!defined __CYGWIN32__)
      mkdir(Line);
#elif (defined __MSDOS__)
      mkdir(Line);
#else
      mkdir(Line, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
      StartFile("intro.html");
    }

    /* otherwise open the single file */

    else
      StartFile(outfilename);

    /* start to parse */

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
            fputs("</CENTER><P>\n", outfile);
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

    fputs("</HTML>\n", outfile);

    for (z = 0; z < IncludeNest; fclose(infiles[z++]));
    fclose(outfile);

    unlink(AuxFile);
    PrintLabels(AuxFile);
    PrintCites(AuxFile);
    PrintToc(TocName);

    FreeLabels();
    FreeCites();
    FreeToc();
    FreeIndex();
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
