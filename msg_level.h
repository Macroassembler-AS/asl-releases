#ifndef _MSGLEVEL_H
#define _MSGLEVEL_H
/* msglevel.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Message Level Handling                                                    */
/*                                                                           */
/*****************************************************************************/

#include "cmdarg.h"

enum
{
  e_msg_level_quiet = 0,
  e_msg_level_normal = 1,
  e_msg_level_verbose = 2
};

extern unsigned msg_level;

extern as_cmd_result_t cmd_msg_level_quiet(Boolean negate, const char *p_arg);
extern as_cmd_result_t cmd_msg_level_verbose(Boolean negate, const char *p_arg);

#define cmds_msg_level \
  { "q"        , cmd_msg_level_quiet },\
  { "quiet"    , cmd_msg_level_quiet },\
  { "v"        , cmd_msg_level_verbose }

extern void msg_level_init(void);

#endif /* _MSGLEVEL_H */
