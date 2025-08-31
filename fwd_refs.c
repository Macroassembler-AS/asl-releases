/* fwd_refs.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Store and resolve symbol forward references for the purpose of symbol     */
/* usage tracking                                                            */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"

#include "trees.h"

#include "fwd_refs.h"

typedef struct as_forward_ref_entry
{
  TTree tree;
  Boolean resolved;
} as_forward_ref_entry_t;

static as_forward_ref_entry_t *p_forward_ref_tree;

#define DBG_FORWARD_REF 0

#if DBG_FORWARD_REF

#include <stdarg.h>

static int fwd_ref_dbg_printf(const char *p_fmt, ...)
{
  va_list ap;
  int ret;
  va_start(ap, p_fmt);
  ret = vfprintf(stderr, p_fmt, ap);
  va_end(ap);
  return ret;
}

#define fwd_ref_dbg_tree(p_tree) DumpTree(stderr, p_tree)

#else /* !DBG_FORWARD_REF */

static int fwd_ref_dbg_printf(const char *p_fmt, ...)
{
  UNUSED(p_fmt);
  return 0;
}

#define fwd_ref_dbg_tree(p_tree) do { UNUSED(p_tree); } while (0)

#endif /* DBG_FORWARD_REF */

/*!------------------------------------------------------------------------
 * \fn     as_forward_ref_entry_t *create_forward_ref_entry(void)
 * \brief  create new (empty) forward ref entry
 * \return * to new entry or NULL
 * ------------------------------------------------------------------------ */

static as_forward_ref_entry_t *create_forward_ref_entry(void)
{
  as_forward_ref_entry_t *p_ret;

  p_ret = (as_forward_ref_entry_t*)calloc(1, sizeof(*p_ret));
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     destroy_forward_ref_entry(as_forward_ref_entry_t *p_entry)
 * \brief  destroy forward ref entry
 * \param  p_entry entry to destroy
 * ------------------------------------------------------------------------ */

static void destroy_forward_ref_entry(as_forward_ref_entry_t *p_entry)
{
  if (p_entry->tree.Name)
  {
    free(p_entry->tree.Name);
    p_entry->tree.Name = NULL;
  }
  free(p_entry);
}

/*!------------------------------------------------------------------------
 * \fn     as_forward_ref_add(const char *p_expanded_name)
 * \brief  add forward reference
 * \param  p_expanded_name fully expanded name of symbol
 * ------------------------------------------------------------------------ */

static Boolean forward_ref_adder(TTree **pp_dest, TTree *p_new, void *p_data)
{
  as_forward_ref_entry_t *p_new_entry = (as_forward_ref_entry_t*)p_new;
  (void)p_data;

  /* empty leaf? */

  if (!pp_dest)
  {
    p_new_entry->resolved = False;
    fwd_ref_dbg_printf("add forward_ref_entry %s/%d\n", p_new_entry->tree.Name, p_new_entry->tree.Attribute);
    return True;
  }

  /* no replacement necessary */

  destroy_forward_ref_entry(p_new_entry);
  return False;
}

void as_forward_ref_add(const char *p_expanded_name)
{
  as_forward_ref_entry_t *p_new = create_forward_ref_entry();

  p_new->tree.Name = as_strdup(p_expanded_name);
  p_new->tree.Attribute = (MomLocHandle >= 0) ? MomLocHandle : MomSectionHandle;
  fwd_ref_dbg_printf("enter_forward_ref_entry %s/%d\n", p_new->tree.Name, p_new->tree.Attribute);

  EnterTree((TTree**)&p_forward_ref_tree, &(p_new->tree), forward_ref_adder, NULL);
  fwd_ref_dbg_tree((TTree*)p_forward_ref_tree);
}

/*!------------------------------------------------------------------------
 * \fn     clear_forward_refs(void)
 * \brief  delete list of forward refs
 * ------------------------------------------------------------------------ */

static void forward_ref_destroyer(PTree p_node, void *p_data)
{
  UNUSED(p_node);
  UNUSED(p_data);
}

static void clear_forward_refs(void)
{
  DestroyTree((TTree**)&p_forward_ref_tree, forward_ref_destroyer, NULL);
}

/*!------------------------------------------------------------------------
 * \fn     as_forward_ref_search_and_mark(const char *p_name, LongInt attribute)
 * \brief  search list of forward refs for entries matching freshly defined symbols
 * \param  p_name symbol's name
 * \param  attribute's macro local handle
 * \return True if any references existed
 * ------------------------------------------------------------------------ */

Boolean as_forward_ref_search_and_mark(const char *p_name, LongInt attribute)
{
  as_tree_path_t tree_path;
  as_forward_ref_entry_t *p_entry = (as_forward_ref_entry_t*)find_tree_first(&p_forward_ref_tree->tree, p_name, &tree_path);
  Boolean ret = False;

  fwd_ref_dbg_printf("search_and_mark_forward_refs '%s' defined in %ld:\n",
                     p_name, (long)attribute);
  for (p_entry = (as_forward_ref_entry_t*)find_tree_first(&p_forward_ref_tree->tree, p_name, &tree_path);
       p_entry;
       p_entry = (as_forward_ref_entry_t*)find_tree_next(p_name, &tree_path))
  {
    fwd_ref_dbg_printf(" forward from %ld:\n", (long)p_entry->tree.Attribute);

    if (p_entry->resolved)
    {
      fwd_ref_dbg_printf(" ->already resolved\n");
      continue;
    }

    /* (1) The symbol is neither in a section, nor in a macro body.
           It matches all references: */

    if (attribute == -1)
      p_entry->resolved = ret = True;

    /* (2) The symbol is in a macro body.  It is only visible for
           references in the same macro body: */

    else if (attribute >= LOC_HANDLE_OFFSET)
    {
      if (p_entry->tree.Attribute == attribute)
        p_entry->resolved = ret = True;
    }

    /* (3) The symbol is in a section.  It is visible for references
           from the same section, or from one of its subsections.
           It should also be visible from macro bodies inside the
           same or a sub section, but we do not yet have this
           information: */

    else
    {
      if ((p_entry->tree.Attribute == attribute)
       || is_sub_section(p_entry->tree.Attribute, attribute))
         p_entry->resolved = ret = True;
    }
    fwd_ref_dbg_printf(" -> %sresolved\n", p_entry->resolved ? "" : "not ");
  }
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_forward_ref_init(void)
 * \brief  module initialization
 * ------------------------------------------------------------------------ */

void as_forward_ref_init(void)
{
  add_exit_pass_proc(clear_forward_refs);
}
