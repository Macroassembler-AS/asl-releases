#ifndef _HEADIDS_H
#define _HEADIDS_H
/* headids.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Makroassembler AS                                                         */
/*                                                                           */
/* Hier sind alle Prozessor-IDs mit ihren Eigenschaften gesammelt            */
/*                                                                           */
/* Historie: 29. 8.1998 angelegt                                             */
/*                                                                           */
/*****************************************************************************/

/* Hex-Formate */

typedef enum
{
  eHexFormatDefault,
  eHexFormatMotoS,
  eHexFormatIntel,
  eHexFormatIntel16,
  eHexFormatIntel32,
  eHexFormatMOS,
  eHexFormatTek,
  eHexFormatTiDSK,
  eHexFormatAtmel,
  eHexFormatMico8,
  eHexFormatC
} tHexFormat;

typedef struct
{
  const char *Name;
  Word Id;
  tHexFormat HexFormat;
} TFamilyDescr, *PFamilyDescr;

extern const TFamilyDescr *FindFamilyByName(const char *Name);

extern const TFamilyDescr *FindFamilyById(Word Id);

extern void headids_init(void);
#endif /* _HEADIDS_H */
