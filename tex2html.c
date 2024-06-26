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

static char *TableName,
            *BiblioName,
            *ContentsName,
            *IndexName,
#define ErrorEntryCnt 3
            *ErrorEntryNames[ErrorEntryCnt];

static char *FontNames[FontCnt] =
{
  "", "EM", "B", "TT", "I", "SUP"
};

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

static char TocName[200];

#define CHAPMAX 6
static int Chapters[CHAPMAX];
static int TableNum, FracState, BibIndent, BibCounter;
#define TABMAX 100
static int TabStops[TABMAX], TabStopCnt, CurrTabStop;
static Boolean InAppendix, InMathMode;
static TTable *pThisTable;
static int CurrRow, CurrCol;
static char SrcDir[TOKLEN + 1];
static Boolean GermanMode;

static int Structured;

static int CurrPass;
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

static char OutLineBuffer[TOKLEN] = "", SideMargin[TOKLEN];

static void PutLine(Boolean DoBlock)
{
  int l, n, ptrcnt, diff, div, mod, divmod;
  char *chz, *ptrs[50];
  Boolean SkipFirst, IsFirst;

  fputs(Blanks(curr_tex_env_data.LeftMargin - 1), p_outfile);
  if ((curr_tex_env == EnvRaggedRight) || (!DoBlock))
  {
    fprintf(p_outfile, "%s", OutLineBuffer);
    l = strlen(OutLineBuffer);
  }
  else
  {
    SkipFirst = ((curr_tex_env == EnvItemize) || (curr_tex_env == EnvEnumerate) || (curr_tex_env == EnvDescription) || (curr_tex_env == EnvBiblio));
    if (curr_tex_env_data.LeftMargin == curr_tex_env_data.ActLeftMargin)
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
    diff = curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1 - l;
    div = (ptrcnt > 0) ? diff / ptrcnt : 0;
    mod = diff - (ptrcnt * div);
    divmod = (mod > 0) ? ptrcnt / mod : ptrcnt + 1;
    IsFirst = SkipFirst;
    ptrcnt = 0;
    for (chz = OutLineBuffer; *chz != '\0'; chz++)
    {
      fputc(*chz, p_outfile);
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
            fputs(Blanks(n), p_outfile);
          ptrcnt++;
        }
        IsFirst = False;
      }
    }
    l = curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1;
  }
  if (*SideMargin != '\0')
  {
    fputs(Blanks(curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 4 - l), p_outfile);
#if 0
    fprintf(p_outfile, "%s", SideMargin);
#endif
    *SideMargin = '\0';
  }
  fputc('\n', p_outfile);
  curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin;
}

static void AddLine(const char *Part, char *Sep)
{
  int mlen = curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1;
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
    if ((*SideMargin != '\0') || (!tex_issep(*Sep)))
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
  while (p_current_tex_output_consumer)
  {
    p_current_tex_output_consumer->consume(p_current_tex_output_consumer, (const char**)&Sep);
    if (p_current_tex_output_consumer)
      p_current_tex_output_consumer->consume(p_current_tex_output_consumer, &Part);
    if (!*Part && !*Sep)
      return;
  }

  if (!strcmp(Part, "<"))
    Part = "&lt;";
  else if (!strcmp(Part, ">"))
    Part = "&gt;";
  else if (!strcmp(Part, "&"))
    Part = "&amp;";

  switch (curr_tex_env)
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

static void InitTableRow(int Index)
{
  int z;

  for (z = 0; z < pThisTable->TColumnCount; pThisTable->Lines[Index][z++] = NULL);
  pThisTable->MultiFlags[Index] = False;
  pThisTable->LineFlags[Index] = False;
}

static void NextTableColumn(void)
{
  if (curr_tex_env != EnvTabular)
    tex_error("table separation char not within tabular environment");

  if ((pThisTable->MultiFlags[CurrRow])
   || (CurrCol >= pThisTable->TColumnCount))
    tex_error("too many columns within row");

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
  indent = (curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1 - sumlen) / 2;
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

  fprintf(p_outfile, "<P><CENTER><TABLE SUMMARY=\"No Summary\" BORDER=1 CELLPADDING=5>\n");

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

      fprintf(p_outfile, "<TR ALIGN=LEFT>\n");

      /* over all columns... */

      colptr = 0;
      for (colz = 0; colz < ((pThisTable->MultiFlags[rowz])?firsttext + 1:pThisTable->ColumnCount); colz++)
        if (pThisTable->ColTypes[colz] != ColBar)
        {
          /* start a column */

          fprintf(p_outfile, "<%s VALIGN=TOP NOWRAP", ColTag);
          if (pThisTable->MultiFlags[rowz])
            fprintf(p_outfile, " COLSPAN=%d", TextCnt);
          switch (pThisTable->ColTypes[colz])
          {
            case ColLeft:
              fputs(" ALIGN=LEFT>", p_outfile);
              break;
            case ColCenter:
              fputs(" ALIGN=CENTER>", p_outfile);
              break;
            case ColRight:
              fputs(" ALIGN=RIGHT>", p_outfile);
              break;
            default:
              break;
          }

          /* write items */

          for (rowz3 = rowz; rowz3 <= rowz2; rowz3++)
          {
            if (pThisTable->Lines[rowz3][colptr])
              fputs(pThisTable->Lines[rowz3][colptr], p_outfile);
            if (rowz3 != rowz2)
              fputs("<BR>\n", p_outfile);
          }

          /* end column */

          fprintf(p_outfile, "</%s>\n", ColTag);

          colptr++;
        }

      /* end row */

      fprintf(p_outfile, "</TR>\n");

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

  fprintf(p_outfile, "</TABLE></CENTER>\n");
}

static void GetTableName(char *Dest, size_t DestSize)
{
  int ThisTableNum = (curr_tex_env == EnvTabular) ? TableNum + 1 : TableNum;

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

static void TeXFlushLine(Word Index)
{
  UNUSED(Index);

  if (curr_tex_env == EnvTabular)
  {
    for (CurrCol++; CurrCol < pThisTable->TColumnCount; pThisTable->Lines[CurrRow][CurrCol++] = as_strdup(""));
    CurrRow++;
    if (CurrRow == MAXROWS)
      tex_error("too many rows in table");
    InitTableRow(CurrRow);
    CurrCol = 0;
  }
  else if (curr_tex_env == EnvTabbing)
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
  if (curr_tex_env == EnvTabbing)
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

  tex_assert_token("=");
  tex_read_token(Token);
}

static void TeXDummyNoBrack(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_read_token(Token);
}

static void TeXDummyInCurl(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_read_token(Token);
  tex_assert_token("}");
}

static void TeXDef(Word Index)
{
  char Token[TOKLEN];
  int level;
  UNUSED(Index);

  tex_assert_token("\\");
  tex_read_token(Token);
  tex_assert_token("{");
  level = 1;
  do
  {
    tex_read_token(Token);
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

  tex_assert_token("\\");
  tex_read_token(Token);
  tex_assert_token("=");
  tex_read_token(Token);
  tex_read_token(Token);
  tex_assert_token("\\");
  tex_read_token(Token);
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
  fputc('\n', p_outfile);

  tex_assert_token("{");
  LastLevel = Level;
  tex_save_env(EnvHeading, NULL);
  curr_tex_env_data.RightMargin = 200;

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

  fprintf(p_outfile, "<H%d>", Level + 1);

  *Line = '\0';
  if (Level < 3)
  {
    GetSectionName(Line, sizeof(Line));
    fprintf(p_outfile, "<A NAME=\"sect_");
    for (rep = Line; *rep; rep++)
      fputc((*rep == '.') ? '_' : *rep, p_outfile);
    fprintf(p_outfile, "\">");
    as_snprcatf(Line, sizeof(Line), " ");
  }
  as_snprcatf(Line, sizeof(Line), "%s", Title);

  fprintf(p_outfile, "%s", Line);

  if (Level < 3)
  {
    fputs("</A>", p_outfile);
    GetSectionName(Line, sizeof(Line));
    as_snprcatf(Line, sizeof(Line), " %s", Title);
    AddToc(Line, 0);
  }

  fprintf(p_outfile, "</H%d>\n", Level + 1);
}

static void TeXBeginEnv(Word Index)
{
  char EnvName[TOKLEN], Add[TOKLEN], *p;
  EnvType NEnv;
  Boolean done;
  TColumn NCol;
  const tex_environment_t *p_user_env;
  UNUSED(Index);

  tex_assert_token("{");
  tex_read_token(EnvName);
  if ((NEnv = tex_get_env_type(EnvName, &p_user_env)) == EnvTable)
  {
    tex_read_token(Add);
    if (!strcmp(Add, "*"))
      tex_assert_token("}");
    else if (strcmp(Add, "}"))
      tex_error("unknown table environment");
  }
  else
    tex_assert_token("}");

  if ((NEnv != EnvVerbatim) && (NEnv != EnvUser))
    tex_save_env(NEnv, EnvName);

  switch (NEnv)
  {
    case EnvDocument:
      fputs("</HEAD>\n", p_outfile);
      fputs("<BODY>\n", p_outfile);
      break;
    case EnvItemize:
      FlushLine();
      fprintf(p_outfile, "<UL>\n");
      ++curr_tex_env_data.ListDepth;
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = (curr_tex_env_data.ListDepth * 4) + 1;
      curr_tex_env_data.RightMargin = 70;
      curr_tex_env_data.EnumCounter = 0;
      curr_tex_env_data.InListItem = False;
      break;
    case EnvDescription:
      FlushLine();
      fprintf(p_outfile, "<DL COMPACT>\n");
      ++curr_tex_env_data.ListDepth;
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = (curr_tex_env_data.ListDepth * 4) + 1;
      curr_tex_env_data.RightMargin = 70;
      curr_tex_env_data.EnumCounter = 0;
      curr_tex_env_data.InListItem = False;
      break;
    case EnvEnumerate:
      FlushLine();
      fprintf(p_outfile, "<OL>\n");
      ++curr_tex_env_data.ListDepth;
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = (curr_tex_env_data.ListDepth * 4) + 1;
      curr_tex_env_data.RightMargin = 70;
      curr_tex_env_data.EnumCounter = 0;
      curr_tex_env_data.InListItem = False;
      break;
    case EnvBiblio:
      FlushLine();
      fprintf(p_outfile, "<P>\n");
      fprintf(p_outfile, "<H1><A NAME=\"sect_bib\">%s</A></H1>\n<DL COMPACT>\n", BiblioName);
      tex_assert_token("{");
      tex_read_token(Add);
      tex_assert_token("}");
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = 4 + (BibIndent = strlen(Add));
      AddToc(BiblioName, 0);
      break;
    case EnvVerbatim:
      FlushLine();
      fprintf(p_outfile, "<PRE>\n");
      if ((*buffer_line != '\0') && (*p_buffer_line_ptr != '\0'))
      {
        fprintf(p_outfile, "%s", p_buffer_line_ptr);
        *buffer_line = '\0';
        p_buffer_line_ptr = buffer_line;
      }
      do
      {
        if (!tex_infile_gets(Add, TOKLEN - 1, p_curr_tex_infile))
          break;
        p_curr_tex_infile->curr_line++;
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
          fprintf(p_outfile, "%s", Add);
        }
      }
      while (!done);
      fprintf(p_outfile, "\n</PRE>\n");
      break;
    case EnvQuote:
      FlushLine();
      fprintf(p_outfile, "<BLOCKQUOTE>\n");
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = 5;
      curr_tex_env_data.RightMargin = 70;
      break;
    case EnvTabbing:
      FlushLine();
      fputs("<TABLE SUMMARY=\"No Summary\" CELLPADDING=2>\n", p_outfile);
      TabStopCnt = 0;
      CurrTabStop = 0;
      curr_tex_env_data.RightMargin = TOKLEN - 1;
      AddLine("<TR><TD NOWRAP>", "");
      PrFontDiff(0, CurrFontFlags);
      break;
    case EnvTable:
      tex_read_token(Add);
      if (strcmp(Add, "["))
        tex_push_back_token(Add);
      else
      {
        do
        {
          tex_read_token(Add);
        }
        while (strcmp(Add, "]"));
      }
      FlushLine();
      fputc('\n', p_outfile);
      ++TableNum;
      break;
    case EnvCenter:
      FlushLine();
      fputs("<CENTER>\n", p_outfile);
      break;
    case EnvRaggedRight:
      FlushLine();
      fputs("<DIV ALIGN=LEFT>\n", p_outfile);
      break;
    case EnvRaggedLeft:
      FlushLine();
      fputs("<DIV ALIGN=RIGHT>\n", p_outfile);
      break;
    case EnvTabular:
      FlushLine();
      tex_assert_token("{");
      pThisTable->ColumnCount = pThisTable->TColumnCount = 0;
      do
      {
        tex_read_token(Add);
        done = !strcmp(Add, "}");
        if (!done)
        {
          if (pThisTable->ColumnCount >= MAXCOLS)
            tex_error("too many columns in table");
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
            tex_error("unknown table column descriptor");
          if ((pThisTable->ColTypes[pThisTable->ColumnCount++] = NCol) != ColBar)
            pThisTable->TColumnCount++;
        }
      }
      while (!done);
      InitTableRow(CurrRow = 0);
      CurrCol = 0;
      break;
    case EnvUser:
      tex_infile_push_line(EnvName, p_user_env->p_begin_commands, False);
      break;
    default:
      break;
  }
}

static void TeXEndEnv(Word Index)
{
  char EnvName[TOKLEN], Add[TOKLEN];
  EnvType NEnv;
  const tex_environment_t *p_user_env;
  UNUSED(Index);

  tex_assert_token("{");
  tex_read_token(EnvName);
  if ((NEnv = tex_get_env_type(EnvName, &p_user_env)) == EnvTable)
  {
    tex_read_token(Add);
    if (!strcmp(Add, "*"))
      tex_assert_token("}");
    else if (strcmp(Add, "}"))
      tex_error("unknown table environment");
  }
  else
    tex_assert_token("}");

  if (!p_env_stack)
    tex_error("end without begin");
  if ((curr_tex_env != NEnv) && (NEnv != EnvUser))
    tex_error("begin (%s) and end (%s) of environment do not match",
              tex_env_names[curr_tex_env], tex_env_names[NEnv]);
  if (curr_tex_env == EnvUser)
  {
    if (as_strcasecmp(EnvName, p_curr_tex_user_env_name))
      tex_error("begin (%s) and end (%s) of environment do not match",
               EnvName, p_curr_tex_user_env_name);
  }

  switch (NEnv)
  {
    case EnvDocument:
      FlushLine();
      fputs("</BODY>\n", p_outfile);
      break;
    case EnvItemize:
      if (curr_tex_env_data.InListItem)
        AddLine("</LI>", "");
      FlushLine();
      fprintf(p_outfile, "</UL>\n");
      break;
    case EnvDescription:
      if (curr_tex_env_data.InListItem)
        AddLine("</DD>", "");
      FlushLine();
      fprintf(p_outfile, "</DL>\n");
      break;
    case EnvEnumerate:
      if (curr_tex_env_data.InListItem)
        AddLine("</LI>", "");
      FlushLine();
      fprintf(p_outfile, "</OL>\n");
      break;
    case EnvQuote:
      FlushLine();
      fprintf(p_outfile, "</BLOCKQUOTE>\n");
      break;
    case EnvBiblio:
      FlushLine();
      fprintf(p_outfile, "</DL>\n");
      break;
    case EnvTabbing:
      PrFontDiff(CurrFontFlags, 0);
      AddLine("</TD></TR>", "");
      FlushLine();
      fputs("</TABLE>", p_outfile);
      break;
    case EnvCenter:
      FlushLine();
      fputs("</CENTER>\n", p_outfile);
      break;
    case EnvRaggedRight:
    case EnvRaggedLeft:
      FlushLine();
      fputs("</DIV>\n", p_outfile);
      break;
    case EnvTabular:
      DumpTable();
      break;
    case EnvTable:
      FlushLine(); fputc('\n', p_outfile);
      break;
    case EnvUser:
      tex_infile_push_line(EnvName, p_user_env->p_end_commands, False);
      break;
    default:
      break;
  }

  if (NEnv != EnvUser)
    tex_restore_env();
}

static void TeXItem(Word Index)
{
  char Token[TOKLEN], Acc[TOKLEN];
  UNUSED(Index);

  if (curr_tex_env_data.InListItem)
    AddLine((curr_tex_env == EnvDescription) ? "</DD>" : "</LI>", "");
  FlushLine();
  curr_tex_env_data.InListItem = True;
  switch(curr_tex_env)
  {
    case EnvItemize:
      fprintf(p_outfile, "<LI>");
      curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - 3;
      break;
    case EnvEnumerate:
      fprintf(p_outfile, "<LI>");
      curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - 4;
      break;
    case EnvDescription:
      tex_read_token(Token);
      if (strcmp(Token, "["))
        tex_push_back_token(Token);
      else
      {
        tex_collect_token(Acc, "]");
        curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - 4;
        fprintf(p_outfile, "<DT>%s", Acc);
      }
      fprintf(p_outfile, "<DD>");
      break;
    default:
      tex_error("\\item not in a list environment");
  }
}

static void TeXBibItem(Word Index)
{
  char NumString[20], Token[TOKLEN], Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  if (curr_tex_env != EnvBiblio)
    tex_error("\\bibitem not in bibliography environment");

  tex_assert_token("{");
  tex_collect_token(Name, "}");

  FlushLine();
  AddLine("<DT>", "");
  ++BibCounter;

  curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - BibIndent - 3;
  as_snprintf(Value, sizeof(Value), "<A NAME=\"cite_%s\">", Name);
  DoAddNormal(Value, "");
  as_snprintf(NumString, sizeof(NumString), "[%*d] </A><DD>", BibIndent, BibCounter);
  AddLine(NumString, "");
  as_snprintf(NumString, sizeof(NumString), "%d", BibCounter);
  AddCite(Name, NumString);
  tex_read_token(Token);
  *tex_token_sep_string = '\0';
  tex_push_back_token(Token);
}

static void TeXAddDollar(Word Index)
{
  UNUSED(Index);

  DoAddNormal("$", tex_backslash_token_sep_string);
}

static void TeXAddUnderbar(Word Index)
{
  UNUSED(Index);

  DoAddNormal("_", tex_backslash_token_sep_string);
}

#if 0
static void TeXAddPot(Word Index)
{
  UNUSED(Index);

  DoAddNormal("^", tex_backslash_token_sep_string);
}
#endif

static void TeXAddAmpersand(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&", tex_backslash_token_sep_string);
}

static void TeXAddAt(Word Index)
{
  UNUSED(Index);

  DoAddNormal("@", tex_backslash_token_sep_string);
}

static void TeXAddImm(Word Index)
{
  UNUSED(Index);

  DoAddNormal("#", tex_backslash_token_sep_string);
}

static void TeXAddPercent(Word Index)
{
  UNUSED(Index);

  DoAddNormal("%", tex_backslash_token_sep_string);
}

static void TeXAddAsterisk(Word Index)
{
  UNUSED(Index);

  DoAddNormal("*", tex_backslash_token_sep_string);
}

static void TeXAddSSharp(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&szlig;", tex_backslash_token_sep_string);
}

static void TeXAddIn(Word Index)
{
  UNUSED(Index);

  DoAddNormal("in", tex_backslash_token_sep_string);
}

static void TeXAddReal(Word Index)
{
  UNUSED(Index);

  DoAddNormal("R", tex_backslash_token_sep_string);
}

static void TeXAddGreekMu(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&micro;", tex_backslash_token_sep_string);
}

static void TeXAddGreekPi(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&pi;", tex_backslash_token_sep_string);
}

static void TeXAddLessEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&le;", tex_backslash_token_sep_string);
}

static void TeXAddGreaterEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&ge;", tex_backslash_token_sep_string);
}

static void TeXAddNotEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&ne;", tex_backslash_token_sep_string);
}

static void TeXAddMid(Word Index)
{
  UNUSED(Index);

  DoAddNormal("|", tex_backslash_token_sep_string);
}

static void TeXAddLAnd(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&and;", tex_backslash_token_sep_string);
}

static void TeXAddLOr(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&or;", tex_backslash_token_sep_string);
}

static void TeXAddOPlus(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&veebar;", tex_backslash_token_sep_string);
}

static void TeXAddRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&rarr;", tex_backslash_token_sep_string);
}

static void TeXAddLongRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&#10230;", tex_backslash_token_sep_string);
}

static void TeXAddLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&larr;", tex_backslash_token_sep_string);
}

static void TeXAddGets(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&larr;", tex_backslash_token_sep_string);
}

static void TeXAddLongLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&#10229;", tex_backslash_token_sep_string);
}

static void TeXAddLeftRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&harr;", tex_backslash_token_sep_string);
}

static void TeXDoFrac(Word Index)
{
  UNUSED(Index);

  tex_assert_token("{");
  *tex_token_sep_string = '\0';
  tex_push_back_token("(");
  FracState = 0;
}

static void NextFracState(void)
{
  if (FracState == 0)
  {
    tex_assert_token("{");
    *tex_token_sep_string = '\0';
    tex_push_back_token(")");
    tex_push_back_token("/");
    tex_push_back_token("(");
  }
  else if (FracState == 1)
  {
    *tex_token_sep_string = '\0';
    tex_push_back_token(")");
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
  tex_assert_token("{");
  tex_read_token(NToken);
  strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  tex_push_back_token(NToken);
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
  tex_assert_token("{");
  tex_read_token(NToken);
  strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  tex_push_back_token(NToken);
}

static void TeXAddMarginPar(Word Index)
{
  UNUSED(Index);

  tex_assert_token("{");
  tex_save_env(EnvMarginPar, NULL);
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

  tex_assert_token("{");
  if ((curr_tex_env != EnvTable) && (curr_tex_env != EnvTabular))
    tex_error("caption outside of a table");
  FlushLine();
  fputs("<P><CENTER>", p_outfile);
  GetTableName(tmp, sizeof(tmp));
  tex_save_env(EnvCaption, NULL);
  AddLine(TableName, "");
  cnt = strlen(TableName);
  strcat(tmp, ": ");
  AddLine(tmp, " ");
  cnt += 1 + strlen(tmp);
  curr_tex_env_data.LeftMargin = 1;
  curr_tex_env_data.ActLeftMargin = cnt + 1;
  curr_tex_env_data.RightMargin = 70;
}

static void TeXHorLine(Word Index)
{
  UNUSED(Index);

  if (curr_tex_env != EnvTabular)
    tex_error("\\hline outside of a table");

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

  if (curr_tex_env != EnvTabular)
    tex_error("\\hline outside of a table");
  if (CurrCol != 0)
    tex_error("\\multicolumn must be in first column");

  tex_assert_token("{");
  tex_read_token(Token);
  tex_assert_token("}");
  cnt = strtol(Token, &endptr, 10);
  if (*endptr != '\0')
    tex_error("invalid numeric format to \\multicolumn");
  if (cnt != pThisTable->TColumnCount)
    tex_error("\\multicolumn must span entire table");
  tex_assert_token("{");
  do
  {
    tex_read_token(Token);
  }
  while (strcmp(Token, "}"));
  pThisTable->MultiFlags[CurrRow] = True;
}

static void TeXIndex(Word Index)
{
  char Token[TOKLEN], Erg[TOKLEN];
  PIndexSave run, prev, neu;
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Token, "}");
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

  tex_assert_token("{");
  tex_collect_token(Acc, "}");
  for (run = UnitNames; **run != '\0'; run++)
    if (!strcmp(*run, Acc + strlen(Acc) - strlen(*run)))
      break;
  if (**run == '\0')
    tex_error("unknown unit for dimension");
  Acc[strlen(Acc) - strlen(*run)] = '\0';
  Value = strtod(Acc, &endptr);
  if (*endptr != '\0')
    tex_error("invalid numeric format for dimension");
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
    fputc('\n', p_outfile);
}

static void TeXRule(Word Index)
{
  int h = GetDim(HFactors);
  char Rule[200];
  UNUSED(Index);

  GetDim(VFactors);
  as_snprintf(Rule, sizeof(Rule), "<HR WIDTH=\"%d%%\" ALIGN=LEFT>", (h * 100) / 70);
  DoAddNormal(Rule, tex_backslash_token_sep_string);
}

static void TeXAddTabStop(Word Index)
{
  int z, n, p;
  UNUSED(Index);

  if (curr_tex_env != EnvTabbing)
    tex_error("tab marker outside of tabbing environment");
  if (TabStopCnt >= TABMAX)
    tex_error("too many tab stops");

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

  if (curr_tex_env != EnvTabbing)
    tex_error("tab trigger outside of tabbing environment");
  if (CurrTabStop >= TabStopCnt)
    tex_error("not enough tab stops");

  PrFontDiff(CurrFontFlags, 0);
  DoAddNormal("</TD><TD NOWRAP>", "");
  PrFontDiff(0, CurrFontFlags);
  CurrTabStop++;
}

static void TeXDoVerb(Word Index)
{
  char Token[TOKLEN], *pos, Marker;
  UNUSED(Index);

  tex_read_token(Token);
  if (*tex_token_sep_string != '\0')
    tex_error("invalid control character for \\verb");
  Marker = (*Token);
  strmov(Token, Token + 1);
  strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  do
  {
    DoAddNormal(tex_token_sep_string, "");
    pos = strchr(Token, Marker);
    if (pos)
    {
      *pos = '\0';
      DoAddNormal(Token, "");
      *tex_token_sep_string = '\0';
      tex_push_back_token(pos + 1);
      break;
    }
    else
    {
      DoAddNormal(Token, "");
      tex_read_token(Token);
    }
  }
  while (True);
}

static void TeXWriteLabel(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Name, "}");

  if ((curr_tex_env == EnvCaption) || (curr_tex_env == EnvTabular))
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

  tex_assert_token("{");
  tex_collect_token(Name, "}");
  GetLabel(Name, Value);
  as_snprintf(HRef, sizeof(HRef), "<A HREF=\"#ref_%s\">", Name);
  DoAddNormal(HRef, tex_backslash_token_sep_string);
  DoAddNormal(Value, "");
  DoAddNormal("</A>", "");
}

static void TeXWriteCitation(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN], HRef[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Name, "}");
  GetCite(Name, Value);
  as_snprintf(HRef, sizeof(HRef), "<A HREF=\"#cite_%s\">", Name);
  DoAddNormal(HRef, tex_backslash_token_sep_string);
  as_snprintf(Name, sizeof(Name), "[%s]", Value);
  DoAddNormal(Name, "");
  DoAddNormal("</A>", "");
}

static void TeXNewParagraph(Word Index)
{
  UNUSED(Index);

  FlushLine();
  fprintf(p_outfile, "<P>\n");
}

static void TeXContents(Word Index)
{
  FILE *file = fopen(TocName, "r");
  char Line[200], Ref[50], *ptr, *run;
  int Level;
  UNUSED(Index);

  if (!file)
  {
    tex_warning("contents file not found.");
    DoRepass = True;
    return;
  }

  FlushLine();
  fprintf(p_outfile, "<P>\n<H1>%s</H1><P>\n", ContentsName);
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
      fprintf(p_outfile, "<P><H%d>", Level);
      if (*Ref != '\0')
        fprintf(p_outfile, "<A HREF=\"#sect_%s\">", Ref);
      fputs(Line, p_outfile);
      if (*Ref != '\0')
        fprintf(p_outfile, "</A></H%d>", Level);
      fputc('\n', p_outfile);
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
  fprintf(p_outfile, "<H1><A NAME=\"sect_index\">%s</A></H1>\n", IndexName);
  AddToc(IndexName, 0);

  fputs("<TABLE SUMMARY=\"Index\" BORDER=0 CELLPADDING=5>\n", p_outfile);
  rz = 0;
  for (run = FirstIndex; run; run = run->Next)
  {
    if ((rz % 5) == 0)
      fputs("<TR ALIGN=LEFT>\n", p_outfile);
    fputs("<TD VALIGN=TOP NOWRAP>", p_outfile);
    fputs(run->Name, p_outfile);
    for (i = 0; i < run->RefCnt; i++)
      fprintf(p_outfile, " <A HREF=\"#index_%s_%d\">%d</A>", run->Name, i + 1, i + 1);
    fputs("</TD>\n", p_outfile);
    if ((rz % 5) == 4)
      fputs("</TR>\n", p_outfile);
    rz++;
  }
  if ((rz % 5) != 0)
    fputs("</TR>\n", p_outfile);
  fputs("</TABLE>\n", p_outfile);
}

static void TeXParSkip(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_read_token(Token);
  do
  {
    tex_read_token(Token);
    if ((!strncmp(Token, "plus", 4)) || (!strncmp(Token, "minus", 5)))
    {
    }
    else
    {
      tex_push_back_token(Token);
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
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSGrave(Word Index)
{
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSAcute(Word Index)
{
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSCirc(Word Index)
{
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSTilde(Word Index)
{
  char Token[TOKLEN], *Repl = "";
  Boolean Found = True;
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXCedilla(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Token, "}");
  if (!strcmp(Token, "c"))
    strcpy(Token, "&ccedil;");
  if (!strcmp(Token, "C"))
    strcpy(Token, "&Ccedil;");

  DoAddNormal(Token, tex_backslash_token_sep_string);
}

static Boolean TeXNLSSpec(char *Line)
{
  Boolean Found = True;
  char *Repl = NULL;
  int cnt = 0;

  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Line);
  return Found;
}

static void TeXHyphenation(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Token, "}");
}

static void TeXDoPot(void)
{
  char Token[TOKLEN];

  tex_read_token(Token);
  if (!strcmp(Token, "1"))
    DoAddNormal("&sup1;", tex_backslash_token_sep_string);
  else if (!strcmp(Token, "2"))
    DoAddNormal("&sup2;", tex_backslash_token_sep_string);
  else if (!strcmp(Token, "3"))
    DoAddNormal("&sup3;", tex_backslash_token_sep_string);
  else if (!strcmp(Token, "{"))
  {
    SaveFont();
    TeXNewFontType(FontSuper);
    tex_read_token(Token);
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
    tex_push_back_token(Token);
  }
  else
  {
    DoAddNormal("^", tex_backslash_token_sep_string);
    AddLine(Token, "");
  }
}

static void TeXDoSpec(void)
{
  strcpy(tex_backslash_token_sep_string, tex_token_sep_string);
  TeXNLS(0);
}

static void TeXInclude(Word Index)
{
  char Token[2 * TOKLEN + 1];
  UNUSED(Index);

  tex_assert_token("{");
  strcpy(Token, SrcDir);
  tex_collect_token(Token + strlen(Token), "}");
  tex_infile_push_file(Token);
}

static void TeXDocumentStyle(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_read_token(Token);
  if (!strcmp(Token, "["))
  {
    do
    {
      tex_read_token(Token);
      if (!strcmp(Token, "german"))
        SetLang(True);
    }
    while (strcmp(Token, "]"));
    tex_assert_token("{");
    tex_read_token(Token);
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
    tex_assert_token("}");
  }
}

/*!------------------------------------------------------------------------
 * \fn     TeXUsePackage(Word Index)
 * \brief  parse \usepackage command
 * ------------------------------------------------------------------------ */

static void TeXUsePackage(Word Index)
{
  char Token[TOKLEN];
  Boolean read_german_opt = False;

  UNUSED(Index);

  while (True)
  {
    tex_read_token(Token);
    if (!strcmp(Token, "["))
    {
      do
      {
        tex_read_token(Token);
        if (!strcmp(Token, "german"))
          read_german_opt = True;
      }
      while (strcmp(Token, "]"));
    }
    else if (!strcmp(Token, "{"))
    {
      tex_read_token(Token);
      if (!as_strcasecmp(Token, "german"))
        SetLang(True);
      else if (!as_strcasecmp(Token, "babel"))
        SetLang(read_german_opt);
      else if (!as_strcasecmp(Token, "makeidx"));
      else if (!as_strcasecmp(Token, "hyperref"));
      else if (!as_strcasecmp(Token, "longtable"));
      else
        tex_error("unknown package '%s'", Token);
      tex_assert_token("}");
      break;
    }
    else
      tex_error("expecting [ or { after \\usepackage");
  }
}

/*!------------------------------------------------------------------------
 * \fn     TeXAlph(Word index)
 * \brief  parse \alph command
 * ------------------------------------------------------------------------ */

static void TeXAlph(Word index)
{
  char counter_name[TOKLEN];
  unsigned value;

  UNUSED(index);

  tex_assert_token("{");
  tex_read_token(counter_name);
  tex_assert_token("}");
  value = tex_counter_get(counter_name);
  if (value > 26)
    tex_warning("'%s': counter out of range for \\alph", counter_name);
  else if ((value > 0) && tex_if_query())
  {
    char str[2] = " ";
    str[0] = (value - 1) + 'a';
    DoAddNormal(str, tex_backslash_token_sep_string);
  }
}

/*!------------------------------------------------------------------------
 * \fn     TeXValue(Word index)
 * \brief  parse \value command
 * ------------------------------------------------------------------------ */

static void TeXValue(Word index)
{
  char counter_name[TOKLEN], str[20];
  unsigned value;

  UNUSED(index);

  tex_assert_token("{");
  tex_read_token(counter_name);
  tex_assert_token("}");
  value = tex_counter_get(counter_name);
  as_snprintf(str, sizeof(str), "%u", value);
  DoAddNormal(str, tex_backslash_token_sep_string);
}
 
static void StartFile(const char *Name)
{
  char comp[TOKLEN];
  struct stat st;

  /* create name ? */

  if (Structured)
  {
    as_snprintf(comp, sizeof(comp), "%s.dir/%s", p_outfile_name, Name);
    Name = comp;
  }

  /* open file */

  if ((p_outfile = fopen(Name, "w")) == NULL)
  {
    tex_error(Name);
    exit(3);
  }

  /* write head */

  fputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n", p_outfile);
  fputs("<HTML>\n", p_outfile);
  fputs("<HEAD>\n", p_outfile);
  fprintf(p_outfile, "<META NAME=\"Author\" CONTENT=\"automatically generated by tex2html from %s\">\n", p_infile_name);
  if (stat(p_infile_name, &st))
    stat(Name, &st);
  strncpy(comp, ctime(&st.st_mtime), TOKLEN - 1);
  if ((*comp) && (comp[strlen(comp) - 1] == '\n'))
    comp[strlen(comp) - 1] = '\0';
  fprintf(p_outfile, "<META NAME=\"Last-modified\" CONTENT=\"%s\">\n", comp);
}

/*--------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  char Line[TOKLEN], *p, AuxFile[200];
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

  p_infile_name = argv[1];
  p_outfile_name = argv[2];

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
  AddInstTable(TeXTable, "newif", 0, TeXNewIf);
  AddInstTable(TeXTable, "fi", 0, TeXFi);
  AddInstTable(TeXTable, "input", 0, TeXInclude);
  AddInstTable(TeXTable, "newcounter", 0, TeXNewCounter);
  AddInstTable(TeXTable, "stepcounter", 0, TeXStepCounter);
  AddInstTable(TeXTable, "setcounter", 0, TeXSetCounter);
  AddInstTable(TeXTable, "newenvironment", 0, TeXNewEnvironment);
  AddInstTable(TeXTable, "value", 0, TeXValue);
  AddInstTable(TeXTable, "alph", 0, TeXAlph);
  AddInstTable(TeXTable, "ifnum", 0, TeXIfNum);

  CurrPass = 0;
  NumPassesLeft = 3;
  do
  {
    CurrPass++;

    /* set up inclusion stack */

    tex_token_reset();
    tex_infile_push_file(argv[1]);
    SetSrcDir(p_infile_name);

    /* preset state variables */

    for (z = 0; z < CHAPMAX; Chapters[z++] = 0);
    TableNum = 0;
    TabStopCnt = 0;
    CurrTabStop = 0;
    FracState = -1;
    InAppendix = False;
    p_env_stack = NULL;
    curr_tex_env = EnvNone;
    curr_tex_env_data.ListDepth = 0;
    curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = 1;
    curr_tex_env_data.RightMargin = 70;
    curr_tex_env_data.EnumCounter = 0;
    curr_tex_env_data.InListItem = False;
    InitFont();
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

    strmaxcpy(TocName, p_infile_name, sizeof(TocName));
    p = strrchr(TocName, '.');
    if (p)
      *p = '\0';
    strcat(TocName, ".htoc");

    strmaxcpy(AuxFile, p_infile_name, sizeof(AuxFile));
    p = strrchr(AuxFile, '.');
    if (p)
      *p = '\0';
    strcat(AuxFile, ".haux");
    ReadAuxFile(AuxFile);

    if (!strcmp(p_outfile_name, "-"))
    {
      if (Structured)
      {
        printf("%s: structured write must be to a directory\n", *argv);
        exit(1);
      }
      else
        p_outfile = stdout;
    }

    /* do we need to make a directory ? */

    else if (Structured)
    {
      as_snprintf(Line, sizeof(Line), "%s.dir", p_outfile_name);
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
      StartFile(p_outfile_name);

    /* start to parse */

    while (1)
    {
      if (!tex_read_token(Line))
        break;
      if (!strcmp(Line, "\\"))
      {
        strcpy(tex_backslash_token_sep_string, tex_token_sep_string);
        if (!tex_read_token(Line))
          tex_error("unexpected end of file");
        if (*tex_token_sep_string != '\0')
          tex_push_back_token(Line);
        else if (LookupInstTable(TeXTable, Line));
        else if (tex_newif_lookup(Line));
        else
        {
          const tex_newcommand_t *p_cmd = tex_newcommand_lookup(Line);

          if (p_cmd)
          {
            DoAddNormal("", tex_backslash_token_sep_string);
            tex_newcommand_expand_push(p_cmd);
          }
          else if (!TeXNLSSpec(Line))
            tex_warning("unknown TeX command %s", Line);
        }
      }
      else if (!strcmp(Line, "$"))
      {
        InMathMode = !InMathMode;
        if (InMathMode)
        {
          strcpy(tex_backslash_token_sep_string, tex_token_sep_string);
          tex_read_token(Line);
          strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
          tex_push_back_token(Line);
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
        if (curr_tex_env_data.FontNest > 0)
          RestoreFont();
        else if (FracState >= 0)
          NextFracState();
        else switch (curr_tex_env)
        {
          case EnvMarginPar:
            tex_restore_env();
            break;
          case EnvCaption:
            FlushLine();
            fputs("</CENTER><P>\n", p_outfile);
            tex_restore_env();
            break;
          case EnvHeading:
            EndSectionHeading();
            tex_restore_env();
            break;
          default:
            RestoreFont();
        }
      }
      else
        DoAddNormal(Line, tex_token_sep_string);
    }
    FlushLine();

    fputs("</HTML>\n", p_outfile);

    tex_infile_pop_all();
    fclose(p_outfile); p_outfile = NULL;

    unlink(AuxFile);
    PrintLabels(AuxFile);
    PrintCites(AuxFile);
    PrintToc(TocName);

    FreeLabels();
    FreeCites();
    tex_counters_free();
    tex_newifs_free();
    tex_environments_free();
    tex_newcommands_free();
    tex_output_consumer_pop_all();
    tex_if_pop_all();
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
