/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "sysdefs.h"
#include "specchars.h"
#include "strutil.h"

#define TMPNAME "tempfile"

int main(int argc, char **argv)
{
   FILE *src,*dest;
   int ch;
   int z,z2,res;
   char cmdline[1024];
   long charcnt,metacnt,crcnt;

   if (argc<2)
    {
     fprintf(stderr,"usage: %s <file> [more files]\n",argv[0]);
     exit(1);
    }

   for (z=1; z<argc; z++)
    {
     src=fopen(argv[z],OPENRDMODE);
     if (src==NULL)
      {
       fprintf(stderr,"error opening %s for reading\n",argv[z]); exit(2);
      }
     dest=fopen(TMPNAME,OPENWRMODE);
     if (dest==NULL)
      {
       fprintf(stderr,"error opening %s for writing\n",TMPNAME); exit(2);
      }
     charcnt=metacnt=crcnt=0;
     while (!feof(src))
      {
       ch=fgetc(src); charcnt++;
       switch (ch)
        {
         case EOF:
          break;
         case 13:
          crcnt++; break;
         default:
          for (z2=0; *specchars[z2]!=0000; z2++)
           if (ch==specchars[z2][0])
           {
             fputc(specchars[z2][1],dest); metacnt++; break;
           }
          if (*specchars[z2]==0000) fputc(ch,dest);
        }
      }
     fclose(src); fclose(dest);
     as_snprintf(cmdline, sizeof(cmdline), "mv %s %s", TMPNAME, argv[z]);
     res = system(cmdline);
     if (res != 0)
      {
       fprintf(stderr,"command \"%s\" failed\n",cmdline); exit(2);
      }
     printf("%s: %ld char(s), %ld cr(s) stripped, %ld meta char(s) converted\n",
            argv[z],charcnt,crcnt,metacnt);
    }

   exit(0);
}
