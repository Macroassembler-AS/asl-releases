;* das.res
;*****************************************************************************
;* AS-Portierung                                                             *
;*                                                                           *
;* String-Definitions for DAS                                                *
;*                                                                           *
;*****************************************************************************
;* $Id: das.res,v 1.1 2015/02/01 20:51:38 alfred Exp $                       *
;*****************************************************************************
;* $Log: das.res,v $
;* Revision 1.1  2015/02/01 20:51:38  alfred
;* - -
;*
;*****************************************************************************

Include header.res

;-----------------------------------------------------------------------------
; Fehlermeldungen

Message ErrMsgInvParam
 "ung&uuml;ltige Option: "
 "invalid option: "

Message ErrMsgInvEnvParam
 "ung&uuml;ltige Environment-Option: "
 "invalid environment option: "

Message ErrMsgFileArgumentMissing
 "Datei-Argument fehlt"
 "file argument missing"

Message ErrMsgInvalidNumericValue
 "ung&uuml;tiger numerischer Wert"
 "invalid numeric value"

Message ErrMsgCannotReadBinaryFile
 "Bin&auml;rdatei nicht lesbar"
 "cannot read binary file"

Message ErrMsgCannotReadHexFile
 "HEX-Datei nicht lesbar"
 "cannot read HEX file"

Message ErrMsgInvalidHexData
 "ung&uuml;tige HEX-Daten"
 "invalid HEX data"

Message ErrMsgHexDataChecksumError
 "Hex-File Pr&uuml;fsummenfehler"
 "HEX file checksum error"

Message ErrMsgAddressArgumentMissing
 "Adressargument fehlt"
 "address argument missing"

Message ErrMsgClosingPatentheseMissing
 "schlie&szlig;ende Klammer fehlt"
 "missing closing parenthese"

Message ErrMsgInvalidEndinaness
 "ung&uuml;tige Endianess"
 "invalid endianess"

Message ErrMsgCannotRetrieveEntryAddressData
 "Daten f&uuml;r Startadresse nicht lesbar"
 "cannot read data for entry address"

Message ErrMsgSymbolArgumentMissing
 "Symbol-Argument fehlt"
 "missing symbol argument"

Message ErrMsgCPUArgumentMissing
 "CPU-Argmuent felht"
 "CPU argument missing"

Message ErrMsgUnknownCPU
 "unbekannte CPU"
 "unknown CPU"


Message KeyWaitMsg
 "--- weiter mit <ENTER> ---"
 "--- <ENTER> to go on ---"

Message InfoMessHead1
 "Aufruf: "
 "Usage: "

Message InfoMessHead2
 " [Optionen] [Datei] [Optionen] ..."
 " [options] [file] [options] ..."

Message InfoMessHelp
 "\n" \
 "Optionen:\n" \
 "  -cpu <Name>              : setze Zielprozessor\n" \
 "                             ('?' oder 'list', um verf&uuml;gbare nur anzuzeigen)\n" \
 "  -binfile <Name>[@Start[,L&auml;nge[,Granularit&auml;t]]] : lade Bin&auml;rdatei\n" \
 "  -hexfile <Name>          : lade HEX-Datei\n" \
 "  -entryaddress (<Adresse>[,L&auml;nge[,MSB|LSB]]),<Name> : definiere indirekte Startadresse\n" \
 "  -entryaddress <Adresse>,<Name> : definiere direkte Startadresse\n" \
 "  -symbol <Adresse>=<Name> : definiere Symbol\n" \
 "  -h                       : Hexadezimalzahlen mit Kleinbuchstaben\n" \
 "  -help                    : nur Hilfe ausgeben\n" \
 "  -version                 : nur Versionsinfo ausgeben\n"
 "  -screenheight <#>        : Bildschirmh&ouml;he f&uuml;r Paging von '-help'\n" \
 "                             und '-cpu list' setzen\n" \
 "\n"
 "\n" \
 "options:\n" \
 "  -cpu <name>              : set target processor\n" \
 "                             ('?' or 'list' to list available ones and exit)\n" \
 "  -binfile <name>[@start[,length[,granularity]]] : load binary file\n" \
 "  -hexfile <name>          : load HEX file\n" \
 "  -entryaddress (<address>[length[,MSB|LSB]]),<name> : define indirect start address\n" \
 "  -entryaddress <address>,<name> : define direct start address\n" \
 "  -symbol <address>=<name> : define symbol\n" \
 "  -h                       : use lower case in hexadecimal output\n" \
 "  -help                    : print help and exit\n" \
 "  -version                 : print version info and exit\n"
 "  -screenheight <#>        : set screen height for paging of '-help'\n" \
 "                             and '-cpu list'\n" \
 "\n"

Message InfoMessCPUList
 "implementierte Prozessoren :"
 "implemented processors :"
