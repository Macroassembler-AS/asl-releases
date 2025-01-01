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
#include "nls.h"
#include "nlmessages.h"
#include "cmdarg.rsc"
#ifdef _USE_MSH
# include "cmdarg.msh"
#endif
#include "cmdarg.h"

/* --------------------------------------------------------------- */

TMsgCat MsgCat;

static as_cmd_rec_t *sum_cmd_recs = NULL;
static size_t sum_cmd_rec_cnt = 0;

/* --------------------------------------------------------------- */

static as_cmd_result_t ProcessFile(const char *Name_O,
                                   const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt,
                                   as_cmd_results_t *p_results);

static as_cmd_result_t cmd_write_help(Boolean negate, const char *p_arg, as_cmd_results_t *p_results)
{
  UNUSED(p_arg);

  if (negate)
    return e_cmd_err;

  p_results->write_help_exit = True;
  return e_cmd_ok;
}

static as_cmd_result_t cmd_write_version(Boolean negate, const char *p_arg, as_cmd_results_t *p_results)
{
  UNUSED(p_arg);

  if (negate)
    return e_cmd_err;

  p_results->write_version_exit = True;
  return e_cmd_ok;
}

static Boolean is_arg_leadin(char p)
{
  return (p == '-')
#ifdef SLASHARGS
      || (p == '/')
#endif
      || (p == '+');
}

static as_cmd_result_t ProcessParam(const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt, const char *p_param,
                                    const char *p_next, Boolean AllowLink,
                                    as_cmd_results_t *p_results)
{
  Boolean Negate;
  as_cmd_result_t TempRes;
  const char *p_act_next;

  if (as_strcasecmp(p_param, "-intsyntax"))
    p_act_next = (is_arg_leadin(*p_next) || (*p_next == '@')) ? "" : p_next;
  else
    p_act_next = p_next;
  if (*p_param == '@')
  {
    if (AllowLink)
    {
      return ProcessFile(p_param + 1, p_cmd_recs, cmd_rec_cnt, p_results);
    }
    else
    {
      fprintf(stderr, "%s\n", catgetmessage(&MsgCat, Num_ErrMsgNoKeyInFile));
      strmaxcpy(p_results->error_arg, p_param, sizeof(p_results->error_arg));
      return e_cmd_err;
    }
  }
  if (is_arg_leadin(*p_param))
  {
    size_t Search;
    int cnv_up_lo = 0;

    Negate = (*p_param == '+');
    p_param++;
    if (*p_param == '#')
    {
      cnv_up_lo = 1;
      p_param++;
    }
    else if (*p_param == '~')
    {
      cnv_up_lo = -1;
      p_param++;
    }

    for (Search = 0; Search < cmd_rec_cnt; Search++)
      if ((strlen(p_cmd_recs[Search].p_ident) > 1) && (!as_strcasecmp(p_param, p_cmd_recs[Search].p_ident)))
        break;
    if (Search < cmd_rec_cnt)
      TempRes = p_cmd_recs[Search].callback(Negate, p_act_next);
    else if (!as_strcasecmp(p_param, "help"))
      TempRes = cmd_write_help(Negate, p_act_next, p_results);
    else if (!as_strcasecmp(p_param, "version"))
      TempRes = cmd_write_version(Negate, p_act_next, p_results);

    else
    {
      TempRes = e_cmd_ok;
      for (; *p_param; p_param++)
      {
        char p = *p_param;

        if (cnv_up_lo)
          p = (cnv_up_lo > 0) ? as_toupper(p) : as_tolower(p);
        Search = 0;
        for (Search = 0; Search < cmd_rec_cnt; Search++)
          if ((strlen(p_cmd_recs[Search].p_ident) == 1) && (p_cmd_recs[Search].p_ident[0] == p))
            break;
        if (Search >= cmd_rec_cnt)
          TempRes = e_cmd_err;
        else
          switch (p_cmd_recs[Search].callback(Negate, p_act_next))
          {
            case e_cmd_err:
              TempRes = e_cmd_err;
              break;
            case e_cmd_arg:
              TempRes = e_cmd_arg;
              p_act_next = "";
              break;
            case e_cmd_ok:
              break;
            case e_cmd_file:
              break; /** **/
          }
        if (TempRes == e_cmd_err)
          break;
      }
    }
    if (TempRes == e_cmd_err)
      strmaxcpy(p_results->error_arg, p_param, sizeof(p_results->error_arg));
    return TempRes;
  }
  else
    return e_cmd_file;
}

static as_cmd_result_t DecodeLine(const as_cmd_rec_t *p_cmd_recs, int cmd_rec_cnt, char *OneLine,
                                  as_cmd_results_t *p_results)
{
  int z;
  char *EnvStr[256], *start, *p;
  int EnvCnt = 0;

  KillPrefBlanks(OneLine);
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
    {
      switch (ProcessParam(p_cmd_recs, cmd_rec_cnt, EnvStr[z], EnvStr[z + 1], False, p_results))
      {
        case e_cmd_file:
          AddStringListLast(&p_results->file_arg_list, EnvStr[z]);
          break;
        case e_cmd_err:
          strmaxcpy(p_results->error_arg, EnvStr[z], sizeof(p_results->error_arg));
          p_results->error_arg_in_env = True;
          return e_cmd_err;
        case e_cmd_arg:
          z++;
          break;
        case e_cmd_ok:
          break;
      }
    }
  }
  return e_cmd_ok;
}

/*!------------------------------------------------------------------------
 * \fn     ProcessFile(const char *Name_O,
                       const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt,
                       as_cmd_results_t *p_results)
 * \brief  process arguments from file
 * \param  Name_O file's name
 * \param  p_cmd_recs, cmd_rec_cnt argument defintions
 * \param  p_results result buffer
 * \return e_cmd_ok or e_cmd_err
 * ------------------------------------------------------------------------ */

static as_cmd_result_t ProcessFile(const char *Name_O,
                                   const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt,
                                   as_cmd_results_t *p_results)
{
  FILE *KeyFile;
  String Name, OneLine;
  as_cmd_result_t ret = e_cmd_ok;

  strmaxcpy(Name, Name_O, STRINGSIZE);
  KillPrefBlanks(OneLine);

  KeyFile = fopen(Name, "r");
  if (!KeyFile)
  {
    strmaxcpy(p_results->error_arg, catgetmessage(&MsgCat, Num_ErrMsgKeyFileNotFound), sizeof(p_results->error_arg));
    ret = e_cmd_err;
  }
  while (!feof(KeyFile) && (ret == e_cmd_ok))
  {
    errno = 0;
    ReadLn(KeyFile, OneLine);
    if ((errno != 0) && !feof(KeyFile))
    {
      strmaxcpy(p_results->error_arg, catgetmessage(&MsgCat, Num_ErrMsgKeyFileError), sizeof(p_results->error_arg));
      ret = e_cmd_err;
    }
    ret = DecodeLine(p_cmd_recs, cmd_rec_cnt, OneLine, p_results);
  }
  fclose(KeyFile);
  return ret;
}

static int cmd_compare(const void *p1, const void *p2)
{
  const as_cmd_rec_t *p_rec1 = (const as_cmd_rec_t*)p1,
                     *p_rec2 = (const as_cmd_rec_t*)p2;
  int cmp_res = strcmp(p_rec1->p_ident, p_rec2->p_ident);

  if (!cmp_res && (p_rec1 != p_rec2))
    fprintf(stderr, "cmd_arg: option '%s' present twice\n", p_rec1->p_ident);
  return cmp_res;
}

/*!------------------------------------------------------------------------
 * \fn     as_cmd_register(const as_cmd_rec_t *p_add_recs, size_t add_rec_cnt)
 * \brief  extend command record list
 * \param  p_add_recs, add_rec_cnt records to add
 * ------------------------------------------------------------------------ */

void as_cmd_register(const as_cmd_rec_t *p_add_recs, size_t add_rec_cnt)
{
  as_cmd_rec_t *p_new_sum_recs;

  if (sum_cmd_recs)
    p_new_sum_recs = (as_cmd_rec_t*)realloc(sum_cmd_recs, sizeof(*p_new_sum_recs) * (sum_cmd_rec_cnt + add_rec_cnt));
  else
    p_new_sum_recs = (as_cmd_rec_t*)malloc(sizeof(*p_new_sum_recs) * (sum_cmd_rec_cnt + add_rec_cnt));
  if (p_new_sum_recs)
  {
    memcpy(&p_new_sum_recs[sum_cmd_rec_cnt], p_add_recs, sizeof(*p_new_sum_recs) * add_rec_cnt);
    sum_cmd_rec_cnt += add_rec_cnt;
    sum_cmd_recs = p_new_sum_recs;
    qsort(p_new_sum_recs, sum_cmd_rec_cnt, sizeof(*p_new_sum_recs), cmd_compare);
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_cmd_process(int argc, char **argv,
                          const char *p_env_name, as_cmd_results_t *p_results)
 * \brief  arguments from command line and environment
 * \param  argc command line arg count as handed to main()
 * \param  argv command line args as handed to main()
 * \param  p_env_name environment variable to draw additional args from
 * \param  p_file_arg_list gets populated with file arguments
 * \return e_cmd_ok, or e_cmd_err on faulty arg
 * ------------------------------------------------------------------------ */

const char *argv0;

as_cmd_result_t as_cmd_process(int argc, char **argv,
                               const char *p_env_name, as_cmd_results_t *p_results)
{
  int z;
  String EnvLine;
  char *pEnv;
  Boolean skip_next;

  p_results->file_arg_list = NULL;
  p_results->write_help_exit =
  p_results->write_version_exit = False;
  p_results->error_arg_in_env = False;
  p_results->error_arg[0] = '\0';

  pEnv = getenv(p_env_name);
  strmaxcpy(EnvLine, pEnv ? pEnv : "", STRINGSIZE);

  if (EnvLine[0] == '@')
  {
    if (e_cmd_err == ProcessFile(EnvLine + 1, sum_cmd_recs, sum_cmd_rec_cnt, p_results))
      return e_cmd_err;
  }
  else
  {
    if (e_cmd_err == DecodeLine(sum_cmd_recs, sum_cmd_rec_cnt, EnvLine, p_results))
      return e_cmd_err;
  }

  argv0 = argv[0];

  skip_next = False;
  for (z = 1; z < argc; z++)
  {
    if (skip_next)
    {
      skip_next = False;
      continue;
    }
    switch (ProcessParam(sum_cmd_recs, sum_cmd_rec_cnt, argv[z], (z + 1 < argc) ? argv[z + 1] : "",
                         True, p_results))
    {
      case e_cmd_err:
        p_results->error_arg_in_env = False;
        strmaxcpy(p_results->error_arg, argv[z], sizeof(p_results->error_arg));
        return e_cmd_err;
      case e_cmd_ok:
        break;
      case e_cmd_arg:
        skip_next = True;
        break;
      case e_cmd_file:
        AddStringListLast(&p_results->file_arg_list, argv[z]);
        break;
    }
  }
  return e_cmd_ok;
}

const char *as_cmdarg_get_executable_name(void)
{
  const char *pos;

  pos = strrchr(argv0, '/');
  return (pos) ? pos + 1 : argv0;
}

void as_cmdarg_init(char *ProgPath)
{
#ifdef _USE_MSH
  msg_catalog_open_buffer(&MsgCat, cmdarg_msh_data, sizeof(cmdarg_msh_data), MsgId1, MsgId2);
  UNUSED(ProgPath);
#else
  msg_catalog_open_file(&MsgCat, "cmdarg.msg", ProgPath, MsgId1, MsgId2);
#endif
}

