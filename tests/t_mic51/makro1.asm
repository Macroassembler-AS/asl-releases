
; Aufgabe Nr.: Speichermodul fuer uP- Praktikum II
; Autor: Joerg Vollandt
; erstellt am : 21.05.1994
; letzte Aenderung am : 01.08.1994
; Bemerkung : Makros
;
; Dateiname : makro1.asm
;

;---------------------------------------------------------------------
; Funktion : Direkter Bitmove- Befehl
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : PSW
; Stackbedarf :
; Zeitbedarf :
;

MOVB    MACRO   ZIEL,QUELLE

        MOV C,QUELLE
        MOV ZIEL,C

        ENDM

;---------------------------------------------------------------------
; Funktion : String auf LCD ausgaben.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

LCD     MACRO   POS,STRG

        PUSH ACC
        PUSH DPH
        PUSH DPL
        MOV A,#POS
        LCALL LCD_SET_DD_RAM_ADDRESS
        MOV DPTR,#STR_ADR
        LCALL LCD_WRITE_STRING
        LJMP WEITER

STR_ADR   DB    STRG,0

WEITER: POP DPL
        POP DPH
        POP ACC

        ENDM

;---------------------------------------------------------------------
; Funktion : A, B, PSW, DPTR, R0 - R7 auf Stack retten
; Aufrufparameter : PUSH_ALL
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf : 2
; Zeitbedarf :
;

PUSH_ALL        MACRO

        PUSH ACC
        PUSH B
        PUSH PSW
        PUSH_DPTR
        PUSH AR0
        PUSH AR1
        PUSH AR2
        PUSH AR3
        PUSH AR4
        PUSH AR5
        PUSH AR6
        PUSH AR7

        ENDM

;---------------------------------------------------------------------
; Funktion : A, B, PSW, DPTR, R0 - R7 von Stack holen
; Aufrufparameter : POP_ALL
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf : 2
; Zeitbedarf :
;

POP_ALL        MACRO

        POP AR7
        POP AR6
        POP AR5
        POP AR4
        POP AR3
        POP AR2
        POP AR1
        POP AR0
        POP_DPTR
        POP PSW
        POP B
        POP ACC

        ENDM

;---------------------------------------------------------------------
; Funktion : DPTR pushen und popen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

PUSH_DPTR       MACRO

        PUSH DPL
        PUSH DPH

        ENDM

POP_DPTR        MACRO

        POP DPH
        POP DPL

        ENDM

;---------------------------------------------------------------------
; Funktion : DPTR decreminieren.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
        ifdef joerg
DEC_DPTR       MACRO

        INC DPL
        DJNZ DPL,DEC_DPTR1
        DEC DPH
DEC_DPTR1:
        DEC DPL

        ENDM

        endif

;---------------------------------------------------------------------
; Funktion : Addieren und subtraieren mit DPTR.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

ADD_DPTR       MACRO    WERT

        PUSH PSW
        PUSH ACC
        MOV A,#(WERT#256)
        ADD A,DPL
        MOV DPL,A
        MOV A,#(WERT/256)
        ADDC A,DPH
        MOV DPH,A
        POP ACC
        POP PSW

        ENDM


SUBB_DPTR       MACRO    WERT

        PUSH PSW
        PUSH ACC
        MOV A,DPL
        CLR C
        SUBB A,#(WERT#256)
        MOV DPL,A
        MOV A,DPH
        SUBB A,#(WERT/256)
        MOV DPH,A
        POP ACC
        POP PSW

        ENDM

;---------------------------------------------------------------------
; Funktion : Rechnen mit 16- Bit- Werten im ext. RAM (L,H).
; Aufrufparameter : DPTR = Wert
; Ruechgabeparameter : DPTR = Wert
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

SET_16  MACRO   NAME

        PUSH ACC
        PUSH_DPTR
        PUSH DPH
        PUSH DPL
        MOV DPTR,#NAME
        POP ACC
        MOVX @DPTR,A
        INC DPTR
        POP ACC
        MOVX @DPTR,A
        POP_DPTR
        POP ACC

        ENDM

GET_16  MACRO   NAME

        PUSH ACC
        MOV DPTR,#NAME
        MOVX A,@DPTR
        PUSH ACC
        INC DPTR
        MOVX A,@DPTR
        MOV DPH,A
        POP DPL
        POP ACC

        ENDM

;---------------------------------------------------------------------
; Funktion : Scheduler.
; Aufrufparameter : ACC = Zeichen
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

IFCALL  MACRO   CONST,ROUTINE

        CJNE A,#CONST,IFCALL1
        LCALL ROUTINE
IFCALL1:

        ENDM

IFMAKE  MACRO   CONST,CODE

        CJNE A,#CONST,IFMAKE1
        CODE
IFMAKE1:

        ENDM

;---------------------------------------------------------------------
; Funktion : Warten bis Netzwerk freiund Message senden.
; Aufrufparameter : ACC = Zeichen
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

SEND_NET  MACRO

        push acc
SEND_NET1: LCALL READ_STATUS
        JB ACC.1,SEND_NET1
        pop acc
        LCALL SEND_MESSAGE

        ENDM

;---------------------------------------------------------------------
; Funktion : Message senden.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

post_Message1 macro Modul,Msg

;        if MY_SLAVE_ADR = uC_Modul
;          call ADR_Msg                   ; interne Message
;        elseif
        PUSH ACC
WAIT_NET: LCALL READ_STATUS
        JB ACC.1,WAIT_NET
        PUSH DPL
        PUSH DPH
        MOV DPTR,#ModulNetAdr_Tab
        MOV A,#Modul
        MOVC A,@A+DPTR
        POP DPH
        POP DPL
        MOV R0,#Modul
        MOV R1,#Msg
        LCALL SEND_MESSAGE              ; Message ins Netz
        POP ACC

;        endif
        endm

;---------------------------------------------------------------------
; Funktion : Message senden, alle Parameter im Mkroaufruf, B automatisch.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : R0- R7
; Stackbedarf :
; Zeitbedarf :
;

post_Message2 macro Modul,Msg,PARA1,PARA2,PARA3,PARA4,PARA5,PARA6

Parameteranzahl SET     2                       ; min. Modulnr. und Msg.-Nr.

        PUSH ACC
        PUSH B

        IF "PARA1"<>""
        MOV R2,PARA1
Parameteranzahl SET     Parameteranzahl+1
        ENDIF
        IF "PARA2"<>""
        MOV R3,PARA2
Parameteranzahl SET     Parameteranzahl+1
        ENDIF
        IF "PARA3"<>""
        MOV R4,PARA3
Parameteranzahl SET     Parameteranzahl+1
        ENDIF
        IF "PARA4"<>""
        MOV R5,PARA4
Parameteranzahl SET     Parameteranzahl+1
        ENDIF
        IF "PARA5"<>""
        MOV R6,PARA5
Parameteranzahl SET     Parameteranzahl+1
        ENDIF
        IF "PARA6"<>""
        MOV R7,PARA6
Parameteranzahl SET     Parameteranzahl+1
        ENDIF

        PUSH DPL
        PUSH DPH
        MOV DPTR,#ModulNetAdr_Tab
        MOV A,Modul
        MOVC A,@A+DPTR
        POP DPH
        POP DPL
        MOV R0,Modul
        MOV R1,Msg
        MOV B,#Parameteranzahl
        PUSH ACC
WAIT_NET: LCALL READ_STATUS
        JB ACC.1,WAIT_NET
        POP ACC
        LCALL SEND_MESSAGE              ; Message ins Netz

        POP B
        POP ACC

        endm

;---------------------------------------------------------------------
; Funktion : Message ausgeben
; Aufrufparameter : wie definiert
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

TEST_MESSAGE_HEX    MACRO   POS

        PUSH ACC
        MOV A,#POS
        LCALL LCD_SET_DD_RAM_ADDRESS
        POP ACC
        PUSH ACC
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,B
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R0
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R1
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R2
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R3
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R4
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R5
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R6
        LCALL A_LCD
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        MOV A,R7
        LCALL A_LCD
        POP ACC

        ENDM

;---------------------------------------------------------------------
; Funktion : Fehlerbehandlung
; Aufrufparameter : Fehlernr.
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

ERROR   MACRO   NR


        ENDM

;---------------------------------------------------------------------










;---------------------------------------------------------------------
TEST_MESSAGE    MACRO   POS,SCHALTER

        IF SCHALTER<=TEST_LEVEL
          PUSH ACC
          MOV A,#POS
          LCALL LCD_SET_DD_RAM_ADDRESS
          MOV A,R0
          LCALL LCD_WRITE_CHAR
          MOV A,R1
          LCALL LCD_WRITE_CHAR
          MOV A,R2
          LCALL LCD_WRITE_CHAR
          MOV A,R3
          LCALL LCD_WRITE_CHAR
          MOV A,R4
          LCALL LCD_WRITE_CHAR
          MOV A,R5
          LCALL LCD_WRITE_CHAR
          MOV A,R6
          LCALL LCD_WRITE_CHAR
          MOV A,R7
          LCALL LCD_WRITE_CHAR
          POP ACC
        ENDIF
        ENDM
;---------------------------------------------------------------------
MAKE_MESSAGE    MACRO   ADR,STRG

        IF 0=0
          MOV A,#0
          MOV DPTR,#STR_ADR
          MOVC A,@A+DPTR
          MOV R0,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R1,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R2,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R3,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R4,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R5,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R6,A
          MOV A,#0
          INC DPTR
          MOVC A,@A+DPTR
          MOV R7,A
          MOV A,#ADR
          MOV B,#8
          LJMP WEITER

STR_ADR   DB    STRG

WEITER:   NOP
        ENDIF
        ENDM

;---------------------------------------------------------------------
MAKE_MESSAGE_HEX    MACRO   ADR,L,A0,A1,A2,A3,A4,A5,A6,A7

        IF 0=0
          MOV R0,#A0
          MOV R1,#A1
          MOV R2,#A2
          MOV R3,#A3
          MOV R4,#A4
          MOV R5,#A5
          MOV R6,#A6
          MOV R7,#A7
          MOV A,#ADR
          MOV B,#L
        ENDIF
        ENDM

;---------------------------------------------------------------------

