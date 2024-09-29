/* ieeefloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* IEEE Floating Point Handling                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <float.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include "be_le.h"
#include "as_float.h"
#ifdef HOST_DECFLOAT
# include "decfloat.h"
#endif
#include "ieeefloat.h"

#define DBG_FLOAT 0

#ifdef IEEEFLOAT_8_DOUBLE
/*!------------------------------------------------------------------------
 * \fn     as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
 * \brief  dissect float into components - version if as_float_t is IEEE Double
 * \param  p_dest result buffer
 * \param  num number to dissect
 * ------------------------------------------------------------------------ */

void as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
{
  Byte buf[8];
  LongWord mant_h, mant_l;

  as_float_zero(p_dest);

  /* binary representation, little endian: */

  memcpy(buf, &num, 8);
  if (HostBigEndian)
    QSwap(buf, 8);

#if DBG_FLOAT
  fprintf(stdout, "%0.15f\n", num);
#endif

  /* (a) Sign is MSB of highest byte: */

  p_dest->negative = !!(buf[7] & 0x80);

  /* (b) Exponent is stored in the following 11 bits, with a bias of 1023: */

  p_dest->exponent = (buf[7] & 0x7f);
  p_dest->exponent = (p_dest->exponent << 4) | ((buf[6] >> 4) & 15);
  switch (p_dest->exponent)
  {
    case 0:
      p_dest->fp_class = AS_FP_SUBNORMAL;
      break;
    case 2047:
      p_dest->fp_class = AS_FP_INFINITE;
      break;
    default:
      p_dest->fp_class = AS_FP_NORMAL;
  }
  p_dest->exponent -= 1023;

  /* (c) Make leading one explicit (if not denormal) */

  as_float_append_mantissa_bits(p_dest, p_dest->fp_class == AS_FP_NORMAL, 1);

  /* (d) Extract 52 bits of mantissa: */

  mant_h = buf[6] & 15;
  mant_h = (mant_h << 8) | buf[5];
  mant_h = (mant_h << 8) | buf[4];
  as_float_append_mantissa_bits(p_dest, mant_h, 20);
  mant_l = buf[3];
  mant_l = (mant_l << 8) | buf[2];
  mant_l = (mant_l << 8) | buf[1];
  mant_l = (mant_l << 8) | buf[0];
  as_float_append_mantissa_bits(p_dest, mant_l, 32);

  /* Distinguish NaN and Infinite */

  if ((p_dest->fp_class == AS_FP_INFINITE)
   && (mant_h || mant_l))
    p_dest->fp_class = AS_FP_NAN;
#if DBG_FLOAT
  as_float_dump(stdout, "0", p_dest);
#endif
}
#endif /* IEEEFLOAT_8_DOUBLE */

/* x86 80 bit float format is just extended with two or six padding bytes
   on i386 resp. x86-64, which can be ignored for dissection: */

#if (defined IEEEFLOAT_10_10_LONG_DOUBLE) || (defined IEEEFLOAT_10_12_LONG_DOUBLE) || (defined IEEEFLOAT_10_16_LONG_DOUBLE)
/*!------------------------------------------------------------------------
 * \fn     as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
 * \brief  dissect float into components - version if as_float_t is x86 extended 80 Bit
 * \param  p_dest result buffer
 * \param  num number to dissect
 * ------------------------------------------------------------------------ */

void as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
{
  Byte buf[16];
  LongWord mant_h, mant_l;

  as_float_zero(p_dest);

  /* binary representation, little endian: */

  memcpy(buf, &num, 10);
  if (HostBigEndian)
    TSwap(buf, 10);

#if DBG_FLOAT
  fprintf(stdout, "%0.15Lf\n", num);
#endif

  /* (a) Sign is MSB of highest byte: */

  p_dest->negative = !!(buf[9] & 0x80);

  /* (b) Exponent is stored in the following 15 bits, with a bias of 16383:
         Note special case for denormal numbers, which is due to the
         explicit leading mantissa bit: */

  p_dest->exponent = (buf[9] & 0x7f);
  p_dest->exponent = (p_dest->exponent << 8) | buf[8];
  switch (p_dest->exponent)
  {
    case 0:
      p_dest->fp_class = AS_FP_SUBNORMAL;
      p_dest->exponent = -16382;
      break;
    case 32767:
      p_dest->fp_class = AS_FP_INFINITE;
      p_dest->exponent -= 16383;
      break;
    default:
      p_dest->fp_class = AS_FP_NORMAL;
      p_dest->exponent -= 16383;
  }

  /* (c) leading one is *not* implicit */

  /* (d) Extract 64 bits of mantissa: */

  mant_h = buf[7];
  mant_h = (mant_h << 8) | buf[6];
  mant_h = (mant_h << 8) | buf[5];
  mant_h = (mant_h << 8) | buf[4];
  as_float_append_mantissa_bits(p_dest, mant_h, 32);
  mant_l = buf[3];
  mant_l = (mant_l << 8) | buf[3];
  mant_l = (mant_l << 8) | buf[2];
  mant_l = (mant_l << 8) | buf[1];
  mant_l = (mant_l << 8) | buf[0];
  as_float_append_mantissa_bits(p_dest, mant_l, 32);

  /* Distinguish NaN and Infinite */

  if ((p_dest->fp_class == AS_FP_INFINITE)
   && !as_float_mantissa_is_zero_from(p_dest, 1))
      p_dest->fp_class = AS_FP_NAN;

#if DBG_FLOAT
  as_float_dump(stdout, "0", p_dest);
#endif
}
#endif /* IEEEFLOAT_10_10_LONG_DOUBLE || IEEEFLOAT_10_12_LONG_DOUBLE || IEEEFLOAT_10_16_LONG_DOUBLE */

#ifdef IEEEFLOAT_10_2P8_LONG_DOUBLE
/*!------------------------------------------------------------------------
 * \fn     as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
 * \brief  dissect float into components - version if as_float_t is 68K extended 96/80 Bit
 * \param  p_dest result buffer
 * \param  num number to dissect
 * ------------------------------------------------------------------------ */

void as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
{
  Byte buf[12];
  LongWord mant_h, mant_l;

  as_float_zero(p_dest);

  /* binary representation, big endian: */

  memcpy(buf, &num, 12);

  /* (a) Sign is MSB of highest byte: */

  p_dest->negative = !!(buf[0] & 0x80);

  /* (b) Exponent is stored in the following 15 bits, with a bias of 16383:
         Note special case for denormal numbers, which is due to the
         explicit leading mantissa bit: */

  p_dest->exponent = (buf[0] & 0x7f);
  p_dest->exponent = (p_dest->exponent << 8) | buf[1];
  switch (p_dest->exponent)
  {
    case 0:
      p_dest->fp_class = AS_FP_SUBNORMAL;
      p_dest->exponent = -16382;
      break;
    case 32767:
      p_dest->fp_class = AS_FP_INFINITE;
      p_dest->exponent -= 16383;
      break;
    default:
      p_dest->fp_class = AS_FP_NORMAL;
      p_dest->exponent -= 16383;
  }

  /* (c) leading one is *not* implicit */

  /* (d) Extract 64 bits of mantissa: */

  mant_h = buf[4];
  mant_h = (mant_h << 8) | buf[5];
  mant_h = (mant_h << 8) | buf[6];
  mant_h = (mant_h << 8) | buf[7];
  as_float_append_mantissa_bits(p_dest, mant_h, 32);
  mant_l = buf[8];
  mant_l = (mant_l << 8) | buf[9];
  mant_l = (mant_l << 8) | buf[10];
  mant_l = (mant_l << 8) | buf[11];
  as_float_append_mantissa_bits(p_dest, mant_l, 32);

  /* Distinguish NaN and Infinite */

  if ((p_dest->fp_class == AS_FP_INFINITE)
   && !as_float_mantissa_is_zero_from(p_dest, 1))
      p_dest->fp_class = AS_FP_NAN;
}
#endif /* IEEEFLOAT_10_12_LONG_DOUBLE */

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ieee4(as_float_t inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to IEEE single (32 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_ieee4(as_float_t inp, Byte *pDest, Boolean NeedsBig)
{
  as_float_dissect_t dissect;
#if (defined IEEEFLOAT_8_DOUBLE) || (defined IEEEFLOAT_10_LONG_DOUBLE)

  float tmp;
  as_float_dissect(&dissect, inp);
  if ((dissect.fp_class != AS_FP_NAN)
   && (dissect.fp_class != AS_FP_INFINITE)
   && (as_fabs(inp) > FLT_MAX))
    return -E2BIG;
  tmp = inp;
  memcpy(pDest, &tmp, 4);
  if (HostBigEndian != NeedsBig)
    DSwap(pDest, 4);

#else

  as_float_round_t round_type;
  unsigned mask = NeedsBig ? 3 : 0;

  /* (1) Dissect IEEE number */

  as_float_dissect(&dissect, inp);

  /* Infinity/NaN: */

  if ((dissect.fp_class == AS_FP_NAN)
   || (dissect.fp_class == AS_FP_INFINITE))
  {
    pDest[3 ^ mask] = (dissect.negative << 7) | 0x7f;
    pDest[2 ^ mask] = 0x80;
    pDest[1 ^ mask] =
    pDest[0 ^ mask] = 0;

    /* clone all-ones mantissa for NaN: */

    if (as_float_mantissa_is_allones(&dissect))
    {
      pDest[2 ^ mask] |= 0x7f;
      pDest[1 ^ mask] |= 0xff;
      pDest[0 ^ mask] |= 0xff;
    }

    /* otherwise clone MSB+LSB of mantissa: */

    else
    {
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 1))
        pDest[2 ^ mask] |= 0x40;
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dissect.mantissa_bits - 1))
        pDest[0 ^ mask] |= 0x01;
    }
    return 2;
  }

  /* (2) Round to target precision: */

  round_type = as_float_round(&dissect, 24);

  /* (3a) Overrange? */

  if (dissect.exponent > 127)
    return -E2BIG;
  else if ((dissect.exponent == 127)
        && as_float_mantissa_is_allones(&dissect)
        && (round_type == e_round_down))
    return -E2BIG;

  /* (3b) number that is too small may degenerate to 0: */

  while ((dissect.exponent < -127) && !as_float_mantissa_is_zero(&dissect))
  {
    dissect.exponent++;
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
  }

  /* numbers too small to represent degenerate to 0 (mantissa was shifted out) */

  if (dissect.exponent < -127)
    dissect.exponent = -127;

  /* For denormal numbers, exponent is 2^(-126) and not 2^(-127)!
     So if we end up with an exponent of 2^(-127), convert
     mantissa so it corresponds to 2^(-127): */

  else if (dissect.exponent == -127)
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);

  /* (3c) add bias to exponent */

  dissect.exponent += 127;

  /* (3d) store result */

  pDest[3 ^ mask] = (dissect.negative << 7)
                  | ((dissect.exponent >> 1) & 0x7f);
  pDest[2 ^ mask] = ((dissect.exponent & 1) << 7)
                  | as_float_mantissa_extract(&dissect, 1, 7);
  pDest[1 ^ mask] = as_float_mantissa_extract(&dissect, 8, 8);
  pDest[0 ^ mask] = as_float_mantissa_extract(&dissect, 16, 8);

#endif
  return 4;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ieee8(as_float_t inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to IEEE double (64 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_ieee8(as_float_t inp, Byte *pDest, Boolean NeedsBig)
{
  as_float_dissect_t dissect;
#if (defined IEEEFLOAT_8_DOUBLE) || (defined IEEEFLOAT_10_LONG_DOUBLE)

  double tmp;
  as_float_dissect(&dissect, inp);
  if ((dissect.fp_class != AS_FP_NAN)
   && (dissect.fp_class != AS_FP_INFINITE)
   && (as_fabs(inp) > DBL_MAX))
    return -E2BIG;
  tmp = inp;
  memcpy(pDest, &tmp, 8);
  if (HostBigEndian != NeedsBig)
    QSwap(pDest, 8);

#else

  as_float_round_t round_type;
  unsigned mask = NeedsBig ? 7 : 0;

  /* (1) Dissect IEEE number */

  as_float_dissect(&dissect, inp);

  /* Infinity/NaN: */

  if ((dissect.fp_class == AS_FP_NAN)
   || (dissect.fp_class == AS_FP_INFINITE))
  {
    pDest[7 ^ mask] = (dissect.negative << 7) | 0x7f;
    pDest[6 ^ mask] = 0xf0;
    pDest[5 ^ mask] =
    pDest[4 ^ mask] =
    pDest[3 ^ mask] =
    pDest[2 ^ mask] =
    pDest[1 ^ mask] =
    pDest[0 ^ mask] = 0;

    /* clone all-ones mantissa for NaN: */

    if (as_float_mantissa_is_allones(&dissect))
    {
      pDest[6 ^ mask] |= 0x0f;
      pDest[5 ^ mask] |= 0xff;
      pDest[4 ^ mask] |= 0xff;
      pDest[3 ^ mask] |= 0xff;
      pDest[2 ^ mask] |= 0xff;
      pDest[1 ^ mask] |= 0xff;
      pDest[0 ^ mask] |= 0xff;
    }

    /* otherwise clone MSB+LSB of mantissa: */

    else
    {
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 1))
        pDest[6 ^ mask] |= 0x08;
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dissect.mantissa_bits - 1))
        pDest[0 ^ mask] |= 0x01;
    }
    return 2;
  }

  /* (2) Round to target precision: */

  round_type = as_float_round(&dissect, 53);

  /* (3a) Overrange? */

  if (dissect.exponent > 1023)
    return -E2BIG;
  else if ((dissect.exponent == 1023)
        && as_float_mantissa_is_allones(&dissect)
        && (round_type == e_round_down))
    return -E2BIG;

  /* (3b) number that is too small may degenerate to 0: */

  while ((dissect.exponent < -1023) && !as_float_mantissa_is_zero(&dissect))
  {
    dissect.exponent++;
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
  }

  /* TODO: This is needed to get a correct exponent value (0) for 0.0,
     if the host format has smaller exponent that IEEE64.
     Ultimately, as_float_dissect() should return an exponent
     of zero. */

  if (as_float_mantissa_is_zero(&dissect))
    dissect.exponent = -1023;

  /* numbers too small to represent degenerate to 0 (mantissa was shifted out) */

  if (dissect.exponent < -1023)
    dissect.exponent = -1023;

  /* For denormal numbers, exponent is 2^(-1022) and not 2^(-1023)!
     So if we end up with an exponent of 2^(-1023), convert
     mantissa so it corresponds to 2^(-1023): */

  else if (dissect.exponent == -1023)
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);

  /* (3c) add bias to exponent */

  dissect.exponent += 1023;

  /* (3d) store result */

  pDest[7 ^ mask] = (dissect.negative << 7)
                  | ((dissect.exponent >> 4) & 0x7f);
  pDest[6 ^ mask] = ((dissect.exponent & 15) << 4)
                  | as_float_mantissa_extract(&dissect, 1, 4);
  pDest[5 ^ mask] = as_float_mantissa_extract(&dissect, 5, 8);
  pDest[4 ^ mask] = as_float_mantissa_extract(&dissect, 13, 8);
  pDest[3 ^ mask] = as_float_mantissa_extract(&dissect, 21, 8);
  pDest[2 ^ mask] = as_float_mantissa_extract(&dissect, 29, 8);
  pDest[1 ^ mask] = as_float_mantissa_extract(&dissect, 37, 8);
  pDest[0 ^ mask] = as_float_mantissa_extract(&dissect, 45, 8);

#endif
  return 8;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ieee10(as_float_t inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to non-IEEE extended (80 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_ieee10(as_float_t inp, Byte *pDest, Boolean NeedsBig)
{
#if (defined IEEEFLOAT_10_10_LONG_DOUBLE) || (defined IEEEFLOAT_10_12_LONG_DOUBLE) || (defined IEEEFLOAT_10_16_LONG_DOUBLE)
  memcpy(pDest, &inp, 10);
  if (HostBigEndian != NeedsBig)
    TSwap(pDest, 10);

#else

  as_float_dissect_t dissect;

  /* (1) Dissect IEEE number */

  as_float_dissect(&dissect, inp);

  /* Infinity/NaN: Note that for IEEEFLOAT_10_2P8_LONG_DOUBLE (M68K Extended float),
     the mantissa's MSB of infinities and NANs may be 1 or 0.  We want the 1-version,
     to be consistent with x86: */

  if ((dissect.fp_class == AS_FP_NAN)
   || (dissect.fp_class == AS_FP_INFINITE))
  {
    pDest[9] = (dissect.negative << 7) | 0x7f;
    pDest[8] = 0xff;

    /* clone all-ones mantissa for NaN: */

    if (as_float_mantissa_is_allones(&dissect))
      memset(&pDest[0], 8, 0xff);

    /* otherwise clone MSB+LSB of mantissa: */

    else
    {
      memset(&pDest[0], 0x00, 8);
      pDest[7] |= 0x80;
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dissect.mantissa_bits - 1))
        pDest[0] |= 0x01;
    }

    if (NeedsBig)
      TSwap(pDest, 10);

    return 10;
  }

# ifdef IEEEFLOAT_10_2P8_LONG_DOUBLE

  Byte *p_src = (Byte*)&inp;
  pDest[NeedsBig ? 0 : 9] = p_src[0];
  pDest[NeedsBig ? 1 : 8] = p_src[1];
  pDest[NeedsBig ? 2 : 7] = p_src[4];
  pDest[NeedsBig ? 3 : 6] = p_src[5];
  pDest[NeedsBig ? 4 : 5] = p_src[6];
  pDest[NeedsBig ? 5 : 4] = p_src[7];
  pDest[NeedsBig ? 6 : 3] = p_src[8];
  pDest[NeedsBig ? 7 : 2] = p_src[9];
  pDest[NeedsBig ? 8 : 1] = p_src[10];
  pDest[NeedsBig ? 9 : 0] = p_src[11];

# else /* !IEEEFLOAT_10_2P8_LONG_DOUBLE */

  as_float_dissect(&dissect, inp);
  /* (2) Round to target precision: */

  as_float_round(&dissect, 64);

  /* (3a) Overrange? */

  if (dissect.exponent > 16383)
    return -E2BIG;

  /* (3b) number that is too small may degenerate to 0: */

  while ((dissect.exponent < -16383) && !as_float_mantissa_is_zero(&dissect))
  {
    dissect.exponent++;
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
  }

  /* TODO: This is needed to get a correct exponent value (0) for 0.0.
     The old routine just added the difference of the 64-bit ad 80-bit
     biases, which resulted in an exponent of 0x3c00 (-1023) in the
     final value.  Ultimately, as_float_dissect() should return an exponent
     of zero. */

  if (as_float_mantissa_is_zero(&dissect))
    dissect.exponent = -16383;

  /* numbers too small to represent degenerate to 0 (mantissa was shifted out) */

  if (dissect.exponent < -16383)
    dissect.exponent = -16383;

  /* For denormal numbers, exponent is 2^(-16382) and not 2^(-16383)!
     So if we end up with an exponent of 2^(-16383), convert
     mantissa so it corresponds to 2^(-16382): */

  else if (dissect.exponent == -16383)
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);

  /* (3c) add bias to exponent */

  dissect.exponent += 16383;

  /* (3d) store result */

  pDest[9] = (dissect.negative << 7)
           | ((dissect.exponent >> 8) & 0x7f);
  pDest[8] = ((dissect.exponent >> 0) & 0xff);
  pDest[7] = as_float_mantissa_extract(&dissect,  0, 8); /* leading one is explicit! */
  pDest[6] = as_float_mantissa_extract(&dissect,  8, 8);
  pDest[5] = as_float_mantissa_extract(&dissect, 16, 8);
  pDest[4] = as_float_mantissa_extract(&dissect, 24, 8);
  pDest[3] = as_float_mantissa_extract(&dissect, 32, 8);
  pDest[2] = as_float_mantissa_extract(&dissect, 40, 8);
  pDest[1] = as_float_mantissa_extract(&dissect, 48, 8);
  pDest[0] = as_float_mantissa_extract(&dissect, 56, 8);

  if (NeedsBig)
    TSwap(pDest, 10);
# endif /* IEEEFLOAT_10_2P8_LONG_DOUBLE */
#endif /* IEEEFLOAT_10_1?_LONG_DOUBLE */
  return 10;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ieee16(as_float_t inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to IEEE quad (128 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_ieee16(as_float_t inp, Byte *pDest, Boolean NeedsBig)
{
  as_float_dissect_t dissect;

  /* (1) Dissect IEEE number */

  as_float_dissect(&dissect, inp);

  /* Infinity/NaN: */

  if ((dissect.fp_class == AS_FP_NAN)
   || (dissect.fp_class == AS_FP_INFINITE))
  {
    pDest[15] = (dissect.negative << 7) | 0x7f;
    pDest[14] = 0xff;

    /* clone all-ones mantissa for NaN: */

    if (as_float_mantissa_is_allones(&dissect))
      memset(&pDest[0], 14, 0xff);

    /* otherwise clone MSB+LSB of mantissa: */

    else
    {
      memset(&pDest[0], 0x00, 14);
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 1))
        pDest[13] |= 0x80;
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dissect.mantissa_bits - 1))
        pDest[0] |= 0x01;
    }
    if (NeedsBig)
      OSwap(pDest, 16);
    return 16;
  }

  /* (2) Round to target precision: */

  as_float_round(&dissect, 113);

  /* (3a) Overrange? */

  if (dissect.exponent > 16383)
    return -E2BIG;

  /* (3b) number that is too small may degenerate to 0: */

  while ((dissect.exponent < -16383) && !as_float_mantissa_is_zero(&dissect))
  {
    dissect.exponent++;
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
  }

  /* TODO: This is needed to get a correct exponent value (0) for 0.0.
     The old routine just added the difference of the 64-bit ad 80-bit
     biases, which resulted in an exponent of 0x3c00 (-1023) in the
     final value.  Ultimately, as_float_dissect() should return an exponent
     of zero. */

  if (as_float_mantissa_is_zero(&dissect))
    dissect.exponent = -16383;

  /* numbers too small to represent degenerate to 0 (mantissa was shifted out) */

  if (dissect.exponent < -16383)
    dissect.exponent = -16383;

  /* For denormal numbers, exponent is 2^(-16382) and not 2^(-16383)!
     So if we end up with an exponent of 2^(-16383), convert
     mantissa so it corresponds to 2^(-16382): */

  else if (dissect.exponent == -16383)
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);

  /* (3c) add bias to exponent */

  dissect.exponent += 16383;

  /* (3d) store result */

  pDest[15] = (dissect.negative << 7)
            | ((dissect.exponent >> 8) & 0x7f);
  pDest[14] = ((dissect.exponent >> 0) & 0xff);
  pDest[13] = as_float_mantissa_extract(&dissect,  1, 8);
  pDest[12] = as_float_mantissa_extract(&dissect,  9, 8);
  pDest[11] = as_float_mantissa_extract(&dissect, 17, 8);
  pDest[10] = as_float_mantissa_extract(&dissect, 25, 8);
  pDest[ 9] = as_float_mantissa_extract(&dissect, 33, 8);
  pDest[ 8] = as_float_mantissa_extract(&dissect, 41, 8);
  pDest[ 7] = as_float_mantissa_extract(&dissect, 49, 8);
  pDest[ 6] = as_float_mantissa_extract(&dissect, 57, 8);
  pDest[ 5] = as_float_mantissa_extract(&dissect, 65, 8);
  pDest[ 4] = as_float_mantissa_extract(&dissect, 73, 8);
  pDest[ 3] = as_float_mantissa_extract(&dissect, 81, 8);
  pDest[ 2] = as_float_mantissa_extract(&dissect, 89, 8);
  pDest[ 1] = as_float_mantissa_extract(&dissect, 97, 8);
  pDest[ 0] = as_float_mantissa_extract(&dissect,105, 8);

  if (NeedsBig)
    OSwap(pDest, 16);

  return 16;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ieee2(as_float_t inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert floating point number to IEEE half size (16 bit) format
 * \param  inp floating point number to store
 * \param  pDest where to write result (2 bytes)
 * \param  NeedsBig req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_ieee2(as_float_t inp, Byte *pDest, Boolean NeedsBig)
{
  as_float_dissect_t dissect;
  as_float_round_t round_type;

  /* (1) Dissect IEEE number */

  as_float_dissect(&dissect, inp);

  /* Infinity/NaN: */

  if ((dissect.fp_class == AS_FP_NAN)
   || (dissect.fp_class == AS_FP_INFINITE))
  {
    pDest[1 ^ !!NeedsBig] = (dissect.negative << 7) | 0x7c;
    pDest[0 ^ !!NeedsBig] = 0;

    /* clone all-ones mantissa for NaN: */

    if (as_float_mantissa_is_allones(&dissect))
    {
      pDest[1 ^ !!NeedsBig] |= 0x03;
      pDest[0 ^ !!NeedsBig] = 0xff;
    }

    /* otherwise clone MSB+LSB of mantissa: */

    else
    {
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 1))
        pDest[1 ^ !!NeedsBig] |= 0x02;
      if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dissect.mantissa_bits - 1))
        pDest[0 ^ !!NeedsBig] |= 0x01;
    }
    return 2;
  }

  /* (2) Round to target precision: */

  round_type = as_float_round(&dissect, 11);

  /* (3a) Overrange? */

  if (dissect.exponent > 15)
    return -E2BIG;
  else if ((dissect.exponent == 15)
        && as_float_mantissa_is_allones(&dissect)
        && (round_type == e_round_down))
    return -E2BIG;

  /* (3b) number that is too small may degenerate to 0: */

  while ((dissect.exponent < -15) && !as_float_mantissa_is_zero(&dissect))
  {
    dissect.exponent++;
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
  }

  /* numbers too small to represent degenerate to 0 (mantissa was shifted out) */

  if (dissect.exponent < -15)
    dissect.exponent = -15;

  /* For denormal numbers, exponent is 2^(-14) and not 2^(-15)!
     So if we end up with an exponent of 2^(-15), convert
     mantissa so it corresponds to 2^(-14): */

  else if (dissect.exponent == -15)
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);

  /* (3c) add bias to exponent */

  dissect.exponent += 15;

  /* (3d) store result */

  pDest[1 ^ !!NeedsBig] = (dissect.negative << 7)
                        | ((dissect.exponent << 2) & 0x7c)
                        | as_float_mantissa_extract(&dissect, 1, 2);
  pDest[0 ^ !!NeedsBig] = as_float_mantissa_extract(&dissect, 3, 8);
  return 2;
}
