/* asmitree.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Opcode-Abfrage als Binaerbaum                                             */
/*                                                                           */
/* Historie: 30.10.1996 Grundsteinlegung                                     */
/*            8.10.1997 Hash-Tabelle                                         */
/*            6.12.1998 dynamisches Kopieren der Namen                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "chunks.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"

#include "asmitree.h"

/*---------------------------------------------------------------------------*/

static int GetKey(const char *Name, LongWord TableSize)
{
  register unsigned char *p;
  LongWord tmp = 0;

  for (p = (unsigned char *)Name; *p != '\0'; p++)
    tmp = (tmp << 2) + ((LongWord)*p);
  return tmp % TableSize;
}

PInstTable CreateInstTable(int TableSize)
{
  int z;
  PInstTableEntry tmp;
  PInstTable tab;

  tmp = (PInstTableEntry) malloc(sizeof(TInstTableEntry) * TableSize);
  for (z = 0; z < TableSize; z++)
    tmp[z].Name = NULL;
  tab = (PInstTable) malloc(sizeof(TInstTable));
  tab->Fill = 0;
  tab->Size = TableSize;
  tab->Entries = tmp;
  tab->Dynamic = FALSE;
  tab->prefix_proc = NULL;
  tab->prefix_proc_index = 0;
  return tab;
}

/*!------------------------------------------------------------------------
 * \fn     inst_fnc_table_create(int table_size)
 * \brief  create new instance of function table
 * \param  table_size max. # of entries in table
 * \return * to table
 * ------------------------------------------------------------------------ */

inst_fnc_table_t *inst_fnc_table_create(int table_size)
{
  int z;
  inst_fnc_table_entry_t *p_entries = NULL;
  inst_fnc_table_t *p_ret = NULL;

  p_entries = (inst_fnc_table_entry_t*)calloc(table_size, sizeof(*p_entries));
  if (!p_entries)
    goto func_exit;
  for (z = 0; z < table_size; z++)
    p_entries[z].p_name = NULL;

  p_ret = (inst_fnc_table_t*) calloc(1, sizeof(*p_ret));
  if (!p_ret)
    goto func_exit;
  p_ret->fill = 0;
  p_ret->size = table_size;
  p_ret->p_entries = p_entries; p_entries = NULL;
  p_ret->dynamic = False;

func_exit:
  if (p_entries)
    free(p_entries);
  return p_ret;
}

void SetDynamicInstTable(PInstTable Table)
{
  Table->Dynamic = TRUE;
}

void DestroyInstTable(PInstTable tab)
{
  int z;

  if (!tab)
    return;

  if (tab->Dynamic)
    for (z = 0; z < tab->Size; z++)
      free(tab->Entries[z].Name);

  free(tab->Entries);
  free(tab);
}

void AddInstTable(PInstTable tab, const char *Name, Word Index, InstProc Proc)
{
  LongWord h0 = GetKey(Name, tab->Size), z = 0;

  /* mindestens ein freies Element lassen, damit der Sucher garantiert terminiert */

  if (tab->Size - 1 <= tab->Fill)
  {
    fprintf(stderr, "\nhash table overflow\n");
    exit(255);
  }
  while (1)
  {
    if (!tab->Entries[h0].Name)
    {
      int dest_index = 0;
      tab->Entries[h0].Name = (tab->Dynamic) ? as_strdup(Name) : (char*)Name;
      memset(tab->Entries[h0].Procs, 0, sizeof(tab->Entries[h0].Procs));
      if (tab->prefix_proc)
      {
        tab->Entries[h0].Procs[dest_index] = tab->prefix_proc;
        tab->Entries[h0].Indices[dest_index++] = tab->prefix_proc_index;
      }
      tab->Entries[h0].Procs[dest_index] = Proc;
      tab->Entries[h0].Indices[dest_index++] = Index;
      tab->Entries[h0].Coll = z;
      tab->Fill++;
      return;
    }
    if (!strcmp(tab->Entries[h0].Name, Name))
    {
      printf("%s double in table\n", Name);
      exit(255);
    }
    z++;
    if ((LongInt)(++h0) == tab->Size)
      h0 = 0;
  }
}

void inst_table_set_prefix_proc(PInstTable tab, InstProc prefix_proc, Word prefix_proc_index)
{
  tab->prefix_proc = prefix_proc;
  tab->prefix_proc_index = prefix_proc_index;
}

/*!------------------------------------------------------------------------
 * \fn     inst_fnc_table_add(inst_fnc_table_t *p_table, const char *p_name, Word index, inst_fnc_t fnc)
 * \brief  augment table by one entry
 * \param  p_table table to add to
 * \param  p_name name of instruction
 * \param  index callback argument
 * \param  fnc callback itself
 * ------------------------------------------------------------------------ */

void inst_fnc_table_add(inst_fnc_table_t *p_table, const char *p_name, Word index, inst_fnc_t fnc)
{
  LongWord tab_index = GetKey(p_name, p_table->size), num_coll = 0;

  /* mindestens ein freies Element lassen, damit der Sucher garantiert terminiert */

  if (p_table->size - 1 <= p_table->fill)
  {
    fprintf(stderr, "\nhash table overflow\n");
    exit(255);
  }
  while (1)
  {
    if (!p_table->p_entries[tab_index].p_name)
    {
      p_table->p_entries[tab_index].p_name = (p_table->dynamic) ? as_strdup(p_name) : (char*)p_name;
      p_table->p_entries[tab_index].fnc = fnc;
      p_table->p_entries[tab_index].index = index;
      p_table->p_entries[tab_index].coll = num_coll;
      p_table->fill++;
      return;
    }
    if (!strcmp(p_table->p_entries[tab_index].p_name, p_name))
    {
      printf("%s double in table\n", p_name);
      exit(255);
    }
    num_coll++;
    if ((LongInt)(++tab_index) == p_table->size)
      tab_index = 0;
  }
}

void RemoveInstTable(PInstTable tab, const char *Name)
{
  LongWord h0 = GetKey(Name, tab->Size);

  while (1)
  {
    if (!tab->Entries[h0].Name)
      return;
    else if (!strcmp(tab->Entries[h0].Name, Name))
    {
      tab->Entries[h0].Name = NULL;
      memset(tab->Entries[h0].Procs, 0, sizeof(tab->Entries[h0].Procs));
      tab->Fill--;
      return;
    }
    if ((LongInt)(++h0) == tab->Size)
      h0 = 0;
  }
}

const TInstTableEntry *inst_table_search(PInstTable p_table, const char *p_name)
{
  LongWord h0 = GetKey(p_name, p_table->Size);

  while (1)
  {
    if (!p_table->Entries[h0].Name)
      return NULL;
    else if (!strcmp(p_table->Entries[h0].Name, p_name))
      return &p_table->Entries[h0];
    if ((LongInt)(++h0) == p_table->Size)
      h0 = 0;
  }
}

void inst_table_exec(const TInstTableEntry *p_inst_table_entry)
{
  size_t index;

  for (index = 0; index < as_array_size(p_inst_table_entry->Procs); index++)
    if (p_inst_table_entry->Procs[index])
      p_inst_table_entry->Procs[index](p_inst_table_entry->Indices[index]);
    else
      return;
}

Boolean LookupInstTable(PInstTable tab, const char *Name)
{
  const TInstTableEntry *p_inst_table_entry;

  p_inst_table_entry = inst_table_search(tab, Name);

  if (p_inst_table_entry)
    inst_table_exec(p_inst_table_entry);
  return !!p_inst_table_entry;
}

/*!------------------------------------------------------------------------
 * \fn     inst_fnc_table_lookup(const inst_fnc_table_t *p_table, const char *p_name)
 * \brief  look up & execute instruction callback
 * \param  p_table table with functions
 * \param  p_name instruction name
 * \return True if callback was found and executed
 * ------------------------------------------------------------------------ */

Boolean inst_fnc_table_lookup(const inst_fnc_table_t *p_table, const char *p_name)
{
  LongWord index = GetKey(p_name, p_table->size);

  while (1)
  {
    if (!p_table->p_entries[index].p_name)
      return False;
    else if (!strcmp(p_table->p_entries[index].p_name, p_name))
      return p_table->p_entries[index].fnc(p_table->p_entries[index].index);
    if ((LongInt)(++index) == p_table->size)
      index = 0;
  }
}

void PrintInstTable(FILE *stream, PInstTable tab)
{
  int z;

  for (z = 0; z < tab->Size; z++)
    if (tab->Entries[z].Name)
      fprintf(stream, "[%3d]: %-10s Index %4d Coll %2d\n", z,
             tab->Entries[z].Name, tab->Entries[z].Indices[0], tab->Entries[z].Coll);
}

/*----------------------------------------------------------------------------*/

void asmitree_init(void)
{
}
