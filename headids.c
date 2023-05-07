/* headids.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Makroassembler AS                                                         */
/*                                                                           */
/* Hier sind alle Prozessor-IDs mit ihren Eigenschaften gesammelt            */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "headids.h"

/*---------------------------------------------------------------------------*/

static const TFamilyDescr Descrs[] =
{
  { "680x0"        , 0x0001, eHexFormatMotoS   },
  { "DSP56000"     , 0x0009, eHexFormatMotoS   },
  { "MPC601"       , 0x0005, eHexFormatMotoS   },
  { "PALM"         , 0x000e, eHexFormatIntel   },
  { "M-CORE"       , 0x0003, eHexFormatMotoS   },
  { "XGATE"        , 0x0004, eHexFormatMotoS   },
  { "68xx"         , 0x0061, eHexFormatMotoS   },
  { "6805/HC08"    , 0x0062, eHexFormatMotoS   },
  { "6809"         , 0x0063, eHexFormatMotoS   },
  { "68HC12"       , 0x0066, eHexFormatMotoS   },
  { "S12Z"         , 0x0045, eHexFormatMotoS   },
  { "68HC16"       , 0x0065, eHexFormatMotoS   },
  { "68RS08"       , 0x005e, eHexFormatMotoS   },
  { "052001"       , 0x0022, eHexFormatMotoS   },
  { "H8/300(H}"    , 0x0068, eHexFormatMotoS   },
  { "H8/500"       , 0x0069, eHexFormatMotoS   },
  { "H16"          , 0x0040, eHexFormatMotoS   },
  { "SH7x00"       , 0x006c, eHexFormatMotoS   },
  { "HMCS400"      , 0x0050, eHexFormatMotoS   },
  { "PPS-4"        , 0x0010, eHexFormatIntel   },
  { "65xx"         , 0x0011, eHexFormatMOS     },
  { "MELPS-7700"   , 0x0019, eHexFormatMOS     },
  { "MELPS-4500"   , 0x0012, eHexFormatIntel   },
  { "M16"          , 0x0013, eHexFormatIntel32 },
  { "M16C"         , 0x0014, eHexFormatIntel   },
  { "MCS-48"       , 0x0021, eHexFormatIntel   },
  { "MCS-(2)51"    , 0x0031, eHexFormatIntel   },
  { "MCS-96/196"   , 0x0039, eHexFormatIntel   },
  { "PDP-11"       , 0x0023, eHexFormatMotoS   },
  { "WD16"         , 0x0024, eHexFormatMotoS   },
  { "VAX"          , 0x0026, eHexFormatMotoS   },
  { "4004/4040"    , 0x003f, eHexFormatIntel   },
  { "8008"         , 0x003e, eHexFormatIntel   },
  { "8080/8085"    , 0x0041, eHexFormatIntel   },
  { "8086"         , 0x0042, eHexFormatIntel16 },
  { "i960"         , 0x002a, eHexFormatIntel32 },
  { "8X30x"        , 0x003a, eHexFormatIntel   },
  { "2650"         , 0x0037, eHexFormatMotoS   },
  { "XA"           , 0x003c, eHexFormatIntel16 },
  { "AVR"          , 0x003b, eHexFormatAtmel   },
  { "AVR(CSEG8)"   , 0x003d, eHexFormatAtmel   },
  { "29xxx"        , 0x0029, eHexFormatIntel32 },
  { "80C166/167"   , 0x004c, eHexFormatIntel16 },
  { "Zx80"         , 0x0051, eHexFormatIntel   },
  { "Z8"           , 0x0079, eHexFormatIntel   },
  { "Super8"       , 0x0035, eHexFormatIntel   },
  { "eZ8"          , 0x0059, eHexFormatIntel   },
  { "Z8000"        , 0x0034, eHexFormatIntel   },
  { "KCPSM"        , 0x006b, eHexFormatIntel   },
  { "KCPSM3"       , 0x005b, eHexFormatIntel   },
  { "Mico8"        , 0x005c, eHexFormatIntel   },
  { "TLCS-900"     , 0x0052, eHexFormatMotoS   },
  { "TLCS-90"      , 0x0053, eHexFormatIntel   },
  { "TLCS-870"     , 0x0054, eHexFormatIntel   },
  { "TLCS-870/C"   , 0x0057, eHexFormatIntel   },
  { "TLCS-47xx"    , 0x0055, eHexFormatIntel   },
  { "TLCS-9000 "   , 0x0056, eHexFormatMotoS   },
  { "TC9331"       , 0x005a, eHexFormatIntel   },
  { "16C8x"        , 0x0070, eHexFormatIntel   },
  { "16C5x"        , 0x0071, eHexFormatIntel   },
  { "17C4x"        , 0x0072, eHexFormatIntel   },
  { "ST6"          , 0x0078, eHexFormatIntel   },
  { "ST7"          , 0x0033, eHexFormatIntel   },
  { "ST9"          , 0x0032, eHexFormatIntel   },
  { "6804"         , 0x0064, eHexFormatMotoS   },
  { "TMS3201x"     , 0x0074, eHexFormatTiDSK   },
  { "TMS3202x"     , 0x0075, eHexFormatTiDSK   },
  { "TMS320C3x/C4x", 0x0076, eHexFormatIntel32 },
  { "TMS320C5x"    , 0x0077, eHexFormatTiDSK   },
  { "TMS320C54x"   , 0x004b, eHexFormatTiDSK   },
  { "TMS320C6x"    , 0x0047, eHexFormatIntel32 },
  { "TMS9900"      , 0x0048, eHexFormatIntel   },
  { "TMS7000"      , 0x0073, eHexFormatIntel   },
  { "TMS370xx"     , 0x0049, eHexFormatIntel   },
  { "MSP430"       , 0x004a, eHexFormatIntel   },
  { "TMS1000"      , 0x0007, eHexFormatIntel   },
  { "IMP-16"       , 0x0017, eHexFormatIntel   },
  { "IPC-16"       , 0x0018, eHexFormatIntel   },
  { "SC/MP"        , 0x006e, eHexFormatIntel   },
  { "807x"         , 0x006a, eHexFormatIntel   },
  { "COP4"         , 0x005f, eHexFormatIntel   },
  { "COP8"         , 0x006f, eHexFormatIntel   },
  { "SC14XXX"      , 0x006d, eHexFormatIntel   },
  { "NS32000"      , 0x0008, eHexFormatIntel   },
  { "ACE"          , 0x0067, eHexFormatIntel   },
  { "CP-3F"        , 0x000f, eHexFormatIntel   },
  { "F8"           , 0x0044, eHexFormatIntel   },
  { "75xx"         , 0x005d, eHexFormatIntel   },
  { "78(C)xx"      , 0x007a, eHexFormatIntel   },
  { "75K0"         , 0x007b, eHexFormatIntel   },
  { "78K0"         , 0x007c, eHexFormatIntel   },
  { "78K2"         , 0x0060, eHexFormatIntel16 },
  { "78K3"         , 0x0058, eHexFormatIntel   },
  { "78K4"         , 0x0046, eHexFormatIntel16 },
  { "7720"         , 0x007d, eHexFormatIntel   },
  { "7725"         , 0x007e, eHexFormatIntel   },
  { "77230"        , 0x007f, eHexFormatIntel   },
  { "V60"          , 0x000d, eHexFormatIntel32 },
  { "SYM53C8xx"    , 0x0025, eHexFormatIntel   },
  { "F2MC8"        , 0x0015, eHexFormatIntel   },
  { "F2MC16"       , 0x0016, eHexFormatIntel   },
  { "MN161x"       , 0x0036, eHexFormatIntel   },
  { "OLMS-40"      , 0x004e, eHexFormatIntel   },
  { "OLMS-50"      , 0x004d, eHexFormatIntel   },
  { "1802"         , 0x0038, eHexFormatIntel   },
  { "SX20"         , 0x0043, eHexFormatIntel   },
  { "KENBAK"       , 0x0027, eHexFormatIntel   },
  { "ATARI_VECTOR" , 0x0002, eHexFormatIntel   },
  { "XCore"        , 0x0006, eHexFormatMotoS   },
  { "PDK13"        , 0x001a, eHexFormatIntel   },
  { "PDK14"        , 0x001b, eHexFormatIntel   },
  { "PDK15"        , 0x001c, eHexFormatIntel   },
  { "PDK16"        , 0x001d, eHexFormatIntel   },
  { "1750"         , 0x004f, eHexFormatIntel   },
  { "CP1600"       , 0x000a, eHexFormatIntel   },
  { "NANO"         , 0x000b, eHexFormatIntel   },
  { "IM6100"       , 0x000c, eHexFormatIntel   },
  { "RX"           , 0x001e, eHexFormatIntel32 },
  { NULL           , 0xffff, eHexFormatDefault }
};

/*---------------------------------------------------------------------------*/

const TFamilyDescr *FindFamilyByName(const char *Name)
{
  const TFamilyDescr *pRun;

  for (pRun = Descrs; pRun->Name != NULL; pRun++)
    if (!strcmp(Name, pRun->Name))
      return pRun;

  return NULL;
}

const TFamilyDescr *FindFamilyById(Word Id)
{
  const TFamilyDescr *pRun;

  for (pRun = Descrs; pRun->Name != NULL; pRun++)
    if (Id == pRun->Id)
      return pRun;

  return NULL;
}

/*---------------------------------------------------------------------------*/

void headids_init(void)
{
}
