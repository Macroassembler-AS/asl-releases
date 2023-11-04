/* stdhandl.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Bereitstellung von fuer AS benoetigten Handle-Funktionen                  */
/*                                                                           */
/* Historie:  5. 4.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <sys/stat.h>
#include "stdhandl.h"

#if defined ( __EMX__ ) || defined ( __IBMC__ )
#include <os2.h>
#endif

#ifdef __TURBOC__
#include <io.h>
#endif

#ifndef S_ISCHR
#ifdef __IBMC__
#define S_ISCHR(m)    ((m) & S_IFCHR)
#else
#define S_ISCHR(m)    (((m) & S_IFMT) == S_IFCHR)
#endif
#endif
#ifndef S_ISREG
#ifdef __IBMC__
#define S_ISREG(m)    ((m) & S_IFREG)
#else
#define S_ISREG(m)    (((m) & S_IFMT) == S_IFREG)
#endif
#endif

TRedirected Redirected;

static void AssignHandle(FILE **ppFile, Word Num)
{
  switch (Num)
  {
    case NumStdIn:
      *ppFile = stdin;
      break;
    case NumStdOut:
      *ppFile = stdout;
      break;
    case NumStdErr:
      *ppFile = stderr;
      break;
    default:
      *ppFile = NULL;
  }
}

/* Eine Datei unter Beruecksichtigung der Standardkanaele oeffnen */

void OpenWithStandard(FILE **ppFile, const char *Path)
{
  if ((strlen(Path) == 2) && (Path[0] == '!') && (Path[1] >= '0') && (Path[1] <= '2'))
    AssignHandle(ppFile, Path[1] - '0');
  else
    *ppFile = fopen(Path, "w");
}

void CloseIfOpen(FILE **ppFile)
{
  if (*ppFile)
  {
    if ((*ppFile != stdin) && (*ppFile != stdout) && (*ppFile != stderr))
      fclose(*ppFile);
    *ppFile = NULL;
  }
}

void stdhandl_init(void)
{
#ifdef __EMX__
  ULONG HandType,DevAttr;

#else
#ifdef __TURBOC__
  int HandErg;

#else
  struct stat stdout_stat;

#endif
#endif

   /* wohin zeigt die Standardausgabe ? */

#ifdef __EMX__
  DosQueryHType(1, &HandType, &DevAttr);
  if ((HandType & 0xff) == FHT_DISKFILE)
    Redirected = RedirToFile;
  else if ((DevAttr & 2) == 0)
    Redirected = RedirToDevice;
  else
    Redirected = NoRedir;

#else
#ifdef __TURBOC__
  HandErg = ioctl(1, 0x00);
  if ((HandErg & 2) == 2)
    Redirected = NoRedir;
  else if ((HandErg & 0x8000) == 0)
    Redirected = RedirToFile;
  else
    Redirected = RedirToDevice;

#else
  fstat(NumStdOut, &stdout_stat);
  if (S_ISREG(stdout_stat.st_mode))
    Redirected = RedirToFile;
# ifdef S_ISFIFO
  else if (S_ISFIFO(stdout_stat.st_mode))
    Redirected = RedirToDevice;
# endif
  else
    Redirected = NoRedir;

#endif
#endif
}
