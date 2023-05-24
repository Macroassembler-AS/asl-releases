/* cpu2phys.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* CPU->Physical Address Translation                                         */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "strutil.h"
#include "tempresult.h"
#include "asmdef.h"
#include "cpu2phys.h"

typedef struct
{
  LargeWord cpu_start, cpu_end;
  LargeWord phys_start, phys_end;
} cpu_2_phys_area_t;

typedef struct
{
  cpu_2_phys_area_t *areas;
  size_t capacity;
  size_t cnt;
  LargeWord end;
} cpu_2_phys_area_list_t;

static cpu_2_phys_area_list_t area_lists[SegCount];

/*!------------------------------------------------------------------------
 * \fn     cpu_2_phys_area_t *insert_area(cpu_2_phys_area_list_t *p_list, unsigned index)
 * \brief  make space for area at given index, implicitly increasing array size
 * \param  p_list list to operate on
 * \param  index place to insert
 * \return * to freed and cleared area
 * ------------------------------------------------------------------------ */

static cpu_2_phys_area_t *insert_area(cpu_2_phys_area_list_t *p_list, unsigned index)
{
  cpu_2_phys_area_t *p_ret;

  assert(index <= p_list->cnt);

  if (p_list->cnt >= p_list->capacity)
  {
    size_t new_capacity = p_list->cnt + 1;
    size_t new_alloc = new_capacity * sizeof(*p_list->areas);
    p_list->areas = p_list->capacity
                  ? (cpu_2_phys_area_t*) realloc(p_list->areas, new_alloc)
                  : (cpu_2_phys_area_t*) malloc(new_alloc);
    p_list->capacity = new_capacity;
  }
  p_ret = &p_list->areas[index];
  p_list->cnt++;
  memmove(p_ret + 1, p_ret, sizeof(*p_ret) * (p_list->cnt - 1 - index));
  memset(p_ret, 0, sizeof(*p_ret));
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     delete_area(cpu_2_phys_area_list_t *p_list, unsigned index)
 * \brief  delete entry from table
 * \param  p_list list to operate on
 * \param  index place to delete
 * ------------------------------------------------------------------------ */

static void delete_area(cpu_2_phys_area_list_t *p_list, unsigned index)
{
  cpu_2_phys_area_t *p_ret;

  assert(index < p_list->cnt);
  p_ret = &p_list->areas[index];
  memmove(p_ret, p_ret + 1, sizeof(*p_ret) * (p_list->cnt - 1 - index));
  p_list->cnt--;
}

/*!------------------------------------------------------------------------
 * \fn     get_list(as_addrspace_t addr_space)
 * \brief  retrieve list per address space
 * \param  addr_space address space
 * \return * to list
 * ------------------------------------------------------------------------ */

static cpu_2_phys_area_list_t *get_list(as_addrspace_t addr_space)
{
  assert(addr_space < SegCount);
  return &area_lists[addr_space];
}

/*!------------------------------------------------------------------------
 * \fn     cpu_2_phys_area_clear(as_addrspace_t addr_space)
 * \brief  clear tables before adding new ones
 * \param  addr_space address space to operate on
 * ------------------------------------------------------------------------ */

void cpu_2_phys_area_clear(as_addrspace_t addr_space)
{
  assert(addr_space < SegCount);
  area_lists[addr_space].cnt = 0;
  area_lists[addr_space].end = 0;
}

/*!------------------------------------------------------------------------
 * \fn     cpu_2_phys_area_add(as_addrspace_t addr_space, LargeWord new_cpu_start, LargeWord new_phys_start, LargeWord new_len)
 * \brief  add another area
 * \param  addr_space address space to operate on
 * \param  new_cpu_start start address in CPU address space
 * \param  new_phys_start start address in physical address space
 * \param  new_len area size in <gran>
 * ------------------------------------------------------------------------ */

void cpu_2_phys_area_add(as_addrspace_t addr_space, LargeWord new_cpu_start, LargeWord new_phys_start, LargeWord new_len)
{
  size_t z, z2;
  cpu_2_phys_area_list_t *p_list = get_list(addr_space);
  cpu_2_phys_area_t *p_area;
  LargeWord new_cpu_end = new_cpu_start + (new_len - 1);
  LargeWord overlap_start, overlap_end, delta;
  Boolean change;

  /* Do not add zero-length areas */

  if (!new_len)
    return;

  /* sort in according to CPU start address */

  for (z = 0; z < p_list->cnt; z++)
    if (p_list->areas[z].cpu_start >= new_cpu_start)
      break;

  p_area = insert_area(p_list, z);
  p_area->cpu_start = new_cpu_start;
  p_area->cpu_end = new_cpu_end;
  p_area->phys_start = new_phys_start;
  p_area->phys_end = new_phys_start + (new_len - 1);

  /* shrink/delete (partially) overlapping areas: */

  do
  {
    change = False;
    for (z2 = 0; z2 < p_list->cnt; z2++)
    {
      /* do not test overlap with new entry itself */

      if (z2 == z)
        continue;

      /* deduce overlapping area */

      overlap_start = max(p_list->areas[z].cpu_start, p_list->areas[z2].cpu_start);
      overlap_end = min(p_list->areas[z].cpu_end, p_list->areas[z2].cpu_end);
      if (overlap_start >= overlap_end)
        continue;

      /* Delete old area entirely? -> possibly correct index of new entry and restart */

      if ((overlap_start == p_list->areas[z2].cpu_start)
       && (overlap_end == p_list->areas[z2].cpu_end))
      {
        delete_area(p_list, z2);
        if (z2 < z)
          z--;
        change = True;
        break;
      }

      /* shorten old area at beginning? */

      else if (overlap_start == p_list->areas[z2].cpu_start)
      {
        delta = overlap_end - overlap_start + 1;
        p_list->areas[z2].cpu_start += delta;
        p_list->areas[z2].phys_start += delta;
      }

      /* shorten old area at end? */

      else if (overlap_end == p_list->areas[z2].cpu_end)
      {
        delta = overlap_end - overlap_start + 1;
        p_list->areas[z2].cpu_end -= delta;
        p_list->areas[z2].phys_end -= delta;
      }

      /* Overlap cuts out in the mid, split into parts. Assuming the addresses were sorted,
         z2 equals z - 1 and the old area's part surround the new one: */

      else
      {
        cpu_2_phys_area_t save = p_list->areas[z2];

        delta = save.cpu_end - overlap_start + 1;
        p_list->areas[z2].cpu_end -= delta;
        p_list->areas[z2].phys_end -= delta;
        p_area = insert_area(p_list, z + 1);
        *p_area = save;
        delta = overlap_end - save.cpu_start + 1;
        p_area->cpu_start += delta;
        p_area->phys_start += delta;
        change = True;
        break;
      }
    }
  }
  while (change);
}

/*!------------------------------------------------------------------------
 * \fn     cpu_2_phys_area_set_cpu_end(as_addrspace_t addr_space, LargeWord cpu_end)
 * \brief  set the end of the CPU's address space
 * \param  addr_space address space to operate on
 * \param  cpu_end end of CPU address space
 * ------------------------------------------------------------------------ */

void cpu_2_phys_area_set_cpu_end(as_addrspace_t addr_space, LargeWord cpu_end)
{
  cpu_2_phys_area_list_t *p_list = get_list(addr_space);

  p_list->end = cpu_end;
}

/*!------------------------------------------------------------------------
 * \fn     cpu_2_phys_area_fill(as_addrspace_t addr_space, LargeWord cpu_start, LargeWord cpu_end)
 * \brief  fill gaps in the CPU-side address space with 1:1 mappings
 * \param  addr_space address space to operate on
 * \param  cpu_start start of CPU address space
 * \param  cpu_end end of CPU address space
 * ------------------------------------------------------------------------ */

void cpu_2_phys_area_fill(as_addrspace_t addr_space, LargeWord cpu_start, LargeWord cpu_end)
{
  LargeWord expected_start;
  size_t z;
  cpu_2_phys_area_list_t *p_list = get_list(addr_space);
  cpu_2_phys_area_t *p_area;

  /* For each mapping, check whether there is no gap between its start address
     and the predecessor's end address.  If so, insert a 1:1 mapped area to
     fill the gap: */

  z = 0;
  while (z < p_list->cnt)
  {
    expected_start = z ? p_list->areas[z - 1].cpu_end + 1 : cpu_start;

    /* no gap -> just continue with the next entry */

    if ((z < p_list->cnt) && (p_list->areas[z].cpu_start <= expected_start))
    {
      z++;
      continue;
    }
    p_area = insert_area(p_list, z);
    p_area->cpu_start = 
    p_area->phys_start = expected_start;
    p_area->cpu_end =
    p_area->phys_end = (z + 1 < p_list->cnt) ? p_list->areas[z + 1].cpu_start - 1 : cpu_end;

    /* We know the entries at z (freshly inserted) and z+1 (moved up one index) are
       continuous, so we may increase the counter by two: */

    z += 2;
  }

  /* Do the same test once again at the very end of the array, to be sure it covers everything
     up to the given cpu_end: */

  if (!p_list->cnt || p_list->areas[p_list->cnt - 1].cpu_end < cpu_end)
  {
    expected_start = p_list->cnt ? p_list->areas[p_list->cnt - 1].cpu_end + 1 : cpu_start;
    p_area = insert_area(p_list, p_list->cnt);
    p_area->cpu_start = 
    p_area->phys_start = expected_start;
    p_area->cpu_end =
    p_area->phys_end = cpu_end;
  }

  /* Save the cpu_end for usage in phys_2_cpu(): */

  cpu_2_phys_area_set_cpu_end(addr_space, cpu_end);
}

/*!------------------------------------------------------------------------
 * \fn     cpu_2_phys_area_dump(as_addrspace_t addr_space, FILE *p_file)
 * \brief  output current mapping
 * \param  addr_space address space to operate on
 * \param  p_file where to dump
 * ------------------------------------------------------------------------ */

void cpu_2_phys_area_dump(as_addrspace_t addr_space, FILE *p_file)
{
  cpu_2_phys_area_list_t *p_list = get_list(addr_space);
  char str[100];
  size_t z;
  int cpu_max_len = 0, phys_max_len = 0;

  for (z = 0; z < p_list->cnt; z++)
  {
    int this_len;

    this_len = as_snprintf(str, sizeof str, "%lllx", p_list->areas[z].cpu_end);
    if (this_len > cpu_max_len)
      cpu_max_len = this_len;
    this_len = as_snprintf(str, sizeof str, "%lllx", p_list->areas[z].phys_end);
    if (this_len > phys_max_len)
      phys_max_len = this_len;
  }
  for (z = 0; z < p_list->cnt; z++)
  {
    as_snprintf(str, sizeof str, "[%2u] %0*lllx...%0*lllx -> %0*lllx...%0*lllx\n", z,
                cpu_max_len, p_list->areas[z].cpu_start,
                cpu_max_len, p_list->areas[z].cpu_end,
                phys_max_len, p_list->areas[z].phys_start,
                phys_max_len, p_list->areas[z].phys_end);
    fputs(str, p_file);
  }
}

/*!------------------------------------------------------------------------
 * \fn     def_phys_2_cpu(as_addrspace_t addr_space, LargeWord *p_address)
 * \brief  physical -> CPU address translation
 * \param  addr_space address space to operate on
 * \param  p_address address to translate
 * \return True if translation succeeded
 * ------------------------------------------------------------------------ */

Boolean def_phys_2_cpu(as_addrspace_t addr_space, LargeWord *p_address)
{
  cpu_2_phys_area_list_t *p_list = get_list(addr_space);
  size_t win;

  for (win = 0; win < p_list->cnt; win++)
    if ((*p_address >= p_list->areas[win].phys_start) && (*p_address <= p_list->areas[win].phys_end))
    {
      *p_address = (*p_address - p_list->areas[win].phys_start) + p_list->areas[win].cpu_start;
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     def_cpu_2_phys(as_addrspace_t addr_space, LargeWord *p_address)
 * \brief  CPU -> physical address translation
 * \param  addr_space address space to operate on
 * \param  p_address address to translate
 * \return True if translation succeeded
 * ------------------------------------------------------------------------ */

Boolean def_cpu_2_phys(as_addrspace_t addr_space, LargeWord *p_address)
{
  cpu_2_phys_area_list_t *p_list = get_list(addr_space);
  size_t win;

  for (win = 0; win < p_list->cnt; win++)
    if ((*p_address >= p_list->areas[win].cpu_start) && (*p_address <= p_list->areas[win].cpu_end))
    {
      *p_address = (*p_address - p_list->areas[win].cpu_start) + p_list->areas[win].phys_start;
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     fnc_phys_2_cpu(TempResult *p_ret, const TempResult *p_args, unsigned arg_cnt)
 * \brief  built-in function for physical -> CPU address translation
 * \param  p_ret returns CPU address
 * \param  p_args physical address argument
 * \return True if translation possible at all
 * ------------------------------------------------------------------------ */

Boolean fnc_phys_2_cpu(TempResult *p_ret, const TempResult *p_args, unsigned arg_cnt)
{
  LargeWord address;

  UNUSED(arg_cnt);

  if (!area_lists[ActPC].cnt)
    return False;

  address = p_args[0].Contents.Int;
  as_tempres_set_int(p_ret, def_phys_2_cpu(ActPC, &address) ? address : area_lists[ActPC].end + 1);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     fnc_cpu_2_phys(TempResult *p_ret, const TempResult *p_args, unsigned arg_cnt)
 * \brief  built-in function for CPU -> physical address translation
 * \param  p_ret returns physical address
 * \param  p_args CPU address argument
 * \return True if translation possible at all
 * ------------------------------------------------------------------------ */

Boolean fnc_cpu_2_phys(TempResult *p_ret, const TempResult *p_args, unsigned arg_cnt)
{
  LargeWord address;

  UNUSED(arg_cnt);

  if (!area_lists[ActPC].cnt)
    return False;

  address = p_args[0].Contents.Int;
  as_tempres_set_int(p_ret, def_cpu_2_phys(ActPC, &address) ? address : SegLimits[ActPC] + 1);
  return True;
}
