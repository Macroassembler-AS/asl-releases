/* texutil.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX-->ASCII/HTML Converter: Common Utils/Variables                        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strutil.h"
#include "texutil.h"

#define DBG_IF 0

/*--------------------------------------------------------------------------*/

Boolean DoRepass;

tex_infile_t *p_curr_tex_infile = NULL;
char buffer_line[1001] = "", *p_buffer_line_ptr = buffer_line, buffer_last_char = '\0';

const char *p_infile_name, *p_outfile_name;
FILE *p_outfile = NULL;

typedef struct tex_pushed_token
{
  char token[TOKLEN], sep[TOKLEN];
} tex_pushed_token_t;
static int pushed_token_cnt;
static tex_pushed_token_t pushed_tokens[16];
static char save_sep;
char tex_token_sep_string[TOKLEN],
     tex_backslash_token_sep_string[TOKLEN];
static Boolean did_eof;

static tex_counter_t *tex_counters = NULL;
static Word next_counter_ref_num = 0;

static tex_newif_t *p_tex_newifs = NULL;

static tex_environment_t *tex_environments = NULL;

static tex_newcommand_t *tex_newcommands = NULL;

tex_env_data_t curr_tex_env_data;
EnvType curr_tex_env;
char *p_curr_tex_user_env_name;
tex_env_save_t *p_env_stack;

const char *tex_env_names[EnvCount] =
{
  "___NONE___", "document", "itemize", "enumerate", "description", "table", "tabular",
  "raggedleft", "raggedright", "center", "verbatim", "quote", "tabbing",
  "thebibliography", "___MARGINPAR___", "___CAPTION___", "___HEADING___", "___USER___"
};

tex_output_consumer_t *p_current_tex_output_consumer;

static tex_if_stack_t *p_if_stack;

/*--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     tex_infile_free(tex_infile_t *p_infile)
 * \brief  free/destroy infile source
 * \param  p_infile source to free
 * ------------------------------------------------------------------------ */

static void tex_infile_free(tex_infile_t *p_infile)
{
  if (p_infile->p_name)
  {
    free(p_infile->p_name);
    p_infile->p_name = NULL;
  }
  if (p_infile->p_file)
  {
    fclose(p_infile->p_file);
    p_infile->p_file = NULL;
  }
  if (p_infile->p_buf_line_to_free)
  {
    free(p_infile->p_buf_line_to_free);
    p_infile->p_buf_line_to_free = NULL;
  }
  if (p_infile->p_saved_buffer_line)
  {
    free(p_infile->p_saved_buffer_line);
    p_infile->p_saved_buffer_line = NULL;
  }
  free(p_infile);
}

/*!------------------------------------------------------------------------
 * \fn     tex_infile_pop(void)
 * \brief  discard topmost infile source
 * ------------------------------------------------------------------------ */

void tex_infile_pop(void)
{
  tex_infile_t *p_old = p_curr_tex_infile;
  if (!p_old)
    return;
  p_curr_tex_infile = p_old->p_next;
  strmaxcpy(buffer_line, p_old->p_saved_buffer_line, sizeof(buffer_line));
  p_buffer_line_ptr = buffer_line + p_old->saved_buffer_line_ptr;
  buffer_last_char = p_old->save_last_char;
  tex_infile_free(p_old);
}

/*!------------------------------------------------------------------------
 * \fn     tex_infile_pop_all(void)
 * \brief  discard all infile sources
 * ------------------------------------------------------------------------ */

void tex_infile_pop_all(void)
{
  while (p_curr_tex_infile)
    tex_infile_pop();
}

/*!------------------------------------------------------------------------
 * \fn     tex_infile_ontop(tex_infile_t *p_new)
 * \brief  put new infile source on top of stack
 * \param  p_new source to become top
 * ------------------------------------------------------------------------ */

static void tex_infile_ontop(tex_infile_t *p_new)
{
  p_new->p_saved_buffer_line = as_strdup(buffer_line);
  p_new->saved_buffer_line_ptr = p_buffer_line_ptr - buffer_line;
  p_new->save_last_char = buffer_last_char;

  p_new->p_next = p_curr_tex_infile;
  p_curr_tex_infile = p_new;

  *buffer_line = '\0'; p_buffer_line_ptr = buffer_line;
  buffer_last_char = '\0';
}

/*!------------------------------------------------------------------------
 * \fn     tex_infile_push_file(const char *p_name)
 * \brief  put another source generator on top - data from file
 * \param  p_name name of file to open
 * ------------------------------------------------------------------------ */

void tex_infile_push_file(const char *p_name)
{
  tex_infile_t *p_new = (tex_infile_t*)calloc(1, sizeof(*p_new));

  p_new->p_name = as_strdup(p_name);
  p_new->p_file = fopen(p_name, "r");
  if (!p_new->p_file)
  {
    char msg[200];

    tex_infile_free(p_new);
    as_snprintf(msg, sizeof(msg), "file %s not found", p_name);
    tex_error(msg);
  }
  else
    tex_infile_ontop(p_new);
}

/*!------------------------------------------------------------------------
 * \fn     tex_infile_push_line(const char *p_name, const char *p_line, Boolean line_dynamic)
 * \brief  put another source generator on top - data from memory buffer
 * \param  p_name command name this body is from
 * \param  p_line sourc line
 * \param  line_dynamic free line after consumption?
 * ------------------------------------------------------------------------ */

void tex_infile_push_line(const char *p_name, const char *p_line, Boolean line_dynamic)
{
  tex_infile_t *p_new = (tex_infile_t*)calloc(1, sizeof(*p_new));

  p_new->p_name = as_strdup(p_name);
  p_new->p_file = NULL;
  p_new->p_buf_line = p_line;
  p_new->p_buf_line_to_free = line_dynamic ? (char*)p_line : NULL;

  tex_infile_ontop(p_new);
}

/*!------------------------------------------------------------------------
 * \fn     tex_infile_gets(char *p_dest, size_t dest_cap, tex_infile_t *p_infile)
 * \brief  read one line of source from infile source
 * \param  p_dest dest buffer
 * \param  dest_cap capacity of dest buffer
 * \param  p_infile source to read from
 * \return * to p_dest or NULL if nothing could be read (EOF)
 * ------------------------------------------------------------------------ */

char *tex_infile_gets(char *p_dest, size_t dest_cap, tex_infile_t *p_infile)
{
  if (p_infile->p_file)
    return fgets(p_dest, dest_cap, p_infile->p_file);
  else
  {
    char *p_pos;
    size_t src_len, dest_len;

    if (!p_infile->p_buf_line || !*p_infile->p_buf_line)
      return NULL;
    p_pos = strchr(p_infile->p_buf_line, '\n');
    src_len = p_pos ? (size_t)(p_pos - p_infile->p_buf_line) : strlen(p_infile->p_buf_line);
    if (dest_cap <= src_len)
    {
      fprintf(stderr, "warning: increase size of buffer line to at least %u\n", (unsigned)(src_len + 1));
      dest_len = dest_cap - 1;
    }
    else
      dest_len = src_len;
    memcpy(p_dest, p_infile->p_buf_line, dest_len); p_dest[dest_len] = '\0';
    p_infile->p_buf_line += src_len;
    return p_dest;
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_get_char(void)
 * \brief  read next character from current input stream
 * \return character or EOF
 * ------------------------------------------------------------------------ */

int tex_get_char(void)
{
  Boolean Comment;
  static Boolean DidPar = False;
  char *Result;

again:
  if (buffer_last_char)
  {
    int ret = buffer_last_char;
    buffer_last_char = '\0';
    return ret;
  }

  if (*p_buffer_line_ptr == '\0')
  {
    do
    {
      if (!p_curr_tex_infile)
        return EOF;
      do
      {
        Result = tex_infile_gets(buffer_line, sizeof(buffer_line), p_curr_tex_infile);
        if (Result)
        {
          p_curr_tex_infile->curr_line++;
          break;
        }
        tex_infile_pop();
        if (!p_curr_tex_infile)
          return EOF;
        if (buffer_last_char || *p_buffer_line_ptr)
          goto again;
      }
      while (True);
      p_buffer_line_ptr = buffer_line;
      Comment = (strlen(buffer_line) >= 2) && (!strncmp(buffer_line, "%%", 2));
      if ((*buffer_line == '\0') || (*buffer_line == '\n'))
      {
        if ((curr_tex_env == EnvDocument) && (!DidPar))
        {
          strcpy(buffer_line, "\\par\n");
          DidPar = True;
          Comment = False;
        }
      }
      else if (!Comment)
        DidPar = False;
    }
    while (Comment);
  }
  return *(p_buffer_line_ptr++);
}

/*!------------------------------------------------------------------------
 * \fn     tex_issep(char inp)
 * \brief  white space delimiter
 * \param  inp character to test
 * \return true if yes
 * ------------------------------------------------------------------------ */

Boolean tex_issep(char inp)
{
  return ((inp == ' ') || (inp == '\t') || (inp == '\n'));
}

/*!------------------------------------------------------------------------
 * \fn     tex_isalphanum(char inp)
 * \brief  character of identifier?
 * \param  inp character to test
 * \return true if yes
 * ------------------------------------------------------------------------ */

Boolean tex_isalphanum(char inp)
{
  return ((inp >= 'A') && (inp <= 'Z'))
      || ((inp >= 'a') && (inp <= 'z'))
      || ((inp >= '0') && (inp <= '9'))
      || (inp == '.');
}

/*!------------------------------------------------------------------------
 * \fn     tex_read_token(char *p_dest)
 * \brief  read next token from input stream
 * \param  p_dest where to put (capacity of TOKLEN)
 * \return True if success
 * ------------------------------------------------------------------------ */

Boolean tex_read_token(char *p_dest)
{
  int ch, z;
  Boolean good;
  char *p_run;

  if (pushed_token_cnt > 0)
  {
    strcpy(p_dest, pushed_tokens[0].token);
    strcpy(tex_token_sep_string, pushed_tokens[0].sep);
    for (z = 0; z < pushed_token_cnt - 1; z++)
      pushed_tokens[z] = pushed_tokens[z + 1];
    pushed_token_cnt--;
    return True;
  }

  if (did_eof)
    return FALSE;

  p_curr_tex_infile->curr_column = p_buffer_line_ptr - buffer_line + 1;

  /* fuehrende Blanks ueberspringen */

  *p_dest = '\0';
  *tex_token_sep_string = save_sep;
  p_run = tex_token_sep_string + ((save_sep == '\0') ? 0 : 1);
  do
  {
    ch = tex_get_char();
    if (ch == '\r')
      ch = tex_get_char();
    if (tex_issep(ch))
      *(p_run++) = ' ';
  }
  while ((tex_issep(ch)) && (ch != EOF));
  *p_run = '\0';
  if (ch == EOF)
  {
    did_eof = TRUE;
    return FALSE;
  }

  /* jetzt Zeichen kopieren, bis Leerzeichen */

  p_run = p_dest;
  save_sep = '\0';
  if (tex_isalphanum(*(p_run++) = ch))
  {
    do
    {
      ch = tex_get_char();
      good = (!tex_issep(ch)) && (tex_isalphanum(ch)) && (ch != EOF);
      if (good)
        *(p_run++) = ch;
    }
    while (good);

    /* Dateiende ? */

    if (ch == EOF)
      did_eof = TRUE;

    /* Zeichen speichern ? */

    else if (!tex_issep(ch) && !tex_isalphanum(ch))
      buffer_last_char = ch;

    /* Separator speichern ? */

    else if (tex_issep(ch))
      save_sep = ' ';
  }

  /* Ende */

  *p_run = '\0';
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     tex_push_back_token(char *p_token)
 * \brief  push back token that was read too much
 * \param  p_token token to put back
 * ------------------------------------------------------------------------ */

void tex_push_back_token(char *p_token)
{
  if (pushed_token_cnt >= 16)
    return;
  strcpy(pushed_tokens[pushed_token_cnt].token, p_token);
  strcpy(pushed_tokens[pushed_token_cnt].sep, tex_token_sep_string);
  pushed_token_cnt++;
}

/*!------------------------------------------------------------------------
 * \fn     tex_token_reset(void)
 * \brief  reset/initialize token read state machine
 * ------------------------------------------------------------------------ */

void tex_token_reset(void)
{
  pushed_token_cnt = 0;
  save_sep = '\0';
  tex_token_sep_string[0] =
  tex_backslash_token_sep_string[0] = '\0';
  did_eof = False;
}

/*!------------------------------------------------------------------------
 * \fn     tex_assert_token(const char *p_ref)
 * \brief  assure next token is as given
 * \param  ref token to check for
 * ------------------------------------------------------------------------ */

void tex_assert_token(const char *p_ref)
{
  char token[TOKLEN];

  tex_read_token(token);
  if (strcmp(p_ref, token))
    tex_error("\"%s\" expected, but got \"%s\"", p_ref, token);
}

/*!------------------------------------------------------------------------
 * \fn     tex_collect_token(char *p_dest, const char *p_term)
 * \brief  collect tokens up to terminator
 * \param  p_dest dest buffer of length TOKLEN
 * \param  p_term terminator
 * ------------------------------------------------------------------------ */

void tex_collect_token(char *p_dest, const char *p_term)
{
  char comp[TOKLEN];
  Boolean first = True, done;

  *p_dest = '\0';
  do
  {
    tex_read_token(comp);
    done = !strcmp(comp, p_term);
    if (!done)
    {
      if (!first)
        strcat(p_dest, tex_token_sep_string);
      strcat(p_dest, comp);
    }
    first = False;
  }
  while (!done);
}

/*!------------------------------------------------------------------------
 * \fn     collect_arg_tokens(char *p_dest, size_t dest_cap)
 * \brief  collect tokens up to }
 * \param  p_dest dest buffer
 * \param  dest_cap capacity of dest buffer
 * ------------------------------------------------------------------------ */

static void collect_arg_tokens(char *p_dest, size_t dest_cap)
{
  int level = 1;
  char token[TOKLEN];
  Boolean first = True;

  *p_dest = '\0';
  do
  {
    tex_read_token(token);
    if (!strcmp(token, "{"))
      level++;
    else if (!strcmp(token, "}"))
      level--;
    if (level != 0)
    {
      if (!first)
        strmaxcat(p_dest, tex_token_sep_string, dest_cap);
      strmaxcat(p_dest, token, dest_cap);
      first = False;
    }
  }
  while (level != 0);
}

/*!------------------------------------------------------------------------
 * \fn     tex_warning(const char *p_msg, ...)
 * \brief  print warning
 * \param  p_msg warning message
 * ------------------------------------------------------------------------ */

void tex_vwarning(const char *p_msg, va_list ap)
{
  fprintf(stderr, "%s:%d.%d: ",
          p_curr_tex_infile ? p_curr_tex_infile->p_name : "<internal>",
          p_curr_tex_infile ? p_curr_tex_infile->curr_line : 0,
          p_curr_tex_infile ? p_curr_tex_infile->curr_column : 0);
  vfprintf(stderr, p_msg, ap);
  fprintf(stderr, "\n");
}

void tex_warning(const char *p_msg, ...)
{
  va_list ap;

  va_start(ap, p_msg);
  tex_vwarning(p_msg, ap);
  va_end(ap);
}

/*!------------------------------------------------------------------------
 * \fn     tex_error(const char *p_msg, ...)
 * \brief  print warning
 * \param  pMsg error message & abort
 * ------------------------------------------------------------------------ */

void tex_verror(const char *p_msg, va_list ap)
{
  tex_vwarning(p_msg, ap);
  tex_infile_pop_all();
  if (p_outfile)
    fclose(p_outfile);
  exit(2);
}

void tex_error(const char *p_msg, ...)
{
  va_list ap;

  va_start(ap, p_msg);
  tex_verror(p_msg, ap);
  va_end(ap);
}

/*!------------------------------------------------------------------------
 * \fn     tex_counter_new(const char *p_name)
 * \brief  create counter structure
 * \param  p_name name of counter
 * \return * to created counter or NULL
 * ------------------------------------------------------------------------ */

static tex_counter_t *tex_counter_new(const char *p_name)
{
  tex_counter_t *p_ret = (tex_counter_t*)calloc(1, sizeof(*p_ret));
  p_ret->p_next = NULL;
  p_ret->p_name = as_strdup(p_name);
  p_ret->ref_num = next_counter_ref_num++;
  p_ret->value = 0;
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     tex_counter_free(tex_counter_t *p_counter)
 * \brief  destroy/free counter structure
 * \param  p_counter counter to free
 * ------------------------------------------------------------------------ */

static void tex_counter_free(tex_counter_t *p_counter)
{
  free(p_counter->p_name);
  free(p_counter);
}

/*!------------------------------------------------------------------------
 * \fn     tex_counter_add(const char *p_name)
 * \brief  add counter of given name to list of counters
 * \param  p_name name of counter
 * ------------------------------------------------------------------------ */

void tex_counter_add(const char *p_name)
{
  tex_counter_t *p_run, *p_prev, *p_new;

  for (p_prev = NULL, p_run = tex_counters; p_run; p_prev = p_run, p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
    {
      tex_warning("counter '%s' re-defined", p_name);
      return;
    }
  p_new = tex_counter_new(p_name);
  if (p_prev)
    p_prev->p_next = p_new;
  else
    tex_counters = p_new;
}

/*!------------------------------------------------------------------------
 * \fn     tex_counters_free(void)
 * \brief  destroy list of counters
 * ------------------------------------------------------------------------ */

void tex_counters_free(void)
{
  while (tex_counters)
  {
    tex_counter_t *p_old = tex_counters;

    tex_counters = p_old->p_next;
    tex_counter_free(p_old);
  }
  next_counter_ref_num = 0;
}

/*!------------------------------------------------------------------------
 * \fn     tex_counter_set(const char *p_name, unsigned new_value)
 * \brief  set counter to value
 * \param  p_name counter's name
 * \param  new_value value to set counter to
 * ------------------------------------------------------------------------ */

void tex_counter_set(const char *p_name, unsigned new_value)
{
  tex_counter_t *p_run;

  for (p_run = tex_counters; p_run; p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
    {
      p_run->value = new_value;
      return;
    }

  tex_warning("counter '%s' undefined", p_name);
}

/*!------------------------------------------------------------------------
 * \fn     tex_counter_get(const char *p_name)
 * \brief  retrieve value of counter
 * \param  p_name counter's name
 * \return counter's value or 0
 * ------------------------------------------------------------------------ */

unsigned tex_counter_get(const char *p_name)
{
  tex_counter_t *p_run;

  for (p_run = tex_counters; p_run; p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
      return p_run->value;

  tex_warning("counter '%s' undefined", p_name);
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     tex_counter_step(const char *p_name)
 * \brief  increment counter
 * \param  p_name counter to increment
 * ------------------------------------------------------------------------ */

void tex_counter_step(const char *p_name)
{
  tex_counter_t *p_run;

  for (p_run = tex_counters; p_run; p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
    {
      p_run->value++;
      return;
    }

  tex_warning("counter '%s' undefined", p_name);
}

/*!------------------------------------------------------------------------
 * \fn     tex_newif_new(const char *p_name)
 * \brief  create newif structure
 * \param  p_name name of newif
 * ------------------------------------------------------------------------ */

static tex_newif_t *tex_newif_new(const char *p_name)
{
  tex_newif_t *p_ret = (tex_newif_t*)calloc(1, sizeof(*p_ret));
  p_ret->p_next = NULL;
  p_ret->p_name = as_strdup(p_name);
  p_ret->value = False;
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     tex_newif_free(tex_newif_t *p_newif)
 * \brief  destroy newif structure
 * \param  p_newif structure to destroy
 * ------------------------------------------------------------------------ */

static void tex_newif_free(tex_newif_t *p_newif)
{
  if (p_newif->p_name) free(p_newif->p_name);
  p_newif->p_name = NULL;
  free(p_newif);
}

/*!------------------------------------------------------------------------
 * \fn     tex_newif_add(const char *p_name)
 * \brief  add newif to newif list
 * \param  p_name name of newif
 * ------------------------------------------------------------------------ */

void tex_newif_add(const char *p_name)
{
  tex_newif_t *p_new, *p_prev, *p_run;

  if (as_strncasecmp(p_name, "if", 2))
  {
    tex_warning("\\newif identifier does not start with \\if");
    return;
  }
  p_name += 2;

  for (p_prev = NULL, p_run = p_tex_newifs; p_run; p_prev = p_run, p_run = p_run->p_next)
    if (as_strcasecmp(p_run->p_name, p_name))
    {
      tex_warning("\\newif %s redefined", p_name);
      return;
    }

  p_new = tex_newif_new(p_name);
  if (p_prev)
    p_prev->p_next = p_new;
  else
    p_tex_newifs = p_new;
}

/*!------------------------------------------------------------------------
 * \fn     tex_newif_lookup(const char *p_name)
 * \brief  match command name against newif commands
 * \param  p_name command name
 * ------------------------------------------------------------------------ */

Boolean tex_newif_lookup(const char *p_name)
{
  tex_newif_t *p_run;
  Boolean is_if = !as_strncasecmp(p_name, "if", 2),
          is_true, is_false;
  size_t l = strlen(p_name), l2;

  is_true = (l > 4) && !as_strcasecmp(p_name + l - 4, "true");
  is_false = (l > 5) && !as_strcasecmp(p_name + l - 5, "false");
  if (!is_if && !is_true && !is_false)
    return False;
  for (p_run = p_tex_newifs; p_run; p_run = p_run->p_next)
  {
    if (is_if && !as_strcasecmp(p_name + 2, p_run->p_name))
    {
      tex_if_push(p_run->value);
      return True;
    }
    l2 = strlen(p_run->p_name);
    if (is_true && (l == l2 + 4) && !as_strncasecmp(p_name, p_run->p_name, l2))
    {
      p_run->value = True;
      return True;
    }
    else if (is_false && (l == l2 + 5) && !as_strncasecmp(p_name, p_run->p_name, l2))
    {
      p_run->value = False;
      return True;
    }
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     tex_newifs_free(void)
 * \brief  free all newifs
 * ------------------------------------------------------------------------ */

void tex_newifs_free(void)
{
  while (p_tex_newifs)
  {
    tex_newif_t *p_old = p_tex_newifs;
    p_tex_newifs = p_old->p_next;
    tex_newif_free(p_old);
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_environment_new(const char *p_name)
 * \brief  create user-defined environment structure
 * \param  p_name name of environment
 * \return * to structure or NULL
 * ------------------------------------------------------------------------ */

static tex_environment_t* tex_environment_new(const char *p_name)
{
  tex_environment_t *p_ret = (tex_environment_t*)calloc(1, sizeof(*p_ret));

  p_ret->p_next = NULL;
  p_ret->p_name = as_strdup(p_name);
  p_ret->p_begin_commands = NULL;
  p_ret->p_end_commands = NULL;
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     tex_environment_free(tex_environment_t *p_env)
 * \brief  destroy/free user-defined environment structure
 * \param  p_env environment to free
 * ------------------------------------------------------------------------ */

static void tex_environment_free(tex_environment_t *p_env)
{
  free(p_env->p_name);
  p_env->p_name = NULL;
  if (p_env->p_begin_commands)
    free(p_env->p_begin_commands);
  p_env->p_begin_commands = NULL;
  if (p_env->p_end_commands)
    free(p_env->p_end_commands);
  p_env->p_end_commands = NULL;
  free(p_env);
}

/*!------------------------------------------------------------------------
 * \fn     tex_environment_add(const char *p_name, const char *p_begin_commands, const char *p_end_commands)
 * \brief  add user-defined environment
 * \param  p_name name of environment
 * \param  p_begin_commands environment entry code
 * \param  p_end_commands environment exit code
 * ------------------------------------------------------------------------ */

void tex_environment_add(const char *p_name, const char *p_begin_commands, const char *p_end_commands)
{
  tex_environment_t *p_run, *p_prev, *p_new;

  for (p_run = tex_environments, p_prev = NULL; p_run; p_prev = p_run, p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
    {
      tex_warning("environment '%s' re-defined", p_name);
      return;
    }

  p_new = tex_environment_new(p_name);
  p_new->p_begin_commands = as_strdup(p_begin_commands);
  p_new->p_end_commands = as_strdup(p_end_commands);
  p_new->p_next = p_run;
  if (p_prev)
    p_prev->p_next = p_new;
  else
    tex_environments = p_new;
}

/*!------------------------------------------------------------------------
 * \fn     tex_environments_free(void)
 * \brief  delete all user-defined environments
 * ------------------------------------------------------------------------ */

void tex_environments_free(void)
{
  while (tex_environments)
  {
    tex_environment_t *p_old = tex_environments;

    tex_environments = p_old->p_next;
    tex_environment_free(p_old);
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_environment_lookup(const char *p_name)
 * \brief  search for user-defined environment
 * \param  p_name name of environment
 * \return * to environment's definition or NULL
 * ------------------------------------------------------------------------ */

const tex_environment_t *tex_environment_lookup(const char *p_name)
{
  tex_environment_t *p_run;

  for (p_run = tex_environments; p_run; p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
      break;
  return p_run;
}


/*!------------------------------------------------------------------------
 * \fn     tex_newcommand_new(const char *p_name)
 * \brief  create user-defined command
 * \param  p_name name of command
 * \return * to created structure or NULL
 * ------------------------------------------------------------------------ */

static tex_newcommand_t* tex_newcommand_new(const char *p_name)
{
  tex_newcommand_t *p_ret = (tex_newcommand_t*)calloc(1, sizeof(*p_ret));

  p_ret->p_next = NULL;
  p_ret->p_name = as_strdup(p_name);
  p_ret->p_body = NULL;
  p_ret->num_args = 0;
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     tex_newcommand_free(tex_newcommand_t *p_cmd)
 * \brief  destroy/free user-defined command
 * \param  p_cmd command to free
 * ------------------------------------------------------------------------ */

static void tex_newcommand_free(tex_newcommand_t *p_cmd)
{
  free(p_cmd->p_name);
  p_cmd->p_name = NULL;
  if (p_cmd->p_body)
    free(p_cmd->p_body);
  p_cmd->p_body = NULL;
  free(p_cmd);
}

/*!------------------------------------------------------------------------
 * \fn     tex_newcommand_add(const char *p_name, const char *p_body, unsigned num_args)
 * \brief  add user-defined command
 * \param  p_name name of command
 * \param  p_body command's body
 * \param  num_args number of arguments
 * ------------------------------------------------------------------------ */

void tex_newcommand_add(const char *p_name, const char *p_body, unsigned num_args)
{
  tex_newcommand_t *p_run, *p_prev, *p_new;

  for (p_run = tex_newcommands, p_prev = NULL; p_run; p_prev = p_run, p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
    {
      tex_warning("newcommand '%s' re-defined", p_name);
      return;
    }

  p_new = tex_newcommand_new(p_name);
  p_new->p_body = as_strdup(p_body);
  p_new->num_args = num_args;
  p_new->p_next = p_run;
  if (p_prev)
    p_prev->p_next = p_new;
  else
    tex_newcommands = p_new;
}

/*!------------------------------------------------------------------------
 * \fn     tex_newcommands_free(void)
 * \brief  destroy list of user-defined commands
 * ------------------------------------------------------------------------ */

void tex_newcommands_free(void)
{
  while (tex_newcommands)
  {
    tex_newcommand_t *p_old = tex_newcommands;

    tex_newcommands = p_old->p_next;
    tex_newcommand_free(p_old);
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_newcommand_lookup(const char *p_name)
 * \brief  lookup user-defined command
 * \param  p_name name of command
 * \return command's definition or NULL
 * ------------------------------------------------------------------------ */

const tex_newcommand_t *tex_newcommand_lookup(const char *p_name)
{
  tex_newcommand_t *p_run;

  for (p_run = tex_newcommands; p_run; p_run = p_run->p_next)
    if (!as_strcasecmp(p_name, p_run->p_name))
      break;
  return p_run;
}

/*!------------------------------------------------------------------------
 * \fn     tex_newcommand_expand_push(const tex_newcommand_t *p_cmd)
 * \brief  expand arguments to command and pushto input stream
 * \param  p_cmd command descriptor
 * ------------------------------------------------------------------------ */

void tex_newcommand_expand_push(const tex_newcommand_t *p_cmd)
{
  unsigned z;
  char arg_token[TOKLEN];
  char *p_expanded_line = (char*)p_cmd->p_body;
  Boolean line_dynamic = False;

  for (z = 0; z < p_cmd->num_args; z++)
  {
    char arg_holder[10], *p_pos;
    size_t holder_len, arg_len;

    tex_assert_token("{");
    collect_arg_tokens(arg_token, sizeof(arg_token));
    arg_len = strlen(arg_token);
    holder_len = as_snprintf(arg_holder, sizeof(arg_holder), "#%u", z + 1);
    while ((p_pos = strstr(p_expanded_line, arg_holder)))
    {
      size_t expanded_len = strlen(p_expanded_line),
             new_expanded_len, pos;
      ptrdiff_t delta;

      pos = p_pos - p_expanded_line;
      /* only generate copy of line if necessary: */
      if (!line_dynamic)
      {
        p_expanded_line = as_strdup(p_expanded_line);
        line_dynamic = True;
        p_pos = p_expanded_line + pos;
      }
      new_expanded_len = expanded_len - holder_len + arg_len;
      if (new_expanded_len > expanded_len)
      {
        p_expanded_line = (char*)realloc(p_expanded_line, new_expanded_len + 1);
        p_pos = p_expanded_line + pos;
      }
      delta = arg_len - holder_len;
      memmove(p_pos + holder_len + delta,
              p_pos + holder_len,
              expanded_len - pos - holder_len + 1);
      memcpy(p_pos, arg_token, arg_len);
    }
  }
  tex_infile_push_line(p_cmd->p_name, p_expanded_line, line_dynamic);
}

EnvType tex_get_env_type(char *p_name, const tex_environment_t **pp_env)
{
  EnvType z;

  if (!as_strcasecmp(p_name, "longtable"))
    return EnvTabular;
  for (z = EnvNone + 1; z < EnvCount; z++)
    if (!as_strcasecmp(p_name, tex_env_names[z]))
    {
      *pp_env = NULL;
      return z;
    }

  *pp_env = tex_environment_lookup(p_name);
  if (*pp_env)
    return EnvUser;

  tex_error("unknown environment '%s'", p_name);
  return EnvNone;
}

void tex_save_env(EnvType new_env, const char *p_user_env_name)
{
  tex_env_save_t *p_new_save;

  p_new_save = (tex_env_save_t*) malloc(sizeof(*p_new_save));
  p_new_save->p_next = p_env_stack;
  p_new_save->save_env_data = curr_tex_env_data;
  p_new_save->save_env = curr_tex_env;
  p_new_save->p_save_user_env_name = p_curr_tex_user_env_name; p_curr_tex_user_env_name = NULL;
  p_env_stack = p_new_save;
  curr_tex_env = new_env;
  p_curr_tex_user_env_name = (new_env == EnvUser) ? as_strdup(p_user_env_name) : NULL;
  curr_tex_env_data.FontNest = 0;
}

void tex_restore_env(void)
{
  tex_env_save_t *p_old_save;

  p_old_save = p_env_stack;
  p_env_stack = p_old_save->p_next;
  curr_tex_env_data = p_old_save->save_env_data;
  curr_tex_env = p_old_save->save_env;
  if (p_curr_tex_user_env_name) free(p_curr_tex_user_env_name);
  p_curr_tex_user_env_name = p_old_save->p_save_user_env_name; p_old_save->p_save_user_env_name = NULL;
  free(p_old_save);
}

/*!------------------------------------------------------------------------
 * \fn     tex_output_consumer_push(tex_output_consumer_t *p_consumer)
 * \brief  put a new output consumer at front
 * \param  p_consumer the new consumer instance
 * ------------------------------------------------------------------------ */

void tex_output_consumer_push(tex_output_consumer_t *p_consumer)
{
  p_consumer->p_next = p_current_tex_output_consumer;
  p_current_tex_output_consumer = p_consumer;
}

/*!------------------------------------------------------------------------
 * \fn     tex_output_consumer_pop(void)
 * \brief  discard innermost output consumer
 * ------------------------------------------------------------------------ */

void tex_output_consumer_pop(void)
{
  tex_output_consumer_t *p_consumer = p_current_tex_output_consumer;
  if (p_consumer)
  {
    p_current_tex_output_consumer = p_consumer->p_next;
    if (p_consumer->destroy) p_consumer->destroy(p_consumer);
    free(p_consumer);
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_output_consumer_pop_all(void)
 * \brief  dissolve output consumer stack
 * ------------------------------------------------------------------------ */

void tex_output_consumer_pop_all(void)
{
  while (p_current_tex_output_consumer)
    tex_output_consumer_pop();
}

/*!------------------------------------------------------------------------
 * \fn     TeXIfNum(Word index)
 * \brief  parse \ifnum command
 * ------------------------------------------------------------------------ */

typedef struct tex_ifnum_output_consumer
{
  tex_output_consumer_t consumer;
  int arg_index;
  int result;
  char op[10];
  unsigned num[2];
} tex_ifnum_output_consumer_t;

static void if_num_consume(struct tex_output_consumer *p_consumer, const char **pp_str)
{
  tex_ifnum_output_consumer_t *p_ctx = (tex_ifnum_output_consumer_t*)p_consumer;
  int read_rhs = 0;

#if DBG_IF
  fprintf(stderr, "if_num_func called with '%s' state %d\n", *pp_str, p_ctx->arg_index);
#endif
  for (; **pp_str && !p_ctx->result; (*pp_str)++)
  {
    int old_arg_index;
    do
    {
#if DBG_IF
      fprintf(stderr, " if_num_func char '%c' %d\n", **pp_str, p_ctx->arg_index);
#endif
      old_arg_index = p_ctx->arg_index;
      switch (old_arg_index)
      {
        case 0: /* leading spaces before left-hand number */
          if (isspace(**pp_str));
          else if (isdigit(**pp_str))
            p_ctx->arg_index = 1;
          else
          {
            tex_warning("non-numeric character '%c'", **pp_str);
            p_ctx->result = -1;
          }
          break;
        case 1: /* left-hand number */
          if (isdigit(**pp_str))
            p_ctx->num[0] = (p_ctx->num[0] * 10) + (**pp_str - '0');
          else if (isspace(**pp_str))
            p_ctx->arg_index = 2;
          else
            p_ctx->arg_index = 3;
          break;
        case 2: /* leading spaces before operator */
          if (isspace(**pp_str));
          else if (isdigit(**pp_str))
          {
            tex_warning("numeric character '%c' in operator", **pp_str);
            p_ctx->result = -1;
          }
          else
            p_ctx->arg_index = 3;
          break;
        case 3: /* operator */
          if (isspace(**pp_str))
            p_ctx->arg_index = 4;
          else if (isdigit(**pp_str))
            p_ctx->arg_index = 5;
          else
          {
            size_t l = strlen(p_ctx->op);
            if (l < sizeof(p_ctx->op) - 1)
            {
              p_ctx->op[l] = **pp_str;
              p_ctx->op[l + 1] = '\0';
            }
          }
          break;
        case 4: /* leading spaces before right-hand number */
          if (isspace(**pp_str));
          else if (isdigit(**pp_str))
            p_ctx->arg_index = 5;
          else
          {
            tex_warning("non-numeric character '%c'", **pp_str);
            p_ctx->result = -1;
          }
          break;
        case 5: /* right-hand number */
          if (isdigit(**pp_str))
          {
            p_ctx->num[1] = (p_ctx->num[1] * 10) + (**pp_str - '0');
            read_rhs = 1;
          }
          else
            p_ctx->result = 1;
          break;
        default:
          p_ctx->result = -1;
      }
    }
    while (!p_ctx->result && (old_arg_index != p_ctx->arg_index));
  }

  /* Last argument is a number and therefore expected to arrive as a single token
     in the output stream.  So when this token is used up and we read the right-hand
     number, we may assume we have read all arguments for \ifnum: */

  if (!**pp_str && read_rhs)
    p_ctx->result = 1;
  if (p_ctx->result)
  {
    int condition;

#if DBG_IF
    fprintf(stderr, "ifnum result %d %u %s %u\n",
            p_ctx->result,
            p_ctx->num[0],
            p_ctx->op,
            p_ctx->num[1]);
#endif
    if (p_ctx->result < 0)
      condition = True;
    else if (!as_strcasecmp(p_ctx->op, ">"))
      condition = (p_ctx->num[0] > p_ctx->num[1]);
    else if (!as_strcasecmp(p_ctx->op, "<"))
      condition = (p_ctx->num[0] < p_ctx->num[1]);
    else
    {
      tex_warning("unknown operator: '%s'", p_ctx->op);
      condition = True;
    }
    
    tex_output_consumer_pop();
    tex_if_push(condition);
  }
}

void TeXIfNum(Word index)
{
  tex_ifnum_output_consumer_t *p_ctx;
  UNUSED(index);

#if DBG_IF
  fprintf(stderr, "insert if\n");
#endif
  p_ctx = (tex_ifnum_output_consumer_t*)calloc(1, sizeof(*p_ctx));
  p_ctx->consumer.consume = if_num_consume;
  p_ctx->consumer.destroy = NULL;
  p_ctx->result = 0;
  p_ctx->op[0] = '\0';
  p_ctx->num[0] =
  p_ctx->num[1] = 0;
  p_ctx->arg_index = 0;
  tex_output_consumer_push(&p_ctx->consumer);
}

/*!------------------------------------------------------------------------
 * \fn     TeXFi(Word index)
 * \brief  handle \fi
 * ------------------------------------------------------------------------ */

void TeXFi(Word index)
{
  UNUSED(index);

  tex_if_pop();
}

/*!------------------------------------------------------------------------
 * \fn     tex_if_nest(void)
 * \brief  query depth of if stack
 * \return depth
 * ------------------------------------------------------------------------ */

unsigned tex_if_nest(void)
{
  unsigned ret;
  tex_if_stack_t *p_run;

  for (ret = 0, p_run = p_if_stack; p_run; ret++, p_run = p_run->p_next);
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     tex_if_push(int condition)
 * \brief  put new if condition on stack
 * \param  condition evaluated if (true/false)
 * ------------------------------------------------------------------------ */

void tex_if_push(int condition)
{
  tex_if_stack_t *p_new = (tex_if_stack_t*)calloc(1, sizeof(*p_new));

#if DBG_IF
  fprintf(stderr, "push if %u", condition);
#endif

  p_new->state = e_tex_if_if;
  p_new->condition = condition;
  p_new->p_next = p_if_stack;
  p_if_stack = p_new;

#if DBG_IF
  fprintf(stderr, ", depth %u\n", tex_if_nest());
#endif
}

/*!------------------------------------------------------------------------
 * \fn     tex_if_toggle(void)
 * \brief  toggle topmost if condition
 * ------------------------------------------------------------------------ */

void tex_if_toggle(void)
{
  if (!p_if_stack)
    tex_warning("\\else without \\if");
  else if (p_if_stack->state == e_tex_if_else)
    tex_warning("extra \\else");
  else
  {
    p_if_stack->state = e_tex_if_else;
    p_if_stack->condition = !p_if_stack->condition;
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_if_pop(void)
 * \brief  remove topmost if condition
 * ------------------------------------------------------------------------ */

void tex_if_pop(void)
{
#if DBG_IF
  fprintf(stderr, "pop if");
#endif

  if (!p_if_stack)
    tex_warning("\\fi without \\if");
  else
  {
    tex_if_stack_t *p_if = p_if_stack;
    p_if_stack = p_if->p_next;
    free(p_if);
#if DBG_IF
    fprintf(stderr, ", depth %u\n", tex_if_nest());
#endif
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_if_pop_all(void)
 * \brief  clear if stack
 * ------------------------------------------------------------------------ */

void tex_if_pop_all(void)
{
  if (p_if_stack)
    tex_warning("\\if without \\fi");
  while (p_if_stack)
    tex_if_pop();
}

/*!------------------------------------------------------------------------
 * \fn     tex_if_query(void)
 * \brief  query current if state
 * ------------------------------------------------------------------------ */

int tex_if_query(void)
{
  tex_if_stack_t *p_run;

  for (p_run = p_if_stack; p_run; p_run = p_run->p_next)
    if (!p_run->condition)
      return False;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     TeXNewCounter(Word index)
 * \brief  parse \newcounter command
 * ------------------------------------------------------------------------ */

void TeXNewCounter(Word index)
{
  char token[TOKLEN];

  UNUSED(index);

  tex_assert_token("{");
  tex_read_token(token);
  tex_assert_token("}");
  if (tex_if_query())
    tex_counter_add(token);
}

/*!------------------------------------------------------------------------
 * \fn     TeXStepCounter(Word index)
 * \brief  parse \stepcounter command
 * ------------------------------------------------------------------------ */

void TeXStepCounter(Word index)
{
  char token[TOKLEN];

  UNUSED(index);

  tex_assert_token("{");
  tex_read_token(token);
  tex_assert_token("}");
  if (tex_if_query())
    tex_counter_step(token);
}

/*!------------------------------------------------------------------------
 * \fn     TeXSetCounter(Word index)
 * \brief  parse \setcounter command
 * ------------------------------------------------------------------------ */

void TeXSetCounter(Word index)
{
  char token[TOKLEN], value_str[TOKLEN], *p_end;
  unsigned value;

  UNUSED(index);

  tex_assert_token("{");
  tex_read_token(token);
  tex_assert_token("}");
  tex_assert_token("{");
  tex_read_token(value_str);
  tex_assert_token("}");
  value = strtol(value_str, &p_end, 10);
  if (*p_end)
  {
    tex_warning("'%s': invalid numeric constant", value_str);
  }
  if (tex_if_query())
    tex_counter_set(token, value);
}

/*!------------------------------------------------------------------------
 * \fn     TeXNewEnvironment(Word Index)
 * \brief  handle \newenvironment
 * ------------------------------------------------------------------------ */

void TeXNewEnvironment(Word Index)
{
  char env_name[TOKLEN], env_entry[TOKLEN], env_exit[TOKLEN];
  UNUSED(Index);

  tex_assert_token("{");
  tex_read_token(env_name);
  tex_assert_token("}");
  tex_assert_token("{");
  collect_arg_tokens(env_entry, sizeof(env_entry));
  tex_assert_token("{");
  collect_arg_tokens(env_exit, sizeof(env_exit));
  tex_environment_add(env_name, env_entry, env_exit);
}

/*!------------------------------------------------------------------------
 * \fn     TeXNewCommand(Word Index)
 * \brief  handle \newcommand
 * ------------------------------------------------------------------------ */

void TeXNewCommand(Word Index)
{
  char token[TOKLEN], command[TOKLEN], sum_token[TOKLEN], arg_cnt[TOKLEN];
  unsigned num_args = 0;
  UNUSED(Index);

  tex_assert_token("{");
  tex_assert_token("\\");
  tex_read_token(command);
  tex_assert_token("}");
  tex_read_token(token);

  if (!strcmp(token, "["))
  {
    char *p_end;

    tex_read_token(arg_cnt);
    num_args = strtoul(arg_cnt, &p_end, 10);
    if (*p_end)
      tex_error("'%s': invalid argument count", arg_cnt);
    tex_assert_token("]");
    tex_read_token(token);
  }
  if (strcmp(token, "{"))
    tex_error("\"{\" expected");

  collect_arg_tokens(sum_token, sizeof(sum_token));
  if (!as_strcasecmp(command, "cpu")
   || !as_strcasecmp(command, "asname")
   || !as_strcasecmp(command, "errentry")
   || !as_strcasecmp(command, "headid")
   || !as_strcasecmp(command, "pcsymname"))
    tex_newcommand_add(command, sum_token, num_args);
}

/*!------------------------------------------------------------------------
 * \fn     TeXNewIf(Word index)
 * \brief  handle \newif command
 * ------------------------------------------------------------------------ */

void TeXNewIf(Word index)
{
  char if_name[TOKLEN];
  UNUSED(index);

  tex_assert_token("\\");
  tex_read_token(if_name);
  tex_newif_add(if_name);
}
