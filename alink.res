;* alink.res
;*****************************************************************************
;* AS-Portierung                                                             *
;*                                                                           *
;* Stringdefinitionen fuer ALINK                                             *
;*                                                                           *
;* Historie:  2. 7.2000 Grundsteinlegung                                     *
;*            4. 7.2000 misc. info messages                                  *
;*           30.10.2000 added byte messages                                  *
;*                                                                           *
;*****************************************************************************

Include header.res

Include tools2.res

;-----------------------------------------------------------------------------
; info messages

Message InfoMsgGetSyms
 "Symbole lesen aus"
 "reading symbols from"

Message InfoMsgOpenSrc
 "&Ouml;ffne Quelldatei"
 "opening source file"

Message InfoMsgReading
 "modifiziere an Adresse"
 "patching at address"

Message InfoMsgLocating
 "reloziere an Adresse"
 "relocating to address"

;-----------------------------------------------------------------------------
; Fehlermeldungen

Message ErrMsgSrcMissing
 "Quelldateiangabe(n) fehlt!"
 "source file specification(s) missing!"

Message FormatRelocInfoMissing
 "Relokationsinformation fehlt hinter Datensatz"
 "relocation info for relocatable record missing"

Message DoubleDefSymbol
 "doppelt definiertes Symbol"
 "double defined symbol"

Message UndefSymbol
 "undefiniertes Symbol"
 "undefined symbol"

Message SumUndefSymbol
 "undefiniertes Symbol"
 "undefined symbol"

Message SumUndefSymbols
 "undefiniertes Symbole"
 "undefined symbols"

;-----------------------------------------------------------------------------
; Ansagen

Message Byte
 "byte"
 "Byte"

Message Bytes
 "bytes"
 "Bytes"

Message InfoMessHead2
 " <Quelldatei(en)> <Zieldatei> [Optionen]"
 " <source file(s)> <target file> [options]"

Message InfoMessHelp
 "\n" \
 "Optionen:\n" \
 "-help      : nur Hilfe ausgeben\n" \
 "-q, -quiet : Stiller Betrieb\n" \
 "-v         : ausf&uuml;hrliche Meldungen\n" \
 "-version   : nur Versionsinfo ausgeben\n" \
 "\n"
 "\n" \
 "options:\n" \
 "-help      : print help and exit\n" \
 "-q, -quiet : silent operation\n" \
 "-v         : verbose messages\n" \
 "-version   : print version info and exit\n" \
 "\n"
