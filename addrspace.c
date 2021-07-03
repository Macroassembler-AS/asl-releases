/* addrspace.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Address Space enumeration                                                 */
/*                                                                           */
/*****************************************************************************/

#include "strutil.h"
#include "addrspace.h"

const char *SegNames[SegCountPlusStruct] =
{
  "NOTHING", "CODE", "DATA", "IDATA", "XDATA", "YDATA",
  "BITDATA", "IO", "REG", "ROMDATA", "EEDATA", "STRUCT"
};

char SegShorts[SegCountPlusStruct] =
{
  '-','C','D','I','X','Y','B','P','R','O','E','S'
};

/*!------------------------------------------------------------------------
 * \fn     addrspace_lookup(const char *p_name)
 * \brief  look up address space's name
 * \param  p_name name in source
 * \return enum or SegCountPlusStruct if not found
 * ------------------------------------------------------------------------ */

as_addrspace_t addrspace_lookup(const char *p_name)
{
  as_addrspace_t res;

  for (res = SegNone; res < SegCountPlusStruct; res++)
    if (!as_strcasecmp(p_name, SegNames[res]))
      break;
  return res;
}
