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
  e_cmd_err,
  e_cmd_file,
  e_cmd_ok,
  e_cmd_arg
} as_cmd_result_t;

typedef as_cmd_result_t (*as_cmd_callback_t)(
#ifdef __PROTOS__
Boolean neg_flag, const char *p_arg
#endif
);

typedef void (*as_cmd_err_callback_t)
(
#ifdef __PROTOS__
Boolean in_env, char *p_arg
#endif
);

typedef struct
{
  const char *p_ident;
  as_cmd_callback_t callback;
} as_cmd_rec_t;

#define MAXPARAM 256
typedef Boolean as_cmd_processed_t[MAXPARAM + 1];

extern StringList FileArgList;

extern Boolean as_cmd_processed_empty(const as_cmd_processed_t processed);

extern void as_cmd_extend(as_cmd_rec_t **p_cmd_recs, size_t *p_cmd_rec_cnt,
                          const as_cmd_rec_t *p_new_recs, size_t new_rec_cnt);

extern void as_cmd_process(int argc, char **argv,
                       const as_cmd_rec_t *p_cmd_recs, size_t cmd_rec_cnt,
                       as_cmd_processed_t unprocessed,
                       const char *p_env_name, as_cmd_err_callback_t err_proc);

extern const char *GetEXEName(const char *argv0);

extern void as_cmdarg_init(char *ProgPath);

#endif /* _CMDARG_H */
