/* striter.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* String Iteration                                                          */
/*                                                                           */
/*****************************************************************************/

#include "striter.h"

/* iterator state variables, used in loop */

typedef struct
{
  Boolean this_escaped, next_escaped;
} iter_data_t;

/*!------------------------------------------------------------------------
 * \fn     iterate_quote_core(const char *p_run, as_quoted_iterator_cb_data_t *p_cb_data, iter_data_t *p_iter_data)
 * \brief  common quoting/escaping character handling while iterating string
 * \param  p_run current position in string
 * \param  p_cb_data, p_iter_data iteration callback
 * \return True if this was a special character
 * ------------------------------------------------------------------------ */

static Boolean iterate_quote_core(const char *p_run, as_quoted_iterator_cb_data_t *p_cb_data, iter_data_t *p_iter_data)
{
  p_iter_data->next_escaped = False;
  switch(*p_run)
  {
    case '\\':
      if ((p_cb_data->in_single_quote || p_cb_data->in_double_quote) && !p_iter_data->this_escaped)
        p_iter_data->next_escaped = True;
      return True;
    case '\'':
      if (p_cb_data->in_double_quote) { }
      else if (!p_cb_data->in_single_quote && (!p_cb_data->qualify_quote || p_cb_data->qualify_quote(p_cb_data->p_str, p_run)))
        p_cb_data->in_single_quote = True;
      else if (!p_iter_data->this_escaped) /* skip escaped ' in '...' */
        p_cb_data->in_single_quote = False;
      return True;
    case '"':
      if (p_cb_data->in_single_quote) { }
      else if (!p_cb_data->in_double_quote)
        p_cb_data->in_double_quote = True;
      else if (!p_iter_data->this_escaped) /* skip escaped " in "..." */
        p_cb_data->in_double_quote = False;
      return True;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_iterate_str(const char *p_str, as_quoted_iterator_cb_t callback, as_quoted_iterator_cb_data_t *p_cb_data)
 * \brief  iterate through string, detecting quoted areas
 * \param  p_str string to iterate through
 * \param  callback is called for all characters outside quoted areas
 * \param  p_cb_data callback data
 * ------------------------------------------------------------------------ */

void as_iterate_str(const char *p_str, as_quoted_iterator_cb_t callback, as_quoted_iterator_cb_data_t *p_cb_data)
{
  iter_data_t iter_data;
  int n_extra_skip;
  const char *p_run;

  p_cb_data->p_str = p_str;
  p_cb_data->in_single_quote =
  p_cb_data->in_double_quote = False;

  for (p_run = p_str, iter_data.this_escaped = False;
       *p_run;
       p_run += (1 + n_extra_skip), iter_data.this_escaped = iter_data.next_escaped)
  {
    n_extra_skip = 0;

    (void)iterate_quote_core(p_run, p_cb_data, &iter_data);
    n_extra_skip = callback(p_run, p_cb_data);
    if (n_extra_skip < 0)
      return;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_iterate_str_quoted(const char *p_str, as_quoted_iterator_cb_t callback, as_quoted_iterator_cb_data_t *p_cb_data)
 * \brief  iterate through string, skipping quoted areas
 * \param  p_str string to iterate through
 * \param  callback is called for all characters outside quoted areas
 * \param  p_cb_data callback data
 * ------------------------------------------------------------------------ */

void as_iterate_str_quoted(const char *p_str, as_quoted_iterator_cb_t callback, as_quoted_iterator_cb_data_t *p_cb_data)
{
  iter_data_t iter_data;
  int n_extra_skip;
  const char *p_run;

  p_cb_data->p_str = p_str;
  p_cb_data->in_single_quote =
  p_cb_data->in_double_quote = False;

  for (p_run = p_str, iter_data.this_escaped = False;
       *p_run;
       p_run += (1 + n_extra_skip), iter_data.this_escaped = iter_data.next_escaped)
  {
    if (p_cb_data->callback_before && !p_cb_data->in_single_quote && !p_cb_data->in_double_quote)
    {
      n_extra_skip = callback(p_run, p_cb_data);
      if (n_extra_skip < 0)
        return;
    }
    else
      n_extra_skip = 0;

    if (!iterate_quote_core(p_run, p_cb_data, &iter_data)
     && !p_cb_data->callback_before
     && !p_cb_data->in_single_quote
     && !p_cb_data->in_double_quote)
    {
      n_extra_skip = callback(p_run, p_cb_data);
      if (n_extra_skip < 0)
        return;
    }
  }
}
