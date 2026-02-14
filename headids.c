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

static Word get_granularity_default(as_addrspace_t addr_space)
{
  UNUSED(addr_space);
  return 1;
}

static Word get_granularity_16(as_addrspace_t addr_space)
{
  UNUSED(addr_space);
  return 2;
}

static Word get_granularity_32(as_addrspace_t addr_space)
{
  UNUSED(addr_space);
  return 4;
}

static Word get_granularity_code_16(as_addrspace_t addr_space)
{
  return (addr_space == SegCode) ? 2 : 1;
}

static Word get_granularity_code_32(as_addrspace_t addr_space)
{
  return (addr_space == SegCode) ? 4 : 1;
}

static Word get_granularity_772x(as_addrspace_t addr_space)
{
   return (addr_space == SegCode) ? 4 : 2;
}

static Word get_granularity_code_1(as_addrspace_t addr_space)
{
  UNUSED(addr_space);
  /* TODO 7 bits unused */
  return 1;
}

static const TFamilyDescr Descrs[] =
{
  { "680x0"        , 0x0001, eHexFormatMotoS   , get_granularity_default },
  { "DSP56000"     , 0x0009, eHexFormatMotoS   , get_granularity_32      },
  { "MPC601"       , 0x0005, eHexFormatMotoS   , get_granularity_default },
  { "PALM"         , 0x000e, eHexFormatIntel   , get_granularity_default },
  { "M-CORE"       , 0x0003, eHexFormatMotoS   , get_granularity_default },
  { "XGATE"        , 0x0004, eHexFormatMotoS   , get_granularity_default },
  { "68xx"         , 0x0061, eHexFormatMotoS   , get_granularity_default },
  { "6805/HC08"    , 0x0062, eHexFormatMotoS   , get_granularity_default },
  { "6809"         , 0x0063, eHexFormatMotoS   , get_granularity_default },
  { "68HC12"       , 0x0066, eHexFormatMotoS   , get_granularity_default },
  { "S12Z"         , 0x0045, eHexFormatMotoS   , get_granularity_default },
  { "68HC16"       , 0x0065, eHexFormatMotoS   , get_granularity_default },
  { "68RS08"       , 0x005e, eHexFormatMotoS   , get_granularity_default },
  { "052001"       , 0x0022, eHexFormatMotoS   , get_granularity_default },
  { "H8/300(H)"    , 0x0068, eHexFormatMotoS   , get_granularity_default },
  { "H8/500"       , 0x0069, eHexFormatMotoS   , get_granularity_default },
  { "H16"          , 0x0040, eHexFormatMotoS   , get_granularity_default },
  { "SH7x00"       , 0x006c, eHexFormatMotoS   , get_granularity_default },
  { "HMCS400"      , 0x0050, eHexFormatMotoS   , get_granularity_code_16 },
  { "PPS-4"        , 0x0010, eHexFormatIntel   , get_granularity_default },
  { "65xx"         , 0x0011, eHexFormatMOS     , get_granularity_default },
  { "MELPS-7700"   , 0x0019, eHexFormatMOS     , get_granularity_default },
  { "MELPS-4500"   , 0x0012, eHexFormatIntel   , get_granularity_code_16 },
  { "M16"          , 0x0013, eHexFormatIntel32 , get_granularity_default },
  { "M16C"         , 0x0014, eHexFormatIntel   , get_granularity_default },
  { "MCS-48"       , 0x0021, eHexFormatIntel   , get_granularity_default },
  { "MCS-(2)51"    , 0x0031, eHexFormatIntel   , get_granularity_default },
  { "MCS-96/196"   , 0x0039, eHexFormatIntel   , get_granularity_default },
  { "PDP-11"       , 0x0023, eHexFormatMotoS   , get_granularity_default },
  { "WD16"         , 0x0024, eHexFormatMotoS   , get_granularity_default },
  { "VAX"          , 0x0026, eHexFormatMotoS   , get_granularity_default },
  { "4004/4040"    , 0x003f, eHexFormatIntel   , get_granularity_default },
  { "8008"         , 0x003e, eHexFormatIntel   , get_granularity_default },
  { "8080/8085"    , 0x0041, eHexFormatIntel   , get_granularity_default },
  { "8086"         , 0x0042, eHexFormatIntel16 , get_granularity_default },
  { "i960"         , 0x002a, eHexFormatIntel32 , get_granularity_default },
  { "8X30x"        , 0x003a, eHexFormatIntel   , get_granularity_16      },
  { "2650"         , 0x0037, eHexFormatMotoS   , get_granularity_default },
  { "XA"           , 0x003c, eHexFormatIntel16 , get_granularity_default },
  { "AVR"          , 0x003b, eHexFormatAtmel   , get_granularity_code_16 },
  { "AVR(CSEG8)"   , 0x003d, eHexFormatAtmel   , get_granularity_default },
  { "29xxx"        , 0x0029, eHexFormatIntel32 , get_granularity_default },
  { "80C166/167"   , 0x004c, eHexFormatIntel16 , get_granularity_default },
  { "Zx80"         , 0x0051, eHexFormatIntel   , get_granularity_default },
  { "Z8"           , 0x0079, eHexFormatIntel   , get_granularity_default },
  { "Super8"       , 0x0035, eHexFormatIntel   , get_granularity_default },
  { "eZ8"          , 0x0059, eHexFormatIntel   , get_granularity_default },
  { "Z8000"        , 0x0034, eHexFormatIntel   , get_granularity_default },
  { "KCPSM"        , 0x006b, eHexFormatIntel   , get_granularity_code_16 },
  { "KCPSM3"       , 0x005b, eHexFormatIntel   , get_granularity_code_32 },
  { "Mico8"        , 0x005c, eHexFormatIntel   , get_granularity_code_32 },
  { "TLCS-900"     , 0x0052, eHexFormatMotoS   , get_granularity_default },
  { "TLCS-90"      , 0x0053, eHexFormatIntel   , get_granularity_default },
  { "TLCS-870"     , 0x0054, eHexFormatIntel   , get_granularity_default },
  { "TLCS-870/C"   , 0x0057, eHexFormatIntel   , get_granularity_default },
  { "TLCS-47xx"    , 0x0055, eHexFormatIntel   , get_granularity_default },
  { "TLCS-42xx"    , 0x002b, eHexFormatIntel   , get_granularity_default },
  { "TLCS-9000"    , 0x0056, eHexFormatMotoS   , get_granularity_default },
  { "TC9331"       , 0x005a, eHexFormatIntel   , get_granularity_32      },
  { "16C8x"        , 0x0070, eHexFormatIntel   , get_granularity_code_16 },
  { "16C5x"        , 0x0071, eHexFormatIntel   , get_granularity_code_16 },
  { "17C4x"        , 0x0072, eHexFormatIntel   , get_granularity_code_16 },
  { "ST6"          , 0x0078, eHexFormatIntel   , get_granularity_default },
  { "ST7"          , 0x0033, eHexFormatIntel   , get_granularity_default },
  { "ST9"          , 0x0032, eHexFormatIntel   , get_granularity_default },
  { "6804"         , 0x0064, eHexFormatMotoS   , get_granularity_default },
  { "TMS3201x"     , 0x0074, eHexFormatTiDSK   , get_granularity_16      },
  { "TMS3202x"     , 0x0075, eHexFormatTiDSK   , get_granularity_16      },
  { "TMS320C3x/C4x", 0x0076, eHexFormatIntel32 , get_granularity_32      },
  { "TMS320C5x"    , 0x0077, eHexFormatTiDSK   , get_granularity_16      },
  { "TMS320C54x"   , 0x004b, eHexFormatTiDSK   , get_granularity_16      },
  { "TMS320C6x"    , 0x0047, eHexFormatIntel32 , get_granularity_default },
  { "TMS340xx"     , 0x002d, eHexFormatMotoS   , get_granularity_code_1  },
  { "TMS9900"      , 0x0048, eHexFormatIntel   , get_granularity_default },
  { "TMS7000"      , 0x0073, eHexFormatIntel   , get_granularity_default },
  { "TMS370xx"     , 0x0049, eHexFormatIntel   , get_granularity_default },
  { "MSP430"       , 0x004a, eHexFormatIntel   , get_granularity_default },
  { "TMS1000"      , 0x0007, eHexFormatIntel   , get_granularity_default },
  { "IMP-16"       , 0x0017, eHexFormatIntel   , get_granularity_16      },
  { "IPC-16"       , 0x0018, eHexFormatIntel   , get_granularity_16      },
  { "SC/MP"        , 0x006e, eHexFormatIntel   , get_granularity_default },
  { "807x"         , 0x006a, eHexFormatIntel   , get_granularity_default },
  { "COP4"         , 0x005f, eHexFormatIntel   , get_granularity_default },
  { "COP8"         , 0x006f, eHexFormatIntel   , get_granularity_default },
  { "SC14XXX"      , 0x006d, eHexFormatIntel   , get_granularity_16      },
  { "NS32000"      , 0x0008, eHexFormatIntel   , get_granularity_default },
  { "WE32xxx"      , 0x002c, eHexFormatIntel   , get_granularity_default },
  { "ACE"          , 0x0067, eHexFormatIntel   , get_granularity_default },
  { "CP-3F"        , 0x000f, eHexFormatIntel   , get_granularity_default },
  { "F8"           , 0x0044, eHexFormatIntel   , get_granularity_default },
  { "75xx"         , 0x005d, eHexFormatIntel   , get_granularity_default },
  { "78(C)xx"      , 0x007a, eHexFormatIntel   , get_granularity_default },
  { "75K0"         , 0x007b, eHexFormatIntel   , get_granularity_default },
  { "78K0"         , 0x007c, eHexFormatIntel   , get_granularity_default },
  { "78K2"         , 0x0060, eHexFormatIntel16 , get_granularity_default },
  { "78K3"         , 0x0058, eHexFormatIntel   , get_granularity_default },
  { "78K4"         , 0x0046, eHexFormatIntel16 , get_granularity_default },
  { "7720"         , 0x007d, eHexFormatIntel   , get_granularity_772x    },
  { "7725"         , 0x007e, eHexFormatIntel   , get_granularity_772x    },
  { "77230"        , 0x007f, eHexFormatIntel   , get_granularity_32      },
  { "V60"          , 0x000d, eHexFormatIntel32 , get_granularity_default },
  { "uCOM-43"      , 0x0028, eHexFormatIntel   , get_granularity_default },
  { "SYM53C8xx"    , 0x0025, eHexFormatIntel   , get_granularity_default },
  { "F2MC8"        , 0x0015, eHexFormatIntel   , get_granularity_default },
  { "F2MC16"       , 0x0016, eHexFormatIntel   , get_granularity_default },
  { "MN161x"       , 0x0036, eHexFormatIntel   , get_granularity_16      },
  { "OLMS-40"      , 0x004e, eHexFormatIntel   , get_granularity_default },
  { "OLMS-50"      , 0x004d, eHexFormatIntel   , get_granularity_code_16 },
  { "1802"         , 0x0038, eHexFormatIntel   , get_granularity_default },
  { "SX20"         , 0x0043, eHexFormatIntel   , get_granularity_code_16 },
  { "KENBAK"       , 0x0027, eHexFormatIntel   , get_granularity_default },
  { "ATARI_VECTOR" , 0x0002, eHexFormatIntel   , get_granularity_16      },
  { "XCore"        , 0x0006, eHexFormatMotoS   , get_granularity_default },
  { "PDK13"        , 0x001a, eHexFormatIntel   , get_granularity_code_16 },
  { "PDK14"        , 0x001b, eHexFormatIntel   , get_granularity_code_16 },
  { "PDK15"        , 0x001c, eHexFormatIntel   , get_granularity_code_16 },
  { "PDK16"        , 0x001d, eHexFormatIntel   , get_granularity_code_16 },
  { "1750"         , 0x004f, eHexFormatIntel   , get_granularity_16      },
  { "CP1600"       , 0x000a, eHexFormatIntel   , get_granularity_16      },
  { "NANO"         , 0x000b, eHexFormatIntel   , get_granularity_default },
  { "IM6100"       , 0x000c, eHexFormatIntel   , get_granularity_16      },
  { "RX"           , 0x001e, eHexFormatIntel32 , get_granularity_default },
  { "SC61860"      , 0x001f, eHexFormatMotoS   , get_granularity_default },
  { "SC62015"      , 0x0020, eHexFormatMotoS   , get_granularity_default },
  { NULL           , 0xffff, eHexFormatDefault , NULL                    }
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
