/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */

#include "version.c"  /* that is *no* error */

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  if (argc != 2)
    return 1;

  if (!strcmp(argv[1], "-v"))
    puts(DebVersion);
  else if (!strcmp(argv[1], "-a"))
    puts(ARCHPRNAME);
  else
    return 1;

  return 0;
}
