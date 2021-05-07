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
#include <math.h>
#include <string.h>

#include "endian.h"
#ifdef VAXFLOAT
# include "vaxfloat.h"
#endif
#include "ieeefloat.h"

#define DBG_FLOAT 0

/*!------------------------------------------------------------------------
 * \fn     as_fpclassify(Double inp)
 * \brief  classify floating point number
 * \param  inp number to classify
 * \return type of number
 * ------------------------------------------------------------------------ */

int as_fpclassify(Double inp)
{
#ifdef IEEEFLOAT
  Byte Buf[8];
  Word Exponent;

  /* extract exponent */

  Double_2_ieee8(inp, Buf, True);
  Exponent = (Buf[0] & 0x7f);
  Exponent = (Exponent << 4) | ((Buf[1] >> 4) & 15);
  switch (Exponent)
  {
    case 0:
      return AS_FP_SUBNORMAL;
    case 2047:
    {
      Byte Acc = Buf[1] & 0x0f;
      int z;

      for (z = 2; z < 8; Acc |= Buf[z++]);
      return Acc ? AS_FP_NAN : AS_FP_INFINITE;
    }
    default:
      return AS_FP_NORMAL;
  }
#else
  (void)inp; return AS_FP_NORMAL;
#endif
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_ieee4(Double inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to IEEE single (32 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * ------------------------------------------------------------------------ */

void Double_2_ieee4(Double inp, Byte *pDest, Boolean NeedsBig)
{
#ifdef IEEEFLOAT
  Single tmp = inp;
  memcpy(pDest, &tmp, 4);
#elif defined VAXFLOAT
  VAXF_2_Single(pDest, inp);
#else
# error define host floating point format
#endif
  if (HostBigEndian != NeedsBig)
    DSwap(pDest, 4);
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_ieee8(Double inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to IEEE double (64 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * ------------------------------------------------------------------------ */

void Double_2_ieee8(Double inp, Byte *pDest, Boolean NeedsBig)
{
#ifdef IEEEFLOAT
  memcpy(pDest, &inp, 8);
#elif defined VAXFLOAT
  VAXD_2_Double(pDest, inp);
#else
# error define host floating point format
#endif
  if (HostBigEndian != NeedsBig)
    QSwap(pDest, 8);
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_ieee10(Double inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert float to non-IEEE extended (80 bit) float binary representation
 * \param  inp input number
 * \param  pDest where to write binary data
 * \param  NeedsBig req's big endian?
 * ------------------------------------------------------------------------ */

void Double_2_ieee10(Double inp, Byte *pDest, Boolean NeedsBig)
{
  Byte Buffer[8];
  Byte Sign;
  Word Exponent;
  int z;

#ifdef IEEEFLOAT
  Boolean Denormal;

  memcpy(Buffer, &inp, 8);
  if (HostBigEndian)
    QSwap(Buffer, 8);
  Sign = (Buffer[7] & 0x80);
  Exponent = (Buffer[6] >> 4) + (((Word) Buffer[7] & 0x7f) << 4);
  Denormal = (Exponent == 0);
  if (Exponent == 2047)
    Exponent = 32767;
  else Exponent += (16383 - 1023);
  Buffer[6] &= 0x0f;
  if (!Denormal)
    Buffer[6] |= 0x10;
  for (z = 7; z >= 2; z--)
    pDest[z] = ((Buffer[z - 1] & 0x1f) << 3) | ((Buffer[z - 2] & 0xe0) >> 5);
  pDest[1] = (Buffer[0] & 0x1f) << 3;
  pDest[0] = 0;
  pDest[9] = Sign | ((Exponent >> 8) & 0x7f);
  pDest[8] = Exponent & 0xff;
#elif defined VAXFLOAT
  VAXD_2_LongDouble(pDest, inp);
#else
# error define host floating point format
#endif
  if (NeedsBig)
    TSwap(pDest, 10);
}

/*!------------------------------------------------------------------------
 * \fn     Double_2_ieee2(Double inp, Byte *pDest, Boolean NeedsBig)
 * \brief  convert floating point number to IEEE half size (16 bit) format
 * \param  inp floating point number to store
 * \param  pDest where to write result (2 bytes)
 * \param  NeedsBig req's big endian?
 * \return True if conversion was successful
 * ------------------------------------------------------------------------ */

Boolean Double_2_ieee2(Double inp, Byte *pDest, Boolean NeedsBig)
{
  Byte Buf[8];
  Word Sign;
  Integer Exponent;
  LongWord Mantissa, Fraction;
  Boolean RoundUp;

#if DBG_FLOAT
  fprintf(stderr, "(0) %g\n", inp);
#endif

  /* get into byte array, MSB first */

  Double_2_ieee8(inp, Buf, True);

  /* (1) Dissect IEEE number */

  /* (1a) Sign is MSB of first byte: */

  Sign = !!(Buf[0] & 0x80);

  /* (1b) Exponent is stored in the following 11 bits, with a bias of 1023:  */

  Exponent = (Buf[0] & 0x7f);
  Exponent = (Exponent << 4) | ((Buf[1] >> 4) & 15);
  Exponent -= 1023;

  /* (1c) Extract 28 bits of mantissa: */

  Mantissa = Buf[1] & 15;
  Mantissa = (Mantissa << 8) | Buf[2];
  Mantissa = (Mantissa << 8) | Buf[3];
  Mantissa = (Mantissa << 8) | Buf[4];

  /* (1d) remaining 24 bits of mantissa, needed for double precision and rounding: */

  Fraction = Buf[5];
  Fraction = (Fraction << 8) | Buf[6];
  Fraction = (Fraction << 8) | Buf[7];

  /* (1e) All-ones Exponent (2047-1023=1024) is reserved for NaN and infinity: */

  if (Exponent == 1024)
  {
    pDest[1 ^ !!NeedsBig] = (Sign << 7) | 0x7c;
    pDest[0 ^ !!NeedsBig] = 0;

    /* clone all-ones mantissa for NaN: */

    if ((Mantissa == 0x0ffffffful) && (Fraction == 0x00ffffff))
    {
      pDest[1 ^ !!NeedsBig] |= 0x03;
      pDest[0 ^ !!NeedsBig] = 0xff;
    }

    /* otherwise clone MSB+LSB of mantissa: */

    else
    {
      if (Mantissa & 0x08000000ul)
        pDest[1 ^ !!NeedsBig] |= 0x02;
      if (Mantissa & 0x00000001ul)
        pDest[0 ^ !!NeedsBig] |= 0x01;
    }
    return True;
  }

  /* (1f) if not denormal, make leading one of mantissa explicit: */

  if (Exponent != -1023)
    Mantissa |= 0x10000000ul;
#if DBG_FLOAT
  fprintf(stderr, "(cnvrt) %2d * 0x%08x * 2^%d Fraction 0x%08x\n", Sign ? -1 : 1, Mantissa, Exponent, Fraction);
#endif

 /* (2) Round-to-the-nearest for FP16: */

   /* Bits 27..18 of fractional part of mantissa will make it into dest, so the decision bit is bit 17: */

   if (Mantissa & 0x20000ul) /* fraction is >= 0.5 */
   {
     if ((Mantissa & 0x1fffful) || Fraction) /* fraction is > 0.5 -> round up */
       RoundUp = True;
     else /* fraction is 0.5 -> round towards even, i.e. round up if mantissa is odd */
       RoundUp = !!(Mantissa & 0x40000ul);
   }
   else /* fraction is < 0.5 -> round down */
     RoundUp = False;
#if DBG_FLOAT
   fprintf(stderr, "RoundUp %u\n", RoundUp);
#endif
   if (RoundUp)
   {
     Mantissa += 0x40000ul - (Mantissa & 0x3fffful);
     Fraction = 0;
     if (Mantissa & 0x20000000ul)
     {
       Mantissa >>= 1;
       Exponent++;
     }
   }
#if DBG_FLOAT
   fprintf(stderr, "(round) %2d * 0x%08x * 2^%d Fraction 0x%08x\n", Sign ? -1 : 1, Mantissa, Exponent, Fraction);
#endif

  /* (3a) Overrange? */

  if (Exponent > 15)
    return False;
  else
  {
    /* (3b) number that is too small may degenerate to 0: */

    while ((Exponent < -15) && Mantissa)
    {
      Exponent++; Mantissa >>= 1;
    }
#if DBG_FLOAT
    fprintf(stderr, "(after denormchk) %2d * 0x%08x * 2^%d Fraction 0x%08x\n", Sign ? -1 : 1, Mantissa, Exponent, Fraction);
#endif

    /* numbers too small to represent degenerate to 0 (mantissa was shifted out) */

    if (Exponent < -15)
      Exponent = -15;

    /* For denormal numbers, exponent is 2^(-14) and not 2^(-15)!
       So if we end up with an exponent of 2^(-15), convert
       mantissa so it corresponds to 2^(-14): */

    else if (Exponent == -15)
      Mantissa >>= 1;

    /* (3c) add bias to exponent */

    Exponent += 15;

    /* (3d) store result */

    pDest[1 ^ !!NeedsBig] = (Sign << 7) | ((Exponent << 2) & 0x7c) | ((Mantissa >> 26) & 3);
    pDest[0 ^ !!NeedsBig] = (Mantissa >> 18) & 0xff;

    return True;
  }
}
