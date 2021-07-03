/* nonzstring.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Handling of non-NUL terminated stringd                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "strutil.h"

#include "nonzstring.h"

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_ini(as_nonz_dynstr_t *p_str, size_t ini_capacity)
 * \brief  create/initialize string
 * \param  p_str string to set up
 * \param  ini_capacity initial capacity
 * ------------------------------------------------------------------------ */

void as_nonz_dynstr_ini(as_nonz_dynstr_t *p_str, size_t ini_capacity)
{
  p_str->p_str = (char*)calloc(1, ini_capacity);
  p_str->capacity = p_str->p_str ? ini_capacity : 0;
  p_str->len = 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_ini_c_str(as_nonz_dynstr_t *p_str, const char *p_src)
 * \brief  create/initialize string from C string
 * \param  p_str string to set up
 * \param  p_src data source
 * ------------------------------------------------------------------------ */

void as_nonz_dynstr_ini_c_str(as_nonz_dynstr_t *p_str, const char *p_src)
{
  size_t len = strlen(p_src);

  as_nonz_dynstr_ini(p_str, as_nonz_dynstr_roundup_len(len));
  p_str->len = (p_str->capacity < len) ? p_str->capacity : len;
  memcpy(p_str->p_str, p_src, p_str->len);
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_realloc(as_nonz_dynstr_t *p_str, size_t new_capacity)
 * \brief  resize buffer of string
 * \param  p_str string to resize
 * \param  new_capacity new capacity
 * ------------------------------------------------------------------------ */

void as_nonz_dynstr_realloc(as_nonz_dynstr_t *p_str, size_t new_capacity)
{
  size_t old_capacity = p_str->capacity;
  char *p_new_str = p_str->p_str
                  ? (char*)realloc(p_str->p_str, new_capacity)
                  : (char*)malloc(new_capacity);

  if (p_new_str)
  {
    p_str->p_str = p_new_str;
    p_str->capacity = new_capacity;
    if (new_capacity > old_capacity)
      memset(p_str->p_str + old_capacity, 0, new_capacity - old_capacity);
    else if (p_str->len > new_capacity)
      p_str->len = new_capacity;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_free(as_nonz_dynstr_t *p_str)
 * \brief  free_destroy string
 * \param  p_str string to free
 * ------------------------------------------------------------------------ */

void as_nonz_dynstr_free(as_nonz_dynstr_t *p_str)
{
  if (p_str->p_str)
    free(p_str->p_str);
  p_str->p_str = NULL;
  p_str->len = p_str->capacity = 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_to_c_str(char *p_dest, const as_nonz_dynstr_t *p_src, size_t dest_len)
 * \brief  convert string to C-style string
 * \param  p_dest destination
 * \param  p_src source
 * \param  dest_len dest capacity
 * \return result length
 * ------------------------------------------------------------------------ */

size_t as_nonz_dynstr_to_c_str(char *p_dest, const as_nonz_dynstr_t *p_src, size_t dest_len)
{
  if (dest_len > 0)
  {
    unsigned TransLen = dest_len - 1;

    if (TransLen > p_src->len)
      TransLen = p_src->len;
    memcpy(p_dest, p_src->p_str, TransLen);
    p_dest[TransLen] = '\0';
    return TransLen;
  }
  else
    return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_copy(as_nonz_dynstr_t *p_dest, const as_nonz_dynstr_t *p_src)
 * \brief  copy string
 * \param  p_dest destination
 * \param  p_src source
 * \return actual dest length after copy
 * ------------------------------------------------------------------------ */

size_t as_nonz_dynstr_copy(as_nonz_dynstr_t *p_dest, const as_nonz_dynstr_t *p_src)
{
  size_t trans_len;

  if (p_dest->capacity < p_src->capacity)
    as_nonz_dynstr_realloc(p_dest, p_src->capacity);
  trans_len = p_src->len;
  if (trans_len > p_dest->capacity)
    trans_len = p_dest->capacity;
  memcpy(p_dest->p_str, p_src->p_str, p_dest->len = trans_len);
  return p_dest->len;
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_append_raw(as_nonz_dynstr_t *p_dest, const char *p_src, int src_len)
 * \brief  extend string
 * \param  p_dest string to extend
 * \param  p_src what to append
 * \param  src_len source length (-1 for NUL-terminated source)
 * \return actual # of bytes transferred
 * ------------------------------------------------------------------------ */

size_t as_nonz_dynstr_append_raw(as_nonz_dynstr_t *p_dest, const char *p_src, int src_len)
{
  size_t trans_len;

  if (src_len < 0)
    src_len = strlen(p_src);

  if (p_dest->len + src_len > p_dest->capacity)
    as_nonz_dynstr_realloc(p_dest, as_nonz_dynstr_roundup_len(p_dest->len + src_len));
  trans_len = p_dest->capacity - p_dest->len;
  if ((size_t)src_len < trans_len)
    trans_len = src_len;
  memcpy(p_dest->p_str + p_dest->len, p_src, trans_len);
  p_dest->len += trans_len;
  return trans_len;
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_copy_c_str(as_nonz_dynstr_t *p_dest, const char *p_src)
 * \brief  convert C-style string to non-NUL terminated string
 * \param  p_dest string to fill
 * \param  p_src string to fill from
 * \return actual # of characters appended
 * ------------------------------------------------------------------------ */

size_t as_nonz_dynstr_copy_c_str(as_nonz_dynstr_t *p_dest, const char *p_src)
{
  p_dest->len = 0;
  return as_nonz_dynstr_append_raw(p_dest, p_src, -1);
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_append(as_nonz_dynstr_t *p_dest, const as_nonz_dynstr_t *p_src)
 * \brief  extend string by non-NUL terminated string
 * \param  p_dest string to extend
 * \param  p_src string to append
 * \return actual # of characters appended
 * ------------------------------------------------------------------------ */

size_t as_nonz_dynstr_append(as_nonz_dynstr_t *p_dest, const as_nonz_dynstr_t *p_src)
{
  return as_nonz_dynstr_append_raw(p_dest, p_src->p_str, p_src->len);
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_cmp(const as_nonz_dynstr_t *p_str1, const as_nonz_dynstr_t *p_str2)
 * \brief  compare two strings
 * \param  p_str1, p_str2 strings to compare
 * \return -1/0/+1 if Str1 </==/> Str2
 * ------------------------------------------------------------------------ */

int as_nonz_dynstr_cmp(const as_nonz_dynstr_t *p_str1, const as_nonz_dynstr_t *p_str2)
{
  return strlencmp(p_str1->p_str, p_str1->len, p_str2->p_str, p_str2->len);
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_find(const as_nonz_dynstr_t *p_haystack, const as_nonz_dynstr_t *p_needle)
 * \brief  find first occurence of substring
 * \param  p_haystack where to search
 * \param  p_needle what to search
 * \return first position or -1 if not found
 * ------------------------------------------------------------------------ */

int as_nonz_dynstr_find(const as_nonz_dynstr_t *p_haystack, const as_nonz_dynstr_t *p_needle)
{
  int pos, maxpos = ((int)p_haystack->len) - ((int)p_needle->len);

  for (pos = 0; pos <= maxpos; pos++)
    if (!memcmp(p_haystack->p_str + pos, p_needle->p_str, p_needle->len))
      return pos;

  return -1;
}

/*!------------------------------------------------------------------------
 * \fn     as_nonz_dynstr_dump_hex(FILE *p_file, const as_dynstr_t *p_str)
 * \brief  debug helper
 * \param  p_file where to dump
 * \param  p_str string to dump
 * ------------------------------------------------------------------------ */

void as_nonz_dynstr_dump_hex(FILE *p_file, const as_nonz_dynstr_t *p_str)
{
  const char *p_run;

  fprintf(p_file, "[%u/%u]", (unsigned)p_str->len, (unsigned)p_str->capacity);
  for (p_run = p_str->p_str; *p_run; p_run++) fprintf(p_file, " %02x", *p_run & 0xff);
  fprintf(p_file, "\n");
}
