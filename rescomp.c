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

#include "endian.h"
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

/*****************************************************************************/

#ifdef __TURBOC__
unsigned _stklen = 16384;
#endif

static char *IdentString = "AS Message Catalog - not readable\n\032\004";

static LongInt MsgCounter;
static PMsgCat MsgCats;
static LongInt CatCount,DefCat;
static FILE *SrcFile,*MsgFile,*HFile;
static char *IncSym;

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
                      void *pBuffer, size_t size, size_t nmemb, FILE *pFile)
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

  ReadLn(SrcFile, Dest);
  if (!as_strncasecmp(Dest, "INCLUDE", 7))
  {
    OneFile = (PIncList) malloc(sizeof(TIncList));
    OneFile->Next = IncList; OneFile->Contents = SrcFile;
    IncList = OneFile;
    strmov(Dest, Dest + 7); KillPrefBlanks(Dest); KillPrefBlanks(Dest);
    SrcFile = fopenchk(Dest, 2, "r");
    GetLine(Dest);
  }
  if ((feof(SrcFile)) && (IncList))
  {
    fclose(SrcFile);
    OneFile = IncList; IncList = OneFile->Next;
    SrcFile = OneFile->Contents; free(OneFile);
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
  if (HFile) fprintf(HFile, "#define Num_%s %d\n", Line, MsgCounter);
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
  const char *pSrcName = NULL, *pHFileName = NULL, *pMsgFileName = NULL;

  endian_init(); strutil_init();

  curridx = 0;
  nextidx = -1;
  for (argz = 1; argz < argc; argz++)
  {
    if (!strcmp(argv[argz], "-h"))
      nextidx = 2;
    else if (!strcmp(argv[argz], "-m"))
      nextidx = 1;
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
          pMsgFileName = argv[argz]; break;
        case 2:
          pHFileName = argv[argz]; break;
      }
    }
  }

  if (!pSrcName)
  {
    fprintf(stderr, "usage: %s <input resource file> <[-m] output msg file> [[-h ]header file]\n", *argv);
    exit(1);
  }

  SrcFile = fopenchk(pSrcName, 2, "r");

  if (pHFileName)
  {
    HFile = fopenchk(pHFileName, 3, "w");
    IncSym = as_strdup(pHFileName);
    for (p = IncSym; *p; p++)
     if (isalpha(((unsigned int) *p) & 0xff))
       *p = as_toupper(*p);
     else
       *p = '_';
  }
  else
    HFile = NULL;

  stamp = MyGetFileTime(argv[1]); Id1 = stamp & 0x7fffffff;
  Id2 = 0;
  for (c = 0; c < min((int)strlen(argv[1]), 4); c++)
   Id2 = (Id2 << 8) + ((Byte) argv[1][c]);
  if (HFile)
  {
    fprintf(HFile, "#ifndef %s\n", IncSym);
    fprintf(HFile, "#define %s\n", IncSym);
    fprintf(HFile, "#define MsgId1 %ld\n", (long) Id1);
    fprintf(HFile, "#define MsgId2 %ld\n", (long) Id2);
  }

  MsgCounter = CatCount = 0; MsgCats = NULL; DefCat = -1;
  while (!feof(SrcFile))
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

  fclose(SrcFile);
  if (HFile)
  {
    fprintf(HFile, "#endif /* #ifndef %s */\n", IncSym);
    fclose(HFile);
  }

  if (pMsgFileName)
  {
    MsgFile = fopenchk(pMsgFileName, 4, OPENWRMODE);

    /* Magic-String */

    fwritechk(pMsgFileName, 5, IdentString, 1, strlen(IdentString), MsgFile);
    Write4(MsgFile, &Id1); Write4(MsgFile, &Id2);

    /* Default nach vorne */

    if (DefCat > 0)
    {
      TmpCat = MsgCats[0]; MsgCats[0] = MsgCats[DefCat]; MsgCats[DefCat] = TmpCat;
    }

    /* Startadressen String-Kataloge berechnen */

    RunPos = ftell(MsgFile) + 1;
    for (z = MsgCats; z < MsgCats + CatCount; z++)
      RunPos += (z->HeadLength = strlen(z->CtryName) + 1 + 4 + 4 + (4 * z->CtryCodeCnt) + 4);
    for (z = MsgCats; z < MsgCats + CatCount; z++)
    {
      z->FilePos = RunPos; RunPos += z->TotLength + (4 * MsgCounter);
    }

    /* Country-Records schreiben */

    for (z = MsgCats; z < MsgCats + CatCount; z++)
    {
      fwritechk(pMsgFileName, 5, z->CtryName, 1, strlen(z->CtryName) + 1, MsgFile);
      Write4(MsgFile, &(z->TotLength));
      Write4(MsgFile, &(z->CtryCodeCnt));
      for (c = 0; c < z->CtryCodeCnt; c++) Write4(MsgFile, z->CtryCodes + c);
      Write4(MsgFile, &(z->FilePos));
    }
    Save = '\0'; fwritechk(pMsgFileName, 5, &Save, 1, 1, MsgFile);

    /* Stringtabellen schreiben */

    for (z = MsgCats; z < MsgCats + CatCount; z++)
    {
      for (List = z->Messages; List; List = List->Next)
      {
        StrPos = z->FilePos + (4 * MsgCounter) + List->Position;
        Write4(MsgFile, &StrPos);
      }
      for (List = z->Messages; List; List = List->Next)
        fwritechk(pMsgFileName, 5, List->Contents, 1, strlen(List->Contents) + 1, MsgFile);
    }

    /* faeaedisch... */

    fclose(MsgFile);
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
