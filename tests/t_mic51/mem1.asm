
; Aufgabe Nr.: Speichermodul fuer uP- Praktikum II
; Autor: Joerg Vollandt
; erstellt am : 21.05.1994
; letzte Aenderung am : 02.08.1994
; Bemerkung : Routinen fuer die Speicherverwaltung
;
; Dateiname : mem1.asm
;

;=====================================================================
; Definitionen der Funktionen des Seichermoduls

;INIT_MEM        Initialisieren des Speichermoduls
;DEINIT_MEM      Deinitialisieren des Speichermoduls
;CLEAR_MEM       Speicher loeschen
;RESET_MEM       Speicher zum lesen zuruecksetzen
;PUT_ELEMENT_MEM Element anf naechste freie Position schreiben
;GET_ELEMENT_MEM Element von akt. Position lesen
;WR_MEM_PC       Speicher auf dem PC speichern.
;RD_MEM_PC       Speicher vom PC laden.

;------------------------------------------------------------------------------

;Messagedefinitionen

;1.Dateityp (Bit 0 und Bit 1)
Msg_PC_To_Net           equ     00b             ;direkte Eingabe von Hex-Messages fr das Netz
Msg_Frs_Datei           equ     01b             ;Fr„stischdatei
Msg_Rob_Teach_Datei     equ     10b             ;Roboter-Teach-In-Datei
Msg_Frs_Teach_Datei     equ     11b             ;Fr„stisch-Teach-In-Datei

;2.Aktion (Bit 2 und Bit 3)
Msg_PC_Get              equ     0000b           ;Rekordanfrage an PC
Msg_PC_Put              equ     0100b           ;Rekordspeichern Slave=>PC, Rekordausgabe PC=>Slave
Msg_PC_Reset            equ     1000b           ;PC Datei ”ffnen zum Lesen
Msg_PC_Rewrite          equ     1100b           ;PC Datei ”ffnen zum Schreiben

;3.Slaveadresse Slave=>PC ; Msg_From_PC PC=>Slave
Msg_From_PC             equ     00000000b       ;Antwort auf Anfrage

EOF_Record              equ     0ffffh          ;
PC_Slave_Adr            equ     0eh             ;

;------------------------------------------------------------------------------
; Speicherdefinitionen


        SEGMENT DATA

CHECKSUMME      DW      ?


        SEGMENT XDATA

POINTER         DW      ?               ; fuer Test
NEXT_FREE       DW      ?
RD_POINTER      DW      ?
BLOCKNR         DW      ?

MEM_ANF         DB      1000 DUP (?)      ; Speichergroesse in Bytes
MEM_ENDE

;---------------------------------------------------------------------
        SEGMENT CODE
;---------------------------------------------------------------------
; Funktion : Initialisieren des Speichermoduls
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

INIT_MEM:
        PUSH PSW
        PUSH_DPTR
        MOV DPTR,#MEM_ANF
        SET_16 NEXT_FREE
        SET_16 RD_POINTER
        MOV CHECKSUMME,#0
        MOV CHECKSUMME+1,#0
        CLR READY
        CLR CRC

        POP_DPTR
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Deinitialisieren des Speichermoduls
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

DEINIT_MEM:

        RET

;---------------------------------------------------------------------
; Funktion : Speicher loeschen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

CLEAR_MEM:
        PUSH PSW
        PUSH_DPTR
        MOV DPTR,#MEM_ANF
        SET_16 NEXT_FREE
        SET_16 RD_POINTER
        MOV CHECKSUMME,#0
        MOV CHECKSUMME+1,#0
        CLR READY
        CLR CRC

        POP_DPTR
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Speicher zum lesen zuruecksetzen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

RESET_MEM:
        PUSH_DPTR
        MOV DPTR,#MEM_ANF
        SET_16 RD_POINTER

        POP_DPTR
        RET

;---------------------------------------------------------------------
; Funktion : Speicher von MEM_ANF bis NEXT_FREE auf dem PC speichern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

WR_MEM_PC:
        PUSH_ALL
        ; MOV A,#MSG_PC_REWRITE+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_REWRITE
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr                       ; Datei oeffnen
        MOV B,#8
        SEND_NET

        ; MOV A,#MSG_PC_PUT+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_PUT
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr                       ; Header zusammenstellen
        MOV B,#8
        MOV DPTR,#0
        SET_16 BLOCKNR                  ; Blocknr.=0 setzen
        MOV R1,DPL                      ; Blocknr.
        MOV R2,DPH
        GET_16 NEXT_FREE
        SUBB_DPTR MEM_ANF
        MOV R3,DPL                      ; Anzahl Bytes
        MOV R4,DPH
        LCALL CHECK_SUM
        MOV R5,CHECKSUMME               ; Pruefsumme
        MOV R6,CHECKSUMME+1
        SEND_NET                        ; Header senden

        MOV DPTR,#MEM_ANF
        SET_16 POINTER                  ; Zeiger auf MEM_ANF setzen

WR_MEM_MSG:
        LCALL CHECK_RD_POINTER          ; Pointer in DPTR!!!
        JNC WR_MEM_MSG1
        LJMP WR_MEM_CLOSE               ; keine Bytes mehr -> close datei
WR_MEM_MSG1:
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        MOV R3,A                        ; Message aufbauen
        LCALL CHECK_RD_POINTER          ; Pointer in DPTR!!!
        JNC WR_MEM_MSG2
        LJMP WR_MEM_REST                ; keine Bytes mehr -> Rest schreiben
WR_MEM_MSG2:
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        MOV R4,A                        ; Message aufbauen
        LCALL CHECK_RD_POINTER          ; Pointer in DPTR!!!
        JNC WR_MEM_MSG3
        LJMP WR_MEM_REST                ; keine Bytes mehr -> Rest schreiben
WR_MEM_MSG3:
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        MOV R5,A                        ; Message aufbauen
        LCALL CHECK_RD_POINTER          ; Pointer in DPTR!!!
        JNC WR_MEM_MSG4
        LJMP WR_MEM_REST                ; keine Bytes mehr -> Rest schreiben
WR_MEM_MSG4:
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        MOV R6,A                        ; Message aufbauen
        LCALL CHECK_RD_POINTER          ; Pointer in DPTR!!!
        JNC WR_MEM_MSG5
        LJMP WR_MEM_REST                ; keine Bytes mehr -> Rest schreiben
WR_MEM_MSG5:
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        MOV R7,A                        ; Message aufbauen
        PUSH_DPTR
        GET_16 BLOCKNR
        INC DPTR
        SET_16 BLOCKNR                  ; Blocknr.=+1 setzen
        MOV R1,DPL                      ; Blocknr.
        MOV R2,DPH
        POP_DPTR
        ; MOV A,#MSG_PC_PUT+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_PUT
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr
        MOV B,#8
        SEND_NET                        ; Message senden
        LJMP WR_MEM_MSG                 ; naechste Message

WR_MEM_REST:
        PUSH_DPTR                       ; nicht volle MSG schreiben
        GET_16 BLOCKNR
        INC DPTR
        SET_16 BLOCKNR                  ; Blocknr.=+1 setzen
        MOV R1,DPL                      ; Blocknr.
        MOV R2,DPH
        POP_DPTR
        ; MOV A,#MSG_PC_PUT+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_PUT
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr
        MOV B,#8
        SEND_NET                                ; Message senden

WR_MEM_CLOSE:
        ; MOV A,#MSG_PC_PUT+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_PUT
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV         A,#PC_Slave_Adr            ; Datei schlieáen
        MOV         B,#8
        MOV         R1,#(EOF_RECORD#256)
        MOV         R2,#(EOF_RECORD/256)
        SEND_NET
        POP_ALL
        RET

;---------------------------------------------------------------------
; Funktion : Speicher vom PC laden.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

RD_MEM_PC:
        PUSH_ALL
        ; MOV A,#MSG_PC_RESET+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_RESET
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr                       ; Datei oeffnen
        MOV B,#8
        SEND_NET
        ; MOV A,#MSG_PC_GET+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_GET
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr                       ; Header laden
        MOV B,#8
        MOV DPTR,#0
        SET_16 BLOCKNR                          ; Blocknr.=0 setzen
        MOV R1,DPL                              ; Blocknr.
        MOV R2,DPH
        SEND_NET                                ; Header anfordern
        POP_ALL
        RET


GET_FROM_PC:
        PUSH_ALL
        CJNE R1,#0,GET_NO_HEADER1       ; wenn Blocknr.=0, dann
        CJNE R2,#0,GET_NO_HEADER1       ; Header
        LJMP GET_HEADER
GET_NO_HEADER1:
        LJMP GET_NO_HEADER
GET_HEADER:
        CJNE R3,#0,GET_NOT_EMPTY_JMP    ; testen ob 0 Bytes in Datei
        CJNE R4,#0,GET_NOT_EMPTY_JMP
        LJMP GET_EMPTY
GET_NOT_EMPTY_JMP:
        LJMP GET_NOT_EMPTY

GET_EMPTY:                              ; Datei leer
        LCALL INIT_MEM                  ; Speicherreset
        ; MOV A,#MSG_PC_PUT+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                          ; *16
        ADD A,#MSG_PC_PUT
        MOVB ACC.0,P3.5                 ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV         A,#PC_Slave_Adr     ; und Datei schlieáen
        MOV         B,#8
        MOV         R1,#(EOF_RECORD#256)
        MOV         R2,#(EOF_RECORD/256)
        SEND_NET
        POP_ALL
        SETB READY
        LCD 40H,"Teachin- Datei leer.                    "
        RET

GET_NOT_EMPTY:                          ; Datei nicht leer
        MOV DPL,R3                      ; Groesse nach DPTR
        MOV DPH,R4
        ADD_DPTR MEM_ANF
        SET_16 NEXT_FREE                ; neues Speicherende setzen
        MOV CHECKSUMME,R5               ; neue Checksumme laden
        MOV CHECKSUMME+1,R6
        PUSH_DPTR
        GET_16 BLOCKNR
        INC DPTR
        SET_16 BLOCKNR                  ; Blocknr.=+1 setzen
        MOV R1,DPL                      ; Blocknr.
        MOV R2,DPH
        POP_DPTR
        ; MOV A,#MSG_PC_GET+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_GET
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr
        MOV B,#8
        SEND_NET                        ; 1. Block anfordern

        MOV DPTR,#MEM_ANF
        SET_16 POINTER                  ; Zeiger auf MEM_ANF setzen

        POP_ALL
        RET

GET_NO_HEADER:
        GET_16 POINTER                  ; Schreibzeiger laden
        MOV A,R3
        LCALL ACC_WR_MEM                ; in den Speicher schreiben
        LCALL CHECK_RD_POINTER          ; pruefen ob noch Bytes in der Datei
        JNC GET_MORE2
        LJMP GET_CLOSE                  ; wenn nicht Datei schliessen
GET_MORE2:
        MOV A,R4
        LCALL ACC_WR_MEM                ; in den Speicher schreiben
        LCALL CHECK_RD_POINTER          ; pruefen ob noch Bytes in der Datei
        JNC GET_MORE3
        LJMP GET_CLOSE                  ; wenn nicht Datei schliessen
GET_MORE3:
        MOV A,R5
        LCALL ACC_WR_MEM                ; in den Speicher schreiben
        LCALL CHECK_RD_POINTER          ; pruefen ob noch Bytes in der Datei
        JNC GET_MORE4
        LJMP GET_CLOSE                  ; wenn nicht Datei schliessen
GET_MORE4:
        MOV A,R6
        LCALL ACC_WR_MEM                ; in den Speicher schreiben
        LCALL CHECK_RD_POINTER          ; pruefen ob noch Bytes in der Datei
        JNC GET_MORE5
        LJMP GET_CLOSE                  ; wenn nicht Datei schliessen
GET_MORE5:
        MOV A,R7
        LCALL ACC_WR_MEM                ; in den Speicher schreiben
        LCALL CHECK_RD_POINTER          ; pruefen ob noch Bytes in der Datei
        JNC GET_MORE6
        LJMP GET_CLOSE                  ; wenn nicht Datei schliessen
GET_MORE6:
        SET_16 POINTER
        GET_16 BLOCKNR
        INC DPTR
        SET_16 BLOCKNR                  ; Blocknr.=+1 setzen
        MOV R1,DPL                      ; Blocknr.
        MOV R2,DPH
        ; MOV A,#MSG_PC_GET+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_GET
        MOVB ACC.0,P3.5                    ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV A,#PC_Slave_Adr
        MOV B,#8
        SEND_NET                        ; naechsten Block anfordern
        POP_ALL
        RET


GET_CLOSE:
        ; MOV A,#MSG_PC_PUT+MY_SLAVE_ADR1*16
        MOV A,MY_SLAVE_ADR
        SWAP A                             ; *16
        ADD A,#MSG_PC_PUT
        MOVB ACC.0,P3.5                      ; Datei fuer Roboter oder Fraese
        SETB ACC.1
        MOV R0,A
        MOV         A,#PC_Slave_Adr               ; und Datei schlieáen
        MOV         B,#8
        MOV         R1,#(EOF_RECORD#256)
        MOV         R2,#(EOF_RECORD/256)
        SEND_NET
        MOV R0,CHECKSUMME
        MOV A,CHECKSUMME+1
        LCALL CHECK_SUM
        CJNE A,CHECKSUMME+1,GET_CRC_ERROR
        MOV A,R0
        CJNE A,CHECKSUMME,GET_CRC_ERROR
        POP_ALL
        CLR CRC
        SETB READY
        LCD 40H,"Teachin- Datei fehlerfrei geladen.      "
        RET

GET_CRC_ERROR:
        POP_ALL
        SETB CRC
        SETB READY
        LCD 40H,"FEHLER bei Laden der Teachin- Datei.    "
        RET

;---------------------------------------------------------------------
; Funktion : Testen ob DPTR zum LESEN auf belegten Speicher zeigt.
;               C=0 ==> MEM_ANF <= DPTR < NEXT_FREE
;               C=1 ==> sonst
; Aufrufparameter : DPTR = Pointer
; Ruechgabeparameter : -
; Veraenderte Register : PSW
; Stackbedarf :
; Zeitbedarf :
;

CHECK_RD_POINTER:
        PUSH PSW
        PUSH ACC
        MOV A,#((MEM_ANF-1)/256)
        CJNE A,DPH,CH_RD1               ; Test ob Pointer >= MEM_ANF
CH_RD1: JC CH_RD_OK1
        CJNE A,DPH,CH_RD_ERROR          ;
        MOV A,#((MEM_ANF-1)#256)
        CJNE A,DPL,CH_RD2
CH_RD2: JC CH_RD_OK1
        LJMP CH_RD_ERROR                ;
CH_RD_OK1:
        PUSH_DPTR
        MOV DPTR,#(NEXT_FREE+1)
        MOVX A,@DPTR
        POP_DPTR
        CJNE A,DPH,CH_RD3               ; Test ob Pointer < NEXT_FREE
CH_RD3: JC CH_RD_ERROR
        CJNE A,DPH,CH_RD_OK2            ;
        PUSH_DPTR
        MOV DPTR,#NEXT_FREE
        MOVX A,@DPTR
        POP_DPTR
        CJNE A,DPL,CH_RD4
CH_RD4: JC CH_RD_ERROR
        CJNE A,DPL,CH_RD_OK2
        LJMP CH_RD_ERROR                ;

CH_RD_OK2:
        POP ACC
        POP PSW
        CLR C                           ; o.k.
        RET

CH_RD_ERROR:
        POP ACC
        POP PSW
        SETB C                          ; Fehler
        RET

;---------------------------------------------------------------------
; Funktion : Testen ob DPTR zum SCHREIBEN auf belegten Speicher zeigt.
;               C=0 ==> MEM_ANF <= DPTR <= NEXT_FREE
;               C=1 ==> sonst
; Aufrufparameter : DPTR = Pointer
; Ruechgabeparameter : -
; Veraenderte Register : PSW
; Stackbedarf :
; Zeitbedarf :
;

CHECK_WR_POINTER:
        PUSH PSW
        PUSH ACC
        MOV A,#((MEM_ANF-1)/256)
        CJNE A,DPH,CH_WR1               ; Test ob Pointer >= MEM_ANF
CH_WR1: JC CH_WR_OK1
        CJNE A,DPH,CH_WR_ERROR          ;
        MOV A,#((MEM_ANF-1)#256)
        CJNE A,DPL,CH_WR2
CH_WR2: JC CH_WR_OK1
        LJMP CH_WR_ERROR                ;
CH_WR_OK1:
        PUSH_DPTR
        MOV DPTR,#(NEXT_FREE+1)
        MOVX A,@DPTR
        POP_DPTR
        CJNE A,DPH,CH_WR3               ; Test ob Pointer <= NEXT_FREE
CH_WR3: JC CH_WR_ERROR
        CJNE A,DPH,CH_WR_OK2            ;
        PUSH_DPTR
        MOV DPTR,#NEXT_FREE
        MOVX A,@DPTR
        POP_DPTR
        CJNE A,DPL,CH_WR4
CH_WR4: JNC CH_WR_OK2
        LJMP CH_WR_ERROR                ;

CH_WR_OK2:
        POP ACC
        POP PSW
        CLR C                           ; o.k.
        RET

CH_WR_ERROR:
        POP ACC
        POP PSW
        SETB C                          ; Fehler
        RET

;---------------------------------------------------------------------
; Funktion : Testen ob DPTR < MEM_ENDE.
;               C=0 ==> DPTR < MEM_ENDE
;               C=1 ==> sonst
; Aufrufparameter : DPTR = Pointer
; Ruechgabeparameter : -
; Veraenderte Register : PSW
; Stackbedarf :
; Zeitbedarf :
;

CHECK_EOM_POINTER:
        PUSH PSW
        PUSH ACC
        MOV A,#(MEM_ENDE/256)
        CJNE A,DPH,CH_EOM3               ; Test ob Pointer < MEM_ENDE
CH_EOM3: JC CH_EOM_ERROR
        CJNE A,DPH,CH_EOM_OK2            ;
        MOV A,#(MEM_ENDE#256)
        CJNE A,DPL,CH_EOM4
CH_EOM4: JC CH_EOM_ERROR
        CJNE A,DPL,CH_EOM_OK2
        LJMP CH_EOM_ERROR                ;

CH_EOM_OK2:
        POP ACC
        POP PSW
        CLR C                           ; o.k.
        RET

CH_EOM_ERROR:
        POP ACC
        POP PSW
        SETB C                          ; Fehler
        RET

;---------------------------------------------------------------------
; Funktion : ACC in den Speicher schreiben, DPTR increminieren.
; Aufrufparameter : ACC = Wert, DPTR = Pointer
; Ruechgabeparameter : -
; Veraenderte Register : DPTR
; Stackbedarf :
; Zeitbedarf :
;

ACC_WR_MEM:
        MOVX @DPTR,A
        INC DPTR
        RET


;---------------------------------------------------------------------
; Funktion : ACC aus dem Speicher lesen, DPTR increminieren.
; Aufrufparameter : DPTR = Pointer
; Ruechgabeparameter : ACC = Wert
; Veraenderte Register : ACC, DPTR
; Stackbedarf :
; Zeitbedarf :
;

ACC_RD_MEM:
        MOVX A,@DPTR
        INC DPTR
        RET

;---------------------------------------------------------------------
; Funktion : Pruefsumme ueber den Speicher bilden.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

CHECK_SUM:
        PUSH PSW
        PUSH ACC
        PUSH_DPTR
        MOV CHECKSUMME,#0
        MOV CHECKSUMME+1,#0
        MOV DPTR,#MEM_ANF               ; Pointer auf MEM_ANF setzen
CHECK_SUM1:
        LCALL CHECK_RD_POINTER          ; Pointer in DPTR!!!
        JC CHECK_SUM_ENDE
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        ADD A,CHECKSUMME
        MOV CHECKSUMME,A
        MOV A,#0
        ADDC A,CHECKSUMME+1
        MOV CHECKSUMME+1,A
        LJMP CHECK_SUM1

CHECK_SUM_ENDE:
        POP_DPTR
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Element in den Speicher auf die naechste frei Position schreiben.
; Aufrufparameter : ACC = Wert
; Ruechgabeparameter : C=0 ==> o.k., C=1 ==> Speicherueberlauf
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

PUT_ELEMENT_MEM:
        PUSH PSW
        GET_16 NEXT_FREE
        LCALL CHECK_EOM_POINTER         ; testen ob DPTR < MEM_ENDE
        JC GET_ELEMENT_ERROR            ; wenn nicht Fehler
        LCALL CHECK_WR_POINTER          ; testen ob MEM_ANF <= DPTR <= NEXT_FREE
        JC PUT_ELEMENT_ERROR            ; wenn nicht Fehler
        LCALL ACC_WR_MEM                ; Byte aus MEM lesen
        SET_16 NEXT_FREE

PUT_EL_OK1:
        POP PSW
        CLR C
        RET

PUT_ELEMENT_ERROR:
        POP PSW
        SETB C
        RET

        RET

;---------------------------------------------------------------------
; Funktion : Element von der akt. Position aus dem Speicher lesen.
; Aufrufparameter : -
; Ruechgabeparameter : ACC = Wert
;                      C=0 ==> o.k., C=1 ==> Schreib- gleich Lesezeiger
;                      oder Lesezeiger ausserhalb des gueltigen Bereiches
; Veraenderte Register : ACC, PSW
; Stackbedarf :
; Zeitbedarf :
;

GET_ELEMENT_MEM:
        PUSH PSW
        GET_16 RD_POINTER
        LCALL CHECK_EOM_POINTER         ; testen ob DPTR < MEM_ENDE
        JC GET_ELEMENT_ERROR            ; wenn nicht Fehler
        LCALL CHECK_RD_POINTER          ; testen ob MEM_ANF <= DPTR < NEXT_FREE
        JC GET_ELEMENT_ERROR            ; wenn nicht Fehler
        LCALL ACC_RD_MEM                ; Byte aus MEM lesen
        SET_16 RD_POINTER

GET_EL_OK1:
        POP PSW
        CLR C
        RET

GET_ELEMENT_ERROR:
        POP PSW
        SETB C
        RET

;=====================================================================
;        END
;---------------------------------------------------------------------

