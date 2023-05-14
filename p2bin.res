;* p2bin.res
;*****************************************************************************
;* AS-Portierung                                                             *
;*                                                                           *
;* Stringdefinitionen fuer P2BIN                                             *
;*                                                                           *
;* Historie:  3. 6.1996 Grundsteinlegung                                     *
;*           24. 3.2000 added byte messages                                  *
;*                                                                           *
;*****************************************************************************
;* $Id: p2bin.res,v 1.3 2006/12/09 18:27:30 alfred Exp $
/*****************************************************************************
;* $Log: p2bin.res,v $
;* Revision 1.3  2006/12/09 18:27:30  alfred
;* - add warning about empty output
;*
;*****************************************************************************

Include header.res

Include tools2.res

;------------------------------------------------------------------------------
; Ansagen

Message InfoMessChecksum
 "Pr&uuml;fsumme: "
 "checksum: "

Message InfoMessHead2
 " <Quelldatei(en)> <Zieldatei> [Optionen]"
 " <source file(s)> <target file> [options]"

Message Byte
 "byte"
 "Byte"

Message Bytes
 "bytes"
 "Bytes"

Message InfoMessHelp
 "\n" \
 "Optionen:\n" \
 "  -f <Headerliste>  : auszufilternde Records\n" \
 "  -r <Start>-<Stop> : auszufilternder Adre&szlig;bereich\n" \
 "                      ($ oder 0x = erste bzw. letzte auftauchende Adresse)\n" \
 "  -segment <Name>   : Quellsegment w&auml;hlen\n" \
 "  -l <8-Bit-Zahl>   : Inhalt unbenutzter Speicherzellen festlegen\n" \
 "  -s                : Pr&uuml;fsumme in Datei ablegen\n" \
 "  -m <Modus>        : EPROM-Modus (odd,even,byte0..byte3)\n" \
 "  -e <Adresse>      : Startadresse festlegen\n" \
 "  -S [L|B]<L&auml;nge> : Startadresse voranstellen\n" \
 "  -k                : Quelldateien automatisch l&ouml;schen\n" \
 "  -q, -quiet        : Stiller Betrieb\n" \
 "  -v                : wortreicher Betrieb\n" \
 "  -help             : nur Hilfe ausgeben\n" \
 "  -version          : nur Versionsinfo ausgeben\n"
 "\n" \
 "options:\n" \
 "  -f <header list>  : records to filter out\n" \
 "  -r <start>-<stop> : address range to filter out\n" \
 "                      ($ or 0x = first resp. last occuring address)\n" \
 "  -segment <name>   : select source segment\n" \
 "  -l <8-bit-number> : set filler value for unused cells\n" \
 "  -s                : put checksum into file\n" \
 "  -m <mode>         : EPROM-mode (odd,even,byte0..byte3)\n" \
 "  -e <address>      : set entry address\n" \
 "  -S [L|B]<length>  : prepend entry address to image\n" \
 "  -k                : automatically erase source files\n" \
 "  -q, -quiet        : quiet operation\n" \
 "  -v                : verbose operation\n" \
 "  -help             : print help and exit\n" \
 "  -version          : print version info and exit\n"

Message InfoMessDeducedRange
 "Ermittelter Adre&szlig;bereich"
 "Deduced address range"

Message WarnEmptyFile
 "Warnung: keine Daten &uuml;bertragen, falsche/fehlende -r Option?\n"
 "Warning: no data transferred, wrong/missing -r option?\n"
