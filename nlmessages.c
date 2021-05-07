/* nlmessages.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Einlesen und Verwalten von Meldungs-Strings                               */
/*                                                                           */
/* Historie: 13. 8.1997 Grundsteinlegung                                     */
/*           17. 8.1997 Verallgemeinerung auf mehrere Kataloge               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "strutil.h"

#include "endian.h"
#include "bpemu.h"
#include "nls.h"
#include "chardefs.h"

#include "nlmessages.h"

/*****************************************************************************/

static const char *IdentString = "AS Message Catalog - not readable\n\032\004";

static const char *EOpenMsg = "cannot open msg file %s";
static const char *ERdMsg = "cannot read from msg file";
static const char *EIndMsg = "string table index error";

static TMsgCat DefaultCatalog =
{
  NULL, NULL, 0
};

/*****************************************************************************/

static void error(const char *Msg)
{
  fprintf(stderr, "message catalog handling: %s - program terminated\n", Msg);
  exit(255);
}

char *catgetmessage(PMsgCat Catalog, int Num)
{
  if ((Num >= 0) && (Num < Catalog->MsgCount))
    return Catalog->MsgBlock + Catalog->StrPosis[Num];
  else
  {
    static char *umess = NULL;

    if (!umess)
      umess = (char*)malloc(sizeof(char) * STRINGSIZE);
    as_snprintf(umess, STRINGSIZE, "catgetmessage: message number %d does not exist", Num);
    return umess;
  }
}

char *getmessage(int Num)
{
  return catgetmessage(&DefaultCatalog, Num);
}

FILE *myopen(const char *name, LongInt MsgId1, LongInt MsgId2)
{
  FILE *tmpfile;
  String line;
  LongInt RId1, RId2;
  Boolean EForm = False;

  tmpfile = fopen(name, OPENRDMODE);
  if (!tmpfile)
    return NULL;
  if (fread(line, 1, strlen(IdentString), tmpfile) != strlen(IdentString))
    EForm = True;
  if (memcmp(line, IdentString, strlen(IdentString)))
    EForm = True;
  if (!Read4(tmpfile, &RId1))
    EForm = True;
  if (RId1 != MsgId1)
    EForm = True;
  if (!Read4(tmpfile, &RId2))
    EForm = True;
  if (RId2 != MsgId2)
    EForm = True;
  if (EForm)
  {
    fclose(tmpfile);
    fprintf(stderr, "message catalog handling: warning: %s has invalid format or is out of date\n", name);
    return NULL;
  }
  else
    return tmpfile;
}

#define MSGPATHNAME "AS_MSGPATH"

void opencatalog(PMsgCat Catalog, const char *File, const char *Path, LongInt MsgId1, LongInt MsgId2)
{
  FILE *MsgFile;
  char str[2048], *ptr;
  const char *pSep;
  Word CountryCode;
  const char *lcstring;
  LongInt DefPos = -1, MomPos, DefLength = 0, MomLength, z, StrStart, CtryCnt, Ctrys[100];
  Boolean fi, Gotcha;

  const tNLSCharacterTab *CharacterTab;
  char *pStr;
  unsigned StrCap;
  tNLSCharacter Ch;

  /* get reference for finding out which language set to use */

  CountryCode = NLS_GetCountryCode();
  lcstring = getenv("LC_MESSAGES");
  if (!lcstring)
    lcstring = getenv("LC_ALL");
  if (!lcstring)
    lcstring = getenv("LANG");
  if (!lcstring)
    lcstring = "";

  /* find first valid message file: current directory has prio 1: */

  MsgFile = myopen(File, MsgId1, MsgId2);
  if (!MsgFile)
  {
    /* if executable path (argv[0]) is given and contains more than just the
       plain name, try its path with next-highest prio: */

    if (*Path != '\0')
    {
#ifdef __CYGWIN32__
      for (ptr = Path; *ptr != '\0'; ptr++)
        if (*ptr == '/') *ptr = '\\';
#endif
      pSep = strrchr(Path, PATHSEP);
      if (!pSep)
        MsgFile = NULL;
      else
      {
        memcpy(str, Path, pSep - Path); str[pSep - Path] = '\0';
        strcat(str, SPATHSEP);
        strcat(str, File);
        MsgFile = myopen(str, MsgId1, MsgId2);
      }
    }
    if (!MsgFile)
    {
      /* search in AS_MSGPATH next */

      ptr = getenv(MSGPATHNAME);
      if (ptr)
      {
        as_snprintf(str, sizeof(str), "%s%c%s", ptr, PATHSEP, File);
        MsgFile = myopen(str, MsgId1, MsgId2);
      }
      else
      {
        /* if everything fails, search via PATH */

        ptr = getenv("PATH");
        if (!ptr)
          MsgFile = NULL;
        else
        {
          String Dest;
          int Result;

#ifdef __CYGWIN32__
          DeCygWinDirList(pCopy);
#endif
          Result = FSearch(Dest, sizeof(Dest), File, NULL, ptr);
          MsgFile = Result ? NULL : myopen(Dest, MsgId1, MsgId2);

          /* absolutely last resort: if we were found via PATH (no slashes in argv[0]/Path), look up this
             path and replace bin/ with lib/ for 'companion path': */

          if (!MsgFile && !strrchr(Path, PATHSEP) && !FSearch(Dest, sizeof(Dest), Path, NULL, ptr))
          {
            char *pSep2;

            strreplace(Dest, SPATHSEP "bin", SPATHSEP "lib", 0, sizeof(Dest));
            pSep2 = strrchr(Dest, PATHSEP);
            if (pSep2)
              *pSep2 = '\0';
            strmaxcat(Dest, SPATHSEP, sizeof(Dest));
            strmaxcat(Dest, File, sizeof(Dest));
            MsgFile = myopen(Dest, MsgId1, MsgId2);
          }
        }
      }
      if (!MsgFile)
      {
        as_snprintf(str, sizeof(str), "%s/%s", LIBDIR, File);
        MsgFile = myopen(str, MsgId1, MsgId2);
        if (!MsgFile)
        {
          as_snprintf(str, sizeof(str), EOpenMsg, File);
          error(str);
        }
      }
    }
  }

  Gotcha = False;
  do
  {
    ptr = str;
    do
    {
      if (fread(ptr, 1, 1, MsgFile) != 1)
        error(ERdMsg);
      fi = (*ptr == '\0');
      if (!fi) ptr++;
    }
    while (!fi);
    if (*str != '\0')
    {
      if (!Read4(MsgFile, &MomLength))
        error(ERdMsg);
      if (!Read4(MsgFile, &CtryCnt))
        error(ERdMsg);
      for (z = 0; z < CtryCnt; z++)
        if (!Read4(MsgFile, Ctrys + z))
          error(ERdMsg);
      if (!Read4(MsgFile, &MomPos))
        error(ERdMsg);
      if (DefPos == -1)
      {
        DefPos = MomPos;
        DefLength = MomLength;
      }
      for (z = 0; z < CtryCnt; z++)
        if (Ctrys[z] == CountryCode)
          Gotcha = True;
      if (!Gotcha)
        Gotcha = !as_strncasecmp(lcstring, str, strlen(str));
    }
  }
  while ((*str != '\0') && (!Gotcha));
  if (*str == '\0')
  {
    MomPos = DefPos;
    MomLength = DefLength;
  }

  /* read pointer table */

  fseek(MsgFile, MomPos, SEEK_SET);
  if (!Read4(MsgFile, &StrStart))
    error(ERdMsg);
  Catalog->MsgCount = (StrStart - MomPos) >> 2;
  Catalog->StrPosis = (LongInt *) malloc(sizeof(LongInt)*Catalog->MsgCount);
  Catalog->StrPosis[0] = 0;
  if ((int)fread(Catalog->StrPosis + 1, 4, Catalog->MsgCount - 1, MsgFile) + 1 != Catalog->MsgCount)
    error(ERdMsg);
  if (HostBigEndian)
    DSwap(Catalog->StrPosis + 1, (Catalog->MsgCount - 1) << 2);
  for (z = 1; z < Catalog->MsgCount; z++)
  {
    Catalog->StrPosis[z] -= StrStart;
    if ((Catalog->StrPosis[z] < 0) || (Catalog->StrPosis[z] >= MomLength))
      error(EIndMsg);
  }

  /* read string table */

  fseek(MsgFile, StrStart, SEEK_SET);
  Catalog->MsgBlock = (char *) malloc(MomLength);
  if ((int)fread(Catalog->MsgBlock, 1, MomLength, MsgFile) != MomLength)
    error(ERdMsg);

  /* character replacement according to runtime codepage */

  CharacterTab = GetCharacterTab(NLS_GetCodepage());
  for (z = 1; z < Catalog->MsgCount; z++)
  {
    pStr = Catalog->MsgBlock + Catalog->StrPosis[z];
    StrCap = strlen(pStr);
    for (Ch = (tNLSCharacter)0; Ch < eCH_cnt; Ch++)
      strreplace(pStr, NLS_HtmlCharacterTab[Ch], (*CharacterTab)[Ch], 2, StrCap);
  }

  fclose(MsgFile);
}

void nlmessages_init(const char *File, char *ProgPath, LongInt MsgId1, LongInt MsgId2)
{
  opencatalog(&DefaultCatalog, File, ProgPath, MsgId1, MsgId2);
}
