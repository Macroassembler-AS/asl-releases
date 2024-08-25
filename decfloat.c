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
#include <string.h>

#include "be_le.h"
#include "as_float.h"
#include "ieeefloat.h"
#include "decfloat.h"

#define DBG_FLOAT 0

#ifdef HOST_DECFLOAT

#ifdef __GFLOAT
/* Some VAX compilers internally seem to use D float
   and are unable to parse the G float DBL_MAX literal
   of 8.98...E+308 from float.h.
   So we put a hand-crafted constant in memory.
   Note this is only about half of the maximum, but
   putting 0x7ff into the exponent results in a
   floating point exception.  Maybe SIMH misinterpretes
   this as infinity, which does not exist for VAX
   floatingpoint formats? */

double as_decfloat_get_max_gfloat(void)
{
  static double max_gfloat;
  static Boolean set = False;

  if (!set)
  {
    Byte raw[8] = { 0xef, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    memcpy(&max_gfloat, raw, 8);
    set = True;
  }
  return max_gfloat;
}
#endif /* __GFLOAT */

/*!------------------------------------------------------------------------
 * \fn     as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
 * \brief  dissect (64 bit) float into components - may be D or G float
 * \param  p_dest result buffer
 * \param  num number to dissect
 * ------------------------------------------------------------------------ */

void as_float_dissect(as_float_dissect_t *p_dest, as_float_t num)
{
  const Byte *p_src = (const Byte*)&num;
  LongWord mant_h, mant_l;
  Integer biased_exponent;

  as_float_zero(p_dest);
#if DBG_FLOAT
  {
    int z;
    printf("%g:", num);
    for (z = 0; z < 8; z++)
      printf(" %02x", p_src[z]);
    printf("\n");
  }
#endif

  /* (a) Sign is MSB of highest byte: */

  p_dest->negative = !!(p_src[1] & 0x80);

  /* (b) Exponent is stored in the following 8/11 bits, with a bias of 128/1024: */

  biased_exponent = p_src[1] & 0x7f;
#ifdef __GFLOAT
  biased_exponent = (biased_exponent << 4) | ((p_src[0] >> 4) & 15);
#else
  biased_exponent = (biased_exponent << 1) | ((p_src[0] >> 7) & 1);
#endif

  /* (c) remove bias, correct mantissa normalization */

#ifdef __GFLOAT
  p_dest->exponent = biased_exponent - 1024;
#else
  p_dest->exponent = biased_exponent - 128;
#endif
  p_dest->exponent--;

  /* (d) mantissa parts: */

  mant_h = p_src[0]
#ifdef __GFLOAT
         & 0x0f;
#else
         & 0x7f;
#endif
  mant_h = (mant_h << 8) | p_src[3];
  mant_h = (mant_h << 8) | p_src[2];
  mant_l = p_src[5];
  mant_l = (mant_l << 8) | p_src[4];
  mant_l = (mant_l << 8) | p_src[7];
  mant_l = (mant_l << 8) | p_src[6];

  /* (e) append leading one (if not zero) and mantissa words: */

  as_float_append_mantissa_bits(p_dest, mant_h || mant_l || biased_exponent, 1);
#ifdef __GFLOAT
  as_float_append_mantissa_bits(p_dest, mant_h, 20);
#else
  as_float_append_mantissa_bits(p_dest, mant_h, 23);
#endif
  as_float_append_mantissa_bits(p_dest, mant_l, 32);

#if DBG_FLOAT
  as_float_dump(stdout, "(0)", p_dest);
#endif
}

/*!------------------------------------------------------------------------
 * \fn     DECF_2_Single(Byte *pDest, float inp)
 * \brief  convert single precision (DEC F) to IEEE single precision
 * \param  pDest where to write
 * \param  inp value to convert
 * ------------------------------------------------------------------------ */

void DECF_2_Single(Byte *pDest, float inp)
{
  float tmp = inp;

  /* IEEE + DEC layout is the same for single, just the exponent offset is different
     by two: */

  tmp /= 4;
  memcpy(pDest, &tmp, 4);
  WSwap(pDest, 4);
}

/*!------------------------------------------------------------------------
 * \fn     DECD_2_Double(Byte *pDest, float inp)
 * \brief  convert double precision (DEC D) to IEEE double precision
 * \param  pDest where to write
 * \param  inp value to convert
 * ------------------------------------------------------------------------ */

void DECD_2_Double(Byte *pDest, as_float_t inp)
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

void DECD_2_LongDouble(Byte *pDest, as_float_t inp)
{
  Byte Buffer[8], Sign;
  Word Exponent;
  int z;

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
 * \fn     as_float_2_dec_lit(as_float_t inp, Byte *p_dest)
 * \brief  convert from host to DEC (VAX) 6 bit float (literal) format
 * \param  inp value to convert
 * \param  p_dest result buffer
 * \return >0 for number of bytes used (1) or <0 for error code
 * ------------------------------------------------------------------------ */

extern int as_float_2_dec_lit(as_float_t inp, Byte *p_dest)
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
 * \fn     as_float_2_dec_f(as_float_t inp, Word *p_dest)
 * \brief  convert from host to DEC (PDP/VAX) 4 byte float (F) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (2) or <0 for error code
 * ------------------------------------------------------------------------ */

int as_float_2_dec_f(as_float_t inp, Word *p_dest)
{
#ifdef HOST_DECFLOAT
  float tmp;

  /* native format: */
  if (fabs(inp) > 1.7E38)
    return -E2BIG;
  tmp = inp;
  memcpy(p_dest, &tmp, 4);

#else /* !HOST_DECFLOAT */

  as_float_dissect_t dissect;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* Inf/NaN cannot be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  as_float_round(&dissect, 24);

  /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

  dissect.exponent++;
  if (dissect.exponent > 127)
    return -E2BIG;

  /* DEC float does not handle denormal numbers and truncates to zero: */

  if (dissect.fp_class == AS_FP_SUBNORMAL)
  {
    dissect.exponent = -128;
    memset(dissect.mantissa, 0, sizeof dissect.mantissa);
  }

  /* add bias to exponent */

  dissect.exponent += 128;

  /* assemble 1st word (seeeeeeeemmmmmmm): */

                                         /* discard highest mantissa bit 23 (implicit leading one) */
  p_dest[0] = (((Word)dissect.negative & 1) << 15)
            | ((dissect.exponent << 7) & 0x7f80u)
            | as_float_mantissa_extract(&dissect, 1, 7);  /* mant bits 22...16 */
  p_dest[1] = as_float_mantissa_extract(&dissect, 8, 16); /* mant bits 15... 0 */

#endif /* HOST_DECFLOAT */

  return 2;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_dec_d(as_float_t inp, Word *p_dest)
 * \brief  convert from host to DEC (PDP/VAX) 8 byte float (D) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (4) or <0 for error code
 * ------------------------------------------------------------------------ */

int as_float_2_dec_d(as_float_t inp, Word *p_dest)
{
#if (defined HOST_DECFLOAT) && (!defined __GFLOAT)
  double tmp;

  /* native format: */
  tmp = inp;
  memcpy(p_dest, &tmp, 8);

#else /* !HOST_DECFLOAT || __GFLOAT*/

  as_float_dissect_t dissect;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* Inf/NaN cannot be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  as_float_round(&dissect, 56);

  /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

  dissect.exponent++;
  if (dissect.exponent > 127)
    return -E2BIG;

  /* DEC float does not handle denormal numbers and truncates to zero: */

  if (dissect.fp_class == AS_FP_SUBNORMAL)
  {
    dissect.exponent = -128;
    memset(dissect.mantissa, 0, sizeof dissect.mantissa);
  }

  /* add bias to exponent */

  dissect.exponent += 128;

  /* assemble 1st word (seeeeeeeemmmmmmm): */

                                         /* discard highest mantissa bit 55 (implicit leading one) */
  p_dest[0] = (((Word)dissect.negative & 1) << 15)
            | ((dissect.exponent << 7) & 0x7f80u)
            | as_float_mantissa_extract(&dissect,  1,  7); /* mant bits 54...48 */
  p_dest[1] = as_float_mantissa_extract(&dissect,  8, 16); /* mant bits 47...32 */
  p_dest[2] = as_float_mantissa_extract(&dissect, 24, 16); /* mant bits 31...24 */
  p_dest[3] = as_float_mantissa_extract(&dissect, 40, 16); /* mant bits 15... 0 */

#endif /* HOST_DECFLOAT && !__GFLOAT */

  return 4;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_dec_g(as_float_t inp, Word *p_dest)
 * \brief  convert from host to DEC (VAX) 8 byte float (G) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (4) or <0 for error code
 * ------------------------------------------------------------------------ */

int as_float_2_dec_g(as_float_t inp, Word *p_dest)
{
#if (defined HOST_DECFLOAT) && (defined __GFLOAT)
  double tmp;

  /* native format: */
  tmp = inp;
  memcpy(p_dest, &tmp, 8);

#else /* !HOST_DECFLOAT || !__GFLOAT*/

  as_float_dissect_t dissect;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* Inf/NaN cannot be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  as_float_round(&dissect, 53);

  /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

  dissect.exponent++;
  if (dissect.exponent > 1023)
    return -E2BIG;

  /* DEC float does not handle denormal numbers and truncates to zero: */

  if (dissect.fp_class == AS_FP_SUBNORMAL)
  {
    dissect.exponent = -1024;
    memset(dissect.mantissa, 0, sizeof dissect.mantissa);
  }

  /* add bias to exponent */

  dissect.exponent += 1024;

  /* assemble 1st word (seeeeeeeeeeemmmm): */

                                         /* discard highest mantissa bit 52 (implicit leading one) */
  p_dest[0] = (((Word)dissect.negative & 1) << 15)
            | ((dissect.exponent << 4) & 0x7ff0u)
            | as_float_mantissa_extract(&dissect,  1,  4); /* mant bits 51...48 */
  p_dest[1] = as_float_mantissa_extract(&dissect,  5, 16); /* mant bits 47...32 */
  p_dest[2] = as_float_mantissa_extract(&dissect, 21, 16); /* mant bits 31...16 */
  p_dest[3] = as_float_mantissa_extract(&dissect, 37, 16); /* mant bits 15... 0 */

#endif /* HOST_DECFLOAT && __GFLOAT */

  return 4;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_dec_h(as_float_t inp, Word *p_dest)
 * \brief  convert from host to DEC (VAX) 16 byte float (h) format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (8) or <0 for error code
 * ------------------------------------------------------------------------ */

int as_float_2_dec_h(as_float_t inp, Word *p_dest)
{
  as_float_dissect_t dissect;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* Inf/NaN cannot be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  as_float_round(&dissect, 113);

  /* For DEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

  dissect.exponent++;
  if (dissect.exponent > 16383)
    return -E2BIG;

  /* DEC float does not handle denormal numbers and truncates to zero: */

  if (dissect.fp_class == AS_FP_SUBNORMAL)
  {
    dissect.exponent = -16384;
    memset(dissect.mantissa, 0, sizeof dissect.mantissa);
  }

  /* add bias to exponent */

  dissect.exponent += 16384;

  /* assemble 1st word (seeeeeeeeeeeeeee): */

  p_dest[0] = (((Word)dissect.negative & 1) << 15)
            | ((dissect.exponent << 0) & 0x7fffu);
                                         /* discard highest mantissa bit 112 (implicit leading one) */
  p_dest[1] = as_float_mantissa_extract(&dissect,  1, 16); /* mant bits 111...96 */
  p_dest[2] = as_float_mantissa_extract(&dissect, 17, 16); /* mant bits  95...80 */
  p_dest[3] = as_float_mantissa_extract(&dissect, 33, 16); /* mant bits  79...64 */
  p_dest[4] = as_float_mantissa_extract(&dissect, 49, 16); /* mant bits  63...48 */
  p_dest[5] = as_float_mantissa_extract(&dissect, 65, 16); /* mant bits  47...32 */
  p_dest[6] = as_float_mantissa_extract(&dissect, 81, 16); /* mant bits  31...16 */
  p_dest[7] = as_float_mantissa_extract(&dissect, 97, 16); /* mant bits  15... 0 */

  return 8;
}
