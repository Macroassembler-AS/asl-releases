#ifndef _ASMLABEL_H
#define _ASMLABEL_H
/* asmlabel.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS Port                                                                   */
/*                                                                           */
/* Handle Label on Source Line                                               */
/*                                                                           */
/*****************************************************************************/

struct sStrComp;

extern Boolean LabelPresent(void);

extern void LabelReset(void);

extern void LabelHandle(const struct sStrComp *pName, LargeWord Value, Boolean ForceGlobal);

extern void LabelModify(LargeWord OldValue, LargeWord NewValue);

extern void asmlabel_init(void);

#endif /* _ASMLABEL_H */
