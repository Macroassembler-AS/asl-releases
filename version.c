/* version.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Lagert die Versionsnummer                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

const char *Version = "1.42 Beta [Bld 235]";
const char *DebVersion = "1.42bld235-1";
LongInt VerNo = 0x142f;

const char *InfoMessCopyright = "(C) 1992,2022 Alfred Arnold";

LongInt Magic = 0x12372e46;

void version_init(void)
{
  unsigned int z;
  const char *CMess = InfoMessCopyright;
  LongWord XORVal;

  for (z = 0; z < strlen(CMess); z++)
  {
    XORVal = CMess[z];
    XORVal = XORVal << (((z + 1) % 4) * 8);
    Magic = Magic ^ XORVal;
  }
}
