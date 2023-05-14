;* plist.res
;*****************************************************************************
;* AS-Portierung                                                             *
;*                                                                           *
;* Stringdefinitionen fuer PLIST                                             *
;*                                                                           *
;* Historie: 31. 5.1996 Grundsteinlegung                                     *
;*            3.12.1996 Erweiterung um Segment-Spalte                        *
;*           21. 1.2000 Meldungen RelocInfo                                  *
;*           26. 6.2000 Meldung ExportInfo                                   *
;*                                                                           *
;*****************************************************************************

Include header.res

Include tools2.res

;-----------------------------------------------------------------------------
; Ansagen

Message MessHeaderLine1
 "Codetyp     Segment    Startadresse   L&auml;nge (Byte)  Endadresse"
 "Code-Type   Segment    Start-Address  Length (Bytes) End-Address"

Message MessHeaderLine2
 "--------------------------------------------------------------"
 "----------------------------------------------------------------"

Message MessHeaderLine1F
 "Datei "
 "File  "

Message MessHeaderLine2F
 "------"
 "------"

Message MessGenerator
 "Erzeuger : "
 "creator : "

Message MessSum1
 "insgesamt "
 "altogether "

Message MessSumSing
 " Byte  "
 " byte  "

Message MessSumPlur
 " Bytes "
 " bytes "

Message MessEntryPoint
 "<Einsprung>           "
 "<entry point>         "

Message MessRelocInfo
 "<Relokationsinfo>   "
 "<relocation info>   "

Message MessExportInfo
 "<export. symbol>    "
 "<exported symbol>   "

Message InfoMessHead2
 " [Optionen] [Programmdateiname(n)]"
 " [options] [name of program file(s)]"

Message InfoMessHelp
 "\n" \
 "Optionen:\n" \
 "  -help      : nur Hilfe ausgeben\n" \
 "  -q, -quiet : Stiller Betrieb\n" \
 "  -v         : wortreicher Betrieb\n" \
 "  -version   : nur Versionsinfo ausgeben\n" \
 "\n"
 "\n" \
 "options:\n" \
 "  -help      : print help and exit\n" \
 "  -q, -quiet : silent operation\n" \
 "  -v         : verbose operation\n" \
 "  -version   : print version info and exit\n" \
 "\n"
