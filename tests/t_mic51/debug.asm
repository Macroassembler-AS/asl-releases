
; Aufgabe Nr.: Speichermodul fuer uP- Praktikum II
; Autor: Joerg Vollandt
; erstellt am : 30.06.1994
; letzte Aenderung am : 01.07.1994
; Bemerkung : Wird in das gegebene File "MAIN.ASM" includet.
;
; Dateiname : debug.asm
;

;---------------------------------------------------------------------
        SEGMENT CODE
;---------------------------------------------------------------------
; Funktion : Dump ext. RAM.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

N_BYTES EQU     12


DUMP:   PUSH PSW
        PUSH ACC
        PUSH B
        PUSH_DPTR
DUMP_ADR: LCALL LCD_CLEAR
        LCD 0h,"Start="
        LCALL IN_DPTR
DUMP_LINE: LCALL LCD_CLEAR                      ; 1. Zeile
        LCALL DPTR_LCD
        MOV A,#':'
        LCALL LCD_WRITE_CHAR
        MOV B,#N_BYTES
DUMP_BYTE: MOVX A,@DPTR
        LCALL A_LCD
        INC DPTR
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        DJNZ B,DUMP_BYTE

        MOV A,#40h                              ; 2. Zeile
        LCALL LCD_SET_DD_RAM_ADDRESS
        LCALL DPTR_LCD
        MOV A,#':'
        LCALL LCD_WRITE_CHAR
        MOV B,#(N_BYTES-1)
DUMP_BYTE2: MOVX A,@DPTR
        LCALL A_LCD
        INC DPTR
        MOV A,#' '
        LCALL LCD_WRITE_CHAR
        DJNZ B,DUMP_BYTE2
        MOVX A,@DPTR
        LCALL A_LCD
        INC DPTR

        LCALL CHAR_ACC
        LCALL UPCASE
        IFMAKE 'D',LJMP DUMP_ADR
        IFMAKE 'X',SUBB_DPTR N_BYTES
        IFMAKE 'E',SUBB_DPTR (3*N_BYTES)
        IFMAKE 'Q',LJMP DUMP_ENDE

        LJMP DUMP_LINE
DUMP_ENDE:
        POP_DPTR
        POP B
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Ausgabe der Start- und Endadr. des definierten ext. Speichers.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

ADRESSEN:
        PUSH ACC
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Startadr.="
        MOV DPTR,#MEM_ANF
        LCALL DPTR_LCD
        LCD 0Eh,";Endadr.="
        MOV DPTR,#MEM_ENDE
        LCALL DPTR_LCD
        LCD 1bh,";Groesse="
        MOV DPTR,#(MEM_ENDE-MEM_ANF)
        LCALL DPTR_LCD
        LCD 40h,"NEXT_FREE="
        GET_16 NEXT_FREE
        LCALL DPTR_LCD
        LCD 4Eh,";RD_P="
        GET_16 RD_POINTER
        LCALL DPTR_LCD
        LCALL CHECK_SUM
        LCD 5bh,";CHECK="
        MOV DPL,CHECKSUMME
        MOV DPH,CHECKSUMME+1
        LCALL DPTR_LCD
        LCALL WAIT_KEY
        POP_DPTR
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : Ausgabe der Variablen fuer die Synchronisation
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

DEBUG_SYNC:
        PUSH ACC
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"R_Sync-C.="
        MOV a,R_Sync_Counter
        LCALL A_LCD
        jb runningbit,DESYNC1
        LCD 0ch,"not "
DESYNC1: LCD 10h,"running"
        jb Sync_Waiting,DESYNC2
        LCD 18h,"not "
DESYNC2: LCD 1ch,"Sync.waiting"
        LCD 0h,"T_Sync-C.="
        MOV a,T_Sync_Counter
        LCALL A_LCD
        jb Ready_Waiting,DESYNC3
        LCD 4ch,"not "
DESYNC3: LCD 50h,"Ready waiting"
        POP_DPTR
        POP ACC
        LCALL WAIT_KEY
        RET

;---------------------------------------------------------------------
; Funktion : Speicher fuellen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

FUELLEN:
        PUSH PSW
        PUSH ACC
        PUSH B
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Fuellwert ="
        LCALL IN_ACC
        MOV B,A
        MOV DPTR,#MEM_ANF
FUELLEN1: MOV A,B
        MOVX @DPTR,A
        INC DPTR
        MOV A,DPH
        CJNE A,#(MEM_ENDE/256),FUELLEN1
        MOV A,DPL
        CJNE A,#(MEM_ENDE#256),FUELLEN1
        LCD 40h,"Speicher gefuellt."
        LCALL WAIT_KEY
        POP_DPTR
        POP B
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Schreiben in Speicher
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

SCHREIBEN:
        PUSH PSW
        PUSH ACC
        PUSH B
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Schreibadr.="
        LCALL IN_DPTR
        LCD 12h,"Schreibwert="
        LCALL IN_ACC
        LCALL ACC_WR_MEM
        LCD 40h,"Wert geschrieben."
        LCALL WAIT_KEY
        POP_DPTR
        POP B
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Lesen aus Speicher
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

LESEN:
        PUSH PSW
        PUSH ACC
        PUSH B
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Leseadr.="
        LCALL IN_DPTR
        LCALL ACC_RD_MEM
        LCD 40h,"Wert    ="
        LCALL A_LCD
        LCALL WAIT_KEY
        POP_DPTR
        POP B
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Speicher auf PC schreiben
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

WR_PC:
        LCALL LCD_CLEAR
        LCD 0h,"Speicher auf PC"
        LCALL WR_MEM_PC
        LCD 40h,"o.k."
        LCALL WAIT_KEY
        RET

;---------------------------------------------------------------------
; Funktion : Speicher initialisieren
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

SPEICHER_INIT:
        LCALL LCD_CLEAR
        LCD 0h,"Speicher initialisieren"
        LCALL INIT_MEM
        LCD 40h,"o.k."
        LCALL WAIT_KEY
        RET

;---------------------------------------------------------------------
; Funktion : Speicher zum lesen zuruecksetzen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

RESET:
        LCALL LCD_CLEAR
        LCD 0h,"Speicher reset"
        LCALL RESET_MEM
        LCD 40h,"o.k."
        LCALL WAIT_KEY
        RET

;---------------------------------------------------------------------
; Funktion : Element aus Speicher lesen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

GET:
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Element lesen von:"
        GET_16 RD_POINTER
        LCALL DPTR_LCD
        LCALL GET_ELEMENT_MEM
        JC GET_ERROR
        LCD 40h,"o.k. ACC="
        LCALL A_LCD
        LCALL WAIT_KEY
        POP_DPTR
        RET

GET_ERROR:
        LCD 40h,"Fehler."
        LCALL WAIT_KEY
        POP_DPTR
        RET

;---------------------------------------------------------------------
; Funktion : Element in den Speicher schreiben
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

PUT:
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Element schreiben ab:"
        GET_16 NEXT_FREE
        LCALL DPTR_LCD
        LCD 20h,"ACC="
        LCALL IN_ACC

        LCALL PUT_ELEMENT_MEM
        JC PUT_ERROR
        LCD 40h,"o.k."
        LCALL WAIT_KEY
        POP_DPTR
        RET

PUT_ERROR:
        LCD 40h,"Fehler."
        LCALL WAIT_KEY
        POP_DPTR
        RET

;---------------------------------------------------------------------
; Funktion : Speicher von PC lesen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

RD_PC:
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"Speicher von PC"
        CLR READY
        LCALL RD_MEM_PC
RD_PC_LOOP:
        JNB MSG,$
        LCALL MEM_SCHEDULER
        JNB READY,RD_PC_LOOP
        JB CRC,RD_PC_ERROR
        LCD 10h,"o.k."
        LCALL WAIT_KEY
        POP_DPTR
        RET

RD_PC_ERROR:
        LCD 10h,"CRC- Fehler beim laden."
        LCALL WAIT_KEY
        POP_DPTR
        RET

;---------------------------------------------------------------------
; Funktion : Zeiger setzen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

SET_NEXT_FREE:
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"next_free="
        lcall IN_DPTR
        SET_16 NEXT_FREE
        LCALL WAIT_KEY
        POP_DPTR
        RET

SET_RD_POINTER:
        PUSH_DPTR
        LCALL LCD_CLEAR
        LCD 0h,"RD_POINTER="
        lcall IN_DPTR
        SET_16 RD_POINTER
        LCALL WAIT_KEY
        POP_DPTR
        RET

;---------------------------------------------------------------------
; Funktion : Msg_Handler_Test
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
; ****************************************************************************
; R0    Empf„nger (logische Adresse)
; R1    Message
; R2 - R7 Parameter
; ****************************************************************************

Msg_Handler_Test:
        PUSH ACC
        PUSH AR0
        PUSH AR1
        LCALL LCD_CLEAR
        LCD 0h,"Msg_Handler_Test"
        LCD 16h,"R0="
        LCALL IN_ACC
        MOV R0,A
        LCD 20h,"R1="
        LCALL IN_ACC
        MOV R1,A
        LCALL Dispatch_Msg
        POP AR1
        POP AR0
        POP ACC
        ret

;---------------------------------------------------------------------
; Funktion : Testbit toggeln
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

T_TEST:
        LCALL LCD_CLEAR
        LCD 0h,"TEST"
        LCALL WAIT_KEY
        CPL TESTBIT
        ret

;---------------------------------------------------------------------
; Funktion : Test RUN- Manager
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

TEST_RUN:
        LCD 0h,"TEST RUN_ Manager"
        LCALL WAIT_KEY
        LCALL START_RUNNING
        LCALL LCD_CLEAR
        ret

;---------------------------------------------------------------------
; Funktion : Debuger fuer das Speichermodul
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

DEBUG_MEM:
        mov a,#1
        lcall lcd_curser_onoff

DEBUG_START:
        LCALL LCD_CLEAR
        LCD 0,"DEBUG SPEICHERMODUL "
DEBUG_LOOP:
        JNB KB_CHAR_READY,DEBUG3
        LJMP DEBUG1
DEBUG3:
        LJMP DEBUG_LOOP

DEBUG1: CLR KB_CHAR_READY
        MOV A,KB_CHAR_BUFFER
        LCALL LCD_WRITE_CHAR
        LCALL UPCASE
        IFCALL 'A',ADRESSEN
        IFCALL 'B',LOAD_ROB
        IFCALL 'C',LOAD_FRAES
        IFCALL 'D',DUMP
        IFCALL 'E',Msg_Handler_Test
        IFCALL 'F',FUELLEN
        IFCALL 'G',GET
        IFCALL 'H',SAVE_ROB
        IFCALL 'I',SAVE_FRAES
        IFCALL 'J',SPEICHER_INIT
        IFCALL 'K',T_TEST
        IFCALL 'L',LESEN
        IFCALL 'M',STORE_ROB
        IFCALL 'O',RESET
        IFCALL 'P',PUT
        IFMAKE 'Q',LJMP DEBUG_ENDE
        IFCALL 'R',RD_PC
        IFCALL 'S',SCHREIBEN
        IFCALL 'T',TEST_RUN
        IFCALL 'U',SET_RD_POINTER
        IFCALL 'W',WR_PC
        IFCALL 'X',DEBUG_SYNC
        IFCALL 'Y',STORE_SYNC
        IFCALL 'Z',SET_NEXT_FREE

        ljmp DEBUG_START

DEBUG_ENDE:

        LCALL LCD_CLEAR
        ret

;=====================================================================
;        END
;---------------------------------------------------------------------


