/* version.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Keeps Version Number(s)                                                   */
/*                                                                           */
/*****************************************************************************/

#include "version.h"

const char *Version = STR(AS_VERSION_MAJOR) "." STR(AS_VERSION_MINOR) " Beta [Bld " STR(AS_VERSION_BUILD) "]";
const char *DebVersion = STR(AS_VERSION_MAJOR) "." STR(AS_VERSION_MINOR) "bld" STR(AS_VERSION_BUILD) "-1";
LongInt VerNo = (AS_VERSION_MAJOR * 4096)
              | ((AS_VERSION_MINOR / 10) * 256)
              | ((AS_VERSION_MINOR % 10) * 16)
              | 15;

const char *InfoMessCopyright = "(C) 1992,2024 Alfred Arnold";

LongInt Magic = 0x12372846;

void version_init(void)
{
  unsigned shift;
  const char *p_mess;

  for (shift = 0, p_mess = InfoMessCopyright; *p_mess; p_mess++)
    Magic ^= (((LongWord)*p_mess) & 0xff) << ((shift += 8) & 31);
}
