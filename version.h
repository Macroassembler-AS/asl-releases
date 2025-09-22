#ifndef _VERSION_H
#define _VERSION_H
/* version.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Keeps Version Number(s)                                                   */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

#define AS_VERSION_MAJOR 1
#define AS_VERSION_MINOR 42
#define AS_VERSION_BUILD 295

/* The standard C stringification magic: */

#define STR_STRING(x)   #x
#define STR(x)          STR_STRING(x)

extern const char *Version;
extern LongInt VerNo;

extern const char *InfoMessCopyright;

extern LongInt Magic;

extern void version_init(void);

#endif /* _VERSION_H */
