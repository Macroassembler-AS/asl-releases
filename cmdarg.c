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

Boolean as_cmd_processed_empty(const as_cmd_processed_t processed)
{
  int z;

  for (z = 1; z <= MAXPARAM; z++)
    if (processed[z])
      return False;
   return True;
}

static void ProcessFile(const char *Name_O, const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt, as_cmd_err_callback_t err_proc);

static as_cmd_result_t ProcessParam(const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt, const char *O_Param,
                                    const char *O_Next, Boolean AllowLink,
                                    as_cmd_err_callback_t err_proc)
{
  size_t Start;
  Boolean Negate;
  size_t z, Search;
  as_cmd_result_t TempRes;
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
      ProcessFile(Param + 1, p_cmd_recs, cmd_rec_cnt, err_proc);
      return e_cmd_ok;
    }
    else
    {
      fprintf(stderr, "%s\n", catgetmessage(&MsgCat, Num_ErrMsgNoKeyInFile));
      return e_cmd_err;
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
      for (z = Start + 1; z < (size_t)strlen(Param); z++)
        Param[z] = as_toupper(Param[z]);
      Start++;
    }
    else if (Param[Start] == '~')
    {
      for (z = Start + 1; z < (size_t)strlen(Param); z++)
        Param[z] = as_tolower(Param[z]);
      Start++;
    }

    TempRes = e_cmd_ok;

    Search = 0;
    strmaxcpy(s, Param + Start, STRINGSIZE);
    for (z = 0; z < (size_t)strlen(s); z++)
      s[z] = as_toupper(s[z]);
    for (Search = 0; Search < cmd_rec_cnt; Search++)
      if ((strlen(p_cmd_recs[Search].p_ident) > 1) && (!strcmp(s, p_cmd_recs[Search].p_ident)))
        break;
    if (Search < cmd_rec_cnt)
      TempRes = p_cmd_recs[Search].callback(Negate, Next);

    else
    {
      for (z = Start; z < (size_t)strlen(Param); z++)
        if (TempRes != e_cmd_err)
        {
          Search = 0;
          for (Search = 0; Search < cmd_rec_cnt; Search++)
            if ((strlen(p_cmd_recs[Search].p_ident) == 1) && (p_cmd_recs[Search].p_ident[0] == Param[z]))
              break;
          if (Search >= cmd_rec_cnt)
            TempRes = e_cmd_err;
          else
          {
            switch (p_cmd_recs[Search].callback(Negate, Next))
            {
              case e_cmd_err:
                TempRes = e_cmd_err;
                break;
              case e_cmd_arg:
                TempRes = e_cmd_arg;
                break;
              case e_cmd_ok:
                break;
              case e_cmd_file:
                break; /** **/
            }
          }
        }
    }
    return TempRes;
  }
  else
    return e_cmd_file;
}

static void DecodeLine(const as_cmd_rec_t *p_cmd_recs, int cmd_rec_cnt, char *OneLine,
                       as_cmd_err_callback_t err_proc)
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
      switch (ProcessParam(p_cmd_recs, cmd_rec_cnt, EnvStr[z], EnvStr[z + 1], False, err_proc))
      {
        case e_cmd_file:
          AddStringListLast(&FileArgList, EnvStr[z]);
          break;
        case e_cmd_err:
          err_proc(True, EnvStr[z]);
          break;
        case e_cmd_arg:
          z++;
          break;
        case e_cmd_ok:
          break;
      }
  }
}

static void ProcessFile(const char *Name_O, const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt, as_cmd_err_callback_t err_proc)
{
  FILE *KeyFile;
  String Name, OneLine;

  strmaxcpy(Name, Name_O, STRINGSIZE);
  ClrBlanks(OneLine);

  KeyFile = fopen(Name, "r");
  if (!KeyFile)
    err_proc(True, catgetmessage(&MsgCat, Num_ErrMsgKeyFileNotFound));
  while (!feof(KeyFile))
  {
    errno = 0;
    ReadLn(KeyFile, OneLine);
    if ((errno != 0) && (!feof(KeyFile)))
      err_proc(True, catgetmessage(&MsgCat, Num_ErrMsgKeyFileError));
    DecodeLine(p_cmd_recs, cmd_rec_cnt, OneLine, err_proc);
  }
  fclose(KeyFile);
}

/*!------------------------------------------------------------------------
 * \fn     
 * \brief  
 * \param  
 * \param  
 * \return 
 * ------------------------------------------------------------------------ */

static int cmd_compare(const void *p1, const void *p2)
{
  const as_cmd_rec_t *p_rec1 = (const as_cmd_rec_t*)p1,
                     *p_rec2 = (const as_cmd_rec_t*)p2;
  int cmp_res = strcmp(p_rec1->p_ident, p_rec2->p_ident);

  if (!cmp_res && (p_rec1 != p_rec2))
    fprintf(stderr, "cmd_arg: option '%s' present twice\n", p_rec1->p_ident);
  return cmp_res;
}

void as_cmd_extend(as_cmd_rec_t **p_cmd_recs, size_t *p_cmd_rec_cnt,
                   const as_cmd_rec_t *p_add_recs, size_t add_rec_cnt)
{
  as_cmd_rec_t *p_new_recs;

  if (p_cmd_recs)
    p_new_recs = (as_cmd_rec_t*)realloc(*p_cmd_recs, sizeof(*p_new_recs) * (*p_cmd_rec_cnt + add_rec_cnt));
  else
    p_new_recs = (as_cmd_rec_t*)malloc(sizeof(*p_new_recs) * (*p_cmd_rec_cnt + add_rec_cnt));
  if (p_new_recs)
  {
    memcpy(&p_new_recs[*p_cmd_rec_cnt], p_add_recs, sizeof(*p_new_recs) * add_rec_cnt);
    *p_cmd_rec_cnt += add_rec_cnt;
    *p_cmd_recs = p_new_recs;
    qsort(p_new_recs, *p_cmd_rec_cnt, sizeof(*p_new_recs), cmd_compare);
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_cmd_process(int argc, char **argv,
                const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt,
                as_cmd_processed_t unprocessed,
                const char *p_env_name, as_cmd_err_callback_t err_proc)
 * \brief  arguments from command line and environment
 * \param  argc command line arg count as handed to main()
 * \param  argv command line args as handed to main()
 * \param  p_cmd_recs command line switch descriptors
 * \param  cmd_rec_cnt # of command line switch descriptors
 * \param  unprocessed returns bit mask of args not handled
 * \param  p_env_name environment variable to draw additional args from
 * \param  err_proc called upon faulty args
 * ------------------------------------------------------------------------ */

void as_cmd_process(int argc, char **argv,
                const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt,
                as_cmd_processed_t unprocessed,
                const char *p_env_name, as_cmd_err_callback_t err_proc)
{
  int z;
  String EnvLine;
  char *pEnv;

  pEnv = getenv(p_env_name);
  strmaxcpy(EnvLine, pEnv ? pEnv : "", STRINGSIZE);

  if (EnvLine[0] == '@')
    ProcessFile(EnvLine + 1, p_cmd_recs, cmd_rec_cnt, err_proc);
  else
    DecodeLine(p_cmd_recs, cmd_rec_cnt, EnvLine, err_proc);

  for (z = 0; z < argc; z++)
    unprocessed[z] = (z != 0);
  for (z = argc; z <= MAXPARAM; z++)
    unprocessed[z] = False;

  for (z = 1; z < argc; z++)
    if (unprocessed[z])
      switch (ProcessParam(p_cmd_recs, cmd_rec_cnt, argv[z], (z + 1 < argc) ? argv[z + 1] : "",
                           True, err_proc))
      {
        case e_cmd_err:
          err_proc(False, argv[z]);
          break;
        case e_cmd_ok:
          unprocessed[z] = False;
          break;
        case e_cmd_arg:
          unprocessed[z] = unprocessed[z + 1] = False;
          break;
        case e_cmd_file:
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

void as_cmdarg_init(char *ProgPath)
{
  InitStringList(&FileArgList);
  opencatalog(&MsgCat, "cmdarg.msg", ProgPath, MsgId1, MsgId2);
}

