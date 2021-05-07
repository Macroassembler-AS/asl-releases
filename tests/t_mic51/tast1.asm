
; Aufgabe Nr.: Speichermodul fuer uP- Praktikum II
; Autor: Joerg Vollandt
; erstellt am : 21.05.1994
; letzte Aenderung am :
; Bemerkung : Routinen fuer ASCII- Tastatur
;
; Dateiname : tast1.asm
;

;---------------------------------------------------------------------	
; Definitionen

        SEGMENT DATA

ZEICHEN         DB      ?

        SEGMENT BITDATA

STROB           DB      ?

;---------------------------------------------------------------------
        SEGMENT CODE
;=====================================================================
; Funktion : Tastaturinterrupt initialisieren
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

INIT_TASTATUR:
        Init_Vektor INT0_VEKTOR,TASTATUR_INT
        SETB IT0
        CLR IE0
        SETB EX0
        RET

;---------------------------------------------------------------------
; Funktion : Interruptroutine fuer Tastatur, setzt bei Tastaturstrob
;            das Bit STROB und schreibt das Zeichen von der Tastatur
;            nach ZEICHEN.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

TASTATUR_INT:
        MOV ZEICHEN,P1
        SETB STROB
        RETI

;---------------------------------------------------------------------
; Funktion : Klein- in Grossbuchstaben umwandeln.
; Aufrufparameter : ACC = Zeichen
; Ruechgabeparameter : ACC = Zeichen
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

        ifdef joerg

UPCASE: PUSH PSW
        CJNE A,#'a',UPCASE1
UPCASE1: JC UPCASE2
        CJNE A,#07bh,UPCASE3
UPCASE3: JNC UPCASE2
        CLR C
        SUBB A,#32
UPCASE2: POP PSW
        RET

        endif

;---------------------------------------------------------------------
; Funktion : Warten bis Tastendruck und Zeichen verwerfen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

WAIT_KEY:
        ifdef joerg

        JNB STROB,$
        CLR STROB
        RET

        elseif

        JNB KB_CHAR_READY,$
        CLR KB_CHAR_READY
        RET

        endif
;---------------------------------------------------------------------
; Funktion : Warten bis Tastendruck und Zeichen nach ACC von der
;            Tastatur einlesen.
; Aufrufparameter : -
; Ruechgabeparameter : ACC = Zeichen
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

CHAR_ACC:
        ifdef joerg

        JNB STROB,$
        CLR STROB
        MOV A,ZEICHEN
        RET

        elseif

        JNB KB_CHAR_READY,$
        CLR KB_CHAR_READY
        MOV A,KB_CHAR_BUFFER
        RET

        endif

;---------------------------------------------------------------------
; Funktion : ACC in hex von der Tastatur einlesen.
; Aufrufparameter : -
; Ruechgabeparameter : ACC = Wert
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

IN_ACC:
        PUSH PSW
        PUSH B
        LCALL CHAR_ACC
        LCALL LCD_WRITE_CHAR
        LCALL UPCASE
        CJNE A,#'A',IN_ACC1
IN_ACC1: JC IN_ACC2
        CJNE A,#'G',IN_ACC3
IN_ACC3: JNC IN_ACC2
        CLR C
        SUBB A,#7
IN_ACC2: CLR C
        SUBB A,#30h
        SWAP A
        MOV B,A
        LCALL CHAR_ACC
        LCALL LCD_WRITE_CHAR
        LCALL UPCASE
        CJNE A,#'A',IN_ACC11
IN_ACC11: JC IN_ACC12
        CJNE A,#'G',IN_ACC13
IN_ACC13: JNC IN_ACC12
        CLR C
        SUBB A,#7
IN_ACC12: CLR C
        SUBB A,#30h
        ORL A,B
        POP B
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : DPTR in hex von der Tastatur einlesen.
; Aufrufparameter : -
; Ruechgabeparameter : DPTR = Wert
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

IN_DPTR:
        PUSH ACC
        LCALL IN_ACC
        MOV DPH,A
        LCALL IN_ACC
        MOV DPL,A
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : ACC in hex auf LCD ausgeben.
; Aufrufparameter : ACC = Wert
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

A_LCD:  PUSH ACC
        PUSH ACC
        SWAP A
        ANL A,#00001111B
        ADD A,#'0'
        CJNE A,#':',A_LCD1
A_LCD1: JC A_LCD2
        ADD A,#7
A_LCD2: LCALL LCD_WRITE_CHAR
        POP ACC
        ANL A,#00001111B
        ADD A,#'0'
        CJNE A,#':',A_LCD3
A_LCD3: JC A_LCD4
        ADD A,#7
A_LCD4: LCALL LCD_WRITE_CHAR
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : DPTR in hex auf LCD ausgeben.
; Aufrufparameter : DPTR = Wert
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

DPTR_LCD:
        PUSH ACC
        MOV A,DPH
        LCALL A_LCD
        MOV A,DPL
        LCALL A_LCD
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : Setzt LCD- Status neu
; Aufrufparameter : A = Status
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

LCD_SET_STATUS:

        RET

;=====================================================================
;        END
;---------------------------------------------------------------------

