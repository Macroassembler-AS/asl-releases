#ifndef _TEXUTIL_H
#define _TEXUTIL_H
/* texutil.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX-->ASCII/HTML Converter: Common Utils/Variables                        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "datatypes.h"

/*---------------------------------------------------------------------------*/

typedef struct tex_infile
{
  struct tex_infile *p_next;
  char *p_name;
  FILE *p_file;
  int curr_line, curr_column;
} tex_infile_t;

extern tex_infile_t *p_curr_tex_infile;

extern const char *p_infile_name, *p_outfile_name;
extern FILE *p_outfile;

extern Boolean DoRepass;

/*---------------------------------------------------------------------------*/

extern void tex_infile_pop(void);
extern void tex_infile_pop_all(void);
extern void tex_infile_push(const char *p_name);

extern void tex_verror(const char *p_msg, va_list ap);
extern void tex_error(const char *p_msg, ...);
extern void tex_vwarning(const char *p_msg, va_list ap);
extern void tex_warning(const char *p_msg, ...);

#endif /* _TEXUTIL_H */
