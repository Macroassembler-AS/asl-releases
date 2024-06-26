#ifndef _TEXUTIL_H
#define _TEXUTIL_H
/* texutil.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX-->ASCII/HTML Converter: Common Utils/Variables                        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "datatypes.h"

/*---------------------------------------------------------------------------*/

#define TOKLEN 350

typedef const char *const_char_ptr_t;

typedef struct tex_infile
{
  struct tex_infile *p_next;
  char *p_name;
  FILE *p_file;
  const char *p_buf_line;
  char *p_buf_line_to_free;
  int curr_line, curr_column;
  char *p_saved_buffer_line;
  int saved_buffer_line_ptr;
  char save_last_char;
} tex_infile_t;

extern tex_infile_t *p_curr_tex_infile;
extern char buffer_line[1001], *p_buffer_line_ptr, buffer_last_char;

extern const char *p_infile_name, *p_outfile_name;
extern FILE *p_outfile;


extern char tex_token_sep_string[TOKLEN],
            tex_backslash_token_sep_string[TOKLEN];


typedef struct tex_counter
{
  struct tex_counter *p_next;
  char *p_name;
  Word ref_num;
  unsigned value;
} tex_counter_t;

typedef struct tex_newif
{
  struct tex_newif *p_next;
  char *p_name;
  Boolean value;
} tex_newif_t;

typedef struct tex_environment
{
  struct tex_environment *p_next;
  char *p_name;
  char *p_begin_commands,
       *p_end_commands;
} tex_environment_t;

typedef struct tex_newcommand
{
  struct tex_newcommand *p_next;
  char *p_name;
  char *p_body;
  unsigned num_args;
} tex_newcommand_t;

typedef enum
{
  EnvNone, EnvDocument, EnvItemize, EnvEnumerate, EnvDescription, EnvTable,
  EnvTabular, EnvRaggedLeft, EnvRaggedRight, EnvCenter, EnvVerbatim,
  EnvQuote, EnvTabbing, EnvBiblio, EnvMarginPar, EnvCaption, EnvHeading, EnvUser,
  EnvCount
} EnvType;

extern const char *tex_env_names[EnvCount];

typedef enum
{
  AlignNone, AlignCenter, AlignLeft, AlignRight
} TAlignment;

typedef struct tex_env_data
{
  int ListDepth, ActLeftMargin, LeftMargin, RightMargin;
  int EnumCounter, FontNest;
  Boolean InListItem;
  TAlignment Alignment;
} tex_env_data_t;

typedef struct tex_env_save
{
  struct tex_env_save *p_next;
  EnvType save_env;
  char *p_save_user_env_name;
  tex_env_data_t save_env_data;
} tex_env_save_t;


extern tex_env_data_t curr_tex_env_data;
extern EnvType curr_tex_env;
extern char *p_curr_tex_user_env_name;
extern tex_env_save_t *p_env_stack;


typedef struct tex_output_consumer
{
  struct tex_output_consumer *p_next;
  void (*consume)(struct tex_output_consumer*, const_char_ptr_t *pp_str);
  void (*destroy)(struct tex_output_consumer*);
} tex_output_consumer_t;

extern tex_output_consumer_t *p_current_tex_output_consumer;

typedef enum { e_tex_if_if, e_tex_if_else } tex_if_state_t;
typedef struct tex_if_stack
{
  struct tex_if_stack *p_next;
  tex_if_state_t state;
  int condition;
} tex_if_stack_t;

extern Boolean DoRepass;

/*---------------------------------------------------------------------------*/

extern void tex_infile_pop(void);
extern void tex_infile_pop_all(void);
extern void tex_infile_push_file(const char *p_name);
extern void tex_infile_push_line(const char *p_src, const char *p_line, Boolean line_dynamic);
extern char *tex_infile_gets(char *p_dest, size_t dest_cap, tex_infile_t *p_infile);

extern int tex_get_char(void);
extern Boolean tex_issep(char inp);
extern Boolean tex_isalphanum(char inp);
extern Boolean tex_read_token(char *p_dest);
extern void tex_push_back_token(char *p_token);
extern void tex_token_reset(void);
extern void tex_assert_token(const char *p_ref);
extern void tex_collect_token(char *p_dest, const char *p_term);

extern void tex_counter_add(const char *p_name);
extern void tex_counters_free(void);
extern void tex_counter_set(const char *p_name, unsigned new_value);
extern unsigned tex_counter_get(const char *p_name);
extern void tex_counter_step(const char *p_name);
extern void TeXNewCounter(Word index);
extern void TeXStepCounter(Word index);
extern void TeXSetCounter(Word index);

extern void tex_newif_add(const char *p_name);
extern void tex_newifs_free(void);
extern Boolean tex_newif_lookup(const char *p_name);
extern void TeXNewIf(Word index);

extern void tex_newcommand_add(const char *p_name, const char *p_body, unsigned num_args);
extern void tex_newcommands_free(void);
extern const tex_newcommand_t *tex_newcommand_lookup(const char *p_name);
extern void tex_newcommand_expand_push(const tex_newcommand_t *p_cmd);
extern void TeXNewCommand(Word Index);

extern void tex_environment_add(const char *p_name, const char *p_begin_commands, const char *p_end_commands);
extern void tex_environments_free(void);
extern const tex_environment_t *tex_environment_lookup(const char *p_name);
extern EnvType tex_get_env_type(char *p_name, const tex_environment_t **pp_env);
extern void TeXNewEnvironment(Word Index);

extern void tex_verror(const char *p_msg, va_list ap);
extern void tex_error(const char *p_msg, ...);
extern void tex_vwarning(const char *p_msg, va_list ap);
extern void tex_warning(const char *p_msg, ...);

extern void tex_save_env(EnvType new_env, const char *p_user_env_name);
extern void tex_restore_env(void);

extern void tex_output_consumer_push(tex_output_consumer_t *p_consumer);
extern void tex_output_consumer_pop(void);
extern void tex_output_consumer_pop_all(void);

extern void tex_if_push(int condition);
extern void tex_if_toggle(void);
extern void tex_if_pop(void);
extern int tex_if_query(void);
extern void tex_if_pop_all(void);
extern unsigned tex_if_nest(void);

extern void TeXIfNum(Word index);
extern void TeXFi(Word index);

#endif /* _TEXUTIL_H */
