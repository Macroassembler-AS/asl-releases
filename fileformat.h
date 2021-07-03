#ifndef _FILEFORMAT_H
#define _FILEFORMAT_H
/* fileformat.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Definition von Konstanten fuer das P-Format                               */
/*                                                                           */
/* Historie: 3.12.1996 Grundsteinlegung                                      */
/*           11. 9.1998 ROMDATA-Segment hinzugenommen                        */
/*           12. 7.1999 RelocRec-Typ hinzugenommen                           */
/*           19. 1.2000 Patch-Typen definiert                                */
/*                                                                           */
/*****************************************************************************/

#include "addrspace.h"

#define FileMagic 0x1489

#define FileHeaderEnd       0x00   /* Dateiende */
#define FileHeaderStartAdr  0x80   /* Einsprungadresse absolut */
#define FileHeaderDataRec   0x81   /* normaler Datenrecord */
#define FileHeaderRDataRec  0x82   /* Datenrecord mit Symbolen */
#define FileHeaderRelocRec  0x83   /* relokatibler Datenrecord */
#define FileHeaderRRelocRec 0x84   /* relokatibler Datenrecord mit Symbolen */
#define FileHeaderRelocInfo 0x85   /* Relokationsinformationen */

/* Definition der im Code liegenden, zu patchenden Typen:

   Dazu wird ein 32-Bit-Wert verwendet.  Das oberste Byte gibt den Basistyp
   an, hier ist momentan nur 0 fuer binaere Integers definiert.  Fuer diesen
   Fall steht in Bit 0..7 die Laenge des Integers in Bits, in Bit 20 die
   Information, ob es sich um einen Big(1)- oder Little-Endian(0)-Typ handelt.
   Bits 8..11 geben die Startposition bzw. Bits 12..15 die Laenge der ersten
   Komponente im ersten Byte an, danach folgen so viele ganze Bytes wie
   moeglich.  Bits 16 bis 19 geben die Lage der verbleibenden Bits im letzten
   Byte an.  Bit 21 zeigt an, ob bei der Relokation addiert oder subtrahiert
   werden soll.  Bit 22 spezifiziert 'Seitenintegers', d.h. die Adresse,
   die an einer bestimmten Stelle eingepatcht wird, muss in den oberen (nicht
   gespeicherten) Bits identisch zur Adresse der Patchstelle selber sein.
   Ist Bit22=0, ist es ein normaler vorzeichenloser Int von 0...(2^n)-1

   Daraus ergeben sich z. B. folgende einfachen Typen: */

#define RelocTypeL8  0x00008008l
#define RelocTypeB8  RelocTypeL8          /* :-) was wunder */
#define RelocTypeL16 0x00008010l
#define RelocTypeB16 0x00108010l
#define RelocTypeL24 0x00008018l
#define RelocTypeB24 0x00108018l
#define RelocTypeL32 0x00008020l
#define RelocTypeB32 0x00108020l
#define RelocTypeL64 0x00008040l
#define RelocTypeB64 0x00108040l

#define RelocFlagBig  0x00100000l
#define RelocFlagSUB  0x00200000l
#define RelocBitCnt(Type) (Type & 0xff)
#define RelocFlagPage 0x00400000l

/* this is an internal symbol name used to signify the start address
   of a segment */

#define RelName_SegStart "$$$"

#define RelFlag_Relative 1

#endif /* _FILEFORMAT_H */
