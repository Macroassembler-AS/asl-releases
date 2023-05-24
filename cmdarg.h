#ifndef _CMDARG_H
#define _CMDARG_H
/* cmdarg.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verarbeitung Kommandozeilenparameter                                      */
/*                                                                           */
/* Historie:  4. 5.1996 Grundsteinlegung                                     */
/*            2001-10-20: option string is pointer instead of array          */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>
#include "stringlists.h"
#include "datatypes.h"

typedef enum
{
  e_cmd_ok,
  e_cmd_err,
  e_cmd_file,
  e_cmd_arg
} as_cmd_result_t;

typedef as_cmd_result_t (*as_cmd_callback_t)(
#ifdef __PROTOS__
Boolean neg_flag, const char *p_arg
#endif
);

typedef struct
{
  const char *p_ident;
  as_cmd_callback_t callback;
} as_cmd_rec_t;

typedef struct
{
  StringList file_arg_list;
  Boolean write_help_exit, write_version_exit;
  char error_arg[100];
  Boolean error_arg_in_env;
} as_cmd_results_t;

extern void as_cmd_register(const as_cmd_rec_t *p_new_recs, size_t new_rec_cnt);

extern as_cmd_result_t as_cmd_process(int argc, char **argv,
                                      const char *p_env_name,
                                      as_cmd_results_t *p_results);

extern const char *as_cmdarg_get_executable_name(void);

extern void as_cmdarg_init(char *ProgPath);

#endif /* _CMDARG_H */
