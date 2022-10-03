/* ibmfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* IBM Floating Point Format                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <math.h>

#include "ieeefloat.h"
#include "errmsg.h"
#include "asmerr.h"
#include "ibmfloat.h"

/*!------------------------------------------------------------------------
 * \fn     Double2IBMFloat(Word *pDest, double Src, Boolean ToDouble)
 * \brief  convert floating point number to IBM single precision format
 * \param  pDest where to write result (2 words)
 * \param  Src floating point number to store
 * \param  ToDouble convert to double precision (64 instead of 32 bits)?
 * \return True if conversion was successful
 * ------------------------------------------------------------------------ */

#define DBG_FLOAT 0

Boolean Double2IBMFloat(Word *pDest, double Src, Boolean ToDouble)
{
  Word Sign;
  Integer Exponent;
  LongWord Mantissa, Fraction;

#if DBG_FLOAT
  fprintf(stderr, "(0) %lf\n", Src);
#endif

  /* (1) Dissect IEEE number */

  ieee8_dissect(&Sign, &Exponent, &Mantissa, &Fraction, Src);

  /* (2) Convert IEEE 2^n exponent to multiple of four since IBM float exponent is to the base of 16: */

  while ((Mantissa & 0x10000000ul) || (Exponent & 3))
  {
    if (Mantissa & 1)
      Fraction |= 0x1000000ul;
    Mantissa >>= 1;
    Fraction >>= 1;
    Exponent++;
  }
#if DBG_FLOAT
  fprintf(stderr, "(expo4) %2d * 0x%08x * 2^%d Fraction 0x%08x\n", Sign ? -1 : 1, Mantissa, Exponent, Fraction);
#endif

  /* (3) make base-16 exponent explicit */

  Exponent /= 4;
#if DBG_FLOAT
  fprintf(stderr, "(exp16) %2d * 0x%08x * 16^%d Fraction 0x%08x\n", Sign ? -1 : 1, Mantissa, Exponent, Fraction);
#endif

  /* (2a) Round-to-the-nearest for single precision: */

  if (!ToDouble)
  {
    Boolean RoundUp;

    /* Bits 27..4 of mantissa will make it into dest, so the decision bit is bit 3: */

    if (Mantissa & 0x8) /* fraction is >= 0.5 */
    {
      if ((Mantissa & 7) || Fraction) /* fraction is > 0.5 -> round up */
        RoundUp = True;
      else /* fraction is 0.5 -> round towards even, i.e. round up if mantissa is odd */
        RoundUp = !!(Mantissa & 0x10);
    }
    else /* fraction is < 0.5 -> round down */
      RoundUp = False;
#if DBG_FLOAT
    fprintf(stderr, "RoundUp %u\n", RoundUp);
#endif
    if (RoundUp)
    {
      Mantissa += 16 - (Mantissa & 15);
      Fraction = 0;
      if (Mantissa & 0x10000000ul)
      {
        Mantissa >>= 4;
        Exponent++;
      }
    }
#if DBG_FLOAT
    fprintf(stderr, "(round) %2d * 0x%08x * 16^%d Fraction 0x%08x\n", Sign ? -1 : 1, Mantissa, Exponent, Fraction);
#endif
  }

  /* (3a) Overrange? */

  if (Exponent > 63)
  {
    WrError(ErrNum_OverRange);
    return False;
  }
  else
  {
    /* (3b) number that is too small may degenerate to 0: */

    while ((Exponent < -64) && Mantissa)
    {
      Exponent++; Mantissa >>= 4;
    }
    if (Exponent < -64)
      Exponent = -64;

    /* (3c) add bias to exponent */

    Exponent += 64;

    /* (3d) store result */

    pDest[0] = (Sign << 15) | ((Exponent << 8) & 0x7f00) | ((Mantissa >> 20) & 0xff);
    pDest[1] = (Mantissa >> 4) & 0xffff;

    /* IBM format has four mantissa bits more than IEEE double, so the 4 LSBs
       remain zero: */

    if (ToDouble)
    {
      pDest[2] = ((Mantissa & 15) << 12) | ((Fraction >> 12) & 0x0fff);
      pDest[3] = (Fraction & 0x0fff) << 4;
    }
    return True;
  }
}
