/* deco87c800.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/*                                                                           */
/* DissectorTLCS-870                                                         */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>

#include "dasmdef.h"
#include "cpulist.h"
#include "codechunks.h"
#include "invaddress.h"
#include "strutil.h"

#include "deco87c800.h"

static CPUVar CPU87C00;

static const char Reg8Names[] = "awcbedlh";
static const char *Reg16Names[] = { "wa", "bc", "de", "hl" };
static const char *RelNames[8] =
{
  "z", "nz", "cs", "cc", "le", "gt", "t", "f"
};
static const char *ALUInstr[8] =
{
  "addc", "add", "subb", "sub", "and" , "xor", "or", "cmp"
};

static unsigned nData;

static char *ZeroHexString(char *pBuffer, size_t BufferSize, LargeWord Num, int ByteLen)
{
  HexString(pBuffer + 1, BufferSize - 1, Num, ByteLen * 2);
  if (isdigit(pBuffer[1]))
    return pBuffer + 1;
  else
  {
    pBuffer[0] = '0';
    return pBuffer;
  }
}

static const char *MakeSymbolic(LargeWord Address, int AddrLen, const char *pSymbolPrefix, char *pBuffer, int BufferSize)
{
  const char *pResult;

  if ((pResult = LookupInvSymbol(Address)))
    return pResult;

  HexString(pBuffer, BufferSize, Address, AddrLen << 1);
  if (!pSymbolPrefix)
  {
    if (isdigit(*pBuffer))
      strmaxprep(pBuffer, "0", BufferSize);
    as_snprcatf(pBuffer, BufferSize, "%c", HexStartCharacter + ('h' - 'a'));
    return pBuffer;
  }

  strmaxprep(pBuffer, pSymbolPrefix, BufferSize);
  AddInvSymbol(pBuffer, Address);
  return pBuffer;
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

      HexString(NumString, sizeof(NumString), Address, 0);
      fprintf(stderr, "cannot retrieve code @ 0x%s\n", NumString);
      return False;
    }
    pBuffer += Trans;
    Count -= Trans;
    Address = (Address + Trans) & 0xffff;
    nData += Trans;
  }
  return TRUE;
}

static void SimpleNextAddress(tDisassInfo *pInfo, LargeWord ThisAddress)
{
  pInfo->NextAddressCount = 1;
  pInfo->NextAddresses[0] = (ThisAddress + pInfo->CodeLen) % 0xffff;
}

static void PrintData(char *pDest, size_t DestSize, const Byte *pData, unsigned DataLen)
{
  char NumBuf[40];
  unsigned z;

  as_snprintf(pDest, DestSize, "db\t");
  for (z = 0; z < DataLen; z++)
  {
    if (z)
      as_snprcatf(pDest, DestSize, ",");
    as_snprcatf(pDest, DestSize, "%sh", ZeroHexString(NumBuf, sizeof(NumBuf), pData[z], 1));
  }
}

static void RegPrefix(LargeWord Address, tDisassInfo *pInfo, unsigned PrefixLen, unsigned SrcRegIndex)
{
  Byte Opcode, Data[5];
  char NumBuf[40], NumBuf2[40];

  if (!RetrieveData(Address + PrefixLen, &Opcode, 1))
    return;

  switch (Opcode)
  {
    case 0x01:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "swap\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x02:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "mul\t%c,%c",
                  Reg8Names[SrcRegIndex * 2 + 1], Reg8Names[SrcRegIndex * 2]);
      break;
    case 0x03:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "div\t%s,c",
                  Reg16Names[SrcRegIndex]);
      break;
    case 0x04:
      if (SrcRegIndex > 0)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "retn");
      break;
    case 0x06:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "pop\t%s",
                  Reg16Names[SrcRegIndex]);
      break;
    case 0x07:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "push\t%s",
                  Reg16Names[SrcRegIndex]);
      break;
    case 0x0a:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "daa\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x0b:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "das\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x10: case 0x11: case 0x12: case 0x13:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "xch\t%s,%s",
                  Reg16Names[Opcode & 3], Reg16Names[SrcRegIndex]);
      break;
    case 0x14: case 0x15: case 0x16: case 0x17:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,%s",
                  Reg16Names[Opcode & 3], Reg16Names[SrcRegIndex]);
      break;
    case 0x1c:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "shlc\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x1d:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "shrc\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x1e:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "rolc\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x1f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "rorc\t%c",
                  Reg8Names[SrcRegIndex]);
      break;
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x34: case 0x35: case 0x36: case 0x37:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\twa,%s",
                  ALUInstr[Opcode & 7], Reg16Names[SrcRegIndex]);
      break;
    case 0x38: case 0x39: case 0x3a: case 0x3b:
    case 0x3c: case 0x3d: case 0x3e: case 0x3f:
      if (SrcRegIndex > 3)
        goto inv16;
      if (!RetrieveData(Address + PrefixLen + 1, Data, 2))
        return;
      pInfo->CodeLen = PrefixLen + 1 + 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s,%sh",
                  ALUInstr[Opcode & 7], Reg16Names[SrcRegIndex],
                  ZeroHexString(NumBuf, sizeof(NumBuf), (((Word)Data[1]) << 8) | Data[0], 2));
      break;
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x46: case 0x47:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "set\t%c.%u",
                  Reg8Names[SrcRegIndex], Opcode & 7);
      break;
    case 0x48: case 0x49: case 0x4a: case 0x4b:
    case 0x4c: case 0x4d: case 0x4e: case 0x4f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\t%c.%u",
                  Reg8Names[SrcRegIndex], Opcode & 7);
      break;
    case 0x58: case 0x59: case 0x5a: case 0x5b:
    case 0x5c: case 0x5d: case 0x5e: case 0x5f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%c,%c",
                  Reg8Names[Opcode & 7], Reg8Names[SrcRegIndex]);
      break;
    case 0x60: case 0x61: case 0x62: case 0x63:
    case 0x64: case 0x65: case 0x66: case 0x67:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\ta,%c",
                  ALUInstr[Opcode & 7], Reg8Names[SrcRegIndex]);
      break;
    case 0x68: case 0x69: case 0x6a: case 0x6b:
    case 0x6c: case 0x6d: case 0x6e: case 0x6f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%c,a",
                  ALUInstr[Opcode & 7], Reg8Names[SrcRegIndex]);
      break;
    case 0x70: case 0x71: case 0x72: case 0x73:
    case 0x74: case 0x75: case 0x76: case 0x77:
      if (!RetrieveData(Address + PrefixLen + 1, Data, 1))
        return;
      pInfo->CodeLen = PrefixLen + 1 + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%c,%s",
                  ALUInstr[Opcode & 7], Reg8Names[SrcRegIndex],
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x82: case 0x83:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "set\t(%s).%c",
                  Reg16Names[Opcode & 3], Reg8Names[SrcRegIndex]);
      break;
    case 0x8a: case 0x8b:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\t(%s).%c",
                  Reg16Names[Opcode & 3], Reg8Names[SrcRegIndex]);
      break;
    case 0x92: case 0x93:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "cpl\t(%s).%c",
                  Reg16Names[Opcode & 3], Reg8Names[SrcRegIndex]);
      break;
    case 0x9a: case 0x9b:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(%s).%c,cf",
                  Reg16Names[Opcode & 3], Reg8Names[SrcRegIndex]);
      break;
    case 0x9e: case 0x9f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\tcf,(%s).%c",
                  Reg16Names[Opcode & 3], Reg8Names[SrcRegIndex]);
      break;
    case 0xa8: case 0xa9: case 0xaa: case 0xab:
    case 0xac: case 0xad: case 0xae: case 0xaf:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "xch\t%c,%c",
                  Reg8Names[Opcode & 7], Reg8Names[SrcRegIndex]);
      break;
    case 0xc0: case 0xc1: case 0xc2: case 0xc3:
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "cpl\t%c.%u",
                  Reg8Names[SrcRegIndex], Opcode & 7);
      break;
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%c.%u,cf",
                  Reg8Names[SrcRegIndex], Opcode & 7);
      break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
    case 0xd4: case 0xd5: case 0xd6: case 0xd7:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "xor\tcf,%c.%u",
                  Reg8Names[SrcRegIndex], Opcode & 7);
      break;
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\tcf,%c.%u",
                  Reg8Names[SrcRegIndex], Opcode & 7);
      break;
    inv16:
    case 0xfa:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\tsp,%s",
                  Reg16Names[Opcode & 3]);
      break;
    case 0xfb:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,sp",
                  Reg16Names[Opcode & 3]);
      break;
    case 0xfc:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "call\t%s",
                  Reg16Names[Opcode & 3]);
      pInfo->pRemark = "indirect jump, investigate here";
      break;
    case 0xfe:
      if (SrcRegIndex > 3)
        goto inv16;
      pInfo->CodeLen = PrefixLen + 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jp\t%s",
                  Reg16Names[Opcode & 3]);
      pInfo->pRemark = "indirect jump, investigate here";
      break;
    default:
      HexString(NumBuf, sizeof(NumBuf), Opcode, 2);
      HexString(NumBuf2, sizeof(NumBuf2), Address, 0);
      printf("unknown reg prefix opcode 0x%s @ %s\n", NumBuf, NumBuf2);
      pInfo->CodeLen = PrefixLen + 1;
      pInfo->NextAddressCount = 0;
      RetrieveData(Address, Data, pInfo->CodeLen);
      nData -= pInfo->CodeLen;
      PrintData(pInfo->SrcLine, sizeof(pInfo->SrcLine), Data, pInfo->CodeLen);
      break;
  }
}

static void MemPrefix(LargeWord Address, tDisassInfo *pInfo, unsigned PrefixLen, const char *pPrefixString)
{
  Byte Opcode, Data[5];
  char NumBuf[40], NumBuf2[40];

  if (!RetrieveData(Address + PrefixLen, &Opcode, 1))
    return;

  switch (Opcode)
  {
    case 0x08:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "rold\ta,%s",
                  pPrefixString);
      break;
    case 0x09:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "rord\ta,%s",
                  pPrefixString);
      break;
    case 0x10: case 0x11: case 0x12: case 0x13:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,%s",
                  pPrefixString, Reg16Names[Opcode & 3]);
      break;
    case 0x14: case 0x15: case 0x16: case 0x17:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,%s",
                  Reg16Names[Opcode & 3], pPrefixString);
      break;
    case 0x20:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "inc\t%s",
                  pPrefixString);
      break;
    case 0x26:
      if (!RetrieveData(Address + PrefixLen + 1, Data, 1))
        return;
      pInfo->CodeLen = PrefixLen + 1 + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(%sh),%s",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1), pPrefixString);
      break;
    case 0x27:
      pInfo->CodeLen = PrefixLen + 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(hl),%s",
                  pPrefixString);
      break;
    case 0x28:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dec\t%s",
                  pPrefixString);
      break;
    case 0x2c:
      if (!RetrieveData(Address + PrefixLen + 1, Data, 1))
        return;
      pInfo->CodeLen = PrefixLen + 1 + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,%sh",
                  pPrefixString, ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x2f:
      if (!RetrieveData(Address + PrefixLen + 1, Data, 1))
        return;
      pInfo->CodeLen = PrefixLen + 1 + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "mcmp\t%s,%sh",
                  pPrefixString, ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x46: case 0x47:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "set\t%s.%u",
                  pPrefixString, Opcode & 7);
      break;
    case 0x48: case 0x49: case 0x4a: case 0x4b:
    case 0x4c: case 0x4d: case 0x4e: case 0x4f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\t%s.%u",
                  pPrefixString, Opcode & 7);
      break;
    case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x56: case 0x57:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,%c",
                  pPrefixString, Reg8Names[Opcode & 7]);
      break;
    case 0x58: case 0x59: case 0x5a: case 0x5b:
    case 0x5c: case 0x5d: case 0x5e: case 0x5f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%c,%s",
                  Reg8Names[Opcode & 7], pPrefixString);
      break;
    case 0x60: case 0x61: case 0x62: case 0x63:
    case 0x64: case 0x65: case 0x66: case 0x67:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s,(hl)",
                  ALUInstr[Opcode & 7], pPrefixString);
      break;
    case 0x70: case 0x71: case 0x72: case 0x73:
    case 0x74: case 0x75: case 0x76: case 0x77:
      if (!RetrieveData(Address + PrefixLen + 1, Data, 1))
        return;
      pInfo->CodeLen = PrefixLen + 1 + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\t%s,%sh",
                  ALUInstr[Opcode & 7], pPrefixString,
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x78: case 0x79: case 0x7a: case 0x7b:
    case 0x7c: case 0x7d: case 0x7e: case 0x7f:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\ta,%s",
                  ALUInstr[Opcode & 7], pPrefixString);
      break;
    case 0xa8: case 0xa9: case 0xaa: case 0xab:
    case 0xac: case 0xad: case 0xae: case 0xaf:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "xch\t%c,%s",
                  Reg8Names[Opcode & 7], pPrefixString);
      break;
    case 0xc0: case 0xc1: case 0xc2: case 0xc3:
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "cpl\t%s.%u",
                  pPrefixString, Opcode & 7);
      break;
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s.%u,cf",
                  pPrefixString, Opcode & 7);
      break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
    case 0xd4: case 0xd5: case 0xd6: case 0xd7:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "xor\tcf,%s.%u",
                  pPrefixString, Opcode & 7);
      break;
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\tcf,%s.%u",
                  pPrefixString, Opcode & 7);
      break;
    case 0xfc:
      pInfo->CodeLen = PrefixLen + 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "call\t%s",
                  pPrefixString);
      pInfo->pRemark = "indirect subroutine call, investigate here";
      break;
    case 0xfe:
      pInfo->CodeLen = PrefixLen + 1;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jp\t%s",
                  pPrefixString);
      pInfo->pRemark = "indirect jump, investigate here";
      break;
    default:
      HexString(NumBuf, sizeof(NumBuf), Opcode, 2);
      HexString(NumBuf2, sizeof(NumBuf2), Address, 0);
      printf("unknown mem opcode 0x%s @ %s\n", NumBuf, NumBuf2);
      pInfo->CodeLen = PrefixLen + 1;
      pInfo->NextAddressCount = 0;
      RetrieveData(Address, Data, pInfo->CodeLen);
      nData -= pInfo->CodeLen;
      PrintData(pInfo->SrcLine, sizeof(pInfo->SrcLine), Data, pInfo->CodeLen);
      break;
  }
}

static void Disassemble_87C800(LargeWord Address, tDisassInfo *pInfo, Boolean AsData, int DataSize)
{
  Byte Opcode, Data[10];
  Word Arg;
  char NumBuf[40], NumBuf2[40];
  LargeInt Dist;
  unsigned Vector;
  Boolean ActAsData = AsData;

  pInfo->CodeLen = 0;
  pInfo->NextAddressCount = 0;
  pInfo->SrcLine[0] = '\0';
  pInfo->pRemark = NULL;

  if (!RetrieveData(Address, &Opcode, 1))
    return;
  nData = 1;

  if (!AsData)
  switch (Opcode)
  {
    case 0x00:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "nop");
      break;
    case 0x01:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "swap\ta");
      break;
    case 0x02:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "mul\tw,a");
      break;
    case 0x03:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "div\twa,c");
      break;
    case 0x04:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "reti");
      break;
    case 0x05:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ret");
      break;
    case 0x06:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "pop\tpsw");
      break;
    case 0x07:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "push\tpsw");
      break;
    case 0x0a:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "daa\ta");
      break;
    case 0x0b:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "das\ta");
      break;
    case 0x0c:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\tcf");
      break;
    case 0x0d:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "set\tcf");
      break;
    case 0x0e:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "cpl\tcf");
      break;
    case 0x0f:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\trbs,%sh",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x10: case 0x11: case 0x12: case 0x13:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "inc\t%s",
                  Reg16Names[Opcode & 3]);
      break;
    case 0x14: case 0x15: case 0x16: case 0x17:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%s,%sh",
                  Reg16Names[Opcode & 3],
                  ZeroHexString(NumBuf, sizeof(NumBuf), (((Word)Data[1]) << 8) | Data[0], 2));
      break;
    case 0x18: case 0x19: case 0x1a: case 0x1b:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dec\t%s",
                  Reg16Names[Opcode & 3]);
      break;
    case 0x1c:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "shlc\ta");
      break;
    case 0x1d:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "shrc\ta");
      break;
    case 0x1e:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "rolc\ta");
      break;
    case 0x1f:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "rorc\ta");
      break;
    case 0x20:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "inc\t(%sh)",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x21:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "inc\t(hl)");
      break;
    case 0x22:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\ta,(%sh)",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x23:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\ta,(hl)");
      break;
    case 0x24:
      if (!RetrieveData(Address + 1, Data, 3))
        return;
      pInfo->CodeLen = 4;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ldw\t(%sh),%sh",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1),
                  ZeroHexString(NumBuf2, sizeof(NumBuf2), (((Word)Data[2]) << 8) | Data[1], 2));
      break;
    case 0x25:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ldw\t(hl),%sh",
                  ZeroHexString(NumBuf, sizeof(NumBuf), (((Word)Data[1]) << 8) | Data[0], 2));
      break;
    case 0x26:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(%sh),(%sh)",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[1], 1),
                  ZeroHexString(NumBuf2, sizeof(NumBuf2), Data[0], 1));
      break;
    case 0x28:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dec\t(%sh)",
                  ZeroHexString(NumBuf2, sizeof(NumBuf2), Data[0], 1));
      break;
    case 0x29:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dec\t(hl)");
      break;
    case 0x2a:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(%sh),a",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x2b:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(hl),a");
      break;
    case 0x2c:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(%sh),%sh",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1),
                  ZeroHexString(NumBuf2, sizeof(NumBuf2), Data[1], 1));
      break;
    case 0x2d:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t(hl),%sh",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x2e:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\t(%sh)",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x2f:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\t(hl)");
      break;
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x34: case 0x35: case 0x36: case 0x37:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%c,%sh",
                  Reg8Names[Opcode & 7], ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x46: case 0x47:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "set\t(%sh).%u",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1), Opcode & 7);
      break;
    case 0x48: case 0x49: case 0x4a: case 0x4b:
    case 0x4c: case 0x4d: case 0x4e: case 0x4f:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "clr\t(%sh).%u",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1), Opcode & 7);
      break;
    case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x56: case 0x57:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\ta,%c",
                  Reg8Names[Opcode & 7]);
      break;
    case 0x58: case 0x59: case 0x5a: case 0x5b:
    case 0x5c: case 0x5d: case 0x5e: case 0x5f:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\t%c,a",
                  Reg8Names[Opcode & 7]);
      break;
    case 0x60: case 0x61: case 0x62: case 0x63:
    case 0x64: case 0x65: case 0x66: case 0x67:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "inc\t%c",
                  Reg8Names[Opcode & 7]);
      break;
    case 0x68: case 0x69: case 0x6a: case 0x6b:
    case 0x6c: case 0x6d: case 0x6e: case 0x6f:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dec\t%c",
                  Reg8Names[Opcode & 7]);
      break;
    case 0x70: case 0x71: case 0x72: case 0x73:
    case 0x74: case 0x75: case 0x76: case 0x77:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\ta,%sh",
                  ALUInstr[Opcode & 7], ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x78: case 0x79: case 0x7a: case 0x7b:
    case 0x7c: case 0x7d: case 0x7e: case 0x7f:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "%s\ta,(%sh)",
                  ALUInstr[Opcode & 7],
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      break;
    case 0x80: case 0x81: case 0x82: case 0x83:
    case 0x84: case 0x85: case 0x86: case 0x87:
    case 0x88: case 0x89: case 0x8a: case 0x8b:
    case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    case 0x90: case 0x91: case 0x92: case 0x93:
    case 0x94: case 0x95: case 0x96: case 0x97:
    case 0x98: case 0x99: case 0x9a: case 0x9b:
    case 0x9c: case 0x9d: case 0x9e: case 0x9f:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      Dist = Opcode & 0x1f;
      if (Dist & 0x10)
        Dist = Dist - 32;
      pInfo->NextAddresses[pInfo->NextAddressCount++] = (Address + 2 + Dist) & 0xffff;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jrs\tt,%sh",
                  MakeSymbolic(pInfo->NextAddresses[1], 2, "lab_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xa0: case 0xa1: case 0xa2: case 0xa3:
    case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    case 0xa8: case 0xa9: case 0xaa: case 0xab:
    case 0xac: case 0xad: case 0xae: case 0xaf:
    case 0xb0: case 0xb1: case 0xb2: case 0xb3:
    case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    case 0xb8: case 0xb9: case 0xba: case 0xbb:
    case 0xbc: case 0xbd: case 0xbe: case 0xbf:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      Dist = Opcode & 0x1f;
      if (Dist & 0x10)
        Dist = Dist - 32;
      pInfo->NextAddresses[pInfo->NextAddressCount++] = (Address + 2 + Dist) & 0xffff;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jrs\tf,%sh",
                  MakeSymbolic(pInfo->NextAddresses[1], 2, "lab_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xc0: case 0xc1: case 0xc2: case 0xc3:
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
      Vector = Opcode & 15;
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      if (RetrieveData(0xffc0 + (Vector << 1), Data, 2))
        pInfo->NextAddresses[pInfo->NextAddressCount++] = (((Word)Data[1]) << 8) | Data[0];
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "callv\t%u\t ; %sh", Vector,
                  MakeSymbolic(pInfo->NextAddresses[1], 2, "subv_", NumBuf, sizeof(NumBuf)));
      nData -= 2;
      break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
    case 0xd4: case 0xd5: case 0xd6: case 0xd7:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      Dist = Data[0];
      if (Dist & 0x80)
        Dist -= 256;
      pInfo->NextAddresses[pInfo->NextAddressCount++] = (Address + 2 + Dist) & 0xffff;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jr\t%s,%sh",
                  RelNames[Opcode & 7],
                  MakeSymbolic(pInfo->NextAddresses[1], 2, "lab_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "test\t(%sh).%u",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1), Opcode & 7);
      break;
    case 0xe0:
    case 0xf0:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      as_snprintf(NumBuf2, sizeof(NumBuf2), "(%sh)", ZeroHexString(NumBuf, sizeof(NumBuf), Data[0], 1));
      MemPrefix(Address, pInfo, 2, NumBuf2);
      break;
    case 0xe1:
    case 0xf1:
      MemPrefix(Address, pInfo, 1, "(pc+a)");
      break;
    case 0xe2:
    case 0xf2:
      MemPrefix(Address, pInfo, 1, "(de)");
      break;
    case 0xe3:
    case 0xf3:
      MemPrefix(Address, pInfo, 1, "(hl)");
      break;
    case 0xe4:
    case 0xf4:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      Dist = Data[0];
      if (Dist & 0x80)
        Dist -= 256;
      as_snprintf(NumBuf2, sizeof(NumBuf2), "(hl%s%d)",
                  Dist < 0 ? "" : "+", (int)Dist);
      MemPrefix(Address, pInfo, 2, NumBuf2);
      break;
    case 0xe5:
    case 0xf5:
      MemPrefix(Address, pInfo, 1, "(hl+c)");
      break;
    case 0xe6:
    case 0xf6:
      MemPrefix(Address, pInfo, 1, "(hl+)");
      break;
    case 0xe7:
    case 0xf7:
      MemPrefix(Address, pInfo, 1, "(-hl)");
      break;
    case 0xe8: case 0xe9: case 0xea: case 0xeb:
    case 0xec: case 0xed: case 0xee: case 0xef:
      RegPrefix(Address, pInfo, 1, Opcode & 7);
      break;
    case 0xfa:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      SimpleNextAddress(pInfo, Address);
      Arg = (((Word)Data[1]) << 8) | Data[0];
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "ld\tsp,%sh",
                  ZeroHexString(NumBuf, sizeof(NumBuf), Arg, 2));
      break;
    case 0xfb:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      Dist = Data[0];
      if (Dist & 0x80)
        Dist -= 256;
      pInfo->NextAddresses[pInfo->NextAddressCount++] = (Address + 2 + Dist) & 0xffff;
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jr\t%sh",
                  MakeSymbolic(pInfo->NextAddresses[0], 2, "lab_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xfc:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      SimpleNextAddress(pInfo, Address);
      pInfo->NextAddresses[pInfo->NextAddressCount++] = (((Word)Data[1]) << 8) | Data[0];
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "call\t%sh",
                  MakeSymbolic(pInfo->NextAddresses[1], 2, "sub_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xfd:
      if (!RetrieveData(Address + 1, Data, 1))
        return;
      pInfo->CodeLen = 2;
      SimpleNextAddress(pInfo, Address);
      pInfo->NextAddresses[pInfo->NextAddressCount++] = 0xff00 + Data[0];
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "callp\t%sh",
                  MakeSymbolic(pInfo->NextAddresses[1], 2, "sub_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xfe:
      if (!RetrieveData(Address + 1, Data, 2))
        return;
      pInfo->CodeLen = 3;
      pInfo->NextAddresses[pInfo->NextAddressCount++] = (((Word)Data[1]) << 8) | Data[0];
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "jp\t%sh",
                  MakeSymbolic(pInfo->NextAddresses[0], 2, "lab_", NumBuf, sizeof(NumBuf)));
      break;
    case 0xff:
      pInfo->CodeLen = 1;
      SimpleNextAddress(pInfo, Address);
      as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "swi");
      break;
    default:
      ActAsData = True;
  }

  if (ActAsData)
  {
    if (DataSize < 0)
      DataSize = 1;
    if (!RetrieveData(Address + 1, Data, DataSize - 1))
      return;
    HexString(NumBuf, sizeof(NumBuf), Opcode, 2);
    HexString(NumBuf2, sizeof(NumBuf2), Address, 0);
    if (!AsData)
      printf("unknown opcode 0x%s @ %s\n", NumBuf, NumBuf2);
    pInfo->NextAddressCount = 0;
    switch (DataSize)
    {
      case 2:
      {
        Word Addr = (((Word)Data[0]) << 8) | Opcode;
        as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "dw\t%s",
                    MakeSymbolic(Addr, 2, NULL, NumBuf, sizeof(NumBuf)));
        pInfo->CodeLen = 2;
        break;
      }
      default:
        pInfo->CodeLen = 1;
        as_snprintf(pInfo->SrcLine, sizeof(pInfo->SrcLine), "db\t%sh",
                    ZeroHexString(NumBuf, sizeof(NumBuf), Opcode, 1));
    }
    if (AsData)
      SimpleNextAddress(pInfo, Address);
  }

  if (nData != pInfo->CodeLen)
    as_snprcatf(pInfo->SrcLine, sizeof(pInfo->SrcLine), " ; ouch %u != %u", nData, pInfo->CodeLen);
}

static void SwitchTo_87C800(void)
{
  Disassemble = Disassemble_87C800;
}

void deco87c800_init(void)
{
  CPU87C00 = AddCPU("87C00", SwitchTo_87C800);
}
