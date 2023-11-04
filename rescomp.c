/* rescomp.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Compiler fuer Message-Dateien                                             */
/*                                                                           */
/*    17. 5.1998  Symbol gegen Mehrfachinklusion eingebaut                   */
/*     5. 7.1998  zusaetzliche Sonderzeichen                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "be_le.h"
#include "strutil.h"
#include "bpemu.h"

/*****************************************************************************/

typedef struct _TMsgList
{
  struct _TMsgList *Next;
  LongInt Position;
  char *Contents;
} TMsgList,*PMsgList;

typedef struct
{
  PMsgList Messages,LastMessage;
  char *CtryName;
  LongInt *CtryCodes,CtryCodeCnt;
  LongInt FilePos,HeadLength,TotLength;
} TMsgCat,*PMsgCat;

typedef struct
{
  char *AbbString,*Character;
} TransRec;

#define IO_RETCODE_SRC 2
#define IO_RETCODE_RSC 3
#define IO_RETCODE_MSG 4
#define IO_RETCODE_MSH 5

/*****************************************************************************/

#ifdef __TURBOC__
unsigned _stklen = 16384;
#endif

static char *IdentString = "AS Message Catalog - not readable\n\032\004";

static LongInt MsgCounter;
static PMsgCat MsgCats;
static LongInt CatCount, DefCat;
static const char *p_msg_file_name = NULL, *p_msh_file_name = NULL;
static FILE *p_src_file, *p_msg_file, *p_rsc_file, *p_msh_file;
static size_t bytes_written;

static TransRec TransRecs[] =
{
  { "\\n"     , "\n"      },
  { NULL      , NULL      }
};

/*****************************************************************************/

typedef struct _TIncList
{
  struct _TIncList *Next;
  FILE *Contents;
} TIncList,*PIncList;

PIncList IncList = NULL;

void WrError(Word Num)
{
  (void)Num;
}

static void fwritechk(const char *pArg, int retcode,
                      const void *pBuffer, size_t size, size_t nmemb, FILE *pFile)
{
  size_t res;

  res = fwrite(pBuffer, size, nmemb, pFile);
  if (res != nmemb)
  {
    perror(pArg);
    exit(retcode);
  }
}

static FILE *fopenchk(const char *pName, int retcode, const char *pOpenMode)
{
  FILE *pRes;

  pRes = fopen(pName, pOpenMode);
  if (!pRes)
  {
    perror(pName);
    exit(retcode);
  }
  return pRes;
}

static void GetLine(char *Dest)
{
  PIncList OneFile;

  ReadLn(p_src_file, Dest);
  if (!as_strncasecmp(Dest, "INCLUDE", 7))
  {
    OneFile = (PIncList) malloc(sizeof(TIncList));
    OneFile->Next = IncList; OneFile->Contents = p_src_file;
    IncList = OneFile;
    strmov(Dest, Dest + 7); KillPrefBlanks(Dest); KillPrefBlanks(Dest);
    p_src_file = fopenchk(Dest, 2, "r");
    GetLine(Dest);
  }
  if (feof(p_src_file) && IncList)
  {
    fclose(p_src_file);
    OneFile = IncList; IncList = OneFile->Next;
    p_src_file = OneFile->Contents; free(OneFile);
  }
}

/*****************************************************************************/

static void SynError(char *LinePart)
{
  fprintf(stderr, "syntax error : %s\n", LinePart); exit(10);
}

/*---------------------------------------------------------------------------*/

static void Process_LANGS(char *Line)
{
  char NLine[1024], *p, *p2, *p3, *end, z, Part[1024];
  PMsgCat PCat;
  int num;

  strmaxcpy(NLine, Line, 1024);
  KillPrefBlanks(NLine); KillPostBlanks(NLine);

  p = NLine; z = 0;
  while (*p)
  {
    for (; *p && !as_isspace(*p); p++);
    for (; *p && as_isspace(*p); p++);
    z++;
  }

  MsgCats = (PMsgCat) malloc(sizeof(TMsgCat) * (CatCount = z)); p = NLine;
  for (z = 0, PCat = MsgCats; z < CatCount; z++, PCat++)
  {
    PCat->Messages = NULL;
    PCat->TotLength = 0;
    PCat->CtryCodeCnt = 0;
    for (p2 = p; *p2 && !as_isspace(*p2); p2++);
    if (*p2 == '\0')
      strcpy(Part, p);
    else
    {
      *p2 = '\0'; strcpy(Part, p);
      for (p = p2 + 1; *p && as_isspace(*p2); p++); /* TODO: p instead of p2? */
    }
    if ((!*Part) || (Part[strlen(Part)-1] != ')')) SynError(Part);
    Part[strlen(Part) - 1] = '\0';
    p2 = strchr(Part, '(');
    if (!p2)
      SynError(Part);
    *p2 = '\0';
    PCat->CtryName = as_strdup(Part); p2++;
    do
    {
      for (p3 = p2; ((*p3) && (*p3 != ',')); p3++);
      switch (*p3)
      {
        case '\0':
          num = strtol(p2, &end, 10); p2 = p3;
          break;
        case ',':
          *p3 = '\0'; num = strtol(p2, &end, 10); p2 = p3 + 1;
          break;
        default:
          num = 0;
          break;
      }
      if (*end) SynError("numeric format");
      PCat->CtryCodes=(PCat->CtryCodeCnt == 0) ?
                      (LongInt *) malloc(sizeof(LongInt)) :
                      (LongInt *) realloc(PCat->CtryCodes, sizeof(LongInt) * (PCat->CtryCodeCnt + 1));
      PCat->CtryCodes[PCat->CtryCodeCnt++] = num;
    }
    while (*p2!='\0');
  }
}

static void Process_DEFAULT(char *Line)
{
  PMsgCat z;

  if (!CatCount) SynError("DEFAULT before LANGS");
  KillPrefBlanks(Line); KillPostBlanks(Line);
  for (z = MsgCats; z < MsgCats + CatCount; z++)
    if (!as_strcasecmp(z->CtryName,Line))
      break;
  if (z >= MsgCats + CatCount)
    SynError("unknown country name in DEFAULT");
  DefCat = z - MsgCats;
}

static void Process_MESSAGE(char *Line)
{
  char Msg[4096];
  String OneLine;
  int l;
  PMsgCat z;
  TransRec *PRec;
  PMsgList List;
  Boolean Cont;

  KillPrefBlanks(Line); KillPostBlanks(Line);
  if (p_rsc_file) fprintf(p_rsc_file, "#define Num_%s %d\n", Line, MsgCounter);
  MsgCounter++;

  for (z = MsgCats; z < MsgCats + CatCount; z++)
  {
    *Msg = '\0';
    do
    {
      GetLine(OneLine); KillPrefBlanks(OneLine); KillPostBlanks(OneLine);
      l = strlen(OneLine);
      Cont = OneLine[l - 1] == '\\';
      if (Cont)
      {
        OneLine[l - 1] = '\0'; KillPostBlanks(OneLine); l = strlen(OneLine);
      }
      if (*OneLine != '"') SynError(OneLine);
      if (OneLine[l - 1] != '"')
        SynError(OneLine);
      OneLine[l - 1] = '\0';
      strmaxcat(Msg, OneLine + 1, 4095);
    }
    while (Cont);
    for (PRec = TransRecs; PRec->AbbString; PRec++)
      strreplace(Msg, PRec->AbbString, PRec->Character, -1, sizeof(Msg));
    List = (PMsgList) malloc(sizeof(TMsgList));
    List->Next = NULL;
    List->Position = z->TotLength;
    List->Contents = as_strdup(Msg);
    if (!z->Messages) z->Messages = List;
    else z->LastMessage->Next = List;
    z->LastMessage = List;
    z->TotLength += strlen(Msg) + 1;
  }
}

static char *make_sym_name(const char *p_file_name, int include_guard)
{
  char *p_ret, *p_run;
  const char *p_base;
  size_t l;
  Boolean no_replace;

  p_base = strrchr(p_file_name, PATHSEP);
  if (p_base)
    p_file_name = p_base + 1;
  l = strlen(p_file_name);
  p_run = p_ret = (char*)malloc(!!include_guard + l + 1);
  if (include_guard)
    *p_run++ = '_';
  for (; *p_file_name; p_file_name++)
  {
    no_replace = (p_run == p_ret) ? as_isalpha(*p_file_name) : as_isalnum(*p_file_name);
    *p_run++ = no_replace
             ? (include_guard ? as_toupper(*p_file_name) : as_tolower(*p_file_name))
             : '_';
  }
  *p_run++ = '\0';
  return p_ret;
}

static void write_dual(const void *p_v_data, size_t data_len)
{
  if (p_msg_file)
    fwritechk(p_msg_file_name, IO_RETCODE_MSG, p_v_data, 1, data_len, p_msg_file);

  if (p_msh_file)
  {
    const unsigned char *p_data = (const unsigned char*)p_v_data;
    size_t z;

    for (z = 0; z < data_len; z++)
    {
      if (!(bytes_written & 15))
      {
        if (bytes_written > 0)
          fprintf(p_msh_file, ",");
        fprintf(p_msh_file, "\n  ");
      }
      else
        fprintf(p_msh_file, ",");
      fprintf(p_msh_file, "0x%02x", p_data[z]);
      bytes_written++;
    }
  }
  else
    bytes_written += data_len;
}

static void write_dual_4_le(LongInt *p_lword)
{
  if (HostBigEndian)
    DSwap(p_lword, 4);
  write_dual(p_lword, 4);
  if (HostBigEndian)
    DSwap(p_lword, 4);
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  char Line[1024], Cmd[1024], *p, Save;
  PMsgCat z;
  TMsgCat TmpCat;
  PMsgList List;
  int c, argz, nextidx, curridx;
  time_t stamp;
  LongInt Id1, Id2;
  LongInt RunPos, StrPos;
  const char *pSrcName = NULL, *p_rsc_file_name = NULL;
  char *p_rsc_file_incl_guard_sym = NULL, *p_msh_file_incl_guard_sym = NULL;

  be_le_init(); strutil_init();

  curridx = 0;
  nextidx = -1;
  for (argz = 1; argz < argc; argz++)
  {
    if (!strcmp(argv[argz], "-h"))
      nextidx = 2;
    else if (!strcmp(argv[argz], "-m"))
      nextidx = 1;
    else if (!strcmp(argv[argz], "-i"))
      nextidx = 3;
    else
    {
      int thisidx;

      if (nextidx >= 0)
      {
        thisidx = nextidx;
        nextidx = -1;
      }
      else
        thisidx = curridx++;
      switch (thisidx)
      {
        case 0:
          pSrcName = argv[argz]; break;
        case 1:
          p_msg_file_name = argv[argz]; break;
        case 2:
          p_rsc_file_name = argv[argz]; break;
        case 3:
          p_msh_file_name = argv[argz]; break;
      }
    }
  }

  if (!pSrcName)
  {
    fprintf(stderr, "usage: %s <input resource file> <[-m] output msg file> <[-i] output include file> [[-h ]header file]\n", *argv);
    exit(1);
  }

  p_src_file = fopenchk(pSrcName, IO_RETCODE_SRC, "r");

  if (p_rsc_file_name)
  {
    p_rsc_file = fopenchk(p_rsc_file_name, IO_RETCODE_RSC, "w");
    p_rsc_file_incl_guard_sym = make_sym_name(p_rsc_file_name, 1);
  }
  else
    p_rsc_file = NULL;

  stamp = MyGetFileTime(argv[1]); Id1 = stamp & 0x7fffffff;
  Id2 = 0;
  for (c = 0; c < min((int)strlen(argv[1]), 4); c++)
   Id2 = (Id2 << 8) + ((Byte) argv[1][c]);
  if (p_rsc_file)
  {
    fprintf(p_rsc_file, "#ifndef %s\n", p_rsc_file_incl_guard_sym);
    fprintf(p_rsc_file, "#define %s\n", p_rsc_file_incl_guard_sym);
    fprintf(p_rsc_file, "#define MsgId1 %ld\n", (long) Id1);
    fprintf(p_rsc_file, "#define MsgId2 %ld\n", (long) Id2);
  }

  MsgCounter = CatCount = 0; MsgCats = NULL; DefCat = -1;
  while (!feof(p_src_file))
  {
    GetLine(Line); KillPrefBlanks(Line); KillPostBlanks(Line);
    if ((*Line != ';') && (*Line != '#') && (*Line != '\0'))
    {
      for (p = Line; !as_isspace(*p) && *p; p++);
      Save = *p; *p = '\0'; strmaxcpy(Cmd, Line, 1024); *p = Save; strmov(Line, p);
      if (!as_strcasecmp(Cmd, "LANGS")) Process_LANGS(Line);
      else if (!as_strcasecmp(Cmd, "DEFAULT")) Process_DEFAULT(Line);
      else if (!as_strcasecmp(Cmd, "MESSAGE")) Process_MESSAGE(Line);
    }
  }
  fclose(p_src_file);

  if (p_rsc_file)
  {
    fprintf(p_rsc_file, "#endif /* #ifndef %s */\n", p_rsc_file_incl_guard_sym);
    free(p_rsc_file_incl_guard_sym); p_rsc_file_incl_guard_sym = NULL;
    fclose(p_rsc_file);
  }

  if (p_msg_file_name || p_msh_file_name)
  {
    if (p_msg_file_name)
      p_msg_file = fopenchk(p_msg_file_name, IO_RETCODE_MSG, OPENWRMODE);
    if (p_msh_file_name)
    {
      char *p_array_name = make_sym_name(p_msh_file_name, 0);

      p_msh_file = fopenchk(p_msh_file_name, IO_RETCODE_MSH, "w");
      p_msh_file_incl_guard_sym = make_sym_name(p_msh_file_name, 1);
      fprintf(p_msh_file, "#ifndef %s\n", p_msh_file_incl_guard_sym);
      fprintf(p_msh_file, "#define %s\n", p_msh_file_incl_guard_sym);
      fprintf(p_msh_file, "static const unsigned char %s_data[] = {", p_array_name);
      free(p_array_name); p_array_name = NULL;
    }
    bytes_written = 0;

    /* Magic-String */

    write_dual(IdentString, strlen(IdentString));
    write_dual_4_le(&Id1); write_dual_4_le(&Id2);

    /* Default nach vorne */

    if (DefCat > 0)
    {
      TmpCat = MsgCats[0]; MsgCats[0] = MsgCats[DefCat]; MsgCats[DefCat] = TmpCat;
    }

    /* Startadressen String-Kataloge berechnen */

    RunPos = bytes_written + 1;
    for (z = MsgCats; z < MsgCats + CatCount; z++)
      RunPos += (z->HeadLength = strlen(z->CtryName) + 1 + 4 + 4 + (4 * z->CtryCodeCnt) + 4);
    for (z = MsgCats; z < MsgCats + CatCount; z++)
    {
      z->FilePos = RunPos; RunPos += z->TotLength + (4 * MsgCounter);
    }

    /* Country-Records schreiben */

    for (z = MsgCats; z < MsgCats + CatCount; z++)
    {
      write_dual(z->CtryName, strlen(z->CtryName) + 1);
      write_dual_4_le(&(z->TotLength));
      write_dual_4_le(&(z->CtryCodeCnt));
      for (c = 0; c < z->CtryCodeCnt; c++) write_dual_4_le(z->CtryCodes + c);
      write_dual_4_le(&(z->FilePos));
    }
    Save = '\0'; write_dual(&Save, 1);

    /* Stringtabellen schreiben */

    for (z = MsgCats; z < MsgCats + CatCount; z++)
    {
      for (List = z->Messages; List; List = List->Next)
      {
        StrPos = z->FilePos + (4 * MsgCounter) + List->Position;
        write_dual_4_le(&StrPos);
      }
      for (List = z->Messages; List; List = List->Next)
        write_dual(List->Contents, strlen(List->Contents) + 1);
    }

    /* faeaedisch... */

    if (p_msg_file)
    {
      fclose(p_msg_file); p_msg_file = NULL;
    }
    if (p_msh_file)
    {
      fprintf(p_msh_file, "\n};\n");
      fprintf(p_msh_file, "#endif /* %s */\n", p_msh_file_incl_guard_sym);
      free(p_msh_file_incl_guard_sym); p_msh_file_incl_guard_sym = NULL;
      fclose(p_msh_file);
    }

  }

  return 0;
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
