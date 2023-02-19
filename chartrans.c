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
#include "asmerr.h"
#include "chartrans.h"

static void clr_mapped(as_chartrans_table_t *p_table, unsigned code)
{
  p_table->mapped[code >> 3] &= ~(1 << (code & 7));
}

static void set_mapped(as_chartrans_table_t *p_table, unsigned code)
{
  p_table->mapped[code >> 3] |= 1 << (code & 7);
}

static Boolean is_mapped(const as_chartrans_table_t *p_table, unsigned code)
{
  return (p_table->mapped[code >> 3] >> (code & 7)) & 1;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_new(void)
 * \brief  allocate new table
 * \return * to table or NULL
 * ------------------------------------------------------------------------ */

as_chartrans_table_t *as_chartrans_table_new(void)
{
  as_chartrans_table_t *p_ret = (as_chartrans_table_t*)calloc(1, sizeof(*p_ret));
  if (p_ret)
    as_chartrans_table_reset(p_ret);
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_free(as_chartrans_table_t *p_table)
 * \brief  deallocate table
 * \param  p_table table to free
 * ------------------------------------------------------------------------ */

void as_chartrans_table_free(as_chartrans_table_t *p_table)
{
  if (p_table)
    free(p_table);
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_dup(const as_chartrans_table_t *p_table)
 * \brief  duplicate table
 * \param  p_table origin
 * \return duplicate or NULL
 * ------------------------------------------------------------------------ */

as_chartrans_table_t *as_chartrans_table_dup(const as_chartrans_table_t *p_table)
{
  as_chartrans_table_t *p_ret = NULL;

  if (!p_table)
    return p_ret;

  p_ret = (as_chartrans_table_t*)calloc(1, sizeof(*p_ret));
  memcpy(p_ret->mapped, p_table->mapped, sizeof(p_ret->mapped));
  memcpy(p_ret->mapping, p_table->mapping, sizeof(p_ret->mapping));
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     void as_chartrans_table_reset(as_chartrans_table_t *p_table)
 * \brief  reset table to default
 * \param  p_table table to reset
 * ------------------------------------------------------------------------ */

void as_chartrans_table_reset(as_chartrans_table_t *p_table)
{
  unsigned z;

  for (z = 0; z < 256; z++)
    p_table->mapping[z] = z;
  memset(p_table->mapped, 0xff, sizeof(p_table->mapped));
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_set(as_chartrans_table_t *p_table, unsigned input_code, unsigned output_code)
 * \brief  change translation entry
 * \param  p_table table to update
 * \param  input_code new source value
 * \param  output_code new translated value
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_table_set(as_chartrans_table_t *p_table, unsigned input_code, unsigned output_code)
{
  if ((input_code > 255) || (output_code > 255))
    return E2BIG;
  set_mapped(p_table, input_code);
  p_table->mapping[input_code] = output_code;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_unset(as_chartrans_table_t *p_table, unsigned input_code)
 * \brief  invalidate translation entry
 * \param  p_table table to update
 * \param  input_code new source value
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_table_unset(as_chartrans_table_t *p_table, unsigned input_code)
{
  if (input_code > 255)
    return E2BIG;
  clr_mapped(p_table, input_code);
  p_table->mapping[input_code] = 0;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_set_mult(as_chartrans_table_t *p_table, unsigned input_code_from, unsigned input_code_to, unsigned output_code)
 * \brief  change translation entry block
 * \param  p_table table to update
 * \param  input_code_from new source start value
 * \param  input_code_to new source end value (inclusive)
 * \param  output_code new translated value
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_table_set_mult(as_chartrans_table_t *p_table, unsigned input_code_from, unsigned input_code_to, unsigned output_code)
{
  if (input_code_from > input_code_to)
    return EBADF;
  if ((input_code_from > 255) || (input_code_to > 255)
   || (output_code > 255) || (output_code + (input_code_to - input_code_from - 1) > 255))
    return E2BIG;
  for (; input_code_from <= input_code_to; input_code_from++, output_code++)
  {
    set_mapped(p_table, input_code_from);
    p_table->mapping[input_code_from] = output_code;
  }
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_unset_mult(as_chartrans_table_t *p_table, unsigned input_code_from, unsigned input_code_to)
 * \brief  invalidate translation entry block
 * \param  p_table table to update
 * \param  input_code_from new source start value
 * \param  input_code_to new source end value (inclusive)
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_table_unset_mult(as_chartrans_table_t *p_table, unsigned input_code_from, unsigned input_code_to)
{
  if (input_code_from > input_code_to)
    return EBADF;
  if ((input_code_from > 255) || (input_code_to > 255))
    return E2BIG;
  for (; input_code_from <= input_code_to; input_code_from++)
  {
    clr_mapped(p_table, input_code_from);
    p_table->mapping[input_code_from] = 0;
  }
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_table_print(const as_chartrans_table_t *p_table, FILE *p_file)
 * \brief  print table contents
 * \param  p_table table to print
 * \param  p_file where to print
 * ------------------------------------------------------------------------ */

void as_chartrans_table_print(const as_chartrans_table_t *p_table, FILE *p_file)
{
  int z, z2, code;

  for (z = 0; z < 16; z++)
  {
    for (z2 = 0; z2 < 16; z2++)
    {
      code = z * 16 + z2;
      if (is_mapped(p_table, code))
        fprintf(p_file, " %02x", p_table->mapping[code]);
      else
        fprintf(p_file, " --");
    }
    fprintf(p_file, "  ");
    for (z2 = 0; z2 < 16; z2++)
    {
      code = z * 16 + z2;
      if (is_mapped(p_table, code))
        fputc(p_table->mapping[code] > ' ' ? p_table->mapping[code] : '.', p_file);
      else
        fputc(' ', p_file);
    }
    fputc('\n', p_file);
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate(const as_chartrans_table_t *p_table, unsigned src)
 * \brief  translate single character
 * \param  p_table translation table to use
 * \param  src character to translate
 * \return translated character or -1
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate(const as_chartrans_table_t *p_table, unsigned src)
{
  return ((src <= 255) && is_mapped(p_table, src)) ? p_table->mapping[src] : -1;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate_rev(const as_chartrans_table_t *p_table, unsigned src)
 * \brief  untranslate single character
 * \param  p_table translation table to use
 * \param  src character to untranslate
 * \return untranslated character or -1
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate_rev(const as_chartrans_table_t *p_table, unsigned src)
{
  unsigned z;

  if (src > 255)
    return -1;
  for (z = 0; z < 256; z++)
    if (is_mapped(p_table, z) && (p_table->mapping[z] == src))
      return z;
  return -1;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate_next(const as_chartrans_table_t *p_table, unsigned *p_dest, const char **pp_src, size_t *p_src_len)
 * \brief  translate single character in string
 * \param  p_table translation table to use
 * \param  p_dest p_dest where to write result
 * \param  pp_src pp_src source string (will be incremented by consumed # of characters)
 * \param  p_src_len remaining length of source string
 * \return 0 if success
 *         ENOMEN -> end of string
 *         ENOENT -> unmappable character
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate_next(const as_chartrans_table_t *p_table, unsigned *p_dest, const char **pp_src, size_t *p_src_len)
{
  usint code;

  if (!*p_src_len)
    return ENOMEM;
  code = ((usint)(**pp_src)) & 0xff;
  if (!is_mapped(p_table, code))
    return ENOENT;
  *p_dest = p_table->mapping[code];
  (*pp_src)++; (*p_src_len)--;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_chartrans_xlate_nonz_dynstr(const as_chartrans_table_t *p_table, struct as_nonz_dynstr *p_str, const struct sStrComp *p_src_arg)
 * \brief  translate complete string
 * \param  p_table translation table to use
 * \param  p_str string to translate
 * \param  p_src_arg source argument in case error occurs
 * \return 0 if success
 * ------------------------------------------------------------------------ */

int as_chartrans_xlate_nonz_dynstr(const as_chartrans_table_t *p_table, struct as_nonz_dynstr *p_str, const struct sStrComp *p_src_arg)
{
  char *p_run, *p_end;

  for (p_run = p_str->p_str, p_end = p_run + p_str->len; p_run < p_end; p_run++)
  {
    usint code = ((usint)(*p_run)) & 0xff;
    if (!is_mapped(p_table, code))
    {
      if (p_src_arg)
        WrStrErrorPos(ErrNum_UnmappedChar, p_src_arg);
      return ENOENT;
    }
    *p_run = p_table->mapping[code];
  }
  return 0;
}
