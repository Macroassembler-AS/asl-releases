/* cmdarg.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verarbeitung Kommandozeilenparameter                                      */
/*                                                                           */
/* Historie:  4. 5.1996 Grundsteinlegung                                     */
/*            1. 6.1996 Empty-Funktion                                       */
/*           17. 4.1999 Key-Files in Kommandozeile                           */
/*            3. 8.2000 added command line args as slashes                   */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "strutil.h"
#include "stringlists.h"
#include "cmdarg.h"
#include "nls.h"
#include "nlmessages.h"
#include "cmdarg.rsc"

TMsgCat MsgCat;
StringList FileArgList;

static void ClrBlanks(char *tmp)
{
  int cnt;

  for (cnt = 0; as_isspace(tmp[cnt]); cnt++);
  if (cnt > 0)
    strmov(tmp, tmp + cnt);
}

Boolean ProcessedEmpty(CMDProcessed Processed)
{
  int z;

  for (z = 1; z <= MAXPARAM; z++)
    if (Processed[z])
      return False;
   return True;
}

static void ProcessFile(char *Name_O, const CMDRec *pCMDRecs, int CMDRecCnt, CMDErrCallback ErrProc);

static CMDResult ProcessParam(const CMDRec *pCMDRecs, int CMDRecCnt, const char *O_Param,
                              const char *O_Next, Boolean AllowLink,
                              CMDErrCallback ErrProc)
{
  int Start;
  Boolean Negate;
  int z, Search;
  CMDResult TempRes;
  String s, Param, Next;

  strmaxcpy(Param, O_Param, STRINGSIZE);
  strmaxcpy(Next, O_Next, STRINGSIZE);

  if ((*Next == '-')
   || (*Next == '+')
#ifdef SLASHARGS
   || (*Next == '/')
#endif
   || (*Next == '@'))
    *Next = '\0';
  if (*Param == '@')
  {
    if (AllowLink)
    {
      ProcessFile(Param + 1, pCMDRecs, CMDRecCnt, ErrProc);
      return CMDOK;
    }
    else
    {
      fprintf(stderr, "%s\n", catgetmessage(&MsgCat, Num_ErrMsgNoKeyInFile));
      return CMDErr;
    }
  }
  if ((*Param == '-')
#ifdef SLASHARGS
   || (*Param == '/')
#endif
   || (*Param == '+'))
  {
    Negate = (*Param == '+');
    Start = 1;

    if (Param[Start] == '#')
    {
      for (z = Start + 1; z < (int)strlen(Param); z++)
        Param[z] = as_toupper(Param[z]);
      Start++;
    }
    else if (Param[Start] == '~')
    {
      for (z = Start + 1; z < (int)strlen(Param); z++)
        Param[z] = as_tolower(Param[z]);
      Start++;
    }

    TempRes = CMDOK;

    Search = 0;
    strmaxcpy(s, Param + Start, STRINGSIZE);
    for (z = 0; z < (int)strlen(s); z++)
      s[z] = as_toupper(s[z]);
    for (Search = 0; Search < CMDRecCnt; Search++)
      if ((strlen(pCMDRecs[Search].Ident) > 1) && (!strcmp(s, pCMDRecs[Search].Ident)))
        break;
    if (Search < CMDRecCnt)
      TempRes = pCMDRecs[Search].Callback(Negate, Next);

    else
    {
      for (z = Start; z < (int)strlen(Param); z++)
        if (TempRes != CMDErr)
        {
          Search = 0;
          for (Search = 0; Search < CMDRecCnt; Search++)
            if ((strlen(pCMDRecs[Search].Ident) == 1) && (pCMDRecs[Search].Ident[0] == Param[z]))
              break;
          if (Search >= CMDRecCnt)
            TempRes = CMDErr;
          else
          {
            switch (pCMDRecs[Search].Callback(Negate, Next))
            {
              case CMDErr:
                TempRes = CMDErr;
                break;
              case CMDArg:
                TempRes = CMDArg;
                break;
              case CMDOK:
                break;
              case CMDFile:
                break; /** **/
            }
          }
        }
    }
    return TempRes;
  }
  else
    return CMDFile;
}

static void DecodeLine(const CMDRec *pCMDRecs, int CMDRecCnt, char *OneLine,
                       CMDErrCallback ErrProc)
{
  int z;
  char *EnvStr[256], *start, *p;
  int EnvCnt = 0;

  ClrBlanks(OneLine);
  if ((*OneLine != '\0') && (*OneLine != ';'))
  {
    start = OneLine;
    while (*start != '\0')
    {
      EnvStr[EnvCnt++] = start;
      p = strchr(start, ' ');
      if (!p)
        p = strchr(start, '\t');
      if (p)
      {
        *p = '\0';
        start = p + 1;
        while (as_isspace(*start))
           start++;
      }
      else
        start += strlen(start);
    }
    EnvStr[EnvCnt] = start;

    for (z = 0; z < EnvCnt; z++)
      switch (ProcessParam(pCMDRecs, CMDRecCnt, EnvStr[z], EnvStr[z + 1], False, ErrProc))
      {
        case CMDFile:
          AddStringListLast(&FileArgList, EnvStr[z]);
          break;
        case CMDErr:
          ErrProc(True, EnvStr[z]);
          break;
        case CMDArg:
          z++;
          break;
        case CMDOK:
          break;
      }
  }
}

static void ProcessFile(char *Name_O, const CMDRec *pCMDRecs, int CMDRecCnt, CMDErrCallback ErrProc)
{
  FILE *KeyFile;
  String Name, OneLine;

  strmaxcpy(Name, Name_O, STRINGSIZE);
  ClrBlanks(OneLine);

  KeyFile = fopen(Name, "r");
  if (!KeyFile)
    ErrProc(True, catgetmessage(&MsgCat, Num_ErrMsgKeyFileNotFound));
  while (!feof(KeyFile))
  {
    errno = 0;
    ReadLn(KeyFile, OneLine);
    if ((errno != 0) && (!feof(KeyFile)))
      ErrProc(True, catgetmessage(&MsgCat, Num_ErrMsgKeyFileError));
    DecodeLine(pCMDRecs, CMDRecCnt, OneLine, ErrProc);
  }
  fclose(KeyFile);
}

/*!------------------------------------------------------------------------
 * \fn     ProcessCMD(int argc, char **argv,
                const CMDRec *pCMDRecs, int CMDRecCnt,
                CMDProcessed Unprocessed,
                char *EnvName, CMDErrCallback ErrProc)
 * \brief  arguments from command line and environment
 * \param  argc command line arg count as handed to main()
 * \param  argv command line args as handed to main()
 * \param  pCMDRecs command line switch descriptors
 * \param  CMDRecCnt # of command line switch descriptors
 * \param  Unprocessed returns bit mask of args not handled
 * \param  EnvName environment variable to draw additional args from
 * \param  CMDErrCallback called upon faulty args
 * ------------------------------------------------------------------------ */

void ProcessCMD(int argc, char **argv,
                const CMDRec *pCMDRecs, int CMDRecCnt,
                CMDProcessed Unprocessed,
                const char *EnvName, CMDErrCallback ErrProc)
{
  int z;
  String EnvLine;
  char *pEnv;

  pEnv = getenv(EnvName);
  strmaxcpy(EnvLine, pEnv ? pEnv : "", STRINGSIZE);

  if (EnvLine[0] == '@')
    ProcessFile(EnvLine + 1, pCMDRecs, CMDRecCnt, ErrProc);
  else
    DecodeLine(pCMDRecs, CMDRecCnt, EnvLine, ErrProc);

  for (z = 0; z < argc; z++)
    Unprocessed[z] = (z != 0);
  for (z = argc; z <= MAXPARAM; z++)
    Unprocessed[z] = False;

  for (z = 1; z < argc; z++)
    if (Unprocessed[z])
      switch (ProcessParam(pCMDRecs, CMDRecCnt, argv[z], (z + 1 < argc) ? argv[z + 1] : "",
                           True, ErrProc))
      {
        case CMDErr:
          ErrProc(False, argv[z]);
          break;
        case CMDOK:
          Unprocessed[z] = False;
          break;
        case CMDArg:
          Unprocessed[z] = Unprocessed[z + 1] = False;
          break;
        case CMDFile:
          AddStringListLast(&FileArgList, argv[z]);
          break;
      }
}

const char *GetEXEName(const char *argv0)
{
  const char *pos;

  pos = strrchr(argv0, '/');
  return (pos) ? pos + 1 : argv0;
}

void cmdarg_init(char *ProgPath)
{
  InitStringList(&FileArgList);
  opencatalog(&MsgCat, "cmdarg.msg", ProgPath, MsgId1, MsgId2);
}

