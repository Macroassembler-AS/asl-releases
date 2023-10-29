/* bpemu.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Emulation einiger Borland-Pascal-Funktionen                               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "strutil.h"
#include "bpemu.h"

#ifdef __MSDOS__
#include <dos.h>
#include <dir.h>
#endif

#if defined( __EMX__ ) || defined( __IBMC__ )
#include <os2.h>
#endif

#ifdef __MINGW32__
#include <direct.h>
#endif

char *FExpand(char *Src)
{
  static String CurrentDir;
  String Copy;
#ifdef DRSEP
  String DrvPart;
#endif /* DRSEP */
  char *p, *p2;

  strmaxcpy(Copy, Src, STRINGSIZE);

#ifdef DRSEP
  p = strchr(Copy,DRSEP);
  if (p)
  {
    memcpy(DrvPart, Copy, p - Copy);
    DrvPart[p - Copy] = '\0';
    strmov(Copy, p + 1);
  }
  else
    *DrvPart = '\0';
#endif

#if (defined __MSDOS__)
  {
    int DrvNum;

    if (*DrvPart == '\0')
    {
      DrvNum = getdisk();
      *DrvPart = DrvNum + 'A';
      DrvPart[1] = '\0';
      DrvNum++;
    }
    else
      DrvNum = toupper(*DrvPart) - '@';
    getcurdir(DrvNum, CurrentDir);
  }
#elif (defined __EMX__) || (defined __IBMC__)
  {
    ULONG DrvNum, Dummy;

    if (*DrvPart == '\0')
    {
      DosQueryCurrentDisk(&DrvNum, &Dummy);
      *DrvPart = DrvNum + '@';
      DrvPart[1] = '\0';
    }
    else
      DrvNum = toupper(*DrvPart) - '@';
    Dummy = 255;
    DosQueryCurrentDir(DrvNum, (PBYTE) CurrentDir, &Dummy);
  }
#elif (defined __MINGW32__)
  {
    int DrvNum;

    if (!*DrvPart)
    {
      DrvNum = _getdrive();
      *DrvPart = DrvNum + '@';
      DrvPart[1] = '\0';
    }
    else
      DrvNum = toupper(*DrvPart) - '@';
    _getdcwd(DrvNum, CurrentDir, STRINGSIZE);
    if (CurrentDir[1] == ':')
      strmov(CurrentDir, CurrentDir + 2);
  }
#elif (defined _WIN32) /* CygWIN */
  if (!getcwd(CurrentDir, STRINGSIZE))
    0[CurrentDir] = '\0';
  for (p = CurrentDir; *p; p++)
    if (*p == '/') *p = '\\';
#else /* UNIX */
  if (!getcwd(CurrentDir, STRINGSIZE))
    0[CurrentDir] = '\0';
#endif

  if ((*CurrentDir) && (CurrentDir[strlen(CurrentDir) - 1] != PATHSEP))
    strmaxcat(CurrentDir, SPATHSEP, STRINGSIZE);
  if (*CurrentDir!=PATHSEP)
    strmaxprep(CurrentDir, SPATHSEP, STRINGSIZE);

  if (*Copy == PATHSEP)
  {
    strmaxcpy(CurrentDir, SPATHSEP, STRINGSIZE);
    strmov(Copy, Copy + 1);
  }

#ifdef DRSEP
#ifdef __CYGWIN32__
  /* win32 getcwd() does not deliver current drive letter, therefore only prepend a drive letter
     if there was one before. */
  if (*DrvPart)
#endif
  {
    strmaxprep(CurrentDir, SDRSEP, STRINGSIZE);
    strmaxprep(CurrentDir, DrvPart, STRINGSIZE);
  }
#endif

  while (True)
  {
    p = strchr(Copy, PATHSEP);
    if (!p)
      break;
    *p = '\0';
    if (!strcmp(Copy, "."));
    else if ((!strcmp(Copy, "..")) && (strlen(CurrentDir) > 1))
    {
      CurrentDir[strlen(CurrentDir) - 1] = '\0';
      p2 = strrchr(CurrentDir, PATHSEP); p2[1] = '\0';
    }
    else
    {
      strmaxcat(CurrentDir, Copy, STRINGSIZE);
      strmaxcat(CurrentDir, SPATHSEP, STRINGSIZE);
    }
    strmov(Copy, p + 1);
  }

  strmaxcat(CurrentDir, Copy, STRINGSIZE);

  return CurrentDir;
}

/*!------------------------------------------------------------------------
 * \fn     FSearch(char *pDest, size_t DestSize, const char *pFileToSearch, const char *pCurrFileName, const char *pSearchPath)
 * \brief  search for file in given path(s)
 * \param  pDest where to put result
 * \param  DestSize size of result buffer
 * \param  pFileToSearch file to search for
 * \param  pCurrFileName file this file was referenced from
 * \param  pSearchPath list of directories to search
 * \return 0 if found or error code
 * ------------------------------------------------------------------------ */

static int AssembleAndCheck(char *pDest, size_t DestSize, const char *pPath, unsigned PathLen, const char *pFileToSearch)
{
  FILE *pDummy;

  if (PathLen > DestSize - 1)
    PathLen = DestSize - 1;
  memcpy(pDest, pPath, PathLen);
  pDest[PathLen] = '\0';
#ifdef __CYGWIN32__
  DeCygwinPath(pDest);
#endif
  if (PathLen > 0)
    strmaxcat(pDest, SPATHSEP, DestSize);
  strmaxcat(pDest, pFileToSearch, DestSize);
  pDummy = fopen(pDest, "r");
  if (pDummy)
  {
    fclose(pDummy);
    return 0;
  }
  else
    return 2;
}

int FSearch(char *pDest, size_t DestSize, const char *pFileToSearch, const char *pCurrFileName, const char *pSearchPath)
{
  /* If the file has an absolute path ('/....', '\....', 'X:....'), do not search relative
     to current file's directory: */

  Boolean Absolute = (*pFileToSearch == '/');
  const char *pPos, *pStart;

#if (defined _WIN32) || (defined __EMX__) || (defined __IBMC__) || (defined __MSDOS__)
  if (*pFileToSearch == PATHSEP)
    Absolute = True;
#endif
#ifdef DRSEP
  if ((as_islower(*pFileToSearch) || as_isupper(*pFileToSearch))
   && (pFileToSearch[1] == DRSEP))
    Absolute = True;
#endif

  if (pCurrFileName && !Absolute)
  {
#if (defined _WIN32) || (defined __EMX__) || (defined __IBMC__) || (defined __MSDOS__)
    /* On systems with \ as path separator, we may get a mixture of / and \ in the path.
       Assure we find the last one of either: */

    pPos = strrmultchr(pCurrFileName, SPATHSEP "/");
#else
    pPos = strrchr(pCurrFileName, PATHSEP);
#endif
    if (!AssembleAndCheck(pDest, DestSize, pCurrFileName, pPos ? pPos - pCurrFileName : 0, pFileToSearch))
      return 0;
  }
  else
  {
    if (!AssembleAndCheck(pDest, DestSize, NULL, 0, pFileToSearch))
      return 0;
  }

  /* TODO: if the file has an absolute path, searching the include path should be pointless: */

  pStart = pSearchPath;
  while (True)
  {
    pPos = strchr(pStart, DIRSEP);

    if (!AssembleAndCheck(pDest, DestSize, pStart, pPos ? pPos - pStart : (int)strlen(pStart), pFileToSearch))
      return 0;
    if (pPos)
      pStart =  pPos+ 1;
    else
      break;
  }

  *pDest = '\0';
  return 2;
}

long FileSize(FILE *file)
{
  long Save = ftell(file), Size;

  fseek(file, 0, SEEK_END);
  Size=ftell(file);
  fseek(file, Save, SEEK_SET);
  return Size;
}

Byte Lo(Word inp)
{
  return (inp & 0xff);
}

Byte Hi(Word inp)
{
  return ((inp >> 8) & 0xff);
}

unsigned LoWord(LongWord Src)
{
  return (Src & 0xffff);
}

unsigned HiWord(LongWord Src)
{
  return ((Src >> 16) & 0xffff);
}

unsigned long LoDWord(LargeWord Src)
{
  return Src & 0xfffffffful;
}

Boolean Odd(int inp)
{
  return ((inp & 1) == 1);
}

Boolean DirScan(const char *Mask, charcallback callback)
{
  char Name[1024];

#ifdef __MSDOS__
  struct ffblk blk;
  int res;
  const char *pos;

  res = findfirst(Mask, &blk, FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC | FA_ARCH);
  if (res < 0)
    return False;
  pos = strrchr(Mask, PATHSEP);
  if (!pos)
    pos = strrchr(Mask, DRSEP);
  pos = pos ? pos + 1 : Mask;
  memcpy(Name, Mask, pos - Mask);
  while (res==0)
  {
    if ((blk.ff_attrib & (FA_LABEL|FA_DIREC)) == 0)
    {
      strcpy(Name + (pos - Mask), blk.ff_name);
      callback(Name);
    }
    res = findnext(&blk);
  }
  return True;
#else
#if defined ( __EMX__ ) || defined ( __IBMC__ )
  HDIR hdir = 1;
  FILEFINDBUF3 buf;
  ULONG rescnt;
  USHORT res;
  char *pos;

  rescnt = 1;
  res = DosFindFirst(Mask, &hdir, 0x16, &buf, sizeof(buf), &rescnt, 1);
  if (res)
    return False;
  pos = strrchr(Mask, PATHSEP);
  if (!pos)
    pos = strrchr(Mask, DRSEP);
  pos = pos ? pos + 1 : Mask;
  memcpy(Name, Mask, pos - Mask);
  while (res == 0)
  {
    strcpy(Name + (pos - Mask), buf.achName);
    callback(Name);
    res = DosFindNext(hdir, &buf, sizeof(buf), &rescnt);
  }
  return True;
#else
  strmaxcpy(Name, Mask, sizeof(Name));
  callback(Name);
  return True;
#endif
#endif
}

LongInt MyGetFileTime(char *Name)
{
  struct stat st;

  if (stat(Name, &st) == -1)
    return 0;
  else
    return st.st_mtime;
}

#ifdef __CYGWIN32__

/* convert CygWin-style paths back to something usable by other Win32 apps */

char *DeCygWinDirList(char *pStr)
{
  char *pRun;

  for (pRun = pStr; *pRun; pRun++)
    if (*pRun == ':')
      *pRun = ';';

  return pStr;
}

char *DeCygwinPath(char *pStr)
{
  char *pRun;

  if ((strlen(pStr) >= 4)
   && (pStr[0] =='/') && (pStr[1] == '/') && (pStr[3] == '/')
   && (isalpha(pStr[2])))
  {
    strmov(pStr, pStr + 1);
    pStr[0] = pStr[1];
    pStr[1] = ':';
  }

  if ((strlen(pStr) >= 4)
   && (pStr[0] =='\\') && (pStr[1] == '\\') && (pStr[3] == '\\')
   && (isalpha(pStr[2])))
  {
    strmov(pStr, pStr + 1);
    pStr[0] = pStr[1];
    pStr[1] = ':';
  }

  for (pRun = pStr; *pRun; pRun++)
    if (*pRun == '/')
      *pRun = '\\';

  return pStr;
}
#endif /* __CYGWIN32__ */

void bpemu_init(void)
{
}
