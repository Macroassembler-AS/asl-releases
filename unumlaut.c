/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */

#include "stdinc.h"

#include <string.h>
#include "strutil.h"

#define TMPNAME "tempfile"

#ifdef CKMALLOC
#undef malloc
#undef realloc

void *ckmalloc(size_t s)
{
  void *tmp = malloc(s);

  if (!tmp)
  {
    fprintf(stderr, "allocation error(malloc): out of memory");
    exit(255);
  }
  return tmp;
}

void *ckrealloc(void *p, size_t s)
{
  void *tmp = realloc(p, s);
  if (!tmp)
  {
    fprintf(stderr, "allocation error(realloc): out of memory");
    exit(255);
  }
  return tmp;
}
#endif
void doexec(char *cmdline)
{
 int res=system(cmdline);
 if (res!=0)
 {
   fprintf(stderr, "command \"%s\" failed\n", cmdline); exit(2);
 }
}

int main(int argc, char **argv)
{
  FILE *src,*dest;
  int ch;
  char cmdline[1024];
  long charcnt,metacnt,crcnt;

  if (argc<2)
  {
    fprintf(stderr, "usage: %s <file> [destfile|destdir/]\n", argv[0]);
    exit(1);
  }

#ifdef _WIN32
  {
    char *p;
    int z;

    for (z = 1; z < argc; z++)
    {
      argv[z] = as_strdup(argv[z]);
      for (p = argv[z]; *p; p++)
        if (*p == '\\')
          *p = '/';
    }
  }
#endif

  src = fopen(argv[1], OPENRDMODE);
  if (src == NULL)
  {
    fprintf(stderr, "error opening %s for reading\n", argv[1]); exit(2);
  }
  if (argc < 3)
    strcpy(cmdline, TMPNAME);
  else
  {
    int l = strlen(argv[2]);

    if (strchr("/\\", argv[2][l - 1]))
    {
      const char *p = strrchr(argv[1], argv[2][l - 1]);
      as_snprintf(cmdline, sizeof(cmdline), "%s%s", argv[2], p ? p + 1 : argv[1]);
    }
    else
      strcpy(cmdline, argv[2]);
  }
  dest = fopen(cmdline, OPENWRMODE);
  if (dest == NULL)
  {
    fprintf(stderr, "error opening %s for writing\n", cmdline); exit(2);
  }
  charcnt = metacnt = crcnt = 0;
  while (!feof(src))
  {
    ch = fgetc(src); charcnt++;
    switch (ch)
    {
      case EOF:
        break;
      case 10:
        fputc(13, dest);
        fputc(10, dest);
        crcnt++;
        break;
      default:
        if (ch & 0x80)
        {
          fprintf(stderr, "%s: non-ASCII character 0x%02x @ 0x%x\n",
                  argv[1], ch, (unsigned)ftell(src));
          metacnt++;
        }
        fputc(ch, dest);
    }
  }
  fclose(src);
  fclose(dest);
  if (argc == 2)
  {
#if defined (__MSDOS__) || defined(__EMX__)
    as_snprintf(cmdline, sizeof(cmdline), "copy %s %s", TMPNAME, argv[1]);
#else
    as_snprintf(cmdline, sizeof(cmdline), "cp %s %s", TMPNAME, argv[1]);
#endif
    doexec(cmdline);
    unlink(TMPNAME);
  }
  printf("%s: %ld char(s), %ld cr(s) added, %ld meta char(s) detected\n",
         argv[1], charcnt, crcnt, metacnt);

  return metacnt ? 2 : 0;
}
