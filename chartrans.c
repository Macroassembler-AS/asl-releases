/* chartrans.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Character Translation                                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <errno.h>
#include <string.h>
#include "nonzstring.h"
#include "chartrans.h"

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_new(void)
 * \brief  allocate new table
 * \return * to table or NULL
 * ------------------------------------------------------------------------ */

as_chartrans_table_t as_chartrans_table_new(void)
{
  as_chartrans_table_t p_ret = (as_chartrans_table_t)calloc(256, sizeof(*p_ret));
  if (p_ret)
    as_chartrans_table_reset(p_ret);
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_free(as_chartrans_table_t p_table)
 * \brief  deallocate table
 * \param  p_table table to free
 * ------------------------------------------------------------------------ */

void as_chartrans_table_free(as_chartrans_table_t p_table)
{
  if (p_table)
    free(p_table);
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_dup(as_chartrans_table_t p_table)
 * \brief  duplicate table
 * \param  p_table origin
 * \return duplicate or NULL
 * ------------------------------------------------------------------------ */

as_chartrans_table_t as_chartrans_table_dup(as_chartrans_table_t p_table)
{
  as_chartrans_table_t p_ret = NULL;

  if (!p_table)
    return p_ret;

  p_ret = (as_chartrans_table_t)calloc(256, sizeof(*p_ret));
  memcpy(p_ret, p_table, 256 * sizeof(*p_table));
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     void as_chartrans_table_reset(as_chartrans_table_t p_table)
 * \brief  reset table to default
 * \param  p_table table to reset
 * ------------------------------------------------------------------------ */

void as_chartrans_table_reset(as_chartrans_table_t p_table)
{
  unsigned z;

  for (z = 0; z < 256; z++)
    p_table[z] = z;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_set(as_chartrans_table_t p_table, unsigned input_code, unsigned output_code)
 * \brief  change translation entry
 * \param  p_table ta ble to update
 * \param  input_code new source value
 * \param  output_code new translated value
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_table_set(as_chartrans_table_t p_table, unsigned input_code, unsigned output_code)
{
  if ((input_code > 255) || (output_code > 255))
    return E2BIG;
  p_table[input_code] = output_code;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_set_mult(as_chartrans_table_t p_table, unsigned input_code_from, unsigned input_code_to, unsigned output_code)
 * \brief  change translation entry block
 * \param  p_table ta ble to update
 * \param  input_code_from new source start value
 * \param  input_code_to new source end value (inclusive)
 * \param  output_code new translated value
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_table_set_mult(as_chartrans_table_t p_table, unsigned input_code_from, unsigned input_code_to, unsigned output_code)
{
  if (input_code_from > input_code_to)
    return EBADF;
  if ((input_code_from > 255) || (input_code_to > 255)
   || (output_code > 255) || (output_code + (input_code_to - input_code_from - 1) > 255))
    return E2BIG;
  for (; input_code_from <= input_code_to; input_code_from++, output_code++)
    p_table[input_code_from] = output_code;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_print(as_chartrans_table_t p_table, FILE *p_file)
 * \brief  print table contents
 * \param  p_table table to print
 * \param  p_file where to print
 * ------------------------------------------------------------------------ */

void as_chartrans_table_print(as_chartrans_table_t p_table, FILE *p_file)
{
  int z, z2;

  for (z = 0; z < 16; z++)
  {
    for (z2 = 0; z2 < 16; z2++)
      fprintf(p_file, " %02x", p_table[z * 16 + z2]);
    fprintf(p_file, "  ");
    for (z2 = 0; z2 < 16; z2++)
      fprintf(p_file, "%c", p_table[z * 16 + z2] > ' ' ? p_table[z * 16 + z2] : '.');
    fputc('\n', p_file);
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate(const as_chartrans_table_t p_table, unsigned src)
 * \brief  translate single character
 * \param  p_table translation table to use
 * \param  src character to translate
 * \return translated character or -1
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate(const as_chartrans_table_t p_table, unsigned src)
{
  return (src > 255) ? -1 : p_table[src];
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate_rev(const as_chartrans_table_t p_table, unsigned src)
 * \brief  untranslate single character
 * \param  p_table translation table to use
 * \param  src character to untranslate
 * \return untranslated character or -1
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate_rev(const as_chartrans_table_t p_table, unsigned src)
{
  unsigned z;

  if (src > 255)
    return -1;
  for (z = 0; z < 256; z++)
    if (p_table[z] == src)
      return z;
  return -1;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate_next(const as_chartrans_table_t p_table, unsigned *p_dest, const char **pp_src, size_t *p_src_len)
 * \brief  translate single character in string
 * \param  p_table translation table to use
 * \param  p_dest p_dest where to write result
 * \param  pp_src pp_src source string (will be incremented by consumed # of characters)
 * \param  p_src_len remaining length of source string
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate_next(const as_chartrans_table_t p_table, unsigned *p_dest, const char **pp_src, size_t *p_src_len)
{
  if (!*p_src_len)
    return ENOMEM;
  *p_dest = p_table[((usint)(**pp_src)) & 0xff];
  (*pp_src)++; (*p_src_len)--;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate_nonz_dynstr(const as_chartrans_table_t p_table, struct as_nonz_dynstr *p_str)
 * \brief  translate complete string
 * \param  p_table translation table to use
 * \param  p_str string to translate
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate_nonz_dynstr(const as_chartrans_table_t p_table, struct as_nonz_dynstr *p_str)
{
  char *p_run, *p_end;

  for (p_run = p_str->p_str, p_end = p_run + p_str->len; p_run < p_end; p_run++)
    *p_run = p_table[((usint)(*p_run)) & 0xff];
  return 0;
}
