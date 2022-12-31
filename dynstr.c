/* dynstr.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Handling of strings with dynamic allocation                               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "dynstr.h"

static void check_dynamic(const as_dynstr_t *p_str)
{
  if (!p_str->dynamic)
  {
    fprintf(stderr, "attemt to resize non-dynamic string\n");
    abort();
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_ini(as_dynstr_t *p_str, size_t AllocLen)
 * \brief  initialize empty string with given capacity
 * \param  p_str string to initialize
 * \param  AllocLen initial alloc length
 * ------------------------------------------------------------------------ */

void as_dynstr_ini(as_dynstr_t *p_str, size_t ini_capacity)
{
  p_str->p_str = ini_capacity ? (char*)malloc(ini_capacity) : NULL;
  p_str->capacity = p_str->p_str ? ini_capacity : 0;
  p_str->dynamic = !!p_str->p_str;
  memset(p_str->p_str, 0, p_str->capacity);
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_ini_clone(as_dynstr_t *p_str, const as_dynstr_t *p_src)
 * \brief  initialize empty string from other string
 * \param  p_str string to initialize
 * \param  p_src string to clone
 * ------------------------------------------------------------------------ */

void as_dynstr_ini_clone(as_dynstr_t *p_str, const as_dynstr_t *p_src)
{
  p_str->p_str = (char*)malloc(p_src->capacity);
  if (p_str->p_str)
  {
    strcpy(p_str->p_str, p_src->p_str);
    p_str->capacity = p_src->capacity;
    p_str->dynamic = 1;
  }
  else
  {
    p_str->capacity = 0;
    p_str->dynamic = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_ini_c_str(as_dynstr_t *p_str, const char *p_src)
 * \brief  initialize string from C string
 * \param  p_str string to initialize
 * \param  p_src init source
 * ------------------------------------------------------------------------ */

void as_dynstr_ini_c_str(as_dynstr_t *p_str, const char *p_src)
{
  size_t capacity = as_dynstr_roundup_len(strlen(p_src));

  p_str->p_str = (char*)malloc(capacity);
  if (p_str->p_str)
  {
    strcpy(p_str->p_str, p_src);
    p_str->capacity = capacity;
    p_str->dynamic = 1;
  }
  else
  {
    p_str->capacity = 0;
    p_str->dynamic = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_realloc(as_dynstr_t *p_str, size_t new_capacity)
 * \brief  shrink/grow string's capacity
 * \param  p_str string to adapt
 * \param  new_capacity new capacity
 * \return 0 if success or error code
 * ------------------------------------------------------------------------ */

int as_dynstr_realloc(as_dynstr_t *p_str, size_t new_capacity)
{
  char *p_new;
  size_t old_capacity;

  check_dynamic(p_str);
  p_new = (char*)realloc(p_str->p_str, new_capacity);
  old_capacity = p_str->capacity;
  if (p_new)
  {
    p_str->p_str = p_new;
    p_str->capacity = new_capacity;
    if (new_capacity > old_capacity)
      memset(p_str->p_str + old_capacity, 0, new_capacity - old_capacity);
    else if (new_capacity > 0)
      p_str->p_str[new_capacity - 1] = '\0';
    return 0;
  }
  else
    return ENOMEM;
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_free(as_dynstr_t *p_str)
 * \brief  free/destroy string
 * \param  p_str string to handle
 * ------------------------------------------------------------------------ */

void as_dynstr_free(as_dynstr_t *p_str)
{
  if (p_str->dynamic && p_str->p_str)
    free(p_str->p_str);
  p_str->p_str = NULL;
  p_str->capacity = 0;
  p_str->dynamic = 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_copy(as_dynstr_t *p_dest, const as_dynstr_t *p_src)
 * \brief  set string from other string
 * \param  p_str string to set
 * \param  p_src init source
 * \return actual # of characters copied
 * ------------------------------------------------------------------------ */

size_t as_dynstr_copy(as_dynstr_t *p_dest, const as_dynstr_t *p_src)
{
  return as_dynstr_copy_c_str(p_dest, p_src->p_str);
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_copy_c_str(as_dynstr_t *p_dest, const char *p_src)
 * \brief  set string from C string
 * \param  p_str string to set
 * \param  p_src init source
 * \return actual # of characters copied
 * ------------------------------------------------------------------------ */

size_t as_dynstr_copy_c_str(as_dynstr_t *p_dest, const char *p_src)
{
  size_t len = strlen(p_src);

  if ((len >= p_dest->capacity) && p_dest->dynamic)
    as_dynstr_realloc(p_dest, as_dynstr_roundup_len(len));

  if (len >= p_dest->capacity)
    len = p_dest->capacity - 1;
  memcpy(p_dest->p_str, p_src, len);
  p_dest->p_str[len] = '\0';
  return len;
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_append(as_dynstr_t *p_dest, const char *p_src, size_t src_len)
 * \brief  extend string
 * \param  p_dest string to extend
 * \param  p_src what to append
 * \param  src_len how much to append
 * \return actual # of bytes transferred
 * ------------------------------------------------------------------------ */

size_t as_dynstr_append(as_dynstr_t *p_dest, const char *p_src, size_t src_len)
{
  size_t dest_len = strlen(p_dest->p_str);

  if ((dest_len + src_len + 1 > p_dest->capacity) && p_dest->dynamic)
    as_dynstr_realloc(p_dest, as_dynstr_roundup_len(dest_len + src_len));
  if (src_len >= p_dest->capacity - dest_len)
    src_len = p_dest->capacity - dest_len - 1;
  memcpy(p_dest->p_str + dest_len, p_src, src_len);
  p_dest->p_str[dest_len + src_len] = '\0';
  return src_len;
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_append_c_str(as_dynstr_t *p_dest, const char *p_src)
 * \brief  extend string
 * \param  p_dest string to extend
 * \param  p_src what to append
 * \return actual # of bytes transferred
 * ------------------------------------------------------------------------ */

size_t as_dynstr_append_c_str(as_dynstr_t *p_dest, const char *p_src)
{
  return as_dynstr_append(p_dest, p_src, strlen(p_src));
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_dump_hex(FILE *p_file, const as_dynstr_t *p_str)
 * \brief  debug helper
 * \param  p_file where to dump
 * \param  p_str string to dump
 * ------------------------------------------------------------------------ */

void as_dynstr_dump_hex(FILE *p_file, const as_dynstr_t *p_str)
{
  const char *p_run;

  fprintf(p_file, "[%u]", (unsigned)p_str->capacity);
  for (p_run = p_str->p_str; *p_run; p_run++) fprintf(p_file, " %02x", *p_run & 0xff);
  fprintf(p_file, "\n");
}
