/* nlmessages.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Einlesen und Verwalten von Meldungs-Strings                               */
/*                                                                           */
/* Historie: 13. 8.1997 Grundsteinlegung                                     */
/*           17. 8.1997 Verallgemeinerung auf mehrere Kataloge               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "strutil.h"

#include "endian.h"
#include "bpemu.h"
#include "nls.h"
#include "chardefs.h"

#include "nlmessages.h"

/*****************************************************************************/

static const char *IdentString = "AS Message Catalog - not readable\n\032\004";

static const char *EOpenMsg = "cannot open msg file %s";
static const char *ERdMsg = "cannot read from msg file";
static const char *EIndMsg = "string table index error";

static TMsgCat DefaultCatalog =
{
  NULL, NULL, 0
};

typedef struct
{
  Byte *p_data;
  size_t data_length, rd_ptr;
  Boolean data_allocated;
} as_raw_msg_struct_t;

/*!------------------------------------------------------------------------
 * \fn     free_raw(as_raw_msg_struct_t *p_raw)
 * \brief  free raw message struct
 * \param  p_raw struct to free
 * ------------------------------------------------------------------------ */

static void free_raw(as_raw_msg_struct_t *p_raw)
{
  if (p_raw)
  {
    if (p_raw->p_data && p_raw->data_allocated)
      free(p_raw->p_data);
    p_raw->p_data = NULL;
    p_raw->data_allocated = False;
    p_raw->data_length = p_raw->rd_ptr = 0;
    free(p_raw);
  }
}

/*!------------------------------------------------------------------------
 * \fn     seek_raw(as_raw_msg_struct_t *p_raw, size_t offset, int whence)
 * \brief  seek in raw message struct
 * \param  p_raw struct to seek in
 * \param  offset offset from ref position
 * \param  whence ref position (like for fseek())
 * \return 0 for OK, -1 for error
 * ------------------------------------------------------------------------ */

static int seek_raw(as_raw_msg_struct_t *p_raw, size_t offset, int whence)
{
  size_t new_pos;

  switch (whence)
  {
    case SEEK_SET: new_pos = offset; break;
    case SEEK_CUR: new_pos = p_raw->rd_ptr + offset; break;
    case SEEK_END: new_pos = p_raw->data_length + offset; break;
    default:
      return -1;
  }

  if (new_pos <= p_raw->data_length)
  {
    p_raw->rd_ptr = new_pos;
    return 0;
  }
  else
    return -1;
}

/*!------------------------------------------------------------------------
 * \fn     read_raw(void *p_buf, size_t memb_size, size_t n_memb, as_raw_msg_struct_t *p_raw)
 * \brief  read data from raw message block
 * \param  p_buf where to read
 * \param  memb_size size of elements to read
 * \param  n_memb # of elements to read
 * \param  p_raw struct to read from
 * \return actual # of elements read
 * ------------------------------------------------------------------------ */

static size_t read_raw(void *p_buf, size_t memb_size, size_t n_memb, as_raw_msg_struct_t *p_raw)
{
  size_t avl = ((p_raw->data_length > p_raw->rd_ptr)
              ? (p_raw->data_length - p_raw->rd_ptr) : 0) / memb_size,
             to_copy;

  if (avl < n_memb)
    n_memb = avl;
  to_copy = n_memb * memb_size;
  memcpy(p_buf, &p_raw->p_data[p_raw->rd_ptr], to_copy);
  p_raw->rd_ptr += to_copy;
  return n_memb;
}

/*!------------------------------------------------------------------------
 * \fn     read_raw_4(as_raw_msg_struct_t *p_raw, LongInt *p_dest)
 * \brief  read 32 bit integer from raw message block
 * \param  p_raw struct to read from
 * \param  p_dest where to read
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean read_raw_4(as_raw_msg_struct_t *p_raw, LongInt *p_dest)
{
  if (read_raw(p_dest, 4, 1, p_raw) != 1)
    return False;
  if (HostBigEndian)
    DSwap(p_dest, 4);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     create_raw_file(const char *p_file_name)
 * \brief  create raw message struct from file
 * \param  p_file_name file path & name
 * \return * to struct or NULL if failed
 * ------------------------------------------------------------------------ */

static as_raw_msg_struct_t *create_raw_file(const char *p_file_name)
{
  FILE *p_file = NULL;
  as_raw_msg_struct_t *p_result = NULL;
  Boolean success = False;

  p_file = fopen(p_file_name, OPENRDMODE);
  if (!p_file)
    goto func_exit;
  p_result = (as_raw_msg_struct_t*)calloc(1, sizeof(*p_result));
  if (!p_result)
    goto func_exit;
  fseek(p_file, 0, SEEK_END);
  p_result->data_length = ftell(p_file);
  if (p_result->data_length >= 65535u)
    p_result->data_length = 65535u;
  fseek(p_file, 0, SEEK_SET);

  p_result->p_data = (Byte*)malloc(p_result->data_length);
  if (p_result->data_length && !p_result->p_data)
    goto func_exit;
  p_result->data_allocated = True;
  if (fread(p_result->p_data, 1, p_result->data_length, p_file) != p_result->data_length)
    goto func_exit;

  success = True;

func_exit:
  if (p_file)
    fclose(p_file);
  if (p_result && !success)
  {
    free_raw(p_result);
    p_result = NULL;
  }
  return p_result;
}

/*****************************************************************************/

static void error(const char *Msg)
{
  fprintf(stderr, "message catalog handling: %s - program terminated\n", Msg);
  exit(255);
}

char *catgetmessage(PMsgCat Catalog, int Num)
{
  if ((Num >= 0) && (Num < Catalog->MsgCount))
    return Catalog->MsgBlock + Catalog->StrPosis[Num];
  else
  {
    static char *umess = NULL;

    if (!umess)
      umess = (char*)malloc(sizeof(char) * STRINGSIZE);
    as_snprintf(umess, STRINGSIZE, "catgetmessage: message number %d does not exist", Num);
    return umess;
  }
}

char *getmessage(int Num)
{
  return catgetmessage(&DefaultCatalog, Num);
}

/*!------------------------------------------------------------------------
 * \fn     check_header_raw(as_raw_msg_struct_t *p_raw, LongInt msg_id1, LongInt msg_id2)
 * \brief  check raw msg file for correct header
 * \param  p_raw file to check
 * \param  msg_id1, msg_id2 expected identifiers
 * \return True if file OK
 * ------------------------------------------------------------------------ */

static Boolean check_header_raw(as_raw_msg_struct_t *p_raw, LongInt msg_id1, LongInt msg_id2)
{
  String line;
  size_t ident_len = strlen(IdentString);
  LongInt r_id1, r_id2;

  return !seek_raw(p_raw, 0, SEEK_SET)
      && (ident_len == read_raw(line, 1, ident_len, p_raw))
      && !memcmp(line, IdentString, ident_len)
      && read_raw_4(p_raw, &r_id1)
      && (r_id1 == msg_id1)
      && read_raw_4(p_raw, &r_id2)
      && (r_id2 == msg_id2);
}

/*!------------------------------------------------------------------------
 * \fn     open_raw_file_and_check(const char *name, LongInt MsgId1, LongInt MsgId2)
 * \brief  open raw msg file from file and check header
 * \param  name file's name & path
 * \param  MsgId1, MsgId2, expected identifiers
 * \return * to raw file or NULL if failed
 * ------------------------------------------------------------------------ */

as_raw_msg_struct_t *open_raw_file_and_check(const char *name, LongInt MsgId1, LongInt MsgId2)
{
  as_raw_msg_struct_t *p_ret = NULL;

  p_ret = create_raw_file(name);
  if (!p_ret)
    return NULL;
  if (!check_header_raw(p_ret, MsgId1, MsgId2))
  {
    fprintf(stderr, "message catalog handling: warning: %s has invalid format or is out of date\n", name);
    free_raw(p_ret); p_ret = NULL;
  }
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     parse_raw_msg_file(PMsgCat p_cat, as_raw_msg_struct_t *p_msg_raw)
 * \brief  parse raw message catalog into runtime structure
 * \param  p_cat where to store result
 * \param  p_raw raw catalog to parse
 * ------------------------------------------------------------------------ */

static void parse_raw_file(PMsgCat p_cat, as_raw_msg_struct_t *p_msg_raw)
{
  LongInt DefPos = -1, MomPos, DefLength = 0, MomLength, z, StrStart, CtryCnt, Ctrys[100];
  unsigned StrCap;
  Boolean fi, Gotcha;
  const tNLSCharacterTab *CharacterTab;
  char *pStr, str[2048], *ptr;
  tNLSCharacter Ch;
  const char *lcstring;
  Word CountryCode;

  /* get reference for finding out which language set to use */

  CountryCode = NLS_GetCountryCode();
  lcstring = getenv("LC_MESSAGES");
  if (!lcstring)
    lcstring = getenv("LC_ALL");
  if (!lcstring)
    lcstring = getenv("LANG");
  if (!lcstring)
    lcstring = "";

  Gotcha = False;
  do
  {
    ptr = str;
    do
    {
      if (read_raw(ptr, 1, 1, p_msg_raw) != 1)
        error(ERdMsg);
      fi = (*ptr == '\0');
      if (!fi) ptr++;
    }
    while (!fi);
    if (*str != '\0')
    {
      if (!read_raw_4(p_msg_raw, &MomLength))
        error(ERdMsg);
      if (!read_raw_4(p_msg_raw, &CtryCnt))
        error(ERdMsg);
      for (z = 0; z < CtryCnt; z++)
        if (!read_raw_4(p_msg_raw, Ctrys + z))
          error(ERdMsg);
      if (!read_raw_4(p_msg_raw, &MomPos))
        error(ERdMsg);
      if (DefPos == -1)
      {
        DefPos = MomPos;
        DefLength = MomLength;
      }
      for (z = 0; z < CtryCnt; z++)
        if (Ctrys[z] == CountryCode)
          Gotcha = True;
      if (!Gotcha)
        Gotcha = !as_strncasecmp(lcstring, str, strlen(str));
    }
  }
  while ((*str != '\0') && (!Gotcha));
  if (*str == '\0')
  {
    MomPos = DefPos;
    MomLength = DefLength;
  }

  /* read pointer table */

  seek_raw(p_msg_raw, MomPos, SEEK_SET);
  if (!read_raw_4(p_msg_raw, &StrStart))
    error(ERdMsg);
  p_cat->MsgCount = (StrStart - MomPos) >> 2;
  p_cat->StrPosis = (LongInt *) malloc(sizeof(LongInt)*p_cat->MsgCount);
  p_cat->StrPosis[0] = 0;
  if ((int)read_raw(p_cat->StrPosis + 1, 4, p_cat->MsgCount - 1, p_msg_raw) + 1 != p_cat->MsgCount)
    error(ERdMsg);
  if (HostBigEndian)
    DSwap(p_cat->StrPosis + 1, (p_cat->MsgCount - 1) << 2);
  for (z = 1; z < p_cat->MsgCount; z++)
  {
    p_cat->StrPosis[z] -= StrStart;
    if ((p_cat->StrPosis[z] < 0) || (p_cat->StrPosis[z] >= MomLength))
      error(EIndMsg);
  }

  /* read string table */

  seek_raw(p_msg_raw, StrStart, SEEK_SET);
  p_cat->MsgBlock = (char *) malloc(MomLength);
  if ((int)read_raw(p_cat->MsgBlock, 1, MomLength, p_msg_raw) != MomLength)
    error(ERdMsg);

  /* character replacement according to runtime codepage */

  CharacterTab = GetCharacterTab(NLS_GetCodepage());
  for (z = 1; z < p_cat->MsgCount; z++)
  {
    pStr = p_cat->MsgBlock + p_cat->StrPosis[z];
    StrCap = strlen(pStr);
    for (Ch = (tNLSCharacter)0; Ch < eCH_cnt; Ch++)
      strreplace(pStr, NLS_HtmlCharacterTab[Ch], (*CharacterTab)[Ch], 2, StrCap);
  }
}

#define MSGPATHNAME "AS_MSGPATH"

/*!------------------------------------------------------------------------
 * \fn     msg_catalog_open_file(PMsgCat p_catalog, const char *p_file_name, const char *p_exe_path, LongInt msg_id1, LongInt msg_id2)
 * \brief  open message catalog from file
 * \param  p_catalog catalog to fill
 * \param  p_file_name catalog's file name
 * \param  p_exe_path executable's path
 * \param  file_msg_id1, file_msg_id1 file magic values to expect
 * ------------------------------------------------------------------------ */

void msg_catalog_open_file(PMsgCat p_catalog, const char *p_file_name, const char *p_exe_path, LongInt msg_id1, LongInt msg_id2)
{
  as_raw_msg_struct_t *p_msg_raw;
  String str;
  char *ptr;
  const char *pSep;

  /* find first valid message file: current directory has prio 1: */

  p_msg_raw = open_raw_file_and_check(p_file_name, msg_id1, msg_id2);

  /* if executable path (argv[0]) is given and contains more than just the
     plain name, try its path with next-highest prio: */

  if (!p_msg_raw && p_exe_path && *p_exe_path)
  {
#ifdef __CYGWIN32__
    for (ptr = (char*)p_exe_path; *ptr != '\0'; ptr++)
      if (*ptr == '/') *ptr = '\\';
#endif
    pSep = strrchr(p_exe_path, PATHSEP);
    if (pSep)
    {
      int path_len = pSep - p_exe_path;

      as_snprintf(str, sizeof(str), "%*.*s%s%s", path_len, path_len, p_exe_path, SPATHSEP, p_file_name);
      p_msg_raw = open_raw_file_and_check(str, msg_id1, msg_id2);
    }
  }

  if (!p_msg_raw)
  {
    /* AS_MSGPATH has priority over.. */

    ptr = getenv(MSGPATHNAME);
    if (ptr)
    {
      as_snprintf(str, sizeof(str), "%s%c%s", ptr, PATHSEP, p_file_name);
      p_msg_raw = open_raw_file_and_check(str, msg_id1, msg_id2);
    }

    /* ...PATH: */

    else
    {
      ptr = getenv("PATH");
      if (ptr)
      {
        String Dest;
        int Result;

#ifdef __CYGWIN32__
        DeCygWinDirList(pCopy);
#endif
        Result = FSearch(Dest, sizeof(Dest), p_file_name, NULL, ptr);
        p_msg_raw = Result ? NULL : open_raw_file_and_check(Dest, msg_id1, msg_id2);

        /* If we were found via PATH (no slashes in argv[0]/p_exe_path), look up this
           path and replace bin/ with lib/ for 'companion path': */

        if (!p_msg_raw && !strrchr(p_exe_path, PATHSEP) && !FSearch(Dest, sizeof(Dest), p_exe_path, NULL, ptr))
        {
          char *pSep2;

          strreplace(Dest, SPATHSEP "bin", SPATHSEP "lib", 0, sizeof(Dest));
          pSep2 = strrchr(Dest, PATHSEP);
          if (pSep2)
            *pSep2 = '\0';
          strmaxcat(Dest, SPATHSEP, sizeof(Dest));
          strmaxcat(Dest, p_file_name, sizeof(Dest));
          p_msg_raw = open_raw_file_and_check(Dest, msg_id1, msg_id2);
        }
      }
    }
  }

#ifdef LIBDIR
  if (!p_msg_raw)
  {
    as_snprintf(str, sizeof(str), "%s%c%s", LIBDIR, PATHSEP, p_file_name);
    p_msg_raw = open_raw_file_and_check(str, msg_id1, msg_id2);
  }
#endif

  if (!p_msg_raw)
  {
    as_snprintf(str, sizeof(str), EOpenMsg, p_file_name);
    error(str);
  }

  parse_raw_file(p_catalog, p_msg_raw);
  free_raw(p_msg_raw);
}

/*!------------------------------------------------------------------------
 * \fn     msg_catalog_open_buffer(PMsgCat p_catalog, const unsigned char *p_buffer, size_t buffer_size, LongInt msg_id1, LongInt msg_id2)
 * \brief  open message catalog from file
 * \param  p_catalog catalog to fill
 * \param  p_buffer buffer containing catalog
 * \param  buffer_size buffer's size
 * \param  msg_id1, msg_id1 file magic values to expect
 * ------------------------------------------------------------------------ */

void msg_catalog_open_buffer(PMsgCat p_catalog, const unsigned char *p_buffer, size_t buffer_size, LongInt msg_id1, LongInt msg_id2)
{
  as_raw_msg_struct_t *p_raw = (as_raw_msg_struct_t*)calloc(1, sizeof(*p_raw));

  p_raw->p_data = (Byte*)p_buffer;
  p_raw->data_allocated = False;
  p_raw->data_length = buffer_size;
  p_raw->rd_ptr = 0;

  if (!check_header_raw(p_raw, msg_id1, msg_id2))
  {
    fprintf(stderr, "message catalog handling: error: catalog'y buffer has invalid format or is out of date\n");
    exit(255);
  }

  parse_raw_file(p_catalog, p_raw);
  free_raw(p_raw);
}

/*!------------------------------------------------------------------------
 * \fn     nlmessages_init_file(const char *p_file_name, char *p_exe_path, LongInt msg_id1, LongInt msg_id2)
 * \brief  module initialization, load default catalog from file
 * \param  p_file_name catalog's file name
 * \param  p_exe_path executable's path
 * \param  msg_id1, msg_id2 catalog file magic to expect
 * ------------------------------------------------------------------------ */

void nlmessages_init_file(const char *p_file_name, char *p_exe_path, LongInt msg_id1, LongInt msg_id2)
{
  msg_catalog_open_file(&DefaultCatalog, p_file_name, p_exe_path, msg_id1, msg_id2);
}

/*!------------------------------------------------------------------------
 * \fn     nlmessages_init_buffer(const unsigned char *p_buffer, size_t buffer_size, LongInt msg_id1, LongInt msg_id2)
 * \brief  module initialization, load default catalog from buffer
 * \param  p_buffer buffer containing catalog
 * \param  buffer_size buffer's size
 * \param  msg_id1, msg_id2 catalog file magic to expect
 * ------------------------------------------------------------------------ */

void nlmessages_init_buffer(const unsigned char *p_buffer, size_t buffer_size, LongInt msg_id1, LongInt msg_id2)
{
  msg_catalog_open_buffer(&DefaultCatalog, p_buffer, buffer_size, msg_id1, msg_id2);
}
