/* codevars.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Gemeinsame Variablen aller Codegeneratoren                                */
/*                                                                           */
/* Historie: 26.5.1997 Grundsteinlegung                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include "asmdef.h"
#include "asmitree.h"

int InstrZ;
int AdrCnt;
struct sInstTable *InstTable,
                  *main_inst_table_no_leading_dot,
                  *main_inst_table_leading_dot;

/*!------------------------------------------------------------------------
 * \fn     as_augment_main_inst_tables(const char *p_name, Word arg, InstProc proc, Boolean nodot_occupied)
 * \brief  add pseudo instruction to premacro-expansion instruction hash tables
 * \param  p_name pseudo instruction's name
 * \param  arg callback arg
 * \param  proc callback itself
 * \param  nodot_occupied instruction without leading dot is a machine instruction?
 * ------------------------------------------------------------------------ */

void as_augment_main_inst_tables(const char *p_name, Word arg, InstProc proc, Boolean nodot_occupied)
{
  /* Add with and without leading dot? */

  if (*p_name == '.')
  {
    /* Allow without dot only if not conflicting with machine instruction: */

    if (!nodot_occupied)
      AddInstTable(main_inst_table_no_leading_dot, p_name + 1, arg, proc);

    /* If the parser will search for an attribute part on the given target,
       '.insn' will be parsed to 'insn' and oppart_leading_dot will be set: */

    if (HasAttrs)
      AddInstTable(main_inst_table_leading_dot, p_name + 1, arg, proc);

    /* No search for attribute part: Enter the instruction including leading
       dot into the 'normal' hash table.  main_inst_table_leading_dot is NULL
       in this case: */

    else
      AddInstTable(main_inst_table_no_leading_dot, p_name, arg, proc);
  }

  /* Add only without leading dot.  Thou should not set nodot_occupied in this
     case... */

  else
  {
    if (!nodot_occupied)
      AddInstTable(main_inst_table_no_leading_dot, p_name, arg, proc);
  }
}
