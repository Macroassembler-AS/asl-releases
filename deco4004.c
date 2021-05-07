/* deco4004.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/*                                                                           */
/* Dissector 4004                                                            */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>

#include "dasmdef.h"
#include "cpulist.h"
#include "codechunks.h"
#include "invaddress.h"
#include "strutil.h"

#include "deco4004.h"

typedef enum
{
  eUnknown,
  eImplicit,
  eFullAddr,
  eFIM,
  eOneReg,
  eOneRReg,
  eImm4,
  eJumpCond,
  eISZ
} tAddrType;

typedef struct
{
  tAddrType Type;
  Byte OpSize, NextAddresses;
  const char *Memo;
} tOpcodeList;

static unsigned nData;

static CPUVar CPU4004;

static const tOpcodeList OpcodeList[256] =
{
  /* 0x00 */ { eImplicit  , 0, 1, "nop"  },
  /* 0x01 */ { eImplicit  , 0, 1, "hlt"  },
  /* 0x02 */ { eImplicit  , 0, 0, "bbs"  },
  /* 0x03 */ { eImplicit  , 0, 0, "lcr"  },
  /* 0x04 */ { eImplicit  , 0, 0, "or4"  },
  /* 0x05 */ { eImplicit  , 0, 0, "or5"  },
  /* 0x06 */ { eImplicit  , 0, 1, "an6"  },
  /* 0x07 */ { eImplicit  , 0, 1, "an7"  },
  /* 0x08 */ { eImplicit  , 1, 1, "db0"  },
  /* 0x09 */ { eImplicit  , 1, 1, "db1"  },
  /* 0x0a */ { eImplicit  , 0, 0, "sb0"  },
  /* 0x0b */ { eImplicit  , 0, 0, "sb1"  },
  /* 0x0c */ { eImplicit  , 0, 1, "ein"  },
  /* 0x0d */ { eImplicit  , 0, 1, "din"  },
  /* 0x0e */ { eImplicit  , 0, 1, "rpm"  },
  /* 0x0f */ { eUnknown   , 0, 0, NULL   },
  /* 0x10 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x11 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x12 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x13 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x14 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x15 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x16 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x17 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x18 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x19 */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x1a */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x1b */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x1c */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x1d */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x1e */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x1f */ { eJumpCond  , 0, 3, "jcn"  },
  /* 0x20 */ { eFIM       , 0, 1, "fim"  },
  /* 0x21 */ { eOneRReg   , 0, 1, "src"  },
  /* 0x22 */ { eFIM       , 0, 1, "fim"  },
  /* 0x23 */ { eOneRReg   , 0, 1, "src"  },
  /* 0x24 */ { eFIM       , 0, 1, "fim"  },
  /* 0x25 */ { eOneRReg   , 0, 1, "src"  },
  /* 0x26 */ { eFIM       , 0, 1, "fim"  },
  /* 0x27 */ { eOneRReg   , 0, 1, "src"  },
  /* 0x28 */ { eFIM       , 0, 1, "fim"  },
  /* 0x29 */ { eOneRReg   , 0, 1, "src"  },
  /* 0x2a */ { eFIM       , 0, 1, "fim"  },
  /* 0x2b */ { eOneRReg   , 0, 1, "src"  },
  /* 0x2c */ { eFIM       , 0, 1, "fim"  },
  /* 0x2d */ { eOneRReg   , 0, 1, "src"  },
  /* 0x2e */ { eFIM       , 0, 1, "fim"  },
  /* 0x2f */ { eOneRReg   , 0, 1, "src"  },
  /* 0x30 */ { eOneRReg   , 1, 1, "fin"  },
  /* 0x31 */ { eOneRReg   , 1, 1, "jin"  },
  /* 0x32 */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x33 */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x34 */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x35 */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x36 */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x37 */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x38 */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x39 */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x3a */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x3b */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x3c */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x3d */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x3e */ { eOneRReg   , 0, 1, "fin"  },
  /* 0x3f */ { eOneRReg   , 0, 1, "jin"  },
  /* 0x40 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x41 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x42 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x43 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x44 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x45 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x46 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x47 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x48 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x49 */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x4a */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x4b */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x4c */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x4d */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x4e */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x4f */ { eFullAddr  , 0, 2, "jun"  },
  /* 0x50 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x51 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x52 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x53 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x54 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x55 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x56 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x57 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x58 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x59 */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x5a */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x5b */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x5c */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x5d */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x5e */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x5f */ { eFullAddr  , 0, 3, "jms"  },
  /* 0x60 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x61 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x62 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x63 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x64 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x65 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x66 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x67 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x68 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x69 */ { eOneReg    , 0, 1, "inc"  },
  /* 0x6a */ { eOneReg    , 0, 1, "inc"  },
  /* 0x6b */ { eOneReg    , 0, 1, "inc"  },
  /* 0x6c */ { eOneReg    , 0, 1, "inc"  },
  /* 0x6d */ { eOneReg    , 0, 1, "inc"  },
  /* 0x6e */ { eOneReg    , 0, 1, "inc"  },
  /* 0x6f */ { eOneReg    , 0, 1, "inc"  },
  /* 0x70 */ { eISZ       , 0, 3, "isz"  },
  /* 0x71 */ { eISZ       , 0, 3, "isz"  },
  /* 0x72 */ { eISZ       , 0, 3, "isz"  },
  /* 0x73 */ { eISZ       , 0, 3, "isz"  },
  /* 0x74 */ { eISZ       , 0, 3, "isz"  },
  /* 0x75 */ { eISZ       , 0, 3, "isz"  },
  /* 0x76 */ { eISZ       , 0, 3, "isz"  },
  /* 0x77 */ { eISZ       , 0, 3, "isz"  },
  /* 0x78 */ { eISZ       , 0, 3, "isz"  },
  /* 0x79 */ { eISZ       , 0, 3, "isz"  },
  /* 0x7a */ { eISZ       , 0, 3, "isz"  },
  /* 0x7b */ { eISZ       , 0, 3, "isz"  },
  /* 0x7c */ { eISZ       , 0, 3, "isz"  },
  /* 0x7d */ { eISZ       , 0, 3, "isz"  },
  /* 0x7e */ { eISZ       , 0, 3, "isz"  },
  /* 0x7f */ { eISZ       , 0, 3, "isz"  },
  /* 0x80 */ { eOneReg    , 0, 1, "add"  },
  /* 0x81 */ { eOneReg    , 0, 1, "add"  },
  /* 0x82 */ { eOneReg    , 0, 1, "add"  },
  /* 0x83 */ { eOneReg    , 0, 1, "add"  },
  /* 0x84 */ { eOneReg    , 0, 1, "add"  },
  /* 0x85 */ { eOneReg    , 0, 1, "add"  },
  /* 0x86 */ { eOneReg    , 0, 1, "add"  },
  /* 0x87 */ { eOneReg    , 0, 1, "add"  },
  /* 0x88 */ { eOneReg    , 0, 1, "add"  },
  /* 0x89 */ { eOneReg    , 0, 1, "add"  },
  /* 0x8a */ { eOneReg    , 0, 1, "add"  },
  /* 0x8b */ { eOneReg    , 0, 1, "add"  },
  /* 0x8c */ { eOneReg    , 0, 1, "add"  },
  /* 0x8d */ { eOneReg    , 0, 1, "add"  },
  /* 0x8e */ { eOneReg    , 0, 1, "add"  },
  /* 0x8f */ { eOneReg    , 0, 1, "add"  },
  /* 0x90 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x91 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x92 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x93 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x94 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x95 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x96 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x97 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x98 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x99 */ { eOneReg    , 0, 1, "sub"  },
  /* 0x9a */ { eOneReg    , 0, 1, "sub"  },
  /* 0x9b */ { eOneReg    , 0, 1, "sub"  },
  /* 0x9c */ { eOneReg    , 0, 1, "sub"  },
  /* 0x9d */ { eOneReg    , 0, 1, "sub"  },
  /* 0x9e */ { eOneReg    , 0, 1, "sub"  },
  /* 0x9f */ { eOneReg    , 0, 1, "sub"  },
  /* 0xa0 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa1 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa2 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa3 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa4 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa5 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa6 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa7 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa8 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xa9 */ { eOneReg    , 0, 1, "ld"   },
  /* 0xaa */ { eOneReg    , 0, 1, "ld"   },
  /* 0xab */ { eOneReg    , 0, 1, "ld"   },
  /* 0xac */ { eOneReg    , 0, 1, "ld"   },
  /* 0xad */ { eOneReg    , 0, 1, "ld"   },
  /* 0xae */ { eOneReg    , 0, 1, "ld"   },
  /* 0xaf */ { eOneReg    , 0, 1, "ld"   },
  /* 0xb0 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb1 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb2 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb3 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb4 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb5 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb6 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb7 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb8 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xb9 */ { eOneReg    , 0, 1, "xch"  },
  /* 0xba */ { eOneReg    , 0, 1, "xch"  },
  /* 0xbb */ { eOneReg    , 0, 1, "xch"  },
  /* 0xbc */ { eOneReg    , 0, 1, "xch"  },
  /* 0xbd */ { eOneReg    , 0, 1, "xch"  },
  /* 0xbe */ { eOneReg    , 0, 1, "xch"  },
  /* 0xbf */ { eOneReg    , 0, 1, "xch"  },
  /* 0xc0 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc1 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc2 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc3 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc4 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc5 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc6 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc7 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc8 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xc9 */ { eImm4      , 0, 0, "bbl"  },
  /* 0xca */ { eImm4      , 0, 0, "bbl"  },
  /* 0xcb */ { eImm4      , 0, 0, "bbl"  },
  /* 0xcc */ { eImm4      , 0, 0, "bbl"  },
  /* 0xcd */ { eImm4      , 0, 0, "bbl"  },
  /* 0xce */ { eImm4      , 1, 0, "bbl"  },
  /* 0xcf */ { eImm4      , 0, 0, "bbl"  },
  /* 0xd0 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd1 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd2 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd3 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd4 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd5 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd6 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd7 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd8 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xd9 */ { eImm4      , 0, 1, "ldm"  },
  /* 0xda */ { eImm4      , 0, 1, "ldm"  },
  /* 0xdb */ { eImm4      , 0, 1, "ldm"  },
  /* 0xdc */ { eImm4      , 0, 1, "ldm"  },
  /* 0xdd */ { eImm4      , 0, 1, "ldm"  },
  /* 0xde */ { eImm4      , 0, 1, "ldm"  },
  /* 0xdf */ { eImm4      , 0, 1, "ldm"  },
  /* 0xe0 */ { eImplicit  , 0, 1, "wrm"  },
  /* 0xe1 */ { eImplicit  , 0, 1, "wmp"  },
  /* 0xe2 */ { eImplicit  , 0, 1, "wrr"  },
  /* 0xe3 */ { eImplicit  , 0, 1, "wpm"  },
  /* 0xe4 */ { eImplicit  , 0, 1, "wr0"  },
  /* 0xe5 */ { eImplicit  , 0, 1, "wr1"  },
  /* 0xe6 */ { eImplicit  , 0, 1, "wr2"  },
  /* 0xe7 */ { eImplicit  , 0, 1, "wr3"  },
  /* 0xe8 */ { eImplicit  , 0, 1, "sbm"  },
  /* 0xe9 */ { eImplicit  , 0, 1, "rdm"  },
  /* 0xea */ { eImplicit  , 0, 1, "rdr"  },
  /* 0xeb */ { eImplicit  , 0, 1, "adm"  },
  /* 0xec */ { eImplicit  , 0, 1, "rd0"  },
  /* 0xed */ { eImplicit  , 0, 1, "rd1"  },
  /* 0xee */ { eImplicit  , 1, 1, "rd2"  },
  /* 0xef */ { eImplicit  , 1, 1, "rd3"  },
  /* 0xf0 */ { eImplicit  , 0, 1, "clb"  },
  /* 0xf1 */ { eImplicit  , 0, 1, "clc"  },
  /* 0xf2 */ { eImplicit  , 0, 1, "iac"  },
  /* 0xf3 */ { eImplicit  , 0, 1, "cmc"  },
  /* 0xf4 */ { eImplicit  , 0, 1, "cma"  },
  /* 0xf5 */ { eImplicit  , 0, 1, "ral"  },
  /* 0xf6 */ { eImplicit  , 0, 1, "rar"  },
  /* 0xf7 */ { eImplicit  , 0, 1, "tcc"  },
  /* 0xf8 */ { eImplicit  , 0, 1, "dac"  },
  /* 0xf9 */ { eImplicit  , 0, 1, "tcs"  },
  /* 0xfa */ { eImplicit  , 0, 1, "stc"  },
  /* 0xfb */ { eImplicit  , 0, 1, "daa"  },
  /* 0xfc */ { eImplicit  , 0, 1, "kbp"  },
  /* 0xfd */ { eImplicit  , 0, 1, "dcl"  },
  /* 0xfe */ { eUnknown   , 1, 1, NULL   },
  /* 0xff */ { eUnknown   , 1, 1, NULL   },
};

static const tOpcodeList DummyOpcode = { eUnknown   , 0, 0, NULL   };

static void IntelHexString(char *pBuf, size_t BufSize, Word Num, int Digits)
{
  HexString(pBuf, BufSize, Num, Digits);
  if (!as_isdigit(*pBuf))
    strmaxprep(pBuf, "0", BufSize);
  strmaxcat(pBuf, "h", BufSize);
}

static Boolean RetrieveData(LargeWord Address, Byte *pBuffer, unsigned Count)
{
  LargeWord Trans;

  while (Count > 0)
  {
    Trans = 0x10000 - Address;
    if (Count < Trans)
      Trans = Count;
    if (!RetrieveCodeFromChunkList(&CodeChunks, Address, pBuffer, Trans))
    {
      char NumString[50];

      HexString(NumString, sizeof(NumString), Address, 1);
      fprintf(stderr, "cannot retrieve instruction arg @ 0x%s\n", NumString);
      return FALSE;
    }
    pBuffer += Trans;
    Count -= Trans;
    Address = (Address + Trans) & 0xffff;
    nData += Trans;
  }
  return TRUE;
}

static const char *MakeSymbolic(Word Address, int AddrLen, const char *pSymbolPrefix, char *pBuffer, int BufferSize)
{
  const char *pResult;

  if ((pResult = LookupInvSymbol(Address)))
    return pResult;

  HexString(pBuffer, BufferSize, Address, AddrLen << 1);
  if (!pSymbolPrefix)
  {
    if (!isdigit(*pBuffer))
    {
      strmaxprep(pBuffer, "0", BufferSize);
      strmaxcat(pBuffer, "h", BufferSize);
    }
    return pBuffer;
  }

  strmaxprep(pBuffer, pSymbolPrefix, BufferSize);
  AddInvSymbol(pBuffer, Address);
  return pBuffer;
}

static void Disassemble_4004(LargeWord Address, tDisassInfo *pInfo, Boolean AsData, int DataSize)
{
  Byte Opcode, Data[10];
  const tOpcodeList *pOpcode;
  char NumBuf[60], NumBuf2[60];
  const char *pOp;
  Word OpAddr = 0;

  pInfo->CodeLen = 0;
  pInfo->NextAddressCount = 0;
  pInfo->SrcLine[0] = '\0';
  pInfo->pRemark = NULL;

  if (!RetrieveData(Address, &Opcode, 1))
    return;
  nData = 1;
  (void)DataSize;

  pOpcode = AsData ? &DummyOpcode : OpcodeList + Opcode;
  switch (pOpcode->Type)
  {
    case eImplicit:
      pInfo->CodeLen = 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s", pOpcode->Memo);
      break;
    case eFullAddr:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      OpAddr = (((Word)Opcode & 0x0f) << 8) | Data[0];
      pOp = MakeSymbolic(OpAddr, 2, strcmp(pOpcode->Memo, "jun") ? "lab_" : "sub_", NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s", pOpcode->Memo, pOp);
      break;
    case eJumpCond:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      OpAddr = ((Address + 2) & 0x0f00) | Data[0];
      pOp = MakeSymbolic(OpAddr, 2, "lab_", NumBuf, sizeof(NumBuf));
      NumBuf2[0] = '\0';
      if (Opcode & 0x01)
        strmaxcat(NumBuf2, "t", sizeof(NumBuf2));
      if (Opcode & 0x02)
        strmaxcat(NumBuf2, "c", sizeof(NumBuf2));
      if (Opcode & 0x04)
        strmaxcat(NumBuf2, "z", sizeof(NumBuf2));
      if (Opcode & 0x08)
        strmaxcat(NumBuf2, "n", sizeof(NumBuf2));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s,%s",
                  pOpcode->Memo, NumBuf2, pOp);
      break;
    case eISZ:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      OpAddr = ((Address + 2) & 0x0f00) | Data[0];
      pOp = MakeSymbolic(OpAddr, 2, "lab_", NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\tr%u,%s",
                  pOpcode->Memo, Opcode & 15, pOp);
      break;
    case eFIM:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      IntelHexString(NumBuf, sizeof(NumBuf), Data[0], 2);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\tr%up,%s",
                  pOpcode->Memo, (Opcode & 15) >> 1, NumBuf);
      break;
    case eOneReg:
      pInfo->CodeLen = 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\tr%u",
                  pOpcode->Memo, Opcode & 15);
      break;
    case eOneRReg:
      pInfo->CodeLen = 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\tr%up",
                  pOpcode->Memo, (Opcode & 15) >> 1);
      break;
    case eImm4:
      pInfo->CodeLen = 1;
      IntelHexString(NumBuf, sizeof(NumBuf), Opcode & 0x0f, 1);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s",
                  pOpcode->Memo, NumBuf);
      break;
    default:
      if (DataSize < 0)
        DataSize = 1;
      if (!RetrieveData(Address + 1, Data, DataSize - 1))
        return;
      HexString(NumBuf, sizeof(NumBuf), Opcode, 2);
      HexString(NumBuf2, sizeof(NumBuf2), Address, 0);
      if (!AsData)
        fprintf(stderr, "unknown opcode 0x%s @ 0x%s\n", NumBuf, NumBuf2);
      switch (DataSize)
      {
        case 2:
          OpAddr = (((Word)Opcode) << 8) | Data[0];
          pOp = MakeSymbolic(OpAddr, 2, NULL, NumBuf, sizeof(NumBuf));
          as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dw\t%s", pOp);
          pInfo->CodeLen = 2;
          break;
        default:
          pInfo->CodeLen = 1;
          IntelHexString(NumBuf, sizeof(NumBuf), Opcode, 2);
          as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "db\t%s", NumBuf);
      }
  }

  if (pOpcode->NextAddresses & 2)
    pInfo->NextAddresses[pInfo->NextAddressCount++] = OpAddr;
  if (pOpcode->NextAddresses & 1)
    pInfo->NextAddresses[pInfo->NextAddressCount++] = (Address + pInfo->CodeLen) % 0xfff;

  if (nData != pInfo->CodeLen)
    as_snprcatf(pInfo->SrcLine, sizeof(pInfo->SrcLine), " ; ouch %u != %u", nData, pInfo->CodeLen);
}

static void SwitchTo_4004(void)
{
  Disassemble = Disassemble_4004;
}

void deco4004_init(void)
{
  CPU4004 = AddCPU("4004", SwitchTo_4004);
}
