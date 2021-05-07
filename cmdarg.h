#ifndef _CMDARG_H
#define _CMDARG_H
/* cmdarg.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verarbeitung Kommandozeilenparameter                                      */
/*                                                                           */
/* Historie:  4. 5.1996 Grundsteinlegung                                     */
/*            2001-10-20: option string is pointer instead of array          */
/*                                                                           */
/*****************************************************************************/

typedef enum {CMDErr, CMDFile, CMDOK, CMDArg} CMDResult;

typedef CMDResult (*CMDCallback)(
#ifdef __PROTOS__
Boolean NegFlag, const char *Arg
#endif
);

typedef void (*CMDErrCallback)
(
#ifdef __PROTOS__
Boolean InEnv, char *Arg
#endif
);

typedef struct
{
  const char *Ident;
  CMDCallback Callback;
} CMDRec;

#define MAXPARAM 256
typedef Boolean CMDProcessed[MAXPARAM + 1];

extern StringList FileArgList;

extern Boolean ProcessedEmpty(CMDProcessed Processed);

extern void ProcessCMD(int argc, char **argv,
                       const CMDRec *pCMDRecs, int CMDRecCnt,
                       CMDProcessed Unprocessed,
                       const char *EnvName, CMDErrCallback ErrProc);

extern const char *GetEXEName(const char *argv0);

extern void cmdarg_init(char *ProgPath);

#endif /* _CMDARG_H */
