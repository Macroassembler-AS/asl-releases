/* ioerrs.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Abliefern der I/O-Fehlermeldungen                                         */
/*                                                                           */
/* Historie: 11.10.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "strutil.h"
#include <errno.h>
#include "nlmessages.h"
#include "ioerrs.rsc"
#ifdef _USE_MSH
# include "ioerrs.msh"
#endif
#include "ioerrs.h"

typedef struct
{
  int Code;
  int Msg;
} ErrorDef;

static TMsgCat MsgCat;

static ErrorDef ErrorDefs[] =
{
#ifdef EPERM
  { EPERM       , Num_IoErr_EPERM },
#endif
#ifdef ENOENT
  { ENOENT      , Num_IoErr_ENOENT },
#endif
#ifdef ESRCH
  { ESRCH       , Num_IoErr_ESRCH },
#endif
#ifdef EINTR
  { EINTR       , Num_IoErr_EINTR },
#endif
#ifdef EIO
  { EIO         , Num_IoErr_EIO },
#endif
#ifdef ENXIO
  { ENXIO       , Num_IoErr_ENXIO },
#endif
#ifdef E2BIG
  { E2BIG       , Num_IoErr_E2BIG },
#endif
#ifdef ENOEXEC
  { ENOEXEC     , Num_IoErr_ENOEXEC },
#endif
#ifdef EBADF
  { EBADF       , Num_IoErr_EBADF },
#endif
#ifdef ECHILD
  { ECHILD      , Num_IoErr_ECHILD },
#endif
#ifdef EDEADLK
  { EDEADLK     , Num_IoErr_EDEADLK },
#endif
#ifdef ENOMEM
  { ENOMEM      , Num_IoErr_ENOMEM },
#endif
#ifdef EACCES
  { EACCES      , Num_IoErr_EACCES },
#endif
#ifdef EFAULT
  { EFAULT      , Num_IoErr_EFAULT },
#endif
#ifdef ENOTBLK
  { ENOTBLK     , Num_IoErr_ENOTBLK },
#endif
#ifdef EBUSY
  { EBUSY       , Num_IoErr_EBUSY },
#endif
#ifdef EEXIST
  { EEXIST      , Num_IoErr_EEXIST },
#endif
#ifdef EXDEV
  { EXDEV       , Num_IoErr_EXDEV },
#endif
#ifdef ENODEV
  { ENODEV      , Num_IoErr_ENODEV },
#endif
#ifdef ENOTDIR
  { ENOTDIR     , Num_IoErr_ENOTDIR },
#endif
#ifdef EISDIR
  { EISDIR      , Num_IoErr_EISDIR },
#endif
#ifdef EINVAL
  { EINVAL      , Num_IoErr_EINVAL },
#endif
#ifdef ENFILE
  { ENFILE      , Num_IoErr_ENFILE },
#endif
#ifdef EMFILE
  { EMFILE      , Num_IoErr_EMFILE },
#endif
#ifdef ENOTTY
  { ENOTTY      , Num_IoErr_ENOTTY },
#endif
#ifdef ETXTBSY
  { ETXTBSY     , Num_IoErr_ETXTBSY },
#endif
#ifdef EFBIG
  { EFBIG       , Num_IoErr_EFBIG },
#endif
#ifdef ENOSPC
  { ENOSPC      , Num_IoErr_ENOSPC },
#endif
#ifdef ESPIPE
  { ESPIPE      , Num_IoErr_ESPIPE },
#endif
#ifdef EROFS
  { EROFS       , Num_IoErr_EROFS },
#endif
#ifdef EMLINK
  { EMLINK      , Num_IoErr_EMLINK },
#endif
#ifdef EPIPE
  { EPIPE       , Num_IoErr_EPIPE },
#endif
#ifdef EDOM
  { EDOM        , Num_IoErr_EDOM },
#endif
#ifdef ERANGE
  { ERANGE      , Num_IoErr_ERANGE },
#endif
#ifdef ENAMETOOLONG
  { ENAMETOOLONG, Num_IoErr_ENAMETOOLONG },
#endif
#ifdef ENOLCK
  { ENOLCK      , Num_IoErr_ENOLCK },
#endif
#ifdef ENOSYS
  { ENOSYS      , Num_IoErr_ENOSYS },
#endif
#ifdef ENOTEMPTY
  { ENOTEMPTY   , Num_IoErr_ENOTEMPTY },
#endif
#ifdef ELOOP
  { ELOOP       , Num_IoErr_ELOOP },
#endif
#ifdef EWOULDBLOCK
  { EWOULDBLOCK , Num_IoErr_EWOULDBLOCK },
#endif
#ifdef ENOMSG
  { ENOMSG      , Num_IoErr_ENOMSG },
#endif
#ifdef EIDRM
  { EIDRM       , Num_IoErr_EIDRM },
#endif
#ifdef ECHRNG
  { ECHRNG      , Num_IoErr_ECHRNG },
#endif
#ifdef EL2NSYNC
  { EL2NSYNC    , Num_IoErr_EL2NSYNC },
#endif
#ifdef EL3HLT
  { EL3HLT      , Num_IoErr_EL3HLT },
#endif
#ifdef EL3RST
  { EL3RST      , Num_IoErr_EL3RST },
#endif
#ifdef ELNRNG
  { ELNRNG      , Num_IoErr_ELNRNG },
#endif
#ifdef EUNATCH
  { EUNATCH     , Num_IoErr_EUNATCH },
#endif
#ifdef ENOCSI
  { ENOCSI      , Num_IoErr_ENOCSI },
#endif
  { -1, -1 }
};

char *hs;

const char *GetErrorMsg(int number)
{
  ErrorDef *z;

  for (z = ErrorDefs; z->Msg != -1; z++)
    if (number == z->Code)
      break;

  if (z->Msg != -1)
    return catgetmessage(&MsgCat, z->Msg);
  else
  {
    as_snprintf(hs, STRINGSIZE, "%s%d", catgetmessage(&MsgCat, Num_IoErrUnknown), number);
    return hs;
  }
}

const char *GetReadErrorMsg(void)
{
  return catgetmessage(&MsgCat, Num_IoErr_READERROR);
}

void ioerrs_init(char *ProgPath)
{
  hs = (char*)malloc(sizeof(char) * STRINGSIZE);
#ifdef _USE_MSH
  msg_catalog_open_buffer(&MsgCat, ioerrs_msh_data, sizeof(ioerrs_msh_data), MsgId1, MsgId2);
  UNUSED(ProgPath);
#else
  msg_catalog_open_file(&MsgCat, "ioerrs.msg", ProgPath, MsgId1, MsgId2);
#endif
}
