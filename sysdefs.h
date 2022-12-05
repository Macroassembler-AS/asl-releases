#ifndef _SYSDEFS_H
#define _SYSDEFS_H
/* sysdefs.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS Port                                                                   */
/*                                                                           */
/* system-specific definitions                                               */
/*                                                                           */
/* History:  2001-04-13 activated DRSEP for Win32 platform                   */
/*           2001-09-11 added MacOSX                                         */
/*           2001-10-13 added ARM/Linux                                      */
/*                                                                           */
/*****************************************************************************/

/* NOTE:
 *
 * when adding new platforms, " gcc -dM -E - <<<'' " might be helpful to
 * find out about predefined symbols
 *
 */

/*---------------------------------------------------------------------------*/
/* unify 68K platforms */

#ifdef __mc68020
#ifndef __m68k
#define __m68k
#endif
#endif

#ifdef m68000
#ifndef __m68k
#define __m68k
#endif
#endif

#ifdef __mc68000
# ifndef __m68k
#  define __m68k
# endif
#endif

/*---------------------------------------------------------------------------*/
/* ditto for i386 platforms */

/* MSDOS only runs on x86s... */

#ifdef __MSDOS__
#define __i386
#endif

/* For IBMC... */

#ifdef _M_I386
#define __i386
#endif

#ifdef __i386__
#ifndef __i386
#define __i386
#endif
#endif

/*---------------------------------------------------------------------------*/
/* ditto for VAX platforms */

#ifdef vax
#define __vax__
#endif

/*---------------------------------------------------------------------------*/
/* ditto for PPC platforms */

#ifdef __PPC
#ifndef _POWER
#define _POWER
#endif
#endif

#ifdef __ppc__
#ifndef _POWER
#define _POWER
#endif
#endif

#ifdef __PPC__
# ifndef _POWER
#  define _POWER
# endif
#endif

#ifdef __PPC64__
# ifndef _POWER4
#  define _POWER4
# endif
#endif

/*---------------------------------------------------------------------------*/
/* ditto for ARM platforms */

#ifdef __arm__
#ifndef __arm
#define __arm
#endif
#endif

/*---------------------------------------------------------------------------*/
/* If the compiler claims to be ANSI, we surely can use prototypes */

#ifdef __STDC__
#define __PROTOS__
#define UNUSED(x) (void)x
#else
#define UNUSED(x) {}
#endif

/*---------------------------------------------------------------------------*/
/* just a hack to allow distinguishing SunOS from Solaris on Sparcs... */

#ifdef sparc
#ifndef __sparc
#define __sparc
#endif
#endif

#ifdef __sparc
# ifndef __NetBSD__
#  ifndef __FreeBSD__
#   ifndef __linux__
#    ifndef __SVR4
#     define __sunos__
#    else /* __SVR4 */
#     define __solaris__
#    endif /* __SVR4 */
#   endif /* __linux__ */
#  endif /* __FreeBSD__ */
# endif /* __NetBSD */
#endif /* __sparc */

#ifdef __sparc__
#ifndef __sparc
#define __sparc
#endif
#endif

/*---------------------------------------------------------------------------*/
/* similar on Sun 3's... */

#ifdef __m68k
#ifndef __NetBSD__
#ifndef __MUNIX__
#ifndef __amiga
#define __sunos__
#endif
#endif
#endif
#endif

/*===========================================================================*/
/* 68K platforms */

#ifdef __m68k

#define ARCHPRNAME "m68k"

/*---------------------------------------------------------------------------*/
/* SUN/3 with SunOS 4.x:

   see my SunOS quarrels in the Sparc section... */

#ifdef __sunos__
#define ARCHSYSNAME "sun-sunos"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifdef __GNUC__
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#else
#define NOLONGLONG
#endif
#define memmove(s1,s2,len) bcopy(s2,s1,len)
extern void bcopy();
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* SUN/3 with NetBSD 1.x:

   quite a normal 32-Bit-UNIX system */

#ifdef __NetBSD__
#define ARCHSYSNAME "sun-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* PCS/Cadmus:

   quite a bare system, lots of work required... */

#ifdef __MUNIX__
#define ARCHSYSNAME "pcs-munix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
#define NEEDS_STRSTR
typedef char Integ8;
typedef unsigned char Card8;
typedef short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#define NOLONGLONG
#define memmove(s1,s2,len) bcopy(s2,s1,len)
extern double strtod();
extern char *getenv();
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Linux/68K:

   quite a normal 32-Bit-UNIX system */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define NO_NLS
#endif

#endif /* __m68k */

/*===========================================================================*/
/* SPARC platforms */

#ifdef __sparc

#define ARCHPRNAME "sparc"

/*---------------------------------------------------------------------------*/
/* SUN Sparc with SunOS 4.1.x:

   don't try cc, use gcc, it's hopeless without an ANSI-compliant compiler...
   SunOS does have NLS support, but it does not have D_FMT and T_FMT
   I should change this ...
   Though the manual pages claim that memmove and atexit exist, I could not
   find them in any library :-(  Fortunately, bcopy claims to be safe for
   overlapping arrays, we just have to reverse source and destination pointers.
   The sources themselves contain a switch to use on_exit instead of atexit
   (it uses a different callback scheme, so we cannot just make a #define here...)
   To get rid of most of the messages about missing prototypes, add
   -D__USE_FIXED_PROTOTYPES__ to your compiler flags!
   Apart from these few points, one could claim SunOS to be quite a normal
   32-bit-UNIX... */

#ifdef __sunos__
#define ARCHSYSNAME "sun-sunos"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifdef __GNUC__
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#else
#define NOLONGLONG
#endif
#define fpos_t long
#ifdef __STDC__
extern void bcopy();
#endif
#define memmove(s1,s2,len) bcopy(s2,s1,len)
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* SUN Sparc with Solaris 2.x:

   quite a normal 32-Bit-UNIX system */

#ifdef __solaris__
#define ARCHSYSNAME "sun-solaris"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Sparc with NetBSD 1.x:

   quite a normal 32-Bit-UNIX system */

#ifdef __NetBSD__
#define ARCHSYSNAME "sun-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Sparc with Linux                                                          */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#endif /* __sparc */

/*===========================================================================*/
/* Mips platforms */

#ifdef __mips

#define ARCHPRNAME "mips"

/*---------------------------------------------------------------------------*/
/* R3000 with Ultrix 4.3:

   nl_langinfo prototype is there, but no function in library ?!
   use long long only if you have gcc, c89 doesn't like them !
   cc isn't worth trying, believe me! */

#ifdef __ultrix
#define ARCHSYSNAME "dec-ultrix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
#define NEEDS_STRDUP
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifdef __GNUC__
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#else
#define NOLONGLONG
#endif
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* R2000/3000 with NetBSD 1.2:

   quite a normal 32-Bit-UNIX system */

#ifdef __NetBSD__
#define ARCHSYSNAME "dec-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* R3000/4x00 with Irix 5.x:

  quite a normal 32-Bit-UNIX system
  seems also to work with 6.2... */

#ifdef __sgi
#define ARCHSYSNAME "sgi-irix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#endif /* __mips */

/*===========================================================================*/
/* HP-PA platforms */

#ifdef __hppa

#define ARCHPRNAME "parisc"

/*---------------------------------------------------------------------------*/
/* HP-PA 1.x with HP-UX: */

#ifdef __hpux
#define ARCHSYSNAME "hp-hpux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#endif /* __hppa */

/*===========================================================================*/
/* POWER 64 bit platforms */

#ifdef _POWER64

#define ARCHPRNAME "ppc64"

/*---------------------------------------------------------------------------*/
/* POWER64 with Linux (Macintosh) */

#ifdef __linux__

#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*===========================================================================*/
/* POWER(32) platforms */

#elif defined _POWER

#define ARCHPRNAME "ppc"

/*---------------------------------------------------------------------------*/
/* POWER with AIX 4.1: rs6000 */

#ifdef _AIX
#define ARCHSYSNAME "ibm-aix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* POWER with Linux (Macintosh) */

#ifdef __linux__

/* no long long data type if C89 is used */

#if (defined __STDC__) && (!defined __STDC_VERSION__)
# define NOLONGLONG
#endif

#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifndef NOLONGLONG
typedef signed long long Integ64;
typedef unsigned long long Card64;
# define HAS64
#endif /* !NOLONGLONG */
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* POWER with OSX (Macintosh) */

#ifdef __APPLE__
#define ARCHSYSNAME "apple-macosx"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define NO_NLS
#endif

#endif /* _POWER */

/*===========================================================================*/
/* VAX platforms */

#ifdef __vax__

#define ARCHPRNAME "vax"

/*---------------------------------------------------------------------------*/
/* VAX with Ultrix: */

#ifdef ultrix
#define ARCHSYSNAME "dec-ultrix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define HOST_DECFLOAT
#define NEEDS_STRDUP
#define BKOKEN_SPRINTF
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#define NOLONGLONG
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* VAX with NetBSD 1.x:

   quite a normal 32-Bit-UNIX system - apart from the float format... */

#ifdef __NetBSD__
#define ARCHSYSNAME "vax-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define HOST_DECFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#endif /* vax */

#ifdef __aarch64__

#define ARCHPRNAME "aarch64"

/*---------------------------------------------------------------------------*/
/* AArch64 with Linux and GCC: */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* AArch64 with macOS (Apple M-series CPU) */

#ifdef __APPLE__
#define ARCHSYSNAME "apple-darwin"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#endif /* __aarch64__ */

/*===========================================================================*/
/* DEC Alpha platforms */

#ifdef __alpha

#define ARCHPRNAME "alpha"

/*---------------------------------------------------------------------------*/
/* DEC Alpha with Digital UNIX and DEC C / GCC:

   Alpha is a 64 bit machine, so we do not need to use extra longs
   OSF has full NLS support */

#ifdef __osf__
#define ARCHSYSNAME "dec-osf"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* DEC Alpha with Linux and GCC:

   see OSF... */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* DEC Alpha with NetBSD and GCC:

   see OSF... */

#ifdef __NetBSD__
#define ARCHSYSNAME "unknown-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS
#endif

#ifdef __FreeBSD__
#define ARCHSYSNAME "unknown-freebsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define NO_NLS
#endif

#endif /* __alpha */

/*===========================================================================*/
/* Intel i386 platforms */

#ifdef __i386

#define ARCHPRNAME "i386"

/*---------------------------------------------------------------------------*/
/* Intel i386 with NetBSD 1. and GCC: (tested on 1.5.3)

   principally, a normal 32-bit UNIX */

#ifdef __NetBSD__
#define ARCHSYSNAME "i386-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with Linux and GCC:

   principally, a normal 32-bit *NIX */

#ifdef __linux__

/* no long long data type if C89 is used */

#if (defined __STDC__) && (!defined __STDC_VERSION__)
# define NOLONGLONG
#endif

#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifndef NOLONGLONG
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#endif /* !NOLONGLONG */
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with FreeBSD and GCC:

   principally, a normal 32-bit *NIX */

#ifdef __FreeBSD__
#define ARCHSYSNAME "unknown-freebsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with Darwin and GCC:
   principally, a normal 32-bit *NIX */

#ifdef __APPLE__
#define ARCHSYSNAME "apple-osx"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with WIN32 and Cygnus GCC:

   well, not really a UNIX... */

#ifdef _WIN32

/* no long long data type if C89 is used */

#if (defined __STDC__) && (!defined __STDC_VERSION__)
# define NOLONGLONG
#endif

#define ARCHSYSNAME "unknown-win32"
#define DEFSMADE
#define OPENRDMODE "rb"
#define OPENWRMODE "wb"
#define OPENUPMODE "rb+"
#define IEEEFLOAT
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifndef NOLONGLONG
typedef signed long long Integ64;
typedef unsigned long long Card64;
# define HAS64
#endif
#define W32_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with OS/2 and emx-GCC:

   well, not really a UNIX... */

#ifdef __EMX__
#define ARCHSYSNAME "unknown-os2"
#define DEFSMADE
#define OPENRDMODE "rb"
#define OPENWRMODE "wb"
#define OPENUPMODE "rb+"
#define IEEEFLOAT
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define OS2_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with OS/2 and IBMC:

well, not really a UNIX... */

#ifdef __IBMC__
#define DEFSMADE
#define NODUP
#define OPENRDMODE "rb"
#define OPENWRMODE "wb"
#define OPENUPMODE "rb+"
#define IEEEFLOAT
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#define NOLONGLONG
#define OS2_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel x86 with MS-DOS and Borland-C:

   well, not really a UNIX...
   assure we get a usable memory model */

#ifdef __MSDOS__
#ifdef __TURBOC__
#ifndef __LARGE__
#error Wrong memory model - use large!
#endif
#undef ARCHPRNAME
#ifdef __DPMI16__
#define ARCHPRNAME "i286"
#define ARCHSYSNAME "unknown-dpmi"
#else
#define ARCHPRNAME "i86"
#define ARCHSYSNAME "unknown-msdos"
#endif
#define CKMALLOC
#define HEAPRESERVE 4096
#define DEFSMADE
#define OPENRDMODE "rb"
#define OPENWRMODE "wb"
#define OPENUPMODE "rb+"
#define IEEEFLOAT
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed long Integ32;
#define PRIInteg32 "ld"
typedef unsigned long Card32;
#define NOLONGLONG
#define DOS_NLS
#define __PROTOS__
#undef UNUSED
#define UNUSED(x) (void)x
#endif
#endif

#endif /* __i386 */


/*===========================================================================*/
/* Intel x86_64 platforms */

#if  (defined __k8__) || (defined __x86_64) || (defined __x86_64__)

#define ARCHPRNAME "x86_64"

/*---------------------------------------------------------------------------*/
/* x86-64/amd64 with Linux/FreeBSD, OSX and GCC:

   Principally, a normal *NIX. */

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)

/* no long long data type if C89 is used */

#if (defined __STDC__) && (!defined __STDC_VERSION__)
# define NOLONGLONG
#endif

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#elif defined __FreeBSD__
#define ARCHSYSNAME "unknown-freebsd"
#elif defined __NetBSD__
#define ARCHSYSNAME "unknown-netbsd"
#else
#define ARCHSYSNAME "apple-osx"
#endif

#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long Integ64;
typedef unsigned long Card64;
#define HAS64
#define LOCALE_NLS

#endif /* __linux__ || __FreeBSD__ || __NetBSD__ || __APPLE__ */

/*---------------------------------------------------------------------------*/
/* Intel i386 with WIN32 and MinGW:

   Well, not really a UNIX...note that in contrast to Unix-like systems,
   the size of 'long' remains 32 bit.  One still has to use 'long long' to
   get 64 bits. */

#ifdef _WIN32

/* no long long data type if C89 is used */

#if (defined __STDC__) && (!defined __STDC_VERSION__)
# define NOLONGLONG
#endif

#define ARCHSYSNAME "unknown-win64"
#define DEFSMADE
#define OPENRDMODE "rb"
#define OPENWRMODE "wb"
#define OPENUPMODE "rb+"
#define IEEEFLOAT
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
#ifndef NOLONGLONG
typedef signed long long Integ64;
typedef unsigned long long Card64;
# define HAS64
#endif
#define NO_NLS

#endif /* _WIN32 */

#endif /* __k8__ || __x86_64 || __x86_64__ */

/*===========================================================================*/
/* ARM platform */

#ifdef __arm

#define ARCHPRNAME "arm"

/*---------------------------------------------------------------------------*/
/* ARM linux with GCC */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux-arm"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define LOCALE_NLS
#endif /* __linux__ */

/*---------------------------------------------------------------------------*/
/* Psion PDA (ARM cpu) with SymbianOS Epoc32 rel.5 and installed epocemx-GCC:

   well, not really a UNIX... */

#ifdef __EPOC32__

#ifdef __EPOCEMX__
#define ARCHSYSNAME "psion-epoc32-epocemx"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ16;
typedef unsigned short Card16;
#define HAS16
typedef signed int Integ32;
#define PRIInteg32 "d"
typedef unsigned int Card32;
typedef signed long long Integ64;
typedef unsigned long long Card64;
#define HAS64
#define NO_NLS
#endif /* __EPOCEMX__ */


#endif /* __EPOC32__ */

#endif /* __arm */

/*===========================================================================*/
/* Misc... */

/*---------------------------------------------------------------------------*/
/* Just for curiosity, it won't work without 16 bit int's... */

#ifdef _CRAYMPP
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT
typedef signed char Integ8;
typedef unsigned char Card8;
typedef signed short Integ32;
#define PRIInteg32 "d"
typedef unsigned short Card32;
typedef signed int Integ64;
typedef unsigned int Card64;
#define HAS64
#define LOCALE_NLS
#endif

/*===========================================================================*/
/* Post-Processing: check for definition, add defaults */


#ifdef DEFSMADE
#ifndef PATHSEP
#define PATHSEP '/'
#define SPATHSEP "/"
#define DIRSEP ':'
#define SDIRSEP ":"
#endif
#ifndef NULLDEV
#define NULLDEV "/dev/null"
#endif
#else
#error "your platform so far is not included in AS's header files!"
#error "please edit sysdefs.h!"
#endif

#ifdef CKMALLOC
#define malloc(s) ckmalloc(s)
#define realloc(p,s) ckrealloc(p,s)

extern void *ckmalloc(size_t s);

extern void *ckrealloc(void *p, size_t s);
#endif
#endif /* _SYSDEFS_H */
