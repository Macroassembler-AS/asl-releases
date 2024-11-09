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

/* Default Assumptions:
   - No long long data type if C89/C++98 is used.
   - long double would be available, but not the ..l() functions
     we need along with it: */

#if (defined __cplusplus) && (__cplusplus>=201103L)
# define AS_HAS_LONGLONG 1
# define AS_HAS_LONGDOUBLE 1
#elif (defined __STDC__) && (defined __STDC_VERSION__)
# define AS_HAS_LONGLONG 1
# define AS_HAS_LONGDOUBLE 1
#else
# define AS_HAS_LONGLONG 0
# define AS_HAS_LONGDOUBLE 0
#endif

/* NOTE:
 *
 * when adding new platforms, " gcc -dM -E - <<<'' " might be helpful to
 * find out about predefined symbols
 *
 */

#ifdef _MSC_VER
# define __PROTOS__
# define UNUSED(x) (void)x

/*
 * Windows systems using Microsoft Visual Studio.
 */
# ifdef _M_ARM
#  define ARCHPRNAME "arm"
# endif
# ifdef _M_ARM64
#  define ARCHPRNAME "arm64"
# endif
# ifdef _M_IX86
#  define ARCHPRNAME "x86"
# endif
# ifdef _M_X64
#  define ARCHPRNAME "x64"
# endif

# define ARCHSYSNAME "windows-msvc"

# define DEFSMADE
# define OPENRDMODE "rb"
# define OPENWRMODE "wb"
# define OPENUPMODE "rb+"
# define IEEEFLOAT_8_DOUBLE
# define SLASHARGS
# define PATHSEP '\\'
# define SPATHSEP "\\"
# define DIRSEP ';'
# define SDIRSEP ";"
# define DRSEP ':'
# define SDRSEP ":"
# define NULLDEV "NUL"
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
# define HAS16
typedef signed int as_int32_t;
# define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
# define W32_NLS
#endif

/*---------------------------------------------------------------------------*/
/* unify 68K platforms */

#ifdef __mc68020
# ifndef __m68k
#  define __m68k
# endif
#endif

#ifdef m68000
# ifndef __m68k
#  define __m68k
# endif
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
# define __i386
#endif

/* For IBMC... */

#ifdef _M_I386
# define __i386
#endif

#ifdef __i386__
# ifndef __i386
#  define __i386
# endif
#endif

/*---------------------------------------------------------------------------*/
/* ditto for VAX platforms */

#ifdef vax
# ifndef __vax__
#  define __vax__
# endif
#endif

/*---------------------------------------------------------------------------*/
/* ditto for PPC platforms */

#ifdef __PPC
# ifndef _POWER
#  define _POWER
# endif
#endif

#ifdef __ppc__
# ifndef _POWER
#  define _POWER
# endif
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
# ifndef __arm
#  define __arm
# endif
#endif

/*---------------------------------------------------------------------------*/
/* If the compiler claims to be ANSI, we surely can use prototypes */

#ifdef __STDC__
# define __PROTOS__
# define UNUSED(x) (void)x
#else
# define UNUSED(x) {}
#endif

/*---------------------------------------------------------------------------*/
/* just a hack to allow distinguishing SunOS from Solaris on Sparcs... */

#ifdef sparc
# ifndef __sparc
#  define __sparc
# endif
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
# ifndef __sparc
#  define __sparc
# endif
#endif

/*---------------------------------------------------------------------------*/
/* similar on Sun 3's... */

#ifdef __m68k
# ifndef __NetBSD__
#  ifndef __linux__
#   ifndef __MUNIX__
#    ifndef __amiga
#     define __sunos__
#    endif
#   endif
#  endif
# endif
#endif

/*===========================================================================*/
/* 68K platforms */

#ifdef __m68k

#define ARCHPRNAME "m68k"

/*---------------------------------------------------------------------------*/
/* SUN/3 with SunOS 4.x:

   see my SunOS quarrels in the Sparc section... */

#ifdef __sunos__
#ifndef __GNUC__
# undef AS_HAS_LONGLONG
# define AS_HAS_LONGLONG 0
#endif
#define ARCHSYSNAME "sun-sunos"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
extern double strtod();
#define NEEDS_STRSTR
typedef char as_int8_t;
typedef unsigned char as_uint8_t;
typedef short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#undef AS_HAS_LONGLONG
#define AS_HAS_LONGLONG 0
#define memmove(s1,s2,len) bcopy(s2,s1,len)
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
#if AS_HAS_LONGDOUBLE
# define IEEEFLOAT_10_2P8_LONG_DOUBLE
#else
# define IEEEFLOAT_8_DOUBLE
#endif
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#ifndef __GNUC__
# undef AS_HAS_LONGLONG
# define AS_HAS_LONGLONG 0
#endif
#define ARCHSYSNAME "sun-sunos"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#ifndef __GNUC__
# undef AS_HAS_LONGLONG
# define AS_HAS_LONGLONG 0
#endif
#define ARCHSYSNAME "dec-ultrix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
#define NEEDS_STRDUP
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* R3000/4x00 with Linux:

  quite a normal 32-Bit-UNIX system */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#undef AS_HAS_LONGLONG
#define AS_HAS_LONGLONG
#define AS_64_IS_LONGLONG
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* POWER with Linux (Macintosh) */

#ifdef __linux__

#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#undef AS_HAS_LONGLONG
#define AS_HAS_LONGLONG
#define AS_64_IS_LONGLONG
#define NO_NLS
#endif

#endif /* _POWER */

/*===========================================================================*/
/* VAX platforms */

#ifdef __vax__

#define ARCHPRNAME "vax"

/*---------------------------------------------------------------------------*/
/* VAX with Ultrix: */

#ifdef __ultrix
#ifndef __GNUC__
# undef AS_HAS_LONGLONG
# define AS_HAS_LONGLONG 0
#endif
#define ARCHSYSNAME "dec-ultrix"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define HOST_DECFLOAT
#define NEEDS_STRDUP
#define NEED_GETTIMEOFDAY
#define BKOKEN_SPRINTF
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
#define NO_NLS
#endif

/*---------------------------------------------------------------------------*/
/* VAX with NetBSD 1.x:

   quite a normal 32-Bit-UNIX system - except for the float format... */

#ifdef __NetBSD__
#define ARCHSYSNAME "vax-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define HOST_DECFLOAT
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
#define LOCALE_NLS
#endif

#ifdef __FreeBSD__
#define ARCHSYSNAME "unknown-freebsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
#define NO_NLS
#endif

#endif /* __alpha */

/*===========================================================================*/
/* Intel i386 platforms */

#ifdef __i386

#define ARCHPRNAME "i386"

/*---------------------------------------------------------------------------*/
/* Intel i386 with NetBSD 1.x and GCC: (tested on 1.5.3)

   principally, a normal 32-bit UNIX */

#ifdef __NetBSD__
#define ARCHSYSNAME "i386-netbsd"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with Linux and GCC:

   principally, a normal 32-bit *NIX */

#ifdef __linux__

#define ARCHSYSNAME "unknown-linux"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#if AS_HAS_LONGDOUBLE
# define IEEEFLOAT_10_12_LONG_DOUBLE
#else
# define IEEEFLOAT_8_DOUBLE
#endif
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
#define LOCALE_NLS
#endif

/*---------------------------------------------------------------------------*/
/* Intel i386 with Windows and Cygnus GCC:

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
#define IEEEFLOAT_8_DOUBLE
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#undef AS_HAS_LONGLONG
#define AS_HAS_LONGLONG 0
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
#define IEEEFLOAT_8_DOUBLE
/*
#define IEEEFLOAT_10_10_LONG_DOUBLE
#define HUGE_VALL _LHUGE_VAL
#define strtold(s,e) _strtold(s,e)
*/
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed long as_int32_t;
#define PRIas_int32_t "ld"
typedef unsigned long as_uint32_t;
#undef AS_HAS_LONGLONG
#define AS_HAS_LONGLONG 0
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
#if AS_HAS_LONGDOUBLE
# define IEEEFLOAT_10_16_LONG_DOUBLE
#else
# define IEEEFLOAT_8_DOUBLE
#endif
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONG
#define LOCALE_NLS

#endif /* __linux__ || __FreeBSD__ || __NetBSD__ || __APPLE__ */

/*---------------------------------------------------------------------------*/
/* Intel x86-64 with WIN32 and MinGW:

   Well, not really a UNIX...note that in contrast to Unix-like systems,
   the size of 'long' remains 32 bit.  One still has to use 'long long' to
   get 64 bits. */

#ifdef _WIN32

#define ARCHSYSNAME "unknown-win64"
#define DEFSMADE
#define OPENRDMODE "rb"
#define OPENWRMODE "wb"
#define OPENUPMODE "rb+"
#if AS_HAS_LONGDOUBLE
# define IEEEFLOAT_10_16_LONG_DOUBLE
#else
# define IEEEFLOAT_8_DOUBLE
#endif
#define SLASHARGS
#define PATHSEP '\\'
#define SPATHSEP "\\"
#define DIRSEP ';'
#define SDIRSEP ";"
#define DRSEP ':'
#define SDRSEP ":"
#define NULLDEV "NUL"
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
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
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
#define NO_NLS
#endif /* __EPOCEMX__ */


#endif /* __EPOC32__ */

#endif /* __arm */

/*===========================================================================*/
/* RISC-V platform */

#ifdef __riscv

#define ARCHPRNAME "riscv"

/*---------------------------------------------------------------------------*/
/* RISC-V linux with GCC */

#ifdef __linux__
#define ARCHSYSNAME "unknown-linux-riscv"
#define DEFSMADE
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int16_t;
typedef unsigned short as_uint16_t;
#define HAS16
typedef signed int as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned int as_uint32_t;
#define AS_64_IS_LONGLONG
#define LOCALE_NLS
#endif /* __linux__ */

#endif /* __riscv */

/*===========================================================================*/
/* Misc... */

/*---------------------------------------------------------------------------*/
/* Just for curiosity, it won't work without 16 bit int's... */

#ifdef _CRAYMPP
#define OPENRDMODE "r"
#define OPENWRMODE "w"
#define OPENUPMODE "r+"
#define IEEEFLOAT_8_DOUBLE
typedef signed char as_int8_t;
typedef unsigned char as_uint8_t;
typedef signed short as_int32_t;
#define PRIas_int32_t "d"
typedef unsigned short as_uint32_t;
#define AS_64_IS_LONG
#define LOCALE_NLS
#endif

/*===========================================================================*/
/* Post-Processing: check for definition, add defaults */

/* Host provides 64 bit int as long long: can only use it if long long
   is available: */

#ifdef AS_64_IS_LONGLONG
# if AS_HAS_LONGLONG
typedef signed long long as_int64_t;
typedef unsigned long long as_uint64_t;
#  define HAS64
# endif /* AS_HAS_LONGLONG */
#endif /* AS_64_IS_LONGLONG */

/* Host provides 64 bit int as long: use it and define long long
   away (we don't need it): */

#ifdef AS_64_IS_LONG
typedef signed long as_int64_t;
typedef unsigned long as_uint64_t;
# define HAS64
# undef AS_HAS_LONGLONG
# define AS_HAS_LONGLONG 0
#endif /* AS_64_IS_LONG */

/* Some VAX compilers internally seem to use D float
   and are unable to parse the G float DBL_MAX literal
   of 8.98...E+308 from float.h.
   So we put a hand-crafted constant in memory: */

#ifdef HOST_DECFLOAT
 typedef double as_float_t;
# ifdef __GFLOAT
   extern double as_decfloat_get_max_gfloat(void);
#  define AS_FLOAT_MAX as_decfloat_get_max_gfloat()
# else
#  define AS_FLOAT_MAX DBL_MAX
# endif
# define AS_FLOAT_DIG DBL_DIG
# define AS_HUGE_VAL HUGE_VAL
# define as_strtof(s,e) strtod(s,e)
#endif /* HOST_DECFLOAT */

#ifdef IEEEFLOAT_8_DOUBLE
 typedef double as_float_t;
# define AS_FLOAT_MAX DBL_MAX
# define AS_FLOAT_DIG DBL_DIG
# define AS_HUGE_VAL HUGE_VAL
# define as_strtof(s,e) strtod(s,e)
#endif /* IEEEFLOAT_8_DOUBLE */

#ifdef IEEEFLOAT_10_16_LONG_DOUBLE
# define IEEEFLOAT_10_LONG_DOUBLE
# define XPRIas_float_t "L"
#endif /* IEEEFLOAT_10_16_LONG_DOUBLE */

#ifdef IEEEFLOAT_10_12_LONG_DOUBLE
# define IEEEFLOAT_10_LONG_DOUBLE
# define XPRIas_float_t "L"
#endif /* IEEEFLOAT_10_12_LONG_DOUBLE */

#ifdef IEEEFLOAT_10_10_LONG_DOUBLE
# define IEEEFLOAT_10_LONG_DOUBLE
# define XPRIas_float_t "L"
#endif /* IEEEFLOAT_10_10_LONG_DOUBLE */

#ifdef IEEEFLOAT_10_2P8_LONG_DOUBLE
# define IEEEFLOAT_10_LONG_DOUBLE
# define XPRIas_float_t "L"
#endif /* IEEEFLOAT_10_2P8_LONG_DOUBLE */

#ifdef IEEEFLOAT_10_LONG_DOUBLE
 typedef long double as_float_t;
# define AS_FLOAT_MAX LDBL_MAX
# define AS_FLOAT_DIG LDBL_DIG
# define AS_HUGE_VAL HUGE_VALL
# define as_strtof(s,e) strtold(s,e)
# define as_fabs(f) fabsl(f)
# define as_ldexp(f,e) ldexpl(f,e)
# define as_frexp(f,e) frexpl(f,e)
# define as_modf(f,e) modfl(f,e)
# define as_sqrt(f) sqrtl(f)
# define as_sin(f) sinl(f)
# define as_cos(f) cosl(f)
# define as_tan(f) tanl(f)
# define as_asin(f) asinl(f)
# define as_acos(f) acosl(f)
# define as_atan(f) atanl(f)
# define as_exp(f) expl(f)
# define as_log(f) logl(f)
# define as_log10(f) log10l(f)
# define as_sinh(f) sinh(f)
# define as_cosh(f) coshl(f)
# define as_tanh(f) tanhl(f)
#endif /* IEEEFLOAT_10_LONG_DOUBLE */

#ifndef as_strtof
# define as_strtof(s,e) strtod(s,e)
#endif
#ifndef XPRIas_float_t
# define XPRIas_float_t ""
#endif
#ifndef as_fabs
# define as_fabs(f) fabs(f)
#endif
#ifndef as_ldexp
# define as_ldexp(f,e) ldexp(f,e)
#endif
#ifndef as_frexp
# define as_frexp(f,e) frexp(f,e)
#endif
#ifndef as_modf
# define as_modf(f,e) modf(f,e)
#endif
#ifndef as_sqrt
# define as_sqrt(f) sqrt(f)
#endif
#ifndef as_sin
# define as_sin(f) sin(f)
#endif
#ifndef as_cos
# define as_cos(f) cos(f)
#endif
#ifndef as_tan
# define as_tan(f) tan(f)
#endif
#ifndef as_asin
# define as_asin(f) asin(f)
#endif
#ifndef as_acos
# define as_acos(f) acos(f)
#endif
#ifndef as_atan
# define as_atan(f) atan(f)
#endif
#ifndef as_sinh
# define as_sinh(f) sinh(f)
#endif
#ifndef as_exp
# define as_exp(f) exp(f)
#endif
#ifndef as_log
# define as_log(f) log(f)
#endif
#ifndef as_log10
# define as_log10(f) log10(f)
#endif
#ifndef as_cosh
# define as_cosh(f) cosh(f)
#endif
#ifndef as_tanh
# define as_tanh(f) tanh(f)
#endif

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

#include <stddef.h>

extern void *ckmalloc(size_t s);

extern void *ckrealloc(void *p, size_t s);
#endif
#endif /* _SYSDEFS_H */
