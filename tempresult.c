/* tempresult.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* internal holder for int/float/string                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "strutil.h"
#include "asmdef.h"
#include "tempresult.h"

/*!------------------------------------------------------------------------
 * \fn     as_tempres_ini(TempResult *p_res)
 * \brief  initialize temp result buffer
 * \param  p_res buffer to initialize
 * ------------------------------------------------------------------------ */

void as_tempres_ini(TempResult *p_res)
{
  p_res->Typ = TempNone;
  p_res->Flags = eSymbolFlag_None;
  p_res->AddrSpaceMask = 0;
  p_res->DataSize = eSymbolSizeUnknown;
  p_res->Relocs = NULL;
  memset(&p_res->Contents, 0, sizeof(p_res->Contents));
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_free(TempResult *p_res)
 * \brief  deinit temp result buffer
 * \param  p_res buffer to deinit
 * ------------------------------------------------------------------------ */

void as_tempres_free(TempResult *p_res)
{
  if (p_res->Typ == TempString)
    as_nonz_dynstr_free(&p_res->Contents.str);
  p_res->Typ = TempNone;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_none(TempResult *p_res)
 * \brief  set temp result to none
 * \param  p_res result to fill
 * ------------------------------------------------------------------------ */

void as_tempres_set_none(TempResult *p_res)
{
  if (p_res->Typ == TempString)
    as_nonz_dynstr_free(&p_res->Contents.str);
  p_res->Typ = TempNone;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_int(TempResult *p_res, LargeInt value)
 * \brief  set temp result to integer value
 * \param  p_res result to fill
 * \param  value integer value to set
 * ------------------------------------------------------------------------ */

void as_tempres_set_int(TempResult *p_res, LargeInt value)
{
  if (p_res->Typ == TempString)
    as_nonz_dynstr_free(&p_res->Contents.str);
  p_res->Typ = TempInt;
  p_res->Contents.Int = value;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_float(TempResult *p_res, Double value)
 * \brief  set temp result to float value
 * \param  p_res result to fill
 * \param  value float value to set
 * ------------------------------------------------------------------------ */

void as_tempres_set_float(TempResult *p_res, Double value)
{
  if (p_res->Typ == TempString)
    as_nonz_dynstr_free(&p_res->Contents.str);
  p_res->Typ = TempFloat;
  p_res->Contents.Float = value;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_str(TempResult *p_res, const as_nonz_dynstr_t *p_value)
 * \brief  set temp result to string value
 * \param  p_res result to fill
 * \param  p_value string value to set
 * ------------------------------------------------------------------------ */

void as_tempres_set_str(TempResult *p_res, const as_nonz_dynstr_t *p_value)
{
  if (p_res->Typ != TempString)
    as_nonz_dynstr_ini(&p_res->Contents.str, p_value->capacity);
  p_res->Typ = TempString;
  as_nonz_dynstr_copy(&p_res->Contents.str, p_value);
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_str_raw(TempResult *p_res, const char *p_src, size_t src_len)
 * \brief  set temp result to string value, with raw source
 * \param  p_res result to fill
 * \param  p_src string value to set
 * \param  src_len length of source
 * ------------------------------------------------------------------------ */

void as_tempres_set_str_raw(TempResult *p_res, const char *p_src, size_t src_len)
{
  if (p_res->Typ != TempString)
    as_nonz_dynstr_ini(&p_res->Contents.str, as_nonz_dynstr_roundup_len(src_len));
  p_res->Typ = TempString;
  as_nonz_dynstr_append_raw(&p_res->Contents.str, p_src, src_len);
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_c_str(TempResult *p_res, const char *p_src)
 * \brief  set temp result to string value, with C string source
 * \param  p_res result to fill
 * \param  p_src string value to set
 * ------------------------------------------------------------------------ */

void as_tempres_set_c_str(TempResult *p_res, const char *p_src)
{
  as_tempres_set_str_raw(p_res, p_src, strlen(p_src));
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_set_reg(TempResult *p_res, const tRegDescr *p_value)
 * \brief  set temp result to register value
 * \param  p_res result to fill
 * \param  p_value register value to set
 * ------------------------------------------------------------------------ */

void as_tempres_set_reg(TempResult *p_res, const tRegDescr *p_value)
{
  if (p_res->Typ == TempString)
    as_nonz_dynstr_free(&p_res->Contents.str);
  p_res->Typ = TempReg;
  p_res->Contents.RegDescr = *p_value;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_copy(TempResult *p_dest, const TempResult *p_src)
 * \brief  copy temp result's value
 * \param  p_dest destination
 * \param  p_src source
 * ------------------------------------------------------------------------ */

void as_tempres_copy_value(TempResult *p_dest, const TempResult *p_src)
{
  switch (p_src->Typ)
  {
    case TempInt:
      as_tempres_set_int(p_dest, p_src->Contents.Int);
      break;
    case TempFloat:
      as_tempres_set_float(p_dest, p_src->Contents.Float);
      break;
    case TempString:
      as_tempres_set_str(p_dest, &p_src->Contents.str);
      break;
    case TempReg:
      as_tempres_set_reg(p_dest, &p_src->Contents.RegDescr);
      break;
    default:
      as_tempres_set_none(p_dest);
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_copy(TempResult *p_dest, const TempResult *p_src)
 * \brief  copy temp result
 * \param  p_dest destination
 * \param  p_src source
 * ------------------------------------------------------------------------ */

void as_tempres_copy(TempResult *p_dest, const TempResult *p_src)
{
  as_tempres_copy_value(p_dest, p_src);
  p_dest->Flags = p_src->Flags;
  p_dest->AddrSpaceMask = p_src->AddrSpaceMask;
  p_dest->DataSize = p_src->DataSize;
  p_dest->Relocs = p_src->Relocs;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_cmp(const TempResult *p_res1, const TempResult *p_res2)
 * \brief  compare two values
 * \param  p_res1, p_res2 values to compare
 * \return -1/0/+1 for p_res1 </=/> p_res2
 * ------------------------------------------------------------------------ */

int as_tempres_cmp(const TempResult *p_res1, const TempResult *p_res2)
{
  if (p_res1->Typ != p_res2->Typ)
    return -1;
  switch (p_res1->Typ)
  {
    case TempString:
      return as_nonz_dynstr_cmp(&p_res1->Contents.str, &p_res2->Contents.str);
    case TempFloat:
      if (p_res1->Contents.Float < p_res2->Contents.Float)
        return -1;
      else if (p_res1->Contents.Float > p_res2->Contents.Float)
        return 1;
      else
        return 0;
    case TempInt:
      if (p_res1->Contents.Int < p_res2->Contents.Int)
        return -1;
      else if (p_res1->Contents.Int > p_res2->Contents.Int)
        return 1;
      else
        return 0;
    case TempReg:
      if (p_res1->Contents.RegDescr.Reg < p_res2->Contents.RegDescr.Reg)
        return -1;
      else if (p_res1->Contents.RegDescr.Reg > p_res2->Contents.RegDescr.Reg)
        return 1;
      else
        return 0;
    default:
      return 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     TempResultToFloat(TempResult *pResult)
 * \brief  convert TempResult to float
 * \param  pResult tempresult to convert
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int TempResultToFloat(TempResult *pResult)
{
  switch (pResult->Typ)
  {
    case TempInt:
      pResult->Contents.Float = pResult->Contents.Int;
      pResult->Typ = TempFloat;
      break;
    case TempFloat:
      break;
    default:
      as_tempres_set_none(pResult);
      return -1;
  }
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_tempres_append_dynstr(as_dynstr_t *p_dest, const TempResult *pResult)
 * \brief  convert result to readable form
 * \param  p_dest where to write ASCII representation
 * \param  pResult result to convert
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int as_tempres_append_dynstr(as_dynstr_t *p_dest, const TempResult *pResult)
{
  switch (pResult->Typ)
  {
    case TempInt:
      as_sdprcatf(p_dest, "%llld", pResult->Contents.Int);
      break;
    case TempFloat:
      as_sdprcatf(p_dest, "%0.16e", pResult->Contents.Float);
      KillBlanks(p_dest->p_str);
      break;
    case TempString:
    {
      char quote_chr = (pResult->Flags & eSymbolFlag_StringSingleQuoted) ? '\'' : '"';
      const char *p_run, *p_end;

      as_sdprcatf(p_dest, "%c", quote_chr);
      for (p_run = pResult->Contents.str.p_str, p_end = p_run + pResult->Contents.str.len;
           p_run < p_end; p_run++)
        if ((*p_run == '\\') || (*p_run == quote_chr))
          as_sdprcatf(p_dest, "\\%c", *p_run);
        else if (!isprint(*p_run))
          as_sdprcatf(p_dest, "\\%03d", *p_run);
        else
          as_sdprcatf(p_dest, "%c", *p_run);

      as_sdprcatf(p_dest, "%c", quote_chr);
      break;
    }
    default:
      return -1;
  }
  return 0;
}
