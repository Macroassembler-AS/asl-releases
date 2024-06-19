/* texutil.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX-->ASCII/HTML Converter: Common Utils/Variables                        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "strutil.h"
#include "texutil.h"

/*--------------------------------------------------------------------------*/

Boolean DoRepass;

tex_infile_t *p_curr_tex_infile = NULL;

const char *p_infile_name, *p_outfile_name;
FILE *p_outfile = NULL;

/*--------------------------------------------------------------------------*/

static void tex_infile_free(tex_infile_t *p_infile)
{
  if (p_infile->p_name)
  {
    free(p_infile->p_name);
    p_infile->p_name = NULL;
  }
  if (p_infile->p_file)
  {
    fclose(p_infile->p_file);
    p_infile->p_file = NULL;
  }
  free(p_infile);
}

void tex_infile_pop(void)
{
  tex_infile_t *p_old = p_curr_tex_infile;
  if (!p_old)
    return;
  p_curr_tex_infile = p_old->p_next;
  tex_infile_free(p_old);
}

void tex_infile_pop_all(void)
{
  while (p_curr_tex_infile)
    tex_infile_pop();
}

void tex_infile_push(const char *p_name)
{
  tex_infile_t *p_new = (tex_infile_t*)calloc(1, sizeof(*p_new));

  p_new->p_name = as_strdup(p_name);
  p_new->p_file = fopen(p_name, "r");
  if (!p_new->p_file)
  {
    char msg[200];

    tex_infile_free(p_new);
    as_snprintf(msg, sizeof(msg), "file %s not found", p_name);
    tex_error(msg);
  }
  else
  {
    p_new->p_next = p_curr_tex_infile;
    p_curr_tex_infile = p_new;
  }
}

/*!------------------------------------------------------------------------
 * \fn     tex_warning(const char *p_msg, ...)
 * \brief  print warning
 * \param  p_msg warning message
 * ------------------------------------------------------------------------ */

void tex_vwarning(const char *p_msg, va_list ap)
{
  fprintf(stderr, "%s:%d.%d: ",
          p_curr_tex_infile ? p_curr_tex_infile->p_name : "<internal>",
          p_curr_tex_infile ? p_curr_tex_infile->curr_line : 0,
          p_curr_tex_infile ? p_curr_tex_infile->curr_column : 0);
  vfprintf(stderr, p_msg, ap);
  fprintf(stderr, "\n");
}

void tex_warning(const char *p_msg, ...)
{
  va_list ap;

  va_start(ap, p_msg);
  tex_vwarning(p_msg, ap);
  va_end(ap);
}

/*!------------------------------------------------------------------------
 * \fn     tex_error(const char *p_msg, ...)
 * \brief  print warning
 * \param  pMsg error message & abort
 * ------------------------------------------------------------------------ */

void tex_verror(const char *p_msg, va_list ap)
{
  tex_vwarning(p_msg, ap);
  tex_infile_pop_all();
  if (p_outfile)
    fclose(p_outfile);
  exit(2);
}

void tex_error(const char *p_msg, ...)
{
  va_list ap;

  va_start(ap, p_msg);
  tex_verror(p_msg, ap);
  va_end(ap);
}
