;* as.res
;*****************************************************************************
;* AS-Portierung                                                             *
;*                                                                           *
;* String-Definitionen fuer AS                                               *
;*                                                                           *
;*****************************************************************************

Include header.res

;-----------------------------------------------------------------------------
; Fehlermeldungen

Message ErrName
 "Fehler"
 "error"

Message WarnName
 "Warnung"
 "warning"

Message InLineName
 " in Zeile "
 " in line "

Message GNUErrorMsg1
 "In Datei included von"
 "In file included from"

Message GNUErrorMsgN
 "                  von"
 "                 from"

Message ErrMsgUselessDisp
 "Displacement=0, &uuml;berfl&uuml;ssig"
 "useless displacement 0"

Message ErrMsgShortAddrPossible
 "Kurzadressierung m&ouml;glich"
 "short addressing possible"

Message ErrMsgShortJumpPossible
 "kurzer Sprung m&ouml;glich"
 "short jump possible"

Message ErrMsgNoShareFile
 "kein Sharefile angelegt, SHARED ignoriert"
 "no sharefile created, SHARED ignored"

Message ErrMsgBigDecFloat
 "FPU liest Wert evtl. nicht korrekt ein (>=1E1000)"
 "FPU possibly cannot read this value (> 1E1000)"

Message ErrMsgPrivOrder
 "privilegierte Anweisung"
 "privileged instruction"

Message ErrMsgDistNull
 "Distanz 0 nicht bei Kurzsprung erlaubt (NOP erzeugt)"
 "distance of 0 not allowed for short jump (NOP created instead)"

Message ErrMsgWrongSegment
 "Symbol aus falschem Segment"
 "symbol out of wrong segment"

Message ErrMsgInAccSegment
 "Segment nicht adressierbar"
 "segment not accessible"

Message ErrMsgPhaseErr
 "&Auml;nderung des Symbolwertes erzwingt zus&auml;tzlichen Pass"
 "change of symbol values forces additional pass"

Message ErrMsgOverlap
 "&uuml;berlappende Speicherbelegung"
 "overlapping memory usage"

Message ErrMsgOverlapReg
 "&uuml;berlappende Registernutzung"
 "overlapping register usage"

Message ErrMsgNoCaseHit
 "keine CASE-Bedingung zugetroffen"
 "none of the CASE conditions was true"

Message ErrMsgInAccPage
 "Seite m&ouml;glicherweise nicht adressierbar"
 "page might not be addressable"

Message ErrMsgRMustBeEven
 "Registernummer mu&szlig; gerade sein"
 "register number must be even"

Message ErrMsgObsolete
 "veralteter Befehl"
 "obsolete instruction, usage discouraged"

Message ErrMsgUnpredictable
 "nicht vorhersagbare Ausf&uuml;hrung dieser Anweisung"
 "unpredictable execution of this instruction"

Message ErrMsgAlphaNoSense
 "Lokaloperator au&szlig;erhalb einer Sektion &uuml;berfl&uuml;ssig"
 "localization operator senseless out of a section"

Message ErrMsgSenseless
 "sinnlose Operation"
 "senseless instruction"

Message ErrMsgRepassUnknown
 "unbekannter Symbolwert erzwingt zus&auml;tzlichen Pass"
 "unknown symbol value forces additional pass"

Message ErrMsgAddrNotAligned
 "Adresse nicht ausgerichtet"
 "address is not properly aligned"

Message ErrMsgIOAddrNotAllowed
 "I/O-Adresse darf nicht verwendet werden"
 "I/O-address must not be used here"

Message ErrMsgPipeline
 "m&ouml;gliche Pipelining-Effekte"
 "possible pipelining effects"

Message ErrMsgDoubleAdrRegUse
 "mehrfache Adre&szlig;registerbenutzung in einer Anweisung"
 "multiple use of address register in one instruction"

Message ErrMsgNotBitAddressable
 "Speicherstelle nicht bitadressierbar"
 "memory location is not bit addressable"

Message ErrMsgStackNotEmpty
 "Stack ist nicht leer"
 "stack is not empty"

Message ErrMsgNULCharacter
 "NUL-Zeichen in String, Ergebnis undefiniert"
 "NUL character in string, result is undefined"

Message ErrMsgPageCrossing
 "Befehl &uuml;berschreitet Seitengrenze"
 "instruction crosses page boundary" 

Message ErrMsgWUnderRange
 "Bereichsunterschreitung"
 "range underflow"

Message ErrMsgWOverRange
 "Bereichs&uuml;berschreitung"
 "range overflow"

Message ErrMsgNegDUP
 "negatives Argument f&uuml;r DUP"
 "negative argument for DUP"

Message ErrMsgConvIndX
 "einzelner X-Operand wird als indizierte und nicht als implizite Adressierung interpretiert"
 "single X operand interpreted as indexed and not implicit addressing"

Message ErrMsgNullResMem
 "kein Speicher reserviert, sind Sie sicher, da&szlig; Sie das wollten?"
 "no memory reserved, are you sure you wanted that?"

Message ErrMsgBitNumberTruncated
 "Bit-Nummer wird abgeschnitten werden"
 "bit number will be truncated"

Message ErrMsgInvRegisterPointer
 "Ung&uuml;ltiger Wert f&uuml;r Registerzeiger"
 "invalid register pointer value"

Message ErrMsgMacArgRedef
 "Makro-Argument umdefiniert"
 "macro argument redefined"

Message ErrMsgDeprecated
 "veraltete Anweisung"
 "deprecated instruction"

Message ErrMsgDeprecated_Instead
 "%s benutzen"
 "use %s"

Message ErrMsgSrcLEThanDest
 "Quelloperand l&auml;nger oder gleich Zieloperand"
 "source operand is longer or same size as destination operand"

Message ErrMsgTrapValidInstruction
 "TRAP-Nummer ist g&uuml;ltige Instruktion"
 "TRAP number represents valid instruction"

Message ErrMsgPaddingAdded
 "Padding hinzugef&uuml;gt"
 "Padding added"

Message ErrMsgRegNumWraparound
 "Registernummer-Umlauf"
 "register number wraparound"

Message ErrMsgIndexedForIndirect
 "verwende indizierte anstelle indirekter Adressierung"
 "using indexed instead of indirect addressing"

Message ErrMsgNotInNormalMode
 "nicht im Normal-Modus erlaubt"
 "not allowed in normal mode"

Message ErrMsgNotInPanelMode
 "nicht im Panel-Modus erlaubt"
 "not allowed in panel mode"

Message ErrMsgArgOutOfRange
 "Argument ausserhalb erlauben Bereichs"
 "argument out of range"

Message ErrMsgTrySkipMultiwordInstruction
 "Versuch, eine Mehrwort-Instruktion zu &uuml;berspringen"
 "attempt to skip multiword instruction"

Message ErrMsgSignExtension
 "implizite Vorzeichenerweiterung"
 "implicit sign extension"

;*****

Message ErrMsgDoubleDef
 "Symbol doppelt definiert"
 "symbol double defined"

Message ErrMsgSymbolUndef
 "Symbol nicht definiert"
 "symbol undefined"

Message ErrMsgInvSymName
 "ung&uuml;ltiger Symbolname"
 "invalid symbol name"

Message ErrMsgInvFormat
 "ung&uuml;ltiges Format"
  "invalid format"

Message ErrMsgUseLessAttr
 "&uuml;berfl&uuml;ssiges Attribut"
 "useless attribute"

Message ErrMsgUndefAttr
 "undefiniertes Attribut"
 "undefined attribute"

Message ErrMsgTooLongAttr
 "Attribut darf nur 1 Zeichen lang sein"
 "attribute may only be one character long"

Message ErrMsgWrongArgCnt
 "unpassende Operandenzahl"
 "wrong number of operands"

Message ErrMsgWrongOptCnt
 "unpassende Optionszahl"
 "wrong number of options"

Message ErrMsgOnlyImmAddr
 "nur immediate-Adressierung erlaubt"
 "addressing mode must be immediate"

Message ErrMsgInvOpSize
 "unpassende Operandengr&ouml;&szlig;e"
 "invalid operand size"

Message ErrMsgConfOpSizes
 "widersprechende Operandengr&ouml;&szlig;en"
 "conflicting operand sizes"

Message ErrMsgUndefOpSizes
 "undefinierte Operandengr&ouml;&szlig;e"
 "undefined operand size"

Message ErrMsgStringOrIntButFloat
 "Ganzzahl oder String erwartet, aber Gleitkommazahl erhalten"
 "expected integer or string, but got floating point number"

Message ErrMsgIntButFloat
 "Ganzzahl erwartet, aber Gleitkommazahl erhalten"
 "expected integer, but got floating point number"

Message ErrMsgFloatButString
 "Gleitkommazahl erwartet, aber String erhalten"
 "expected floating point number, but got string"

Message ErrMsgOpTypeMismatch
 "Operandentyp-Diskrepanz"
 "operand type mismatch"

Message ErrMsgStringButInt
 "String erwartet, aber Ganzzahl erhalten"
 "expected string, but got integer"

Message ErrMsgStringButFloat
 "String erwartet, aber Gleitkommazahl erhalten"
 "expected string, but got floating point number"

Message ErrMsgTooManyArgs
 "zu viele Argumente"
 "too many arguments"

Message ErrMsgIntButString
 "Ganzzahl erwartet, aber String erhalten"
 "expected integer, but got string"

Message ErrMsgIntOrFloatButString
 "Ganz- oder Gleitkommazahl erwartet, aber String erhalten"
 "expected integer or floating point number, but got string"

Message ErrMsgExpectString
 "String erwartet"
 "expected string"

Message ErrMsgExpectInt
 "Ganzzahl erwartet"
 "expected integer"

Message ErrMsgStringOrIntOrFloatButReg
 "Ganz-, Gleitkommazahl oder String erwartet, aber Register bekommen"
 "expected integer, floating point number or string but got register"

Message ErrMsgExpectIntOrString
 "Ganzzahl oder String erwartet"
 "expected integer or string"

Message ErrMsgExpectReg
 "Register erwartet"
 "expected register"

Message ErrMsgRegWrongTarget
 "Registersymbol f&uuml;r anderes Ziel"
 "register symbol for different target"

Message ErrMsgFloatButInt
 "Gleitkomma-Argument erwartet, aber Ganzzahl bekommen"
 "expected floating point argument but got integer"

Message ErrMsgUnknownInstruction
 "unbekannter Befehl"
 "unknown instruction"

Message ErrMsgBrackErr
 "Klammerfehler"
 "number of opening/closing parentheses does not match"

Message ErrMsgDivByZero
 "Division durch 0"
 "division by 0"

Message ErrMsgUnderRange
 "Bereichsunterschreitung"
 "range underflow"

Message ErrMsgOverRange
 "Bereichs&uuml;berschreitung"
 "range overflow"

Message ErrMsgNotPwr2
 "keine Zweierpotenz"
 "not a power of two"

Message ErrMsgNotAligned
 "Adresse nicht ausgerichtet"
 "address is not properly aligned"

Message ErrMsgDistTooBig
 "Distanz zu gro&szlig;"
 "distance too big"

Message ErrMsgTargOnDiffPage
 "Ziel nicht auf gleicher Seite"
 "target not on same page"

Message ErrMsgInAccReg
 "Register nicht zugreifbar"
 "register not accessible"

Message ErrMsgNoShortAddr
 "Kurzadressierung nicht m&ouml;glich"
 "short addressing not allowed"

Message ErrMsgInvAddrMode
 "unerlaubter Adressierungsmodus"
 "addressing mode not allowed here"

Message ErrMsgAddrMustBeEven
 "Addresse mu&szlig; gerade sein"
 "address must be even"

Message ErrMsgAddrMustBeAligned
 "Addresse mu&szlig; ausgerichtet sein"
 "address must be aligned"

Message ErrMsgInvParAddrMode
 "Adressierungsmodus im Parallelbetrieb nicht erlaubt"
 "addressing mode not allowed in parallel operation"

Message ErrMsgUndefCond
 "undefinierte Bedingung"
 "undefined condition"

Message ErrMsgIncompCond
 "inkompatible Bedingungen"
 "incompatible conditions"

Message ErrMsgUnknownFlag
 "unbekanntes Flag"
 "unknown flag"

Message ErrMsgDuplicateFlag
 "doppeltes Flag"
 "duplicate flag"

Message ErrMsgUnknownInt
 "unbekannter Interrupt"
 "unknown interrupt"

Message ErrMsgDuplicateInt
 "doppelter Interrupt"
 "duplicate interrupt"

Message ErrMsgJmpDistTooBig
 "Sprungdistanz zu gro&szlig;"
 "jump distance too big"

Message ErrMsgJmpDistIsZero
 "Sprungdistanz ist Null"
 "jump distance is zero"

Message ErrMsgDistIsOdd
 "Sprungdistanz ist ungerade"
 "jump distance is odd"

Message ErrMsgSkipTargetMismatch
 "Skip-Ziel passt nicht"
 "skip target mismatch"

Message ErrMsgInvShiftArg
 "ung&uuml;ltiges Schiebeargument"
 "invalid argument for shifting"

Message ErrMsgOnly1
 "nur Eins als Argument erlaubt"
 "operand must be one"

Message ErrMsgRange18
 "nur Bereich 1..8 erlaubt"
 "operand must be in range 1..8"

Message ErrMsgShiftCntTooBig
 "Schiebezahl zu gro&szlig;"
 "shift amplitude too big"

Message ErrMsgInvRegList
 "ung&uuml;ltige Registerliste"
 "invalid register list"

Message ErrMsgInvCmpMode
 "ung&uuml;ltiger Modus mit CMP"
 "invalid addressing mode for CMP"

Message ErrMsgInvCPUType
 "ung&uuml;ltiger Prozessortyp"
 "invalid CPU type"

Message ErrMsgInvFPUType
 "ung&uuml;ltiger FPU-Typ"
 "invalid FPU type"

Message ErrMsgInvPMMUType
 "ung&uuml;ltiger PMMU-Typ"
 "invalid PMMU type"

Message ErrMsgInvCtrlReg
 "ung&uuml;ltiges Kontrollregister"
 "invalid control register"

Message ErrMsgInvReg
 "ung&uuml;ltiges Register"
 "invalid register"

Message ErrMsgDoubleReg
 "Register mehr als einmal gelistet"
 "register(s) listed more than once"

Message ErrMsgRegBankMismatch
 "Register-Bank-Diskrepanz"
 "register bank mismatch"

Message ErrMsgUndefRegSize
 "Registerl&auml;nge undefiniert"
 "undefined register length"

Message ErrMsgInvOpOnReg
 "ung&auml;ltige Operation auf Register"
 "invalid operation on register"

Message ErrMsgNoSaveFrame
 "RESTORE ohne SAVE"
 "RESTORE without SAVE"

Message ErrMsgNoRestoreFrame
 "fehlendes RESTORE"
 "missing RESTORE"

Message ErrMsgUnknownMacArg
 "unbekannte Makro-Steueranweisung"
 "unknown macro control instruction"

Message ErrMsgMissEndif
 "fehlendes ENDIF/ENDCASE"
 "missing ENDIF/ENDCASE"

Message ErrMsgInvIfConst
 "ung&uuml;ltiges IF-Konstrukt"
 "invalid IF-structure"

Message ErrMsgDoubleSection
 "doppelter Sektionsname"
 "section name double defined"

Message ErrMsgInvSection
 "unbekannte Sektion"
 "unknown section"

Message ErrMsgMissingEndSect
 "fehlendes ENDSECTION"
 "missing ENDSECTION"

Message ErrMsgWrongEndSect
 "falsches ENDSECTION"
 "wrong ENDSECTION"

Message ErrMsgNotInSection
 "ENDSECTION ohne SECTION"
 "ENDSECTION without SECTION"

Message ErrMsgUndefdForward
 "nicht aufgel&ouml;ste Vorw&auml;rtsdeklaration"
 "unresolved forward declaration"

Message ErrMsgContForward
 "widersprechende FORWARD <-> PUBLIC-Deklaration"
 "conflicting FORWARD <-> PUBLIC-declaration"

Message ErrMsgInvFuncArgCnt
 "falsche Argumentzahl f&uuml;r Funktion"
 "wrong numbers of function arguments"

Message ErrMsgMissingLTORG
 "unaufgel&ouml;ste Literale (LTORG fehlt)"
 "unresolved literals (missing LTORG)"

Message ErrMsgInstructionNotOnThisCPUSupported
 "Befehl auf dem %s nicht unterst&uuml;tzt"
 "instruction not supported on %s"

Message ErrMsgAddrModeNotOnThisCPUSupported
 "Adressierungsart auf dem %s nicht unterst&uuml;tzt"
 "addressing mode not supported on %s"

Message ErrMsgFPUNotEnabled
 "FPU-Befehle nicht freigeschaltet"
 "FPU instructions are not enabled"

Message ErrMsgPMMUNotEnabled
 "PMMU-Befehle nicht freigeschaltet"
 "PMMU instructions are not enabled"

Message ErrMsgFullPMMUNotEnabled
 "voller PMMU-Befehlssatz nicht freigeschaltet"
 "full PMMU instruction set is not enabled"

Message ErrMsgCustomNotEnabled
 "Custom-Befehle nicht freigeschaltet"
 "Custom instructions are not enabled"

Message ErrMsgZ80SyntaxNotEnabled
 "Z80-Syntax nicht erlaubt"
 "Z80 syntax was not allowed"

Message ErrMsgZ80SyntaxExclusive
 "nicht im Z80-Syntax Exklusiv-Modus erlaubt"
 "not allowed in exclusive Z80 syntax mode"

Message ErrMsgInstructionNotOnThisFPUSupported
 "FPU-Befehl auf dem %s nicht unterst&uuml;tzt"
 "FPU instruction not supported on %s"

Message ErrMsgInvBitPos
 "ung&uuml;ltige Bitstelle"
 "invalid bit position"

Message ErrMsgOnlyOnOff
 "nur ON/OFF erlaubt"
 "only ON/OFF allowed"

Message ErrMsgStackEmpty
 "Stack ist leer oder nicht definiert"
 "stack is empty or undefined"

Message ErrMsgNotOneBit
 "Nicht genau ein Bit gesetzt"
 "not exactly one bit set"

Message ErrMsgMissingStruct
 "ENDSTRUCT ohne STRUCT"
 "ENDSTRUCT without STRUCT"

Message ErrMsgOpenStruct
 "offene Strukturdefinition"
 "open structure definition"

Message ErrMsgWrongStruct
 "falsches ENDSTRUCT"
 "wrong ENDSTRUCT"

Message ErrMsgPhaseDisallowed
 "Phasendefinition nicht in Strukturen erlaubt"
 "phase definition not allowed in structure definition"

Message ErrMsgInvStructDir
 "Ung&uuml;ltige STRUCT-Direktive"
 "invalid STRUCT directive"

Message ErrMsgDoubleStruct
 "Struktur redefiniert"
 "structure re-defined"

Message ErrMsgUnresolvedStructRef
 "nicht aufl&ouml;sbare Strukturelement-Referenz"
 "unresolvable structure element reference"

Message ErrMsgDuplicateStructElem
 "Strukturelement doppelt"
 "duplicate structure element"

Message ErrMsgNotRepeatable
 "Anweisung nicht wiederholbar"
 "instruction is not repeatable"

Message ErrMsgShortRead
 "vorzeitiges Dateiende"
 "unexpected end of file"

Message ErrMsgUnknownCodepage
 "unbekannte Zeichentabelle"
 "unknown codepage"

Message ErrMsgRomOffs063
 "ROM-Offset geht nur von 0..63"
 "ROM-offset must be in range 0..63"

Message ErrMsgInvFCode
 "ung&uuml;ltiger Funktionscode"
 "invalid function code"

Message ErrMsgInvFMask
 "ung&uuml;ltige Funktionscodemaske"
 "invalid function code mask"

Message ErrMsgInvMMUReg
 "ung&uuml;ltiges MMU-Register"
 "invalid MMU register"

Message ErrMsgLevel07
 "Level nur von 0..7"
 "level must be in range 0..7"

Message ErrMsgInvBitMask
 "ung&uuml;ltige Bitmaske" 
 "invalid bit mask"

Message ErrMsgInvRegPair
 "ung&uuml;ltiges Registerpaar"
 "invalid register pair"

Message ErrMsgOpenMacro
 "offene Makrodefinition"
 "open macro definition"

Message ErrMsgOpenIRP
 "IRP ohne ENDM"
 "IRP without ENDM"

Message ErrMsgOpenIRPC
 "IRPC ohne ENDM"
 "IRPC without ENDM"

Message ErrMsgOpenREPT
 "REPT ohne ENDM"
 "REPT without ENDM"

Message ErrMsgOpenWHILE
 "WHILE ohne ENDM"
 "WHILE without ENDM"

Message ErrMsgDoubleMacro
 "doppelte Makrodefinition"
 "macro double defined"

Message ErrMsgTooManyMacParams
 "mehr als 10 Makroparameter"
 "more than 10 macro parameters"

Message ErrMsgUndefKeyArg
 "Schl&uuml;sselwortargument nicht in Makro definiert"
 "keyword argument not defined in macro"

Message ErrMsgNoPosArg
 "Positionsargument nach Schl&uuml;sselwortargumenten nicht mehr erlaubt"
 "positional argument no longer allowed after keyword argument"

Message ErrMsgEXITMOutsideMacro
 "EXITM au&szlig;erhalb eines Makrorumpfes"
 "EXITM not called from within macro"

Message ErrMsgFirstPassCalc
 "Ausdruck mu&szlig; im ersten Pass berechenbar sein"
 "expression must be evaluatable in first pass"

Message ErrMsgTooManyNestedIfs
 "zu viele verschachtelte IFs"
 "too many nested IFs"

Message ErrMsgMissingIf
 "ELSEIF/ENDIF ohne IF"
 "ELSEIF/ENDIF without IF"

Message ErrMsgRekMacro
 "zu tief verschachtelter/rekursiver Makroaufruf"
 "too deeply nested/recursive makro call"

Message ErrMsgUnknownFunc
 "unbekannte Funktion"
 "unknown function"

Message ErrMsgInvFuncArg
 "Funktionsargument au&szlig;erhalb Definitionsbereich"
 "function argument out of definition range"

Message ErrMsgFloatOverflow
 "Gleitkomma&uuml;berlauf"
 "floating point overflow"

Message ErrMsgInvArgPair
 "ung&uuml;ltiges Wertepaar"
 "invalid value pair"

Message ErrMsgNotOnThisAddress
 "Befehl darf nicht auf dieser Adresse liegen"
 "instruction must not start on this address"

Message ErrMsgNotFromThisAddress
 "ung&uuml;ltiges Sprungziel"
 "invalid jump target"

Message ErrMsgJmpTargOnDiffPage
 "Sprungziel nicht auf gleicher Seite"
 "jump target not on same page"

Message ErrMsgTargOnDiffSection
 "Sprungziel nicht in gleicher Sektion"
 "jump target not in same section"

Message ErrMsgCodeOverflow
 "Code&uuml;berlauf"
 "code overflow"

Message ErrMsgMixDBDS
 "Konstanten und Platzhalter nicht mischbar"
 "constants and placeholders cannot be mixed"

Message ErrMsgNotInStruct
 "Codeerzeugung in Strukturdefinition nicht zul&auml;ssig"
 "code must not be generated in structure definition"

Message ErrMsgParNotPossible
 "paralleles Konstrukt nicht m&ouml;glich"
 "parallel construct not possible here"

Message ErrMsgAdrOverflow
 "Adre&szlig;&uuml;berlauf"
 "address overflow"

Message ErrMsgInvSegment
 "ung&uuml;ltiges Segment"
 "invalid segment"

Message ErrMsgUnknownSegment
 "unbekanntes Segment"
 "unknown segment"

Message ErrMsgUnknownSegReg
 "unbekanntes Segmentregister"
 "unknown segment register"

Message ErrMsgInvString
 "ung&uuml;ltiger String"
 "invalid string"

Message ErrMsgInvRegName
 "ung&uuml;ltiger Registername"
 "invalid register name"

Message ErrMsgInvArg
 "ung&uuml;ltiges Argument"
 "invalid argument"

Message ErrMsgNoIndir
 "keine Indirektion erlaubt"
 "indirect mode not allowed"

Message ErrMsgNotInThisSegment
 "nicht im aktuellen Segment erlaubt"
 "not allowed in current segment"

Message ErrMsgNotInMaxmode
 "nicht im Maximum-Modus zul&auml;ssig"
 "not allowed in maximum mode"

Message ErrMsgOnlyInMaxmode
 "nicht im Minimum-Modus zul&auml;ssig"
 "not allowed in minimum mode"

Message ErrMsgPackCrossBoundary
 "Anweisungspaket &uuml;berschreitet Adre&szlig;grenze"
 "execution packet crosses address boundary"

Message ErrMsgUnitMultipleUsed
 "Ausf&uuml;hrungseinheit mehrfach benutzt"
 "multiple use of same execution unit"

Message ErrMsgMultipleLongRead
 "mehrfache Lang-Leseoperation"
 "multiple long read operations"

Message ErrMsgMultipleLongWrite
 "mehrfache Lang-Schreiboperation"
 "multiple long write operations"

Message ErrMsgLongReadWithStore
 "Lang-Lese- mit Schreiboperation"
 "long read with write operation"

Message ErrMsgTooManyRegisterReads
 "zu viele Lesezugriffe auf ein Register"
 "too many reads of one register"

Message ErrMsgOverlapDests
 "&uuml;berlappende Ziele"
 "overlapping destinations"

Message ErrMsgTooManyBranchesInExPacket
 "zu viele absolute Spr&uuml;nge in einem Anweisungspaket"
 "too many absolute branches in one execution packet"

Message ErrMsgCannotUseUnit
 "Anweisung nicht auf diese Funktionseinheit ausf&uuml;hrbar"
 "instruction cannot be executed on this unit"

Message ErrMsgInvEscSequence
 "Ung&uuml;ltige Escape-Sequenz"
 "invalid escape sequence"

Message ErrMsgInvPrefixCombination
 "ung&uuml;ltige Pr&auml;fix-Kombination"
 "invalid combination of prefixes"

Message ErrMsgConstantRedefinedAsVariable
 "Konstante kann nicht als Variable redefiniert werden"
 "constants cannot be redefined as variables"

Message ErrMsgVariableRedefinedAsConstant
 "Variable kann nicht als Konstante redefiniert werden"
 "variables cannot be redefined as constants"

Message ErrMsgStructNameMissing
 "Strukturname fehlt"
 "structure name missing"

Message ErrMsgEmptyArgument
 "leeres Argument"
 "empty argument"

Message ErrMsgUnimplemented
 "nicht implementierte Anweisung"
 "unimplemented instruction"

Message ErrMsgFreestandingUnnamedStruct
 "namenlose Struktur nicht Teil einer anderen Struktur"
 "unnamed structure is not part of another structure"

Message ErrMsgSTRUCTEndedByENDUNION
 "STRUCT durch ENDUNION beendet"
 "STRUCT ended by ENDUNION"

Message ErrMsgAddrOnDifferentPage
 "Speicheradresse nicht auf aktiver Seite"
 "Memory address mot on active memory page"

Message ErrMsgUnknownMacExpMod
 "unbekanntes Makro-Expansions-Argument"
 "unknown macro expansion argument"

Message ErrMsgTooManyMacExpMod
 "zu viele Makro-Expansions-Argumente"
 "too many macro expansion arguments"

Message ErrMsgConflictingMacExpMod
 "widerspr&uuml;chliche Angaben zur Makro-Expansion"
 "contradicting macro expansion specifications"

Message ErrMsgInvalidPrepDir
 "unbekannte Pr&auml;prozessoranweisung"
 "unknown preprocessing directive"

Message ErrMsgExpectedError
 "erwarteter Fehler nicht eingetreten"
 "expected error did not occur"

Message ErrMsgNoNestExpect
 "Verschachtelung von EXPECT/ENDEXPECT nicht erlaubt"
 "nesting of EXPECT/ENDEXPECT not allowed"

Message ErrMsgMissingENDEXPECT
 "fehlendes ENDEXPECT"
 "missing ENDEXPECT"

Message ErrMsgMissingEXPECT
 "ENDEXPECT ohne EXPECT"
 "ENDEXPECT without EXPECT"

Message ErrMsgNoDefCkptReg
 "kein Default-Checkpoint-Register definiert"
 "no default checkpoint register defined"

Message ErrMsgInvBitField
 "ung&uuml;tiges Bitfeld"
 "invalid bit field"

Message ErrMsgArgValueMissing
 "Argument-Wert fehlt"
 "argument value missing"

Message ErrMsgUnknownArg
 "unbekanntes Argument"
 "unknown argument"

Message ErrMsgIndexRegMustBe16Bit
 "Indexregister muss 16 Bit sein"
 "index register must be 16 bit"

Message ErrMsgIOAddrRegMustBe16Bit
 "I/O Adressregister muss 16 Bit sein"
 "I/O address register must be 16 bit"

Message ErrMsgSegAddrRegMustBe32Bit
 "Adressregister im segmentierten Modus muss 32 Bit sein"
 "address register in segmented mode must be 32 bit"

Message ErrMsgNonSegAddrRegMustBe16Bit
 "Adressregister im nicht-segmentierten Modus muss 16 Bit sein"
 "address register in non-segmented mode must be 16 bit"

Message ErrMsgInvStructArgument
 "ung&uuml;ltiges Strukturargument"
 "invalid structure argument"

Message ErrMsgTooManyArrayDimensions
 "zu viele Array-Dimensionen"
 "too many array dimensions"

Message ErrMsgInvIntFormat
 "unbekannte Integer-Notation"
 "unknown integer notation"

Message ErrMsgInvIntFormatList
 "ung&uuml;ltige Liste an Integer-Schreibweisen"
 "invalid list of integer notations"

Message ErrMsgInvScale
 "ung&uuml;tige Skalierung"
 "invalid scale"

Message ErrMsgConfStringOpt
 "widerspr&uuml;chliche String-Optionen"
 "conflicting string options"

Message ErrMsgUnknownStringOpt
 "unbekannte String-Option"
 "unknown string option"

Message ErrMsgInvCacheInvMode
 "ung&uuml;ltiger Cache-Invalidierungs-Modus"
 "invalid cache invalidate mode"

Message ErrMsgInvCfgList
 "ung&uuml;ltige Config-Liste"
 "invalid config list"

Message ErrMsgConfBitBltOpt
 "widerspr&uuml;chliche BITBLT-Optionen"
 "conflicting BITBLT options"

Message ErrMsgUnknownBitBltOpt
 "unbekannte BITBLT Option"
 "unknown BITBLT option"

Message ErrMsgInvCBAR
 "ung&uuml;ltiger Wert f&uuml;r CBAR"
 "invalid CBAR value"

Message ErrMsgInAccPageErr
 "Seite nicht zugreifbar"
 "page not accessible"

Message ErrMsgInAccFieldErr
 "Feld nicht zugreifbar"
 "field not accessible"

Message ErrMsgTargInDiffField
 "Ziel nicht in gleichem Feld"
 "target not in same field"

Message ErrMsgInvCombination
 "Ung&uuml;tige Instruktions-Kombination"
 "invalid instruction combination"

Message ErrMsgInternalError
 "interner Fehler"
 "internal error"

Message ErrMsgOpeningFile
 "Fehler beim &Ouml;ffnen der Datei"
 "error in opening file"

Message ErrMsgListWrError
 "Listingschreibfehler"
 "error in writing listing"

Message ErrMsgFileReadError
 "Dateilesefehler"
 "file read error"

Message ErrMsgFileWriteError
 "Dateischreibfehler"
 "file write error"

Message ErrMsgIntError
 "interne(r) Fehler/Warnung"
 "internal error/warning"

Message ErrMsgHeapOvfl
 "Speicher&uuml;berlauf"
 "heap overflow"

Message ErrMsgStackOvfl
 "Stapel&uuml;berlauf"
 "stack overflow"
 
Message ErrMsgMaxIncLevelExceeded
 "INCLUDE zu tief verschachtelt"
 "INCLUDE nested too deeply"

Message ErrMsgTooManyErrors
 "zu viele Fehler, Assembler abgebrochen"
 "too many errors, assembly terminated"

Message ErrMsgIsFatal
 "Fataler Fehler, Assembler abgebrochen"
 "fatal error, assembly terminated"

Message ErrMsgOvlyError
 "Overlayfehler - Programmabbruch"
 "overlay error - program terminated"

Message PrevDefMsg
 "vorherige Definition in"
 "previous definition in"

Message ErrMsgInvSwapSize
 "ung&uuml;ltige Gr&ouml;&szlig;enangabe f&uuml;r Swapfile - Programmabbruch"
 "swap file size not correctly specified - program terminated"

Message ErrMsgSwapTooBig
 "zuwenig Platz f&uuml;r Swapfile - Programmabbruch"
 "insufficient space for swap file - program terminated"

Message ErrMsgNoRelocs
 "relokatible Symbole nicht erlaubt"
 "relocatable symbols not allowed"

Message ErrMsgIntOrFloatButReg
 "Ganz- oder Gleitkommazahl erwartet, aber Register bekommen"
 "expected integer or floating point number but got register"

Message ErrMsgIntOrStringButReg
 "Ganzzahl oder String erwartet, aber Register bekommen"
 "expected integer or string but got register"

Message ErrMsgIntButReg
 "Ganzzahl erwartet, aber Register bekommen"
 "expected integer but got register"

Message ErrMsgUnresRelocs
 "unverarbeitete externe Referenzen"
 "unprocessed external references"

Message ErrMsgUnexportable
 "Symbol nicht exportierbar"
 "cannot export this symbol"

Message ErrMsgArgCntZero
 "erwarte kein Argument, erhielt %d"
 "expected no argument but got %d"

Message ErrMsgArgCntOne
 "erwarte ein Argument, erhielt %d"
 "expected one argument but got %d"

Message ErrMsgArgCntMulti
 "erwarte %d Argumente, erhielt %d"
 "expected %d arguments but got %d"

Message ErrMsgArgCntFromTo
 "erwarte zwischen %d und %d Argumente, erhielt %d"
 "expected between %d and %d arguments but got %d"

Message ErrMsgArgCntEitherOr
 "erwarte %d oder %d Argumente, erhielt %d"
 "expected %d or %d arguments but got %d"

Message ErrMsgAddrArgCnt
 "Adre&szlig;ausdruck erwartet zwischen %d und %d Argumente, erhielt %d"
 "address expression expects between %d and %d arguments but got %d"

Message ErrMsgCannotSplitArg
 "Kann Argument nicht in Teile aufspalten"
 "failed splitting argument into parts"

Message ErrMsgMinCPUSupported
 "ab %s unterst&uuml;tzt"
 "supported by %s and above"

Message ErrMsgMaxCPUSupported
 "bis %s unterst&uuml;tzt"
 "supported by %s and below"

Message ErrMsgRangeCPUSupported
 "von %s bis %s unterst&uuml;tzt"
 "supported by %s to %s" 

Message ErrMsgOnlyCPUSupported1
 "nur von "
 "supported only by "

Message ErrMsgOnlyCPUSupportedOr
 " oder "
 " or "

Message ErrMsgOnlyCPUSupported2
 " unterst&uuml;tzt"
 ""

;----------------------------------------------------------------------------
; Strings in Listingkopfzeile

Message HeadingFileNameLab
 " - Quelle "
 " - Source File "

Message HeadingPageLab
 " - Seite "
 " - Page "

;----------------------------------------------------------------------------
; Strings in Listing

Message ListSymListHead1
 "  Symboltabelle (* = unbenutzt):"
 "  Symbol Table (* = unused):"

Message ListSymListHead2
 "  ----------------------------"
 "  --------------------------"

Message ListSymSumMsg
 " Symbol"
 " symbol"

Message ListSymSumsMsg
 " Symbole"
 " symbols"

Message ListUSymSumMsg
 " unbenutztes Symbol"
 " unused symbol"

Message ListUSymSumsMsg
 " unbenutzte Symbole"
 " unused symbols"

Message ListRegDefListHead1
 "  Registerdefinitionen (* = unbenutzt):"
 "  Register Definitions (* = unused):"

Message ListRegDefListHead2
 "  -----------------------------------"
 "  ----------------------------------"

Message ListRegDefSumMsg
 " Definition"
 " definition"

Message ListRegDefSumsMsg
 " Definitionen"
 " definitions"

Message ListRegDefUSumMsg
 " unbenutzte Definition"
 " unused definition"

Message ListRegDefUSumsMsg
 " unbenutzte Definitionen"
 " unused definitions"

Message ListCodepageListHead1
 "  Zeichentabellen:"
 "  Code Pages:"

Message ListCodepageListHead2
 "  ----------------"
 "  ----------"

Message ListCodepageChange
 " ver&auml;ndertes Zeichen"
 " changed character"

Message ListCodepagePChange
 " ver&auml;nderte Zeichen"
 " changed characters"

Message ListCodepageSumMsg
 " Zeichentabelle"
 " code page"

Message ListCodepageSumsMsg
 " Zeichentabellen"
 " code pages"

Message ListMacListHead1
 "  definierte Makros:"
 "  Defined Macros:"

Message ListMacListHead2
 "  ------------------"
 "  ---------------"

Message ListMacSumMsg
 " Makro"
 " macro"

Message ListMacSumsMsg
 " Makros"
 " macros"

Message ListStructListHead1
 "  definierte Strukturen/Unions:"
 "  Defined Structures/Unions:"

Message ListStructListHead2
 "  -----------------------------"
 "  --------------------------"

Message ListStructSumMsg
 " Struktur"
 " structure"

Message ListStructSumsMsg
 " Strukturen"
 " structures"

Message ListFuncListHead1
 "  definierte Funktionen:"
 "  Defined Functions:"

Message ListFuncListHead2
 "  ----------------------"
 "  ------------------"

Message ListDefListHead1
 "  DEFINEs:"
 "  DEFINEs:"

Message ListDefListHead2
 "  --------"
 "  --------"

Message ListSegListHead1
 "in "
 "Space Used in "

Message ListSegListHead2
 " belegte Bereiche:"
 " :"

Message ListCrossListHead1
 "  Querverweisliste:"
 "  Cross Reference List:"

Message ListCrossListHead2
 "  -----------------"
 "  ---------------------"

Message ListSectionListHead1
 "  Sektionen:"
 "  Sections:"

Message ListSectionListHead2
 "  ----------"
 "  ---------"

Message ListIncludeListHead1
 "  Include-Verschachtelung:"
 "  Nested Include Files:"

Message ListIncludeListHead2
 "  ------------------------"
 "  ---------------------"

Message ListCrossSymName
 "Symbol "
 "symbol "

Message ListCrossFileName
 "Datei "
 "file "

Message ListPlurName
 "n"
 "s"

Message ListHourName
 " Stunde"
 " hour"

Message ListMinuName
 " Minute"
 " minute"

Message ListSecoName
 " Sekunde"
 " second"

;---------------------------------------------------------------------------
; Durchsagen...

Message ErrMessNoInputFiles
 "keine Quelldateien (-help f&uuml; Hilfe)"
 "no input files (-help for help)"

Message InfoMessAssembling
 "Assembliere "
 "Assembling "

Message InfoMessPass
 "PASS "
 "PASS "

Message InfoMessPass1
 "PASS 1                             "
 "PASS 1                             "

Message InfoMessPass2
 "PASS 2                             "
 "PASS 2                             "

Message InfoMessAssTime
 " Assemblierzeit"
 " assembly time"

Message InfoMessAssLine
 " Zeile Quelltext"
 " line source file"

Message InfoMessAssLines
 " Zeilen Quelltext"
 " lines source file"

Message InfoMessPassCnt
 " Durchlauf"
 " pass"

Message InfoMessPPassCnt
 " Durchl&auml;ufe"
 " passes"

Message InfoMessNoPass
 "        zus&auml;tzliche erforderliche Durchl&auml;ufe wegen Fehlern nicht\n        durchgef&uuml;hrt, Listing m&ouml;glicherweise inkorrekt."
 "        Additional necessary passes not started due to\n        errors, listing possibly incorrect."

Message InfoMessMacAssLine
 " Zeile inkl. Makroexpansionen"
 " line incl. macro expansions"

Message InfoMessMacAssLines
 " Zeilen inkl. Makroexpansionen"
 " lines incl. macro expansions"

Message InfoMessWarnCnt
 " Warnung"
 " warning"

Message InfoMessWarnPCnt
 "en"
 "s"

Message InfoMessErrCnt
 " Fehler"
 " error"

Message InfoMessErrPCnt
 ""
 "s"

Message InfoMessRemainMem
 " KByte verf&uuml;gbarer Restspeicher"
 " Kbytes available memory"

Message InfoMessRemainStack
 " Byte verf&uuml;gbarer Stack"
 " bytes available stack"

Message InfoMessNFilesFound
 ": keine Datei(en) zu assemblieren!"
 ": No file(s) to assemble!"

Message InfoMessMacroAss
 "Makroassembler "
 "Macro Assembler "

Message InfoMessVar
 "Version"
 "Version"

Message InfoMessHead1
 "Aufruf: "
 "usage: "

Message InfoMessHead2
 " [Optionen] [Datei] [Optionen] ..."
 " [options] [file] [options] ..."

Message KeyWaitMsg
 "--- weiter mit <ENTER> ---"
 "--- <ENTER> to go on ---"

Message ErrMsgInvParam
 "ung&uuml;ltige Option (-help f&uuml; Hilfe): "
 "Invalid option (-help for help): "

Message ErrMsgInvEnvParam
 "ung&uuml;ltige Environment-Option (-help f&uuml; Hilfe): "
 "Invalid environment option (-help for help): "

Message InvMsgSource
 "Quelldatei?"
 "Source file?"

Message InfoMessHelp
 "\n" \
 "Optionen:\n" \
 "  -p                   : Sharefile im Pascal-Format\n" \
 "  -c                   : Sharefile im C-Format\n" \
 "  -a                   : Sharefile im AS-Format\n" \
 "  -o <Name>            : Namen der Code-Datei neu setzen\n" \
 "  -olist <Name>        : Namen der Listdatei neu setzen\n" \
 "  -shareout <Name>     : Namen des Sharefiles neu setzen\n" \
 "  -q, -quiet           : Stiller Betrieb\n" \
 "  -cpu <Name>          : Zielprozessor setzen\n" \
 "                         ('?' oder 'list', um verf&uuml;gbare nur anzuzeigen)\n" \
 "  -alias <neu>=<alt>   : Prozessor-Alias definieren\n" \
 "  -l                   : Listing auf Konsole\n" \
 "  -L                   : Listing auf Datei\n" \
 "  -listradix <2...36>  : Zahlensystem im Listing\n" \
 "  -i <Pfad>[:Pfad]...  : Pfadliste f&uuml;r Includedateien\n" \
 "  -D <Symbol>[,Symbol] : Symbole vordefinieren\n" \
 "  -gnuerrors           : Fehlermeldungen im GNU-Format\n" \
 "  -maxerrors <Anzahl>  : Assemblierung nach <Anzahl> Fehlern abbrechen\n" \
 "  -maxinclevel <Anzahl>: Include-Verschachtelungstiefe auf <Anzahl> begrenzen\n" \
 "  -Werror              : Warnungen als Fehler behandeln\n" \
 "  -w[no-]implicit-sign-extension : Warnungen &uuml;ber Vorzeichen-Erweiterungen ein/ausschalten\n" \
 "  -version             : nur Versionsinfo ausgeben\n" \
 "  -E [Name]            : Zieldatei f&uuml;r Fehlerliste,\n" \
 "                         !0..!4 f&uuml;r Standardhandles\n" \
 "                         Default <Quelldatei>.LOG\n" \
 "  -r                   : Meldungen erzeugen, falls zus&auml;tzlicher Pass erforderlich\n" \
 "  -relaxed             : beliebige Integer-Syntax im Default zulassen\n" \
 "  -Y                   : Sprungfehlerunterdr&uuml;ckung (siehe Anleitung)\n" \
 "  -v                   : wortreicher Betrieb\n" \
 "  -w                   : Warnungen unterdr&uuml;cken\n" \
 "  +G                   : Code-Erzeugung unterdr&uuml;cken\n" \
 "  -s                   : Sektionsliste erzeugen\n" \
 "  -t                   : Listing-Teile ein/ausblenden\n" \
 "  -u                   : Belegungsliste erzeugen\n" \
 "  -C                   : Querverweisliste erzeugen\n" \
 "  -I                   : Include-Verschachtelungsliste ausgeben\n" \
 "  -g [map|atmel|noice] : Debug-Informationen schreiben\n" \
 "  -A                   : kompaktere Symbolablage\n" \
 "  -U                   : case-sensitiv arbeiten\n" \
 "  -x                   : erweiterte Fehlermeldungen\n" \
 "  -n                   : Fehlermeldungen mit Nummer\n" \
 "  -P                   : Makroprozessorausgabe erzeugen\n" \
 "  -M                   : Makrodefinitionen extrahieren\n" \
 "  -h                   : Hexadezimalzahlen mit Kleinbuchstaben\n" \
 "  -help                : nur Hilfe ausgeben\n" \
 "  -splitbyte [char]    : Split-Byte-Darstellung von Zahlen\n" \
 "  -screenheight <#>    : Bildschirmh&ouml;he f&uuml;r Paging von '-help'\n" \
 "                         und '-cpu list' setzen\n" \
 "\n" \
 "Quelldateiangabe darf Jokerzeichen enthalten\n"
 "\n" \
 "options:\n" \
 "  -p                   : share file formatted for Pascal\n" \
 "  -c                   : share file formatted for C\n" \
 "  -a                   : share file formatted for AS\n" \
 "  -o <name>            : change name of code file\n" \
 "  -olist <nname>       : change name of list file\n" \
 "  -shareout <nname>    : change name of share file\n" \
 "  -q, -quiet           : silent operation\n" \
 "  -cpu <name>          : set target processor\n" \
 "                         ('?' or 'list' to list available ones and exit)\n" \
 "  -alias <new>=<old>   : define processor alias\n" \
 "  -l                   : listing to console\n" \
 "  -L                   : listing to file\n" \
 "  -listradix <2...36>  : number system in listing\n" \
 "  -i <path>[;path]...  : list of paths for include files\n" \
 "  -D <symbol>[,symbol] : predefine symbols\n" \
 "  -gnuerrors           : error messages in GNU format\n" \
 "  -maxerrors <number>  : terminate assembly after <number> errors\n" \
 "  -maxinclevel <number>: limit include nesting level to <number>\n" \
 "  -Werror              : treat warnings as errors\n" \
 "  -w[no-]implicit-sign-extension : enable/disable sign extension warnings\n" \
 "  -version             : print version info and exit\n" \
 "  -E <name>            : target file for error list,\n" \
 "                         !0..!4 for standard handles\n" \
 "                         default is <srcname>.LOG\n" \
 "  -r                   : generate messages if repassing necessary\n" \
 "  -relaxed             : allow arbitrary integer syntax by default\n" \
 "  -Y                   : branch error suppression (see manual)\n" \
 "  -v                   : verbose operation\n" \
 "  -w                   : suppress warnings\n" \
 "  +G                   : suppress code generation\n" \
 "  -s                   : generate section list\n" \
 "  -t                   : enable/disable parts of listing\n" \
 "  -u                   : generate usage list\n" \
 "  -C                   : generate cross reference list\n" \
 "  -I                   : generate include nesting list\n" \
 "  -g [map|atmel|noice] : write debug info\n" \
 "  -A                   : compact symbol table\n" \
 "  -U                   : case-sensitive operation\n" \
 "  -x                   : extended error messages\n" \
 "  -n                   : add error #s to error messages\n" \
 "  -P                   : write macro processor output\n" \
 "  -M                   : extract macro definitions\n" \
 "  -h                   : use lower case in hexadecimal output\n" \
 "  -help                : print help and exit\n" \
 "  -splitbyte [char]    : use split byte display of numbers\n" \
 "  -screenheight <#>    : set screen height for paging of '-help'\n" \
 "                         and '-cpu list'\n" \
 "\n" \
 "source file specification may contain wildcards\n"

Message InfoMessCPUList
 "implementierte Prozessoren :"
 "implemented processors :"
