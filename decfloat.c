/* decfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* DEC<->IEEE Floating Point Conversion on host                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <math.h>
#include <errno.h>

#include "be_le.h"
#include "ieeefloat.h"
#include "decfloat.h"

/* NOTE: if host uses DEC float, double is assumed to be D_float */

#ifdef DECFLOAT

/*!------------------------------------------------------------------------
 * \fn     DECF_2_Single(Byte *pDest, float inp)
 * \brief  convert single precision (DEC F) to IEEE single precision
 * \param  pDest where to write
 * \param  inp value to convert
 * ------------------------------------------------------------------------ */

void DECF_2_Single(Byte *pDest, float inp)
{
  /* IEEE + DEC layout is the same for single, just the exponent offset is different
     by two: */

  inp /= 4;
  memcpy(pDest, &tmp, 4);
  WSwap(pDest, 4);
}

/*!------------------------------------------------------------------------
 * \fn     DECD_2_Double(Byte *pDest, float inp)
 * \brief  convert double precision (DEC D) to IEEE double precision
 * \param  pDest where to write
 * \param  inp value to convert
 * ------------------------------------------------------------------------ */

void DECD_2_Double(Byte *pDest, Double inp)
{
  Byte tmp[8];
  Word Exp;
  int z;
  Boolean cont;

  memcpy(tmp, &inp, 8);
  WSwap(tmp, 8);
  Exp = ((tmp[0] << 1) & 0xfe) + (tmp[1] >> 7);
  Exp += 894; /* =1023-129 */
  tmp[1] &= 0x7f;
  if ((tmp[7] & 7) > 4)
  {
    for (tmp[7] += 8, cont = tmp[7] < 8, z = 0; cont && z > 1; z--)
    {
      tmp[z]++;
      cont = (tmp[z] == 0);
    }
    if (cont)
    {
      tmp[1]++;
      if (tmp[1] > 127)
        Exp++;
    }
  }
  pDest[7] = (tmp[0] & 0x80) + ((Exp >> 4) & 0x7f);
  pDest[6] = ((Exp & 0x0f) << 4) + ((tmp[1] >> 3) & 0x0f);
  for (z = 5; z >= 0; z--)
    pDest[z] = ((tmp[6 - z] & 7) << 5) | ((tmp[7 - z] >> 3) & 0x1f);
}

/*!------------------------------------------------------------------------
 * \fn     DECD_2_LongDouble(Byte *pDest, float inp)
 * \brief  convert double precision (DEC D) to non-IEEE extended precision
 * \param  pDest where to write
 * \param  inp value to convert
 * ------------------------------------------------------------------------ */

void DECD_2_LongDouble(Byte *pDest, Double inp)
{
  memcpy(Buffer, &inp, 8);
  WSwap(Buffer, 8);
  Sign = (*Buffer) & 0x80;
  Exponent = ((*Buffer) << 1) + ((Buffer[1] & 0x80) >> 7);
  Exponent += (16383 - 129);
  Buffer[1] |= 0x80;
  for (z = 1; z < 8; z++)
    pDest[z] = Buffer[8 - z];
  pDest[0] = 0;
  pDest[9] = Sign | ((Exponent >> 8) & 0x7f);
  pDest[8] = Exponent & 0xff;
}

#endif /* DECFLOAT */

/*!------------------------------------------------------------------------
 * \fn     Double_2_dec_lit(Double inp, Byte *p_dest)
 * \brief  convert from host to DEC (VAX) 6 bit float (literal) format
 * \param  inp value to convert
 * \param  p_dest result buffer
 * \return >0 for number of bytes used (1) or <0 for error code
 * ------------------------------------------------------------------------ */

extern int Double_2_dec_lit(Double inp, Byte *p_dest)
{
  int exp;
  double fract_part, nonfract_part;
  int int_part;

  for (exp = 7; exp >= 0; exp--, inp *= 2.0)
  {
    if (inp > 120.0)
      return -E2BIG;
    if (inp < 64.0)
      continue;
    fract_part = modf(inp, &nonfract_part);
    if (fract_part != 0.0)
      return -EBADF;
    int_part = (int)nonfract_part;
    if ((int_part & 7) || (int_part < 64))
      return -EBADF;
    *p_dest = (exp << 3) | ((int_part & 0x38) >> 3);
    return 1;
  }
  return -EIO;
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_dec_f(Double inp, Word *p_dest)
 * \brief  convert from host to DEC (PDP/VAX) 4 byte float (F) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (2) or <0 for error code
 * ------------------------------------------------------------------------ */

int Double_2_dec_f(Double inp, Word *p_dest)
{
#ifdef HOST_DECFLOAT

  /* native format: */

  {
    Single tmp = inp;
    memcpy(p_dest, &tmp, 4);
  }

#else /* !HOST_DECFLOAT */

  /* Otherwise, convert to IEEE 32 bit.  Conversion
     from IEEE -> DEC F means just multiplying by four,
     otherwise the layout is the same: */

  if (fabs(inp) > 1.70141e+38)
    return -E2BIG;

# ifdef IEEEFLOAT
  {
    int fp_class = as_fpclassify(inp);
    if ((fp_class != AS_FP_NORMAL) && (fp_class != AS_FP_SUBNORMAL))
      return -EINVAL;
  }
# endif /* IEEEFLOAT */
  {
    Byte buf[4];
    Double_2_ieee4(inp * 4.0, buf, True);

    p_dest[0] = ((Word)(buf[0])) << 8 | buf[1];
    p_dest[1] = ((Word)(buf[2])) << 8 | buf[3];
  }
#endif /* HOST_DECFLOAT */
  return 2;
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_dec_d(Double inp, Word *p_dest)
 * \brief  convert from host to DEC (PDP/VAX) 8 byte float (D) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (4) or <0 for error code
 * ------------------------------------------------------------------------ */

int Double_2_dec_d(Double inp, Word *p_dest)
{
#ifdef HOST_DECFLOAT

  /* native format: */

  memcpy(p_dest, &tmp, 8);

#else /* !HOST_DECFLOAT */

  /* Otherwise, convert to IEEE 64 bit: */

  if (fabs(inp) > 1.70141e+38)
    return -E2BIG;

# ifdef IEEEFLOAT
  {
    int fp_class = as_fpclassify(inp);
    if ((fp_class != AS_FP_NORMAL) && (fp_class != AS_FP_SUBNORMAL))
      return -EINVAL;
  }
# endif /* IEEEFLOAT */

  {
    Word sign;
    Integer exponent;
    LongWord mantissa, fraction;

    /* Dissect */

    ieee8_dissect(&sign, &exponent, &mantissa, &fraction, inp);

    /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

    exponent++;

    /* DEC float does not handle denormal numbers and truncates to zero: */

    if (!(mantissa & 0x10000000ul))
    {
      fraction = mantissa = 0;
      exponent = -128;
    }

    /* add bias to exponent */

    exponent += 128;

    /* assemble 1st word (seeeeeeeemmmmmmm): */

    p_dest[0] = ((sign & 1) << 15) 
              | ((exponent << 7) & 0x7f80u)
              | ((mantissa >> 21) & 0x7f);  /* mant bits 27..21 */
    p_dest[1] = (mantissa >> 5) & 0xffff;   /* mant bits 20..5 */
    p_dest[2] = ((mantissa & 0x1f) << 11)   /* mant bits 4..0 */
              | ((fraction >> 13) & 0x7ff); /* fract bits 23..13 */
    p_dest[3] = (fraction & 0x1fff) << 3;  /* fract bits 12..0 */
  }
#endif /* HOST_DECFLOAT */

  return 4;
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_dec_g(Double inp, Word *p_dest)
 * \brief  convert from host to DEC (VAX) 8 byte float (G) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (4) or <0 for error code
 * ------------------------------------------------------------------------ */

int Double_2_dec_g(Double inp, Word *p_dest)
{
  /* Convert to IEEE 64 bit: */

  if (fabs(inp) > 0.9e+308)
    return -E2BIG;

# ifdef IEEEFLOAT
  {
    int fp_class = as_fpclassify(inp);
    if ((fp_class != AS_FP_NORMAL) && (fp_class != AS_FP_SUBNORMAL))
      return -EINVAL;
  }
# endif /* IEEEFLOAT */

  {
    Word sign;
    Integer exponent;
    LongWord mantissa, fraction;

    /* Dissect */

    ieee8_dissect(&sign, &exponent, &mantissa, &fraction, inp);

    /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

    exponent++;

    /* DEC float does not handle denormal numbers and truncates to zero: */

    if (!(mantissa & 0x10000000ul))
    {
      fraction = mantissa = 0;
      exponent = -1024;
    }

    /* add bias to exponent */

    exponent += 1024;

    /* assemble 1st word (seeeeeeeeeeemmmm): */

    p_dest[0] = ((sign & 1) << 15)
              | ((exponent << 4) & 0x7ff0u)
              | ((mantissa >> 24) & 0x0f);  /* mant bits 27..24 */
    p_dest[1] = (mantissa >> 8) & 0xffff;   /* mant bits 23..8 */
    p_dest[2] = ((mantissa & 0xff) << 8)   /* mant bits 7..0 */
              | ((fraction >> 16) & 0xff); /* fract bits 23..16 */
    p_dest[3] = (fraction & 0xffff) << 0;  /* fract bits 15..0 */
  }

  return 4;
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_dec_h(Double inp, Word *p_dest)
 * \brief  convert from host to DEC (VAX) 16 byte float (h) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (8) or <0 for error code
 * ------------------------------------------------------------------------ */

int Double_2_dec_h(Double inp, Word *p_dest)
{
  /* Convert to IEEE 64 bit: */

  if (fabs(inp) > 0.9e+308)
    return -E2BIG;

# ifdef IEEEFLOAT
  {
    int fp_class = as_fpclassify(inp);
    if ((fp_class != AS_FP_NORMAL) && (fp_class != AS_FP_SUBNORMAL))
      return -EINVAL;
  }
# endif /* IEEEFLOAT */

  {
    Word sign;
    Integer exponent;
    LongWord mantissa, fraction;

    /* Dissect */

    ieee8_dissect(&sign, &exponent, &mantissa, &fraction, inp);

    /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

    exponent++;

    /* DEC float does not handle denormal numbers and truncates to zero: */

    if (!(mantissa & 0x10000000ul))
    {
      fraction = mantissa = 0;
      exponent = -16384;
    }

    /* add bias to exponent */

    exponent += 16384;

    /* assemble 1st word (seeeeeeeeeeeeeee): */

    p_dest[0] = ((sign & 1) << 15) 
              | ((exponent << 0) & 0x7fffu);
    p_dest[1] = (mantissa >> 12) & 0xffff; /* mant bits 27..12 */
    p_dest[2] = ((mantissa & 0xfff) << 4)  /* mant bits 11..0 */
              | ((fraction >> 20) & 0xf);  /* fract bits 23..20 */
    p_dest[3] = (fraction >> 4) & 0xffff;  /* fract bits 19..4 */
    p_dest[4] = (fraction & 0xf) << 12;    /* fract bits 3..0 */
    p_dest[5] =
    p_dest[6] =
    p_dest[7] = 0x0000;
  }

  return 8;
}

#include "asmerr.h"
#include "strcomp.h"

/*!------------------------------------------------------------------------
 * \fn     check_dec_fp_dispose_result(int ret, const struct sStrComp *p_arg)
 * \brief  check the result of Double_2...and throw associated error messages
 * \param  ret return code
 * \param  p_arg associated source argument
 * ------------------------------------------------------------------------ */

Boolean check_dec_fp_dispose_result(int ret, const struct sStrComp *p_arg)
{
  if (ret >= 0) 
    return True;
  switch (ret)
  {
    case -EIO:
      WrStrErrorPos(ErrNum_UnderRange, p_arg);
      return False;
    case -EBADF:
      WrXErrorPos(ErrNum_InvArg, "raster", &p_arg->Pos);
      return False;
    case -E2BIG:
      WrStrErrorPos(ErrNum_OverRange, p_arg);
      return False;
    case -EINVAL:
      WrXErrorPos(ErrNum_InvArg, "INF/NaN", &p_arg->Pos);
      return False;
    default:
      WrStrErrorPos(ErrNum_InvArg, p_arg);
      return False;
  }
}
