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

static char *TableName,
            *BiblioName,
            *ContentsName,
#define ErrorEntryCnt 3
            *ErrorEntryNames[ErrorEntryCnt];

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

static char TocName[200];
static char SrcDir[TOKLEN + 1];

#define CHAPMAX 6
static int Chapters[CHAPMAX];
static int TableNum, FracState, BibIndent, BibCounter;
#define TABMAX 100
static int TabStops[TABMAX], TabStopCnt, CurrTabStop;
static Boolean InAppendix, InMathMode;
static TTable *pThisTable;
static int CurrRow, CurrCol;
static Boolean GermanMode;

static int CurrPass;

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
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_ae, Buf), p_outfile);
  else if (ch == *CHR_oe)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_oe, Buf), p_outfile);
  else if (ch == *CHR_ue)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_ue, Buf), p_outfile);
  else if (ch == *CHR_AE)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_Ae, Buf), p_outfile);
  else if (ch == *CHR_OE)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_Oe, Buf), p_outfile);
  else if (ch == *CHR_UE)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_Ue, Buf), p_outfile);
  else if (ch == *CHR_sz)
    fputs(CharTab_GetNULTermString(pCharacterTab, eCH_sz, Buf), p_outfile);
  else
    fputc(ch, p_outfile);
}

static void outs(const char *pStr)
{
  while (*pStr)
    outc(*pStr++);
}

static char OutLineBuffer[TOKLEN] = "", SideMargin[TOKLEN];

static void PutLine(Boolean DoBlock)
{
  int ll = curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1;
  int l, n, ptrcnt, diff, div, mod, divmod;
  char *chz, *ptrs[50];
  Boolean SkipFirst, IsFirst;

  outs(Blanks(curr_tex_env_data.LeftMargin - 1));
  if ((curr_tex_env_data.Alignment != AlignNone) || (!DoBlock))
  {
    l = visible_strlen(OutLineBuffer);
    diff = ll - l;
    switch (curr_tex_env_data.Alignment)
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
    l = curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1;
  }
  if (*SideMargin != '\0')
  {
    outs(Blanks(ll + 3 - l));
    outs(SideMargin);
    *SideMargin = '\0';
  }
  outc('\n');
  curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin;
}

static void AddLine(const char *Part, char *Sep)
{
  int mlen = curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1, *hyppos, hypcnt, z, hlen, vlen;
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
  indent = (curr_tex_env_data.RightMargin - curr_tex_env_data.LeftMargin + 1 - sumlen) / 2;
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
    outs(Blanks(curr_tex_env_data.LeftMargin - 1 + indent));
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
  while (p_current_tex_output_consumer)
  {
    p_current_tex_output_consumer->consume(p_current_tex_output_consumer, (const char**)&Sep);
    if (p_current_tex_output_consumer)
      p_current_tex_output_consumer->consume(p_current_tex_output_consumer, &Part);
    if (!*Part && !*Sep)
      return;
  }
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
  else
  {
    if (*OutLineBuffer == '\0')
      strcpy(OutLineBuffer, " ");
    FlushLine();
  }
  if (curr_tex_env == EnvTabbing)
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

  tex_read_token(Token);
}

static void TeXDummyEqual(Word Index)
{
  char Token[TOKLEN];
  UNUSED(Index);

  tex_assert_token("=");
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
  outc('\n');

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

static void TeXBeginEnv(Word Index)
{
  char EnvName[TOKLEN], Add[TOKLEN];
  EnvType NEnv;
  Boolean done;
  TColumn NCol;
  int z;
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
    case EnvItemize:
    case EnvEnumerate:
    case EnvDescription:
      FlushLine();
      if (curr_tex_env_data.ListDepth == 0)
        outc('\n');
      ++curr_tex_env_data.ListDepth;
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = (curr_tex_env_data.ListDepth * 4) + 1;
      curr_tex_env_data.RightMargin = 70;
      curr_tex_env_data.EnumCounter = 0;
      break;
    case EnvBiblio:
      FlushLine(); outc('\n');
      outs("        ");
      outs(BiblioName);
      outs("\n        ");
      for (z = 0; z < (int)strlen(BiblioName); z++)
        outc('=');
      outc('\n');
      tex_assert_token("{");
      tex_read_token(Add);
      tex_assert_token("}");
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = 4 + (BibIndent = strlen(Add));
      break;
    case EnvVerbatim:
      FlushLine();
      if ((*buffer_line != '\0') && (*p_buffer_line_ptr != '\0'))
      {
        outs(p_buffer_line_ptr);
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
          outs(Add);
      }
      while (!done);
      outc('\n');
      break;
    case EnvQuote:
      FlushLine();
      outc('\n');
      curr_tex_env_data.ActLeftMargin = curr_tex_env_data.LeftMargin = 5;
      curr_tex_env_data.RightMargin = 70;
      break;
    case EnvTabbing:
      FlushLine();
      outc('\n');
      TabStopCnt = 0;
      CurrTabStop = 0;
      break;
    case EnvTable:
      tex_read_token(Add);
      if (strcmp(Add, "["))
        tex_push_back_token(Add);
      else
        do
        {
          tex_read_token(Add);
        }
        while (strcmp(Add, "]"));
      FlushLine();
      outc('\n');
      ++TableNum;
      break;
    case EnvCenter:
      FlushLine();
      curr_tex_env_data.Alignment = AlignCenter;
      break;
    case EnvRaggedRight:
      FlushLine();
      curr_tex_env_data.Alignment = AlignLeft;
      break;
    case EnvRaggedLeft:
      FlushLine();
      curr_tex_env_data.Alignment = AlignRight;
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
            tex_error("unknown table column descriptor");
          }
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
    case EnvItemize:
    case EnvEnumerate:
    case EnvDescription:
      FlushLine();
      if (curr_tex_env_data.ListDepth == 1)
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
  char NumString[20], Token[TOKLEN], Acc[TOKLEN];
  UNUSED(Index);

  FlushLine();
  switch(curr_tex_env)
  {
    case EnvItemize:
      curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - 3;
      AddLine(" - ", "");
      break;
    case EnvEnumerate:
      curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - 4;
      as_snprintf(NumString, sizeof(NumString), "%3d ", ++curr_tex_env_data.EnumCounter);
      AddLine(NumString, "");
      break;
    case EnvDescription:
      tex_read_token(Token);
      if (strcmp(Token, "[")) tex_push_back_token(Token);
      else
      {
        tex_collect_token(Acc, "]");
        curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - 4;
        as_snprintf(NumString, sizeof(NumString), "%3s ", Acc);
        AddLine(NumString, "");
      }
      break;
    default:
      tex_error("\\item not in a list environment");
  }
}

static void TeXBibItem(Word Index)
{
  char NumString[20], Token[TOKLEN], Name[TOKLEN], Format[10];
  UNUSED(Index);

  if (curr_tex_env != EnvBiblio)
    tex_error("\\bibitem not in bibliography environment");

  tex_assert_token("{");
  tex_collect_token(Name, "}");

  FlushLine();
  outc('\n');
  ++BibCounter;

  curr_tex_env_data.LeftMargin = curr_tex_env_data.ActLeftMargin - BibIndent - 3;
  as_snprintf(Format, sizeof(Format), "[%%%dd] ", BibIndent);
  as_snprintf(NumString, sizeof(NumString), Format, BibCounter);
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

static void TeXAddSSharp(Word Index)
{
  UNUSED(Index);

  DoAddNormal(HYPHEN_CHR_sz, tex_backslash_token_sep_string);
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
  char Buf[3];

  UNUSED(Index);

  DoAddNormal(CharTab_GetNULTermString(pCharacterTab, eCH_mu, Buf), tex_backslash_token_sep_string);
}

static void TeXAddGreekPi(Word Index)
{
  UNUSED(Index);

  DoAddNormal("Pi", tex_backslash_token_sep_string);
}

static void TeXAddLessEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<=", tex_backslash_token_sep_string);
}

static void TeXAddGreaterEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal(">=", tex_backslash_token_sep_string);
}

static void TeXAddNotEq(Word Index)
{
  UNUSED(Index);

  DoAddNormal("!=", tex_backslash_token_sep_string);
}

static void TeXAddLAnd(Word Index)
{
  UNUSED(Index);

  DoAddNormal("&", tex_backslash_token_sep_string);
}

static void TeXAddLOr(Word Index)
{
  UNUSED(Index);

  DoAddNormal("|", tex_backslash_token_sep_string);
}

static void TeXAddOPlus(Word Index)
{
  UNUSED(Index);

  DoAddNormal("^", tex_backslash_token_sep_string);
}

static void TeXAddMid(Word Index)
{
  UNUSED(Index);

  DoAddNormal("|", tex_backslash_token_sep_string);
}

static void TeXAddRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("->", tex_backslash_token_sep_string);
}

static void TeXAddLongRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("-->", tex_backslash_token_sep_string);
}

static void TeXAddLongLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<--", tex_backslash_token_sep_string);
}

static void TeXAddLeftArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<-", tex_backslash_token_sep_string);
}

static void TeXAddGets(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<-", tex_backslash_token_sep_string);
}

static void TeXAddLeftRightArrow(Word Index)
{
  UNUSED(Index);

  DoAddNormal("<->", tex_backslash_token_sep_string);
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
  CurrFontType = (tFontType) Index;
}

static void TeXEnvNewFontType(Word Index)
{
  char NToken[TOKLEN];

  SaveFont();
  CurrFontType = (tFontType) Index;
  tex_assert_token("{");
  tex_read_token(NToken);
  strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  tex_push_back_token(NToken);
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

static void TeXAddCaption(Word Index)
{
  char tmp[100];
  int cnt;
  UNUSED(Index);

  tex_assert_token("{");
  if ((curr_tex_env != EnvTable) && (curr_tex_env != EnvTabular))
    tex_error("caption outside of a table");
  FlushLine();
  outc('\n');
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

static void TeXEndHead(Word Index)
{
  UNUSED(Index);
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

  if (curr_tex_env != EnvTabular) tex_error("\\multicolumn outside of a table");
  if (CurrCol != 0) tex_error("\\multicolumn must be in first column");

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
  char Token[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  do
  {
    tex_read_token(Token);
  }
  while (strcmp(Token, "}"));
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
}

static void TeXJmpTabStop(Word Index)
{
  int diff;
  UNUSED(Index);

  if (curr_tex_env != EnvTabbing)
    tex_error("tab trigger outside of tabbing environment");
  if (CurrTabStop >= TabStopCnt)
    tex_error("not enough tab stops");

  diff = TabStops[CurrTabStop] - strlen(OutLineBuffer);
  if (diff > 0)
    DoAddNormal(Blanks(diff), "");
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
}

static void TeXWriteRef(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Name, "}");
  GetLabel(Name, Value);
  DoAddNormal(Value, tex_backslash_token_sep_string);
}

static void TeXWriteCitation(Word Index)
{
  char Name[TOKLEN], Value[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Name, "}");
  GetCite(Name, Value);
  as_snprintf(Name, sizeof(Name), "[%s]", Value);
  DoAddNormal(Name, tex_backslash_token_sep_string);
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
    tex_warning("contents file not found.");
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
  char Token[TOKLEN], Buf[3];
  const char *Repl = NULL;
  UNUSED(Index);

  /* NOTE: For characters relevant to hyphenation, insert the
           (codepage-independent) hyphen characters at this place.
           Transformation to codepage-dependent character takes
           place @ output: */

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSGrave(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = NULL;
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSAcute(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = NULL;
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSCirc(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = "";
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXNLSTilde(Word Index)
{
  char Token[TOKLEN], Buf[3];
  const char *Repl = "";
  UNUSED(Index);

  *Token = '\0';
  tex_read_token(Token);
  if (*tex_token_sep_string == '\0')
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
    strcpy(tex_token_sep_string, tex_backslash_token_sep_string);
  }
  else
    DoAddNormal("\"", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
}

static void TeXCedilla(Word Index)
{
  char Token[TOKLEN], Buf[3];
  UNUSED(Index);

  tex_assert_token("{");
  tex_collect_token(Token, "}");
  if (!strcmp(Token, "c"))
    strcpy(Token, CharTab_GetNULTermString(pCharacterTab, eCH_ccedil, Buf));
  if (!strcmp(Token, "C"))
    strcpy(Token, CharTab_GetNULTermString(pCharacterTab, eCH_Ccedil, Buf));

  DoAddNormal(Token, tex_backslash_token_sep_string);
}

static void TeXAsterisk(Word Index)
{
  (void)Index;
  DoAddNormal("*", tex_backslash_token_sep_string);
}

static Boolean TeXNLSSpec(char *Line)
{
  Boolean Found = True;
  char Buf[3];
  const char *Repl = NULL;
  int cnt = 0;

  if (*tex_token_sep_string == '\0')
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
  AddException(Token);
}

static void TeXDoPot(void)
{
  char Token[TOKLEN];

  tex_read_token(Token);
  if (*Token == '2')
  {
    char Buf[3];
    const char *pRepl = CharTab_GetNULTermString(pCharacterTab, eCH_e2, Buf);

    if (strlen(pRepl) > 1)
      memmove(Token + strlen(pRepl), Token + 1, strlen(Token));
    memcpy(Token, pRepl, strlen(pRepl));
  }
  else
    DoAddNormal("^", tex_backslash_token_sep_string);

  tex_push_back_token(Token);
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

/*--------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  char Line[TOKLEN], *p, AuxFile[200];
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
  AddInstTable(TeXTable, "gets", 0, TeXAddGets);
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

    tex_token_reset();
    tex_infile_push_file(p_infile_name);
    SetSrcDir(p_curr_tex_infile->p_name);
    if (!strcmp(p_outfile_name, "-"))
      p_outfile = stdout;
    else
    {
      p_outfile = fopen(p_outfile_name, "w");
      if (!p_outfile)
      {
        tex_error(p_outfile_name);
        exit(3);
      }
    }

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
    curr_tex_env_data.Alignment = AlignNone;
    curr_tex_env_data.EnumCounter = 0;
    InitFont();
    InitLabels();
    InitCites();
    InitToc();
    *SideMargin = '\0';
    DoRepass = False;
    BibIndent = BibCounter = 0;
    GermanMode = True;
    SetLang(False);

    strcpy(TocName, p_infile_name);
    p = strrchr(TocName, '.');
    if (p)
      *p = '\0';
    strcat(TocName, ".dtoc");

    strcpy(AuxFile, p_infile_name);
    p = strrchr(AuxFile, '.');
    if (p)
      *p = '\0';
    strcat(AuxFile, ".daux");
    ReadAuxFile(AuxFile);

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

