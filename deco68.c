/* deco6800.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/*                                                                           */
/* Dissector 6800/02                                                         */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>

#include "dasmdef.h"
#include "cpulist.h"
#include "codechunks.h"
#include "invaddress.h"
#include "strutil.h"

#include "deco68.h"

static unsigned nData;

static CPUVar CPU6800, CPU6802;

typedef enum
{
  eUnknown,
  eImplicit,
  eDirect,
  eIndexed,
  eExtended,
  eImmediate,
  eRelative
} tAddrType;

typedef struct
{
  tAddrType Type;
  Byte OpSize, NextAddresses;
  const char *Memo;
} tOpcodeList;

static const tOpcodeList OpcodeList[256] =
{
  /* 0x00 */ { eUnknown   , 0, 0, NULL   },
  /* 0x01 */ { eImplicit  , 0, 1, "nop"  },
  /* 0x02 */ { eUnknown   , 0, 0, NULL   },
  /* 0x03 */ { eUnknown   , 0, 0, NULL   },
  /* 0x04 */ { eUnknown   , 0, 0, NULL   },
  /* 0x05 */ { eUnknown   , 0, 0, NULL   },
  /* 0x06 */ { eImplicit  , 0, 1, "tap"  },
  /* 0x07 */ { eImplicit  , 0, 1, "tpa"  },
  /* 0x08 */ { eImplicit  , 1, 1, "inx"  },
  /* 0x09 */ { eImplicit  , 1, 1, "dex"  },
  /* 0x0a */ { eImplicit  , 0, 0, "clv"  },
  /* 0x0b */ { eImplicit  , 0, 0, "sev"  },
  /* 0x0c */ { eImplicit  , 0, 1, "clc"  },
  /* 0x0d */ { eImplicit  , 0, 1, "sec"  },
  /* 0x0e */ { eImplicit  , 0, 1, "cli"  },
  /* 0x0f */ { eImplicit  , 0, 1, "sei"  },
  /* 0x10 */ { eImplicit  , 0, 1, "sba"  },
  /* 0x11 */ { eImplicit  , 0, 1, "cba"  },
  /* 0x12 */ { eUnknown   , 0, 0, NULL   },
  /* 0x13 */ { eUnknown   , 0, 0, NULL   },
  /* 0x14 */ { eImplicit  , 0, 1, "nba"  },
  /* 0x15 */ { eUnknown   , 0, 0, NULL   },
  /* 0x16 */ { eImplicit  , 0, 1, "tab"  },
  /* 0x17 */ { eImplicit  , 0, 1, "tba"  },
  /* 0x18 */ { eUnknown   , 0, 0, NULL   },
  /* 0x19 */ { eImplicit  , 0, 1, "daa"  },
  /* 0x1a */ { eUnknown   , 0, 0, NULL   },
  /* 0x1b */ { eImplicit  , 0, 1, "aba"  },
  /* 0x1c */ { eUnknown   , 0, 0, NULL   },
  /* 0x1d */ { eUnknown   , 0, 0, NULL   },
  /* 0x1e */ { eUnknown   , 0, 0, NULL   },
  /* 0x1f */ { eUnknown   , 0, 0, NULL   },
  /* 0x20 */ { eRelative  , 0, 2, "bra"  },
  /* 0x21 */ { eUnknown   , 0, 0, NULL   },
  /* 0x22 */ { eRelative  , 0, 3, "bhi"  },
  /* 0x23 */ { eRelative  , 0, 3, "bls"  },
  /* 0x24 */ { eRelative  , 0, 3, "bcc"  },
  /* 0x25 */ { eRelative  , 0, 3, "bcs"  },
  /* 0x26 */ { eRelative  , 0, 3, "bne"  },
  /* 0x27 */ { eRelative  , 0, 3, "beq"  },
  /* 0x28 */ { eRelative  , 0, 3, "bvc"  },
  /* 0x29 */ { eRelative  , 0, 3, "bvs"  },
  /* 0x2a */ { eRelative  , 0, 3, "bpl"  },
  /* 0x2b */ { eRelative  , 0, 3, "bmi"  },
  /* 0x2c */ { eRelative  , 0, 3, "bge"  },
  /* 0x2d */ { eRelative  , 0, 3, "blt"  },
  /* 0x2e */ { eRelative  , 0, 3, "bgt"  },
  /* 0x2f */ { eRelative  , 0, 3, "ble"  },
  /* 0x30 */ { eImplicit  , 1, 1, "tsx"  },
  /* 0x31 */ { eImplicit  , 1, 1, "ins"  },
  /* 0x32 */ { eImplicit  , 0, 1, "pula" },
  /* 0x33 */ { eImplicit  , 0, 1, "pulb" },
  /* 0x34 */ { eImplicit  , 0, 0, "dess" },
  /* 0x35 */ { eImplicit  , 0, 0, "txs"  },
  /* 0x36 */ { eImplicit  , 0, 1, "psha" },
  /* 0x37 */ { eImplicit  , 0, 1, "pshb" },
  /* 0x38 */ { eUnknown   , 0, 0, NULL   },
  /* 0x39 */ { eImplicit  , 0, 0, "rts"  },
  /* 0x3a */ { eUnknown   , 0, 0, NULL   },
  /* 0x3b */ { eImplicit  , 0, 0, "rti"  },
  /* 0x3c */ { eUnknown   , 0, 0, NULL   },
  /* 0x3d */ { eUnknown   , 0, 0, NULL   },
  /* 0x3e */ { eImplicit  , 0, 1, "wai"  },
  /* 0x3f */ { eImplicit  , 0, 1, "swi"  },
  /* 0x40 */ { eImplicit  , 0, 1, "nega" },
  /* 0x41 */ { eUnknown   , 0, 0, NULL   },
  /* 0x42 */ { eUnknown   , 0, 0, NULL   },
  /* 0x43 */ { eImplicit  , 0, 1, "coma" },
  /* 0x44 */ { eImplicit  , 0, 1, "lsra" },
  /* 0x45 */ { eUnknown   , 0, 0, NULL   },
  /* 0x46 */ { eImplicit  , 0, 1, "rora" },
  /* 0x47 */ { eImplicit  , 0, 1, "asra" },
  /* 0x48 */ { eImplicit  , 0, 1, "asla" },
  /* 0x49 */ { eImplicit  , 0, 1, "rola" },
  /* 0x4a */ { eImplicit  , 0, 1, "deca" },
  /* 0x4b */ { eUnknown   , 0, 0, NULL   },
  /* 0x4c */ { eImplicit  , 0, 1, "inca" },
  /* 0x4d */ { eImplicit  , 0, 1, "tsta" },
  /* 0x4e */ { eUnknown   , 0, 0, NULL   },
  /* 0x4f */ { eImplicit  , 0, 1, "clra" },
  /* 0x50 */ { eImplicit  , 0, 1, "negb" },
  /* 0x51 */ { eUnknown   , 0, 0, NULL   },
  /* 0x52 */ { eUnknown   , 0, 0, NULL   },
  /* 0x53 */ { eImplicit  , 0, 1, "comb" },
  /* 0x54 */ { eImplicit  , 0, 1, "lsrb" },
  /* 0x55 */ { eUnknown   , 0, 0, NULL   },
  /* 0x56 */ { eImplicit  , 0, 1, "rorb" },
  /* 0x57 */ { eImplicit  , 0, 1, "asrb" },
  /* 0x58 */ { eImplicit  , 0, 1, "aslb" },
  /* 0x59 */ { eImplicit  , 0, 1, "rolb" },
  /* 0x5a */ { eImplicit  , 0, 1, "decb" },
  /* 0x5b */ { eUnknown   , 0, 0, NULL   },
  /* 0x5c */ { eImplicit  , 0, 1, "incb" },
  /* 0x5d */ { eImplicit  , 0, 1, "tstb" },
  /* 0x5e */ { eUnknown   , 0, 0, NULL   },
  /* 0x5f */ { eImplicit  , 0, 1, "clrb" },
  /* 0x60 */ { eIndexed   , 0, 1, "neg"  },
  /* 0x61 */ { eUnknown   , 0, 0, NULL   },
  /* 0x62 */ { eUnknown   , 0, 0, NULL   },
  /* 0x63 */ { eIndexed   , 0, 1, "com"  },
  /* 0x64 */ { eIndexed   , 0, 1, "lsr"  },
  /* 0x65 */ { eUnknown   , 0, 0, NULL   },
  /* 0x66 */ { eIndexed   , 0, 1, "ror"  },
  /* 0x67 */ { eIndexed   , 0, 1, "asr"  },
  /* 0x68 */ { eIndexed   , 0, 1, "asl"  },
  /* 0x69 */ { eIndexed   , 0, 1, "rol"  },
  /* 0x6a */ { eIndexed   , 0, 1, "dec"  },
  /* 0x6b */ { eUnknown   , 0, 0, NULL   },
  /* 0x6c */ { eIndexed   , 0, 1, "inc"  },
  /* 0x6d */ { eIndexed   , 0, 1, "tst"  },
  /* 0x6e */ { eIndexed   , 0, 0, "jmp"  },
  /* 0x6f */ { eIndexed   , 0, 1, "clr"  },
  /* 0x70 */ { eExtended  , 0, 1, "neg"  },
  /* 0x71 */ { eUnknown   , 0, 0, NULL   },
  /* 0x72 */ { eUnknown   , 0, 0, NULL   },
  /* 0x73 */ { eExtended  , 0, 1, "com"  },
  /* 0x74 */ { eExtended  , 0, 1, "lsr"  },
  /* 0x75 */ { eUnknown   , 0, 0, NULL   },
  /* 0x76 */ { eExtended  , 0, 1, "ror"  },
  /* 0x77 */ { eExtended  , 0, 1, "asr"  },
  /* 0x78 */ { eExtended  , 0, 1, "asl"  },
  /* 0x79 */ { eExtended  , 0, 1, "rol"  },
  /* 0x7a */ { eExtended  , 0, 1, "dec"  },
  /* 0x7b */ { eUnknown   , 0, 0, NULL   },
  /* 0x7c */ { eExtended  , 0, 1, "inc"  },
  /* 0x7d */ { eExtended  , 0, 1, "tst"  },
  /* 0x7e */ { eExtended  , 0, 2, "jmp"  },
  /* 0x7f */ { eExtended  , 0, 1, "clr"  },
  /* 0x80 */ { eImmediate , 0, 1, "suba" },
  /* 0x81 */ { eImmediate , 0, 1, "cmpa" },
  /* 0x82 */ { eImmediate , 0, 1, "sbca" },
  /* 0x83 */ { eUnknown   , 0, 0, NULL   },
  /* 0x84 */ { eImmediate , 0, 1, "anda" },
  /* 0x85 */ { eImmediate , 0, 1, "bita" },
  /* 0x86 */ { eImmediate , 0, 1, "ldaa" },
  /* 0x87 */ { eUnknown   , 0, 0, NULL   },
  /* 0x88 */ { eImmediate , 0, 1, "eora" },
  /* 0x89 */ { eImmediate , 0, 1, "adca" },
  /* 0x8a */ { eImmediate , 0, 1, "oraa" },
  /* 0x8b */ { eImmediate , 0, 1, "adda" },
  /* 0x8c */ { eImmediate , 1, 1, "cpx"  },
  /* 0x8d */ { eRelative  , 0, 3, "bsr"  },
  /* 0x8e */ { eImmediate , 1, 1, "lds"  },
  /* 0x8f */ { eUnknown   , 0, 0, NULL   },
  /* 0x90 */ { eDirect    , 0, 1, "suba" },
  /* 0x91 */ { eDirect    , 0, 1, "cmpa" },
  /* 0x92 */ { eDirect    , 0, 1, "sbca" },
  /* 0x93 */ { eUnknown   , 0, 0, NULL   },
  /* 0x94 */ { eDirect    , 0, 1, "anda" },
  /* 0x95 */ { eDirect    , 0, 1, "bita" },
  /* 0x96 */ { eDirect    , 0, 1, "ldaa" },
  /* 0x97 */ { eDirect    , 0, 1, "staa" },
  /* 0x98 */ { eDirect    , 0, 1, "eora" },
  /* 0x99 */ { eDirect    , 0, 1, "adca" },
  /* 0x9a */ { eDirect    , 0, 1, "oraa" },
  /* 0x9b */ { eDirect    , 0, 1, "adda" },
  /* 0x9c */ { eDirect    , 1, 1, "cpx"  },
  /* 0x9d */ { eUnknown   , 0, 0, NULL   },
  /* 0x9e */ { eDirect    , 1, 1, "lds"  },
  /* 0x9f */ { eDirect    , 1, 1, "sts"  },
  /* 0xa0 */ { eIndexed   , 0, 1, "suba" },
  /* 0xa1 */ { eIndexed   , 0, 1, "cmpa" },
  /* 0xa2 */ { eIndexed   , 0, 1, "sbca" },
  /* 0xa3 */ { eUnknown   , 0, 0, NULL   },
  /* 0xa4 */ { eIndexed   , 0, 1, "anda" },
  /* 0xa5 */ { eIndexed   , 0, 1, "bita" },
  /* 0xa6 */ { eIndexed   , 0, 1, "ldaa" },
  /* 0xa7 */ { eIndexed   , 0, 1, "staa" },
  /* 0xa8 */ { eIndexed   , 0, 1, "eora" },
  /* 0xa9 */ { eIndexed   , 0, 1, "adca" },
  /* 0xaa */ { eIndexed   , 0, 1, "oraa" },
  /* 0xab */ { eIndexed   , 0, 1, "adda" },
  /* 0xac */ { eIndexed   , 1, 1, "cpx"  },
  /* 0xad */ { eIndexed   , 0, 1, "jsr"  },
  /* 0xae */ { eIndexed   , 1, 1, "lds"  },
  /* 0xaf */ { eIndexed   , 1, 1, "sts"  },
  /* 0xb0 */ { eExtended  , 0, 1, "suba" },
  /* 0xb1 */ { eExtended  , 0, 1, "cmpa" },
  /* 0xb2 */ { eExtended  , 0, 1, "sbca" },
  /* 0xb3 */ { eUnknown   , 0, 0, NULL   },
  /* 0xb4 */ { eExtended  , 0, 1, "anda" },
  /* 0xb5 */ { eExtended  , 0, 1, "bita" },
  /* 0xb6 */ { eExtended  , 0, 1, "ldaa" },
  /* 0xb7 */ { eExtended  , 0, 1, "staa" },
  /* 0xb8 */ { eExtended  , 0, 1, "eora" },
  /* 0xb9 */ { eExtended  , 0, 1, "adca" },
  /* 0xba */ { eExtended  , 0, 1, "oraa" },
  /* 0xbb */ { eExtended  , 0, 1, "adda" },
  /* 0xbc */ { eExtended  , 1, 1, "cpx"  },
  /* 0xbd */ { eExtended  , 0, 3, "jsr"  },
  /* 0xbe */ { eExtended  , 1, 0, "lds"  },
  /* 0xbf */ { eExtended  , 1, 0, "sts"  },
  /* 0xc0 */ { eImmediate , 0, 1, "subb" },
  /* 0xc1 */ { eImmediate , 0, 1, "cmpb" },
  /* 0xc2 */ { eImmediate , 0, 1, "sbcb" },
  /* 0xc3 */ { eUnknown   , 0, 0, NULL   },
  /* 0xc4 */ { eImmediate , 0, 1, "andb" },
  /* 0xc5 */ { eImmediate , 0, 1, "bitb" },
  /* 0xc6 */ { eImmediate , 0, 1, "ldab" },
  /* 0xc7 */ { eImmediate , 0, 1, "stab" },
  /* 0xc8 */ { eImmediate , 0, 1, "eorb" },
  /* 0xc9 */ { eImmediate , 0, 1, "adcb" },
  /* 0xca */ { eImmediate , 0, 1, "orab" },
  /* 0xcb */ { eImmediate , 0, 1, "addb" },
  /* 0xcc */ { eUnknown   , 0, 0, NULL   },
  /* 0xcd */ { eUnknown   , 0, 0, NULL   },
  /* 0xce */ { eImmediate , 1, 1, "ldx"  },
  /* 0xcf */ { eUnknown   , 0, 0, NULL   },
  /* 0xd0 */ { eDirect    , 0, 1, "subb" },
  /* 0xd1 */ { eDirect    , 0, 1, "cmpb" },
  /* 0xd2 */ { eDirect    , 0, 1, "sbcb" },
  /* 0xd3 */ { eUnknown   , 0, 0, NULL   },
  /* 0xd4 */ { eDirect    , 0, 1, "andb" },
  /* 0xd5 */ { eDirect    , 0, 1, "bitb" },
  /* 0xd6 */ { eDirect    , 0, 1, "ldab" },
  /* 0xd7 */ { eDirect    , 0, 1, "stab" },
  /* 0xd8 */ { eDirect    , 0, 1, "eorb" },
  /* 0xd9 */ { eDirect    , 0, 1, "adcb" },
  /* 0xda */ { eDirect    , 0, 1, "orab" },
  /* 0xdb */ { eDirect    , 0, 1, "addb" },
  /* 0xdc */ { eUnknown   , 0, 0, NULL   },
  /* 0xdd */ { eUnknown   , 0, 0, NULL   },
  /* 0xde */ { eDirect    , 0, 1, "ldx"  },
  /* 0xdf */ { eDirect    , 0, 1, "stx"  },
  /* 0xe0 */ { eIndexed   , 0, 1, "subb" },
  /* 0xe1 */ { eIndexed   , 0, 1, "cmpb" },
  /* 0xe2 */ { eIndexed   , 0, 1, "sbcb" },
  /* 0xe3 */ { eUnknown   , 0, 0, NULL   },
  /* 0xe4 */ { eIndexed   , 0, 1, "andb" },
  /* 0xe5 */ { eIndexed   , 0, 1, "bitb" },
  /* 0xe6 */ { eIndexed   , 0, 1, "ldab" },
  /* 0xe7 */ { eIndexed   , 0, 1, "stab" },
  /* 0xe8 */ { eIndexed   , 0, 1, "eorb" },
  /* 0xe9 */ { eIndexed   , 0, 1, "adcb" },
  /* 0xea */ { eIndexed   , 0, 1, "orab" },
  /* 0xeb */ { eIndexed   , 0, 1, "addb" },
  /* 0xec */ { eUnknown   , 0, 0, NULL   },
  /* 0xed */ { eUnknown   , 0, 0, NULL   },
  /* 0xee */ { eIndexed   , 1, 1, "ldx"  },
  /* 0xef */ { eIndexed   , 1, 1, "stx"  },
  /* 0xf0 */ { eExtended  , 0, 1, "subb" },
  /* 0xf1 */ { eExtended  , 0, 1, "cmpb" },
  /* 0xf2 */ { eExtended  , 0, 1, "sbcb" },
  /* 0xf3 */ { eUnknown   , 0, 0, NULL   },
  /* 0xf4 */ { eExtended  , 0, 1, "andb" },
  /* 0xf5 */ { eExtended  , 0, 1, "bitb" },
  /* 0xf6 */ { eExtended  , 0, 1, "ldab" },
  /* 0xf7 */ { eExtended  , 0, 1, "stab" },
  /* 0xf8 */ { eExtended  , 0, 1, "eorb" },
  /* 0xf9 */ { eExtended  , 0, 1, "adcb" },
  /* 0xfa */ { eExtended  , 0, 1, "orab" },
  /* 0xfb */ { eExtended  , 0, 1, "addb" },
  /* 0xfc */ { eUnknown   , 0, 0, NULL   },
  /* 0xfd */ { eUnknown   , 0, 0, NULL   },
  /* 0xfe */ { eExtended  , 1, 1, "ldx"  },
  /* 0xff */ { eExtended  , 1, 1, "stx"  },
};

static const tOpcodeList DummyOpcode = { eUnknown   , 0, 0, NULL   };

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

      HexString(NumString, sizeof(NumString), Address, 0);
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
    strmaxprep(pBuffer, "$", BufferSize);
    return pBuffer;
  }

  strmaxprep(pBuffer, pSymbolPrefix, BufferSize);
  AddInvSymbol(pBuffer, Address);
  return pBuffer;
}

static void Disassemble_68(LargeWord Address, tDisassInfo *pInfo, Boolean AsData, int DataSize)
{
  Byte Opcode, Data[10];
  LargeInt Dist;
  char NumBuf[60], NumBuf2[60];
  const char *pOp, *pSymbolPrefix;
  Word OpAddr = 0;
  const tOpcodeList *pOpcode;

  pInfo->CodeLen = 0;
  pInfo->NextAddressCount = 0;
  pInfo->SrcLine[0] = '\0';
  pInfo->pRemark = NULL;

  if (!RetrieveData(Address, &Opcode, 1))
    return;
  nData = 1;

  pOpcode = AsData ? &DummyOpcode : OpcodeList + Opcode;
  switch (pOpcode->Type)
  {
    case eImplicit:
      pInfo->CodeLen = 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s", pOpcode->Memo);
      break;
    case eDirect:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      OpAddr = Data[0];
      pOp = MakeSymbolic(OpAddr, 1, NULL, NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s", pOpcode->Memo, pOp);
      break;
    case eIndexed:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      OpAddr = Data[0];
      pOp = MakeSymbolic(OpAddr, 1, NULL, NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s,x", pOpcode->Memo, pOp);
      if (!pOpcode->NextAddresses)
        pInfo->pRemark = "indirect jump, investigate here";
      if ((pOpcode->NextAddresses == 1) && (!strcmp(pOpcode->Memo, "jsr")))
        pInfo->pRemark = "indirect subroutine call, investigate here";
      break;
    case eExtended:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      OpAddr = (((Word)Data[0]) << 8) | Data[1];
      if (pOpcode->NextAddresses & 2)
        pSymbolPrefix = strcmp(pOpcode->Memo, "jsr") ? "lab_" : "sub_";
      else
        pSymbolPrefix = NULL;
      pOp = MakeSymbolic(OpAddr, 2, pSymbolPrefix, NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s", pOpcode->Memo, pOp);
      break;
    case eImmediate:
      if (!RetrieveData(Address + 1, Data, pOpcode->OpSize + 1))
        return;
      pInfo->CodeLen = 2 + pOpcode->OpSize;
      OpAddr = pOpcode->OpSize ? (((Word)Data[0]) << 8) | Data[1] : Data[0];
      pOp = MakeSymbolic(OpAddr, pOpcode->OpSize + 1, NULL, NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t#%s", pOpcode->Memo, pOp);
      break;
    case eRelative:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      Dist = Data[0];
      if (Dist & 0x80)
        Dist -= 256;
      OpAddr = (Address + 2 + Dist) & 0xffff;
      pOp = MakeSymbolic(OpAddr, 2, strcmp(pOpcode->Memo, "bsr") ? "lab_" : "sub_", NumBuf, sizeof(NumBuf));
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s", pOpcode->Memo, pOp);
      break;
    default:
      if (DataSize < 0)
        DataSize = 1;
      if (!RetrieveData(Address + 1, Data, DataSize - 1))
        return;
      HexString(NumBuf, sizeof(NumBuf), Opcode, 2);
      HexString(NumBuf2, sizeof(NumBuf2), Address, 0);
      if (!AsData)
        fprintf(stderr, "unknown opcode 0x%s @ %s\n", NumBuf, NumBuf2);
      switch (DataSize)
      {
        case 2:
          OpAddr = (((Word)Opcode) << 8) | Data[0];
          pOp = MakeSymbolic(OpAddr, 2, NULL, NumBuf, sizeof(NumBuf));
          as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "adr\t%s", pOp);
          pInfo->CodeLen = 2;
          break;
        default:
          pInfo->CodeLen = 1;
          HexString(NumBuf, sizeof(NumBuf), Opcode, 2);
          as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "byt\t$%s", NumBuf);
      }
  }

  pInfo->NextAddressCount = 0;
  if (pOpcode->NextAddresses & 2)
    pInfo->NextAddresses[pInfo->NextAddressCount++] = OpAddr;
  if (pOpcode->NextAddresses & 1)
    pInfo->NextAddresses[pInfo->NextAddressCount++] = (Address + pInfo->CodeLen) % 0xffff;

  if (nData != pInfo->CodeLen)
    as_snprcatf(pInfo->SrcLine, sizeof(pInfo->SrcLine), " ; ouch %u != %u", nData, pInfo->CodeLen);
}

static void SwitchTo_68(void)
{
  Disassemble = Disassemble_68;
}

void deco68_init(void)
{
  CPU6800 = AddCPU("6800", SwitchTo_68);
  CPU6802 = AddCPU("6802", SwitchTo_68);
}
