/* literals.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Literal Handling Functions                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <assert.h>

#include "strutil.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmerr.h"
#include "literals.h"

/*---------------------------------------------------------------------------*/

as_literal_t *p_first_literal;

LongInt literal_forward_count;

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     literals_chk_alldone(void)
 * \brief  check whether all literals have been resolved
 * ------------------------------------------------------------------------ */

void literals_chk_alldone(void)
{
  while (p_first_literal)
  {
    as_literal_t *p_run;
    WrError(ErrNum_MsgMissingLTORG);

    p_run = p_first_literal;
    p_first_literal = p_run->p_next;
    literals_free(p_run);
  }
}

/*!------------------------------------------------------------------------
 * \fn     literals_new(void)
 * \brief  create new literal
 * \return * to new instance or NULL
 * ------------------------------------------------------------------------ */

as_literal_t *literals_new(void)
{
  as_literal_t *p_ret = (as_literal_t*)calloc(1, sizeof(*p_ret));
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     literals_free(as_literal_t* p_literal)
 * \brief  destroy literal
 * \param   p_literalliteral to free
 * ------------------------------------------------------------------------ */

void literals_free(as_literal_t* p_literal)
{
  free(p_literal);
}

/*!------------------------------------------------------------------------
 * \fn     literals_get_name(const as_literal_t *p_lit, char *p_result, size_t result_size)
 * \brief  deduce name of literal
 * \param  p_lit literal in question
 * \param  p_result where to write result
 * \param  result_size capacity of buffer
 * \return * to b uffer
 * ------------------------------------------------------------------------ */

const char *literals_get_name(const as_literal_t *p_lit, char *p_result, size_t result_size)
{
  as_snprintf(p_result, result_size, "LITERAL_");
  if (p_lit->is_forward)
    as_snprcatf(p_result, result_size, "F_%lllx", (LargeWord)p_lit->f_count);
  else switch (p_lit->size)
  {
    case eSymbolSize32Bit:
      as_snprcatf(p_result, result_size, "L_%08lllx", (LargeWord)p_lit->value);
      break;
    case eSymbolSize16Bit:
      as_snprcatf(p_result, result_size, "W_%04x", (unsigned)p_lit->value);
      break;
    case eSymbolSize12Bit:
      as_snprcatf(p_result, result_size, "W_%03x", (unsigned)p_lit->value);
      break;
    default:
      assert(0);
  }
  as_snprcatf(p_result, result_size, "_%x", (unsigned)p_lit->pass_no);
  return p_result;
}

/*!------------------------------------------------------------------------
 * \fn     literals_print(void)
 * \brief  dump current list of literals
 * ------------------------------------------------------------------------ */

void literals_print(void)
{
  as_literal_t *p_run;
  String Name;

  WrLstLine("LiteralList");
  p_run = p_first_literal;
  while (p_run)
  {
    literals_get_name(p_run, Name, sizeof(Name));
    WrLstLine(Name); p_run = p_run->p_next;
  }
}

/*!------------------------------------------------------------------------
 * \fn     literal_make(tStrComp *p_comp, Byte *p_data_offset, LongInt value, tSymbolSize value_size, Boolean force_create)
 * \brief  create/lookup literal, and return its symbolic name
 * \param  p_comp destination buffer for symbol name
 * \param  p_data_offset address offset into symbols's address (assuming big endian)
 * \param  value value of literal
 * \param  value_size operand size of literal
 * \param  force_create do not re-use esisting literals
 * ------------------------------------------------------------------------ */

void literal_make(tStrComp *p_comp, Byte *p_data_offset, LongInt value, tSymbolSize value_size, Boolean force_create)
{
  as_literal_t *p_run, *p_prev;
  String name;

  switch (value_size)
  {
    case eSymbolSize32Bit:
      break;
    case eSymbolSize16Bit:
      value &= 0xffff;
      break;
    case eSymbolSize12Bit:
      value &= 07777;
      break;
    default:
      assert(0);
  }

  /* existing literal available? */

  if (p_data_offset) *p_data_offset = 0;
  for (p_run = p_first_literal, p_prev = NULL; p_run; p_prev = p_run, p_run = p_run->p_next)
  {
    if (!force_create && !p_run->is_forward && (p_run->def_section == MomSectionHandle))
    {
      if (((p_run->size == value_size) && (value == p_run->value))
       || ((p_run->size == eSymbolSize32Bit) && p_data_offset && (value_size == eSymbolSize16Bit) && (value == (p_run->value >> 16))))
        break;
      else if ((p_run->size == eSymbolSize32Bit) && p_data_offset && (value_size == eSymbolSize16Bit) && (value == (p_run->value & 0xffff)))
      {
        *p_data_offset = 2;
        break;
      }
    }
  }

  /* no - create new one */

  if (!p_run)
  {
    p_run = literals_new();
    p_run->size = value_size;
    p_run->value = value;
    p_run->is_forward = force_create;
    if (force_create)
      p_run->f_count = literal_forward_count++;
    p_run->def_section = MomSectionHandle;
    for (p_run->pass_no = 1; ; p_run->pass_no++)
    {
      /* literals are section-specific */

      as_snprintf(p_comp->str.p_str, p_comp->str.capacity, "%s[PARENT0]",
                  literals_get_name(p_run, name, sizeof(name)));
      if (!IsSymbolDefined(p_comp))
        break;
    }
    if (!p_prev)
      p_first_literal = p_run;
    else
      p_prev->p_next = p_run;
  }
  else
  {
    as_snprintf(p_comp->str.p_str, p_comp->str.capacity, "%s[PARENT0]",
                literals_get_name(p_run, name, sizeof(name)));
  }
}

/*!------------------------------------------------------------------------
 * \fn     literals_dump(literal_dump_fnc_t dump, tSymbolSize value_size, LongInt section_handle, Boolean compress)
 * \brief  dump & free accumulated literals
 * \param  dump callback to actually dispose value in memory
 * \param  value_size only dump literals of given size
 * \param  section_handle only dump literals out of given section
 * \param  compress reference literals of same value to same address?
 * ------------------------------------------------------------------------ */

void literals_dump(literal_dump_fnc_t dump, tSymbolSize value_size, LongInt section_handle, Boolean compress)
{
  as_literal_t *p_curr, *p_last, *p_tmp;
  LargeInt address;
  String name;
  tStrComp tmp_comp;

  StrCompMkTemp(&tmp_comp, name, sizeof(name));
  p_curr = p_first_literal;
  p_last = NULL;
  while (p_curr)
  {
    if ((p_curr->size == value_size) && (p_curr->def_section == section_handle))
    {
      literals_get_name(p_curr, name, sizeof(name));
      address = dump(p_curr, &tmp_comp);
      if (compress)
      {
        as_literal_t *p_eq_curr = p_curr->p_next, *p_eq_last = NULL;
        while (p_eq_curr)
        {
          if ((p_eq_curr->size == value_size) && (p_eq_curr->def_section == section_handle) && (p_eq_curr->value == p_curr->value))
          {
            literals_get_name(p_eq_curr, name, sizeof(name));
            EnterIntSymbol(&tmp_comp, address, ActPC, False);
            p_tmp = p_eq_curr->p_next;
            if (!p_eq_last)
              p_curr->p_next = p_tmp;
            else
              p_eq_last->p_next = p_tmp;
            literals_free(p_eq_curr);
            p_eq_curr = p_tmp;
          }
          else
          {
            p_eq_last = p_eq_curr;
            p_eq_curr = p_eq_curr->p_next;
          }
        }
      }
      p_tmp = p_curr->p_next;
      if (!p_last)
        p_first_literal = p_tmp;
      else
        p_last->p_next = p_tmp;
      literals_free(p_curr);
      p_curr = p_tmp;
    }
    else
    {
      p_last = p_curr;
      p_curr = p_curr->p_next;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     literals_init(void)
 * \brief  module initialization
 * ------------------------------------------------------------------------ */

static void init_pass_literals(void)
{
  literal_forward_count = 0;
}

void literals_init(void)
{
  AddInitPassProc(init_pass_literals);
  p_first_literal = NULL;
}
