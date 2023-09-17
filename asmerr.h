#ifndef _ASMERR_H
#define _ASMERR_H
/* asmerr.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Error Handling Functions                                                  */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"
#include "errmsg.h"

extern Word ErrorCount, WarnCount;

struct sLineComp;
struct sStrComp;
extern void WrErrorString(const char *Message, const char *Add, Boolean Warning, Boolean Fatal,
                          const char *pExtendError, const struct sLineComp *pLineComp);

extern void WrError(tErrorNum Num);

extern void WrXError(tErrorNum Num, const char *pExtError);

extern void WrXErrorPos(tErrorNum Num, const char *pExtError, const struct sLineComp *pLineComp);

extern void WrStrErrorPos(tErrorNum Num, const struct sStrComp *pStrComp);


extern void CodeEXPECT(Word Code);
extern void CodeENDEXPECT(Word Code);

extern Boolean FindAndTakeExpectError(tErrorNum Num);

extern void AsmErrPassInit(void);
extern void AsmErrPassExit(void);

extern void ChkIO(tErrorNum ErrNo);
extern void ChkXIO(tErrorNum ErrNo, char *pExtError);
extern void ChkStrIO(tErrorNum ErrNo, const struct sStrComp *pComp);

extern void asmerr_init(void);

#endif /* _ASMERR_H */

