/* msg_level.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Message Level Handling                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "msg_level.h"

unsigned msg_level = e_msg_level_normal;

static as_cmd_result_t cmd_msg_level_quiet(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);

  msg_level = negate ? e_msg_level_normal : e_msg_level_quiet;
  return e_cmd_ok;
}

static as_cmd_result_t cmd_msg_level_verbose(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);

  if (negate)
  {
    if (msg_level > 0) msg_level--;
  }
  else
    msg_level++;
  return e_cmd_ok;
}

static const as_cmd_rec_t cmds_msg_level[] =
{
  { "q"        , cmd_msg_level_quiet },
  { "quiet"    , cmd_msg_level_quiet },
  { "v"        , cmd_msg_level_verbose }
};

void msg_level_init(void)
{
  msg_level = e_msg_level_normal;
  as_cmd_register(cmds_msg_level, as_array_size(cmds_msg_level));
}
