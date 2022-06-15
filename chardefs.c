/* chardefs.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* System-Dependent Definition of National Specific Characters               */
/*                                                                           */
/*****************************************************************************/

#include <string.h>

#include "chardefs.h"

static const tNLSCharacterTab Tab8859_1 =
{
  { '\344', '\0' }, /* eCH_ae */
  { '\353', '\0' }, /* eCH_ee */
  { '\357', '\0' }, /* eCH_ie */
  { '\366', '\0' }, /* eCH_oe */
  { '\374', '\0' }, /* eCH_ue */
  { '\304', '\0' }, /* eCH_Ae */
  { '\313', '\0' }, /* eCH_Ee */
  { '\317', '\0' }, /* eCH_Ie */
  { '\326', '\0' }, /* eCH_Oe */
  { '\334', '\0' }, /* eCH_Ue */
  { '\337', '\0' }, /* eCH_sz */
  { '\262', '\0' }, /* eCH_e2 */
  { '\265', '\0' }, /* eCH_mu */
  { '\340', '\0' }, /* eCH_agrave */
  { '\300', '\0' }, /* eCH_Agrave */
  { '\350', '\0' }, /* eCH_egrave */
  { '\310', '\0' }, /* eCH_Egrave */
  { '\354', '\0' }, /* eCH_igrave */
  { '\314', '\0' }, /* eCH_Igrave */
  { '\362', '\0' }, /* eCH_ograve */
  { '\322', '\0' }, /* eCH_Ograve */
  { '\371', '\0' }, /* eCH_ugrave */
  { '\331', '\0' }, /* eCH_Ugrave */
  { '\341', '\0' }, /* eCH_aacute */
  { '\301', '\0' }, /* eCH_Aacute */
  { '\351', '\0' }, /* eCH_eacute */
  { '\311', '\0' }, /* eCH_Eacute */
  { '\355', '\0' }, /* eCH_iacute */
  { '\315', '\0' }, /* eCH_Iacute */
  { '\363', '\0' }, /* eCH_oacute */
  { '\323', '\0' }, /* eCH_Oacute */
  { '\372', '\0' }, /* eCH_uacute */
  { '\332', '\0' }, /* eCH_Uacute */
  { '\342', '\0' }, /* eCH_acirc */
  { '\302', '\0' }, /* eCH_Acirc */
  { '\352', '\0' }, /* eCH_ecirc */
  { '\312', '\0' }, /* eCH_Ecirc */
  { '\356', '\0' }, /* eCH_icirc */
  { '\316', '\0' }, /* eCH_Icirc */
  { '\364', '\0' }, /* eCH_ocirc */
  { '\324', '\0' }, /* eCH_Ocirc */
  { '\373', '\0' }, /* eCH_ucirc */
  { '\333', '\0' }, /* eCH_Ucirc */
  { '\347', '\0' }, /* eCH_ccedil */
  { '\307', '\0' }, /* eCH_Ccedil */
  { '\361', '\0' }, /* eCH_ntilde */
  { '\321', '\0' }, /* eCH_Ntilde */
  { '\345', '\0' }, /* eCH_aring */
  { '\305', '\0' }, /* eCH_Aring */
  { '\346', '\0' }, /* eCH_aelig */
  { '\306', '\0' }, /* eCH_Aelig */
  { '\370', '\0' }, /* eCH_oslash */
  { '\330', '\0' }, /* eCH_Oslash */
  { '\277', '\0' }, /* eCH_iquest */
  { '\241', '\0' }, /* eCH_iexcl */
};

static const tNLSCharacterTab Tab437 =
{
  { '\204', '\0' }, /* eCH_ae */
  { '\211', '\0' }, /* eCH_ee */
  { '\213', '\0' }, /* eCH_ie */
  { '\224', '\0' }, /* eCH_oe */
  { '\201', '\0' }, /* eCH_ue */
  { '\216', '\0' }, /* eCH_Ae */
  { 'E'   , 'e'  }, /* eCH_Ee */
  { 'I'   , 'e'  }, /* eCH_Ie */
  { '\231', '\0' }, /* eCH_Oe */
  { '\232', '\0' }, /* eCH_Ue */
  { '\341', '\0' }, /* eCH_sz */
  { '\375', '\0' }, /* eCH_e2 */
  { '\346', '\0' }, /* eCH_mu */
  { '\205', '\0' }, /* eCH_agrave */
  { '`'   , 'A'  }, /* eCH_Agrave */
  { '\212', '\0' }, /* eCH_egrave */
  { '`'   , 'E'  }, /* eCH_Egrave */
  { '\215', '\0' }, /* eCH_igrave */
  { '`'   , 'I'  }, /* eCH_Igrave */
  { '\225', '\0' }, /* eCH_ograve */
  { '`'   , 'O'  }, /* eCH_Ograve */
  { '\227', '\0' }, /* eCH_ugrave */
  { '`'   , 'U'  }, /* eCH_Ugrave */
  { '\240', '\0' }, /* eCH_aacute */
  { '\''  , 'A'  }, /* eCH_Aacute */
  { '\202', '\0' }, /* eCH_eacute */
  { '\220', '\0' }, /* eCH_Eacute */
  { '\241', '\0' }, /* eCH_iacute */
  { '\''  , 'I'  }, /* eCH_Iacute */
  { '\242', '\0' }, /* eCH_oacute */
  { '\''  , 'O'  }, /* eCH_Oacute */
  { '\243', '\0' }, /* eCH_uacute */
  { '\''  , 'U'  }, /* eCH_Uacute */
  { '\203', '\0' }, /* eCH_acirc */
  { '^'   , 'A'  }, /* eCH_Acirc */
  { '\210', '\0' }, /* eCH_ecirc */
  { '^'   , 'E'  }, /* eCH_Ecirc */
  { '\214', '\0' }, /* eCH_icirc */
  { '^'   , 'I'  }, /* eCH_Icirc */
  { '\223', '\0' }, /* eCH_ocirc */
  { '^'   , 'O'  }, /* eCH_Ocirc */
  { '\226', '\0' }, /* eCH_ucirc */
  { '^'   , 'U'  }, /* eCH_Ucirc */
  { '\207', '\0' }, /* eCH_ccedil */
  { '\200', '\0' }, /* eCH_Ccedil */
  { '\244', '\0' }, /* eCH_ntilde */
  { '\245', '\0' }, /* eCH_Ntilde */
  { '\206', '\0' }, /* eCH_aring */
  { '\217', '\0' }, /* eCH_Aring */
  { '\221', '\0' }, /* eCH_aelig */
  { '\222', '\0' }, /* eCH_Aelig */
  { 'o'   , '\0' }, /* eCH_oslash */
  { 'O'   , '\0' }, /* eCH_Oslash */
  { '\250', '\0' }, /* eCH_iquest */
  { '\255', '\0' }, /* eCH_iexcl */
};

static const tNLSCharacterTab Tab850 =
{
  { '\204', '\0' }, /* eCH_ae */
  { '\211', '\0' }, /* eCH_ee */
  { '\213', '\0' }, /* eCH_ie */
  { '\224', '\0' }, /* eCH_oe */
  { '\201', '\0' }, /* eCH_ue */
  { '\216', '\0' }, /* eCH_Ae */
  { '\323', '\0' }, /* eCH_Ee */
  { '\330', '\0' }, /* eCH_Ie */
  { '\231', '\0' }, /* eCH_Oe */
  { '\232', '\0' }, /* eCH_Ue */
  { '\341', '\0' }, /* eCH_sz */
  { '\375', '\0' }, /* eCH_e2 */
  { '\346', '\0' }, /* eCH_mu */
  { '\205', '\0' }, /* eCH_agrave */
  { '\267', '\0' }, /* eCH_Agrave */
  { '\212', '\0' }, /* eCH_egrave */
  { '\324', '\0' }, /* eCH_Egrave */
  { '\215', '\0' }, /* eCH_igrave */
  { '\336', '\0' }, /* eCH_Igrave */
  { '\225', '\0' }, /* eCH_ograve */
  { '\343', '\0' }, /* eCH_Ograve */
  { '\227', '\0' }, /* eCH_ugrave */
  { '\353', '\0' }, /* eCH_Ugrave */
  { '\240', '\0' }, /* eCH_aacute */
  { '\265', '\0' }, /* eCH_Aacute */
  { '\202', '\0' }, /* eCH_eacute */
  { '\220', '\0' }, /* eCH_Eacute */
  { '\241', '\0' }, /* eCH_iacute */
  { '\326', '\0' }, /* eCH_Iacute */
  { '\242', '\0' }, /* eCH_oacute */
  { '\340', '\0' }, /* eCH_Oacute */
  { '\243', '\0' }, /* eCH_uacute */
  { '\351', '\0' }, /* eCH_Uacute */
  { '\203', '\0' }, /* eCH_acirc */
  { '\266', '\0' }, /* eCH_Acirc */
  { '\210', '\0' }, /* eCH_ecirc */
  { '\322', '\0' }, /* eCH_Ecirc */
  { '\214', '\0' }, /* eCH_icirc */
  { '\327', '\0' }, /* eCH_Icirc */
  { '\223', '\0' }, /* eCH_ocirc */
  { '\342', '\0' }, /* eCH_Ocirc */
  { '\226', '\0' }, /* eCH_ucirc */
  { '\352', '\0' }, /* eCH_Ucirc */
  { '\207', '\0' }, /* eCH_ccedil */
  { '\200', '\0' }, /* eCH_Ccedil */
  { '\244', '\0' }, /* eCH_ntilde */
  { '\245', '\0' }, /* eCH_Ntilde */
  { '\206', '\0' }, /* eCH_aring */
  { '\217', '\0' }, /* eCH_Aring */
  { '\221', '\0' }, /* eCH_aelig */
  { '\222', '\0' }, /* eCH_Aelig */
  { '\233', '\0' }, /* eCH_oslash */
  { '\235', '\0' }, /* eCH_Oslash */
  { '\250', '\0' }, /* eCH_iquest */
  { '\255', '\0' }, /* eCH_iexcl */
};

static const tNLSCharacterTab TabUTF8 =
{
  { '\303', '\244' }, /* eCH_ae 0xe4 */
  { '\303', '\253' }, /* eCH_ee 0xeb */
  { '\303', '\257' }, /* eCH_ie 0xef */
  { '\303', '\266' }, /* eCH_oe 0xf6 */
  { '\303', '\274' }, /* eCH_ue 0xfc */
  { '\303', '\204' }, /* eCH_Ae 0xc4 */
  { '\303', '\213' }, /* eCH_Ee 0xcb */
  { '\303', '\217' }, /* eCH_Ie 0xcf */
  { '\303', '\226' }, /* eCH_Oe 0xd6 */
  { '\303', '\234' }, /* eCH_Ue 0xdc */
  { '\303', '\237' }, /* eCH_sz 0xdf */
  { '\302', '\262' }, /* eCH_e2 0xb2 */
  { '\302', '\265' }, /* eCH_mu 0xb5 */
  { '\303', '\240' }, /* eCH_agrave 0xe0 */
  { '\303', '\200' }, /* eCH_Agrave 0xc0 */
  { '\303', '\250' }, /* eCH_egrave 0xe8 */
  { '\303', '\210' }, /* eCH_Egrave 0xc8 */
  { '\303', '\254' }, /* eCH_igrave 0xec */
  { '\303', '\214' }, /* eCH_Igrave 0xcc */
  { '\303', '\262' }, /* eCH_ograve 0xf2 */
  { '\303', '\222' }, /* eCH_Ograve 0xd2 */
  { '\303', '\271' }, /* eCH_ugrave 0xf9 */
  { '\303', '\231' }, /* eCH_Ugrave 0xd9 */
  { '\303', '\241' }, /* eCH_aacute 0xe1 */
  { '\303', '\201' }, /* eCH_Aacute 0xc1 */
  { '\303', '\251' }, /* eCH_eacute 0xe9 */
  { '\303', '\211' }, /* eCH_Eacute 0xc9 */
  { '\303', '\255' }, /* eCH_iacute 0xed */
  { '\303', '\215' }, /* eCH_Iacute 0xcd */
  { '\303', '\263' }, /* eCH_oacute 0xf3 */
  { '\303', '\223' }, /* eCH_Oacute 0xd3 */
  { '\303', '\272' }, /* eCH_uacute 0xfa */
  { '\303', '\232' }, /* eCH_Uacute 0xda */
  { '\303', '\242' }, /* eCH_acirc 0xe2 */
  { '\303', '\202' }, /* eCH_Acirc 0xc2 */
  { '\303', '\252' }, /* eCH_ecirc 0xea */
  { '\303', '\212' }, /* eCH_Ecirc 0xca */
  { '\303', '\256' }, /* eCH_icirc 0xee */
  { '\303', '\216' }, /* eCH_Icirc 0xce */
  { '\303', '\264' }, /* eCH_ocirc 0xf4 */
  { '\303', '\224' }, /* eCH_Ocirc 0xd4 */
  { '\303', '\273' }, /* eCH_ucirc 0xfb */
  { '\303', '\233' }, /* eCH_Ucirc 0xdb */
  { '\303', '\247' }, /* eCH_ccedil 0xe7 */
  { '\303', '\207' }, /* eCH_Ccedil 0xc7 */
  { '\303', '\261' }, /* eCH_ntilde 0xf1 */
  { '\303', '\221' }, /* eCH_Ntilde 0xd1 */
  { '\303', '\245' }, /* eCH_aring 0xe5 */
  { '\303', '\205' }, /* eCH_Aring 0xc5 */
  { '\303', '\246' }, /* eCH_aelig 0xe6 */
  { '\303', '\206' }, /* eCH_Aelig 0xc6 */
  { '\303', '\270' }, /* eCH_oslash 0xf8 */
  { '\303', '\230' }, /* eCH_Oslash 0xd8 */
  { '\302', '\277' }, /* eCH_iquest 0xbf */
  { '\302', '\241' }, /* eCH_iexcl 0xa1 */
};

static const tNLSCharacterTab TabASCII7 =
{
  { 'a', 'e'  }, /* eCH_ae */
  { 'e', 'e'  }, /* eCH_ee */
  { 'i', 'e'  }, /* eCH_ie */
  { 'o', 'e'  }, /* eCH_oe */
  { 'u', 'e'  }, /* eCH_ue */
  { 'A', 'e'  }, /* eCH_Ae */
  { 'E', 'e'  }, /* eCH_Ee */
  { 'I', 'e'  }, /* eCH_Ie */
  { 'O', 'e'  }, /* eCH_Oe */
  { 'U', 'e'  }, /* eCH_Ue */
  { 's', 's'  }, /* eCH_sz */
  { '^', '2'  }, /* eCH_e2 */
  { 'u', '\0' }, /* eCH_mu */
  { '`', 'a'  }, /* eCH_agrave */
  { '`', 'A'  }, /* eCH_Agrave */
  { '`', 'e'  }, /* eCH_egrave */
  { '`', 'E'  }, /* eCH_Egrave */
  { '`', 'i'  }, /* eCH_igrave */
  { '`', 'I'  }, /* eCH_Igrave */
  { '`', 'o'  }, /* eCH_ograve */
  { '`', 'O'  }, /* eCH_Ograve */
  { '`', 'u'  }, /* eCH_ugrave */
  { '`', 'U'  }, /* eCH_Ugrave */
  { '\'','a'  }, /* eCH_aacute */
  { '\'','A'  }, /* eCH_Aacute */
  { '\'','e'  }, /* eCH_eacute */
  { '\'','E'  }, /* eCH_Eacute */
  { '\'','i'  }, /* eCH_iacute */
  { '\'','I'  }, /* eCH_Iacute */
  { '\'','o'  }, /* eCH_oacute */
  { '\'','O'  }, /* eCH_Oacute */
  { '\'','u'  }, /* eCH_uacute */
  { '\'','U'  }, /* eCH_Uacute */
  { '^', 'a'  }, /* eCH_acirc */
  { '^', 'A'  }, /* eCH_Acirc */
  { '^', 'e'  }, /* eCH_ecirc */
  { '^', 'E'  }, /* eCH_Ecirc */
  { '^', 'i'  }, /* eCH_icirc */
  { '^', 'I'  }, /* eCH_Icirc */
  { '^', 'o'  }, /* eCH_ocirc */
  { '^', 'O'  }, /* eCH_Ocirc */
  { '^', 'u'  }, /* eCH_ucirc */
  { '^', 'U'  }, /* eCH_Ucirc */
  { 'c', '\0' }, /* eCH_ccedil */
  { 'C', '\0' }, /* eCH_Ccedil */
  { '~', 'n'  }, /* eCH_ntilde */
  { '~', 'N'  }, /* eCH_Ntilde */
  { 'a', '\0' }, /* eCH_aring */
  { 'A', '\0' }, /* eCH_Aring */
  { 'a', 'e'  }, /* eCH_aelig */
  { 'A', 'E'  }, /* eCH_Aelig */
  { 'o', '\0' }, /* eCH_oslash */
  { 'O', '\0' }, /* eCH_Oslash */
  { '?', '\0' }, /* eCH_iquest */
  { '!', '\0' }, /* eCH_iexcl */
};

const char NLS_HtmlCharacterTab[eCH_cnt][9] =
{
  "&auml;"  , /* eCH_ae     */
  "&euml;"  , /* eCH_ee     */
  "&iuml;"  , /* eCH_ie     */
  "&ouml;"  , /* eCH_oe     */
  "&uuml;"  , /* eCH_ue     */
  "&Auml;"  , /* eCH_Ae     */
  "&Euml;"  , /* eCH_Ee     */
  "&Iuml;"  , /* eCH_Ie     */
  "&Ouml;"  , /* eCH_Oe     */
  "&Uuml;"  , /* eCH_Ue     */
  "&szlig;" , /* eCH_sz     */
  "&sup2;"  , /* eCH_e2     */
  "&micro;" , /* eCH_mu     */
  "&agrave;", /* eCH_agrave */
  "&Agrave;", /* eCH_Agrave */
  "&egrave;", /* eCH_egrave */
  "&Egrave;", /* eCH_Egrave */
  "&igrave;", /* eCH_igrave */
  "&Igrave;", /* eCH_Igrave */
  "&ograve;", /* eCH_ograve */
  "&Ograve;", /* eCH_Ograve */
  "&ugrave;", /* eCH_ugrave */
  "&Ugrave;", /* eCH_Ugrave */
  "&aacute;", /* eCH_aacute */
  "&Aacute;", /* eCH_Aacute */
  "&eacute;", /* eCH_eacute */
  "&Eacute;", /* eCH_Eacute */
  "&iacute;", /* eCH_iacute */
  "&Iacute;", /* eCH_Iacute */
  "&oacute;", /* eCH_oacute */
  "&Oacute;", /* eCH_Oacute */
  "&uacute;", /* eCH_uacute */
  "&Uacute;", /* eCH_Uacute */
  "&acirc;" , /* eCH_acirc  */
  "&Acirc;" , /* eCH_Acirc  */
  "&ecirc;" , /* eCH_ecirc  */
  "&Ecirc;" , /* eCH_Ecirc  */
  "&icirc;" , /* eCH_icirc  */
  "&Icirc;" , /* eCH_Icirc  */
  "&ocirc;" , /* eCH_ocirc  */
  "&Ocirc;" , /* eCH_Ocirc  */
  "&ucirc;" , /* eCH_ucirc  */
  "&Ucirc;" , /* eCH_Ucirc  */
  "&ccedil;", /* eCH_ccedil */
  "&Ccedil;", /* eCH_Ccedil */
  "&ntilde;", /* eCH_ntilde */
  "&Ntilde;", /* eCH_Ntilde */
  "&aring;" , /* eCH_aring  */
  "&Aring;" , /* eCH_Aring  */
  "&aelig;" , /* eCH_aelig  */
  "&Aelig;" , /* eCH_Aelig  */
  "&oslash" , /* eCH_oslash */
  "&Oslash" , /* eCH_Oslash */
  "&iquest;", /* eCH_iquest */
  "&iexcl;" , /* eCH_iexcl  */
};

/*!------------------------------------------------------------------------
 * \fn     tNLSCharacterTab *GetCharacterTab(tCodepage Codepage)
 * \brief  retrieve character encoding for given code page
 * \param  Codepage code page to query
 * \return * to character list
 * ------------------------------------------------------------------------ */

const tNLSCharacterTab *GetCharacterTab(tCodepage Codepage)
{
  switch (Codepage)
  {
    case eCodepageISO8859_1:
    case eCodepageISO8859_15:
    case eCodepage1252:
      return &Tab8859_1;
    case eCodepage437:
      return &Tab437;
    case eCodepage850:
      return &Tab850;
    case eCodepageUTF8:
      return &TabUTF8;
    default:
      return &TabASCII7;
  }
}

/*!------------------------------------------------------------------------
 * \fn     CharTab_GetLength(const tNLSCharacterTab *pTab, tNLSCharacter Character)
 * \brief  retrive length of of character string from table
 * \param  pTab table to use
 * \param  Character character to extract
 * \return length of character string
 * ------------------------------------------------------------------------ */

int CharTab_GetLength(const tNLSCharacterTab *pTab, tNLSCharacter Character)
{
  if (!(*pTab)[Character][0])
    return 0;
  else if (!(*pTab)[Character][1])
    return 1;
  else
    return 2;
}

/*!------------------------------------------------------------------------
 * \fn     CharTab_GetNULTermString(const tNLSCharacterTab *pTab, tNLSCharacter Character, char *pBuffer)
 * \brief  Provide NUL-terminated version of character string from table
 * \param  pTab table to use
 * \param  Character character to extract
 * \param  pBuffer temp buffer to built NUL-terminated version
 * \return * to NUL-termainted version
 * ------------------------------------------------------------------------ */

const char *CharTab_GetNULTermString(const tNLSCharacterTab *pTab, tNLSCharacter Character, char *pBuffer)
{
  if ((*pTab)[Character][1] == '\0')
    return (*pTab)[Character];
  else
  {
    memcpy(pBuffer, (*pTab)[Character], 2);
    pBuffer[2] = '\0';
    return pBuffer;
  }
}

/*!------------------------------------------------------------------------
 * \fn     UTF8ToUnicode(const char* *ppChr)
 * \brief  convert UTF-8 encoded char to Unicode
 * \param  ppChr * to character (points afterwards to next character)
 * \return Unicode value
 * ------------------------------------------------------------------------ */

static int CheckOneUTF8(unsigned char Val, unsigned char Mask, LongWord *pPart)
{
  if ((Val & Mask) == ((Mask << 1) & 0xff))
  {
    *pPart = (Val & ~Mask) & 0xff;
    return 1;
  }
  else
    return 0;
}

LongWord UTF8ToUnicode(const char* *ppChr)
{
  LongWord Part1, Part2, Part3, Part4;

  if (CheckOneUTF8((*ppChr)[0], 0x80, &Part1))
  {
    (*ppChr)++;
    return Part1;
  }
  else if (CheckOneUTF8((*ppChr)[0], 0xe0, &Part1)
        && CheckOneUTF8((*ppChr)[1], 0xc0, &Part2))
  {
    (*ppChr) += 2;
    return (Part1 << 6) | Part2;
  }
  else if (CheckOneUTF8((*ppChr)[0], 0xf0, &Part1)
        && CheckOneUTF8((*ppChr)[1], 0xc0, &Part2)
        && CheckOneUTF8((*ppChr)[2], 0xc0, &Part3))
  {
    (*ppChr) +=3;
    return (Part1 << 12) | (Part2 << 6) | Part3;
  }
  else if (CheckOneUTF8((*ppChr)[0], 0xf8, &Part1)
        && CheckOneUTF8((*ppChr)[1], 0xc0, &Part2)
        && CheckOneUTF8((*ppChr)[2], 0xc0, &Part3)
        && CheckOneUTF8((*ppChr)[3], 0xc0, &Part4))
  {
    (*ppChr) +=3;
    return (Part1 << 18) | (Part2 << 12) | (Part3 << 6) | Part4;
  }
  else
    return (unsigned char) *((*ppChr)++);
}

/*!------------------------------------------------------------------------
 * \fn     UnicodeToUTF8(char* *ppChr, LongWord Unicode)
 * \brief  convert UTF-8 encoded char to Unicode
 * \param  ppChr * to destination (points afterwards to next character)
 * \param  Unicode Unicode value
 * ------------------------------------------------------------------------ */

void UnicodeToUTF8(char* *ppChr, LongWord Unicode)
{
  if (Unicode <= 0x7f)
    *(*ppChr)++ = Unicode;
  else if (Unicode <= 0x7ff)
  {
    *(*ppChr)++ = 0xc0 | ((Unicode >> 6) & 0x1f);
    *(*ppChr)++ = 0x80 | ((Unicode >> 0) & 0x3f);
  }
  else if (Unicode <= 0xffff)
  {
    *(*ppChr)++ = 0xc0 | ((Unicode >> 12) & 0x1f);
    *(*ppChr)++ = 0x80 | ((Unicode >> 6) & 0x3f);
    *(*ppChr)++ = 0x80 | ((Unicode >> 0) & 0x3f);
  }
  else if (Unicode <= 0x10fffful)
  {
    *(*ppChr)++ = 0xc0 | ((Unicode >> 18) & 0x1f);
    *(*ppChr)++ = 0x80 | ((Unicode >> 12) & 0x3f);
    *(*ppChr)++ = 0x80 | ((Unicode >> 6) & 0x3f);
    *(*ppChr)++ = 0x80 | ((Unicode >> 0) & 0x3f);
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_wcwidth(unsigned codepoint)
 * \brief  return display width of Unicode code point
 * \param  codepoint code point
 * \return width on display (0...)
 * ------------------------------------------------------------------------ */

unsigned as_wcwidth(unsigned codepoint)
{
  if (((codepoint >= 0x1100) && (codepoint <= 0x115F)) /* HANGUL CHOSEONG KIYEOK..HANGUL CHOSEONG FILLER */
   || ((codepoint >= 0x231A) && (codepoint <= 0x231B)) /* WATCH..HOURGLASS */
   || ((codepoint >= 0x2329) && (codepoint <= 0x232A)) /* LEFT/RIGHT-POINTING ANGLE BRACKET */
   || ((codepoint >= 0x23E9) && (codepoint <= 0x23EC)) /* BLACK RIGHT-POINTING DOUBLE TRIANGLE..BLACK DOWN-POINTING DOUBLE TRIANGLE */
   || (codepoint == 0x23F0)                            /* ALARM CLOCK */
   || (codepoint == 0x23F3)                            /* HOURGLASS WITH FLOWING SAND */
   || ((codepoint >= 0x25FD) && (codepoint <= 0x25FE)) /* WHITE MEDIUM SMALL SQUARE..BLACK MEDIUM SMALL SQUARE */
   || ((codepoint >= 0x2614) && (codepoint <= 0x2615)) /* UMBRELLA WITH RAIN DROPS..HOT BEVERAGE */
   || ((codepoint >= 0x2648) && (codepoint <= 0x2653)) /* ARIES..PISCES */
   || (codepoint == 0x267F)                            /* WHEELCHAIR SYMBOL */
   || (codepoint == 0x2693)                            /* ANCHOR */
   || (codepoint == 0x26A1)                            /* HIGH VOLTAGE SIGN */
   || (codepoint == 0x26CE)                            /* OPHIUCHUS */
   || (codepoint == 0x26D4)                            /* NO ENTRY */
   || (codepoint == 0x26EA)                            /* CHURCH */
   || ((codepoint >= 0x26F2) && (codepoint <= 0x26F3)) /* FOUNTAIN..FLAG IN HOLE */
   || (codepoint == 0x26F5)                            /* SAILBOAT */
   || (codepoint == 0x26FA)                            /* TENT */
   || (codepoint == 0x26FD)                            /* FUEL PUMP */
   || (codepoint == 0x2705)                            /* WHITE HEAVY CHECK MARK */
   || (codepoint == 0x2728)                            /* SPARKLES */
   || (codepoint == 0x274C)                            /* CROSS MARK */
   || ((codepoint >= 0x2753) && (codepoint <= 0x2755)) /* BLACK QUESTION MARK ORNAMENT..WHITE EXCLAMATION MARK ORNAMENT */
   || (codepoint == 0x2757)                            /* HEAVY EXCLAMATION MARK SYMBOL */
   || ((codepoint >= 0x2795) && (codepoint <= 0x2797)) /* HEAVY PLUS SIGN..HEAVY DIVISION SIGN */
   || (codepoint == 0x27B0)                            /* CURLY LOOP */
   || (codepoint == 0x27BF)                            /* DOUBLE CURLY LOOP */
   || ((codepoint >= 0x2B1B) && (codepoint <= 0x2B1C)) /* BLACK LARGE SQUARE..WHITE LARGE SQUARE */
   || (codepoint == 0x2B50)                            /* WHITE MEDIUM STAR */
   || (codepoint == 0x2B55)                            /* HEAVY LARGE CIRCLE */
   || ((codepoint >= 0x2E80) && (codepoint <= 0x2FFB)) /* CJK RADICAL REPEAT..IDEOGRAPHIC DESCRIPTION CHARACTER OVERLAID */
   || ((codepoint >= 0x3004) && (codepoint <= 0x303E)) /* JAPANESE INDUSTRIAL STANDARD SYMBOL..IDEOGRAPHIC VARIATION INDICATOR */
   || ((codepoint >= 0x3041) && (codepoint <= 0x3247)) /* HIRAGANA LETTER SMALL A..CIRCLED NUMBER EIGHTY ON BLACK SQUARE */
   || ((codepoint >= 0x3250) && (codepoint <= 0x4DBF)) /* PARTNERSHIP SIGN..<reserved-4DBF> */
   || ((codepoint >= 0x4E00) && (codepoint <= 0xA4C6)) /* CJK UNIFIED IDEOGRAPH-4E00..YI RADICAL KE */
   || ((codepoint >= 0xA960) && (codepoint <= 0xA97C)) /* HANGUL CHOSEONG TIKEUT-MIEUM..HANGUL CHOSEONG SSANGYEORINHIEUH */
   || ((codepoint >= 0xAC00) && (codepoint <= 0xD7A3)) /* HANGUL SYLLABLE GA..HANGUL SYLLABLE HIH */
   || ((codepoint >= 0xF900) && (codepoint <= 0xFAFF)) /* CJK COMPATIBILITY IDEOGRAPH-F900..<reserved-FAFF> */
   || ((codepoint >= 0xFE10) && (codepoint <= 0xFE19)) /* PRESENTATION FORM FOR VERTICAL COMMA..PRESENTATION FORM FOR VERTICAL HORIZONTAL ELLIPSIS */
   || ((codepoint >= 0xFE31) && (codepoint <= 0xFE6B)) /* PRESENTATION FORM FOR VERTICAL EM DASH..SMALL COMMERCIAL AT */
  )
    return 2;
  else
    return 1;
}
