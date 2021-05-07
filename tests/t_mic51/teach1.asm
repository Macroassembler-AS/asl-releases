
; Aufgabe Nr.: Teach- In Einheit fuer uP- Praktikum II
; Autor: Joerg Vollandt
; erstellt am : 31.05.1994
; letzte Aenderung am : 14.07.1994
; Bemerkung :
;
; Dateiname : teach1.asm
;

;=====================================================================
; Definitionen der Funktionen der Teach- In Einheit

;INIT_TEACH      Initialisieren der Teach- In Einheit
;DEINIT_TEACH    Deinitialisieren der Teach- In Einheit
;CLEAR_TEACH     Speicher loeschen
;RESET_TEACH     Speicher zum lesen zuruecksetzen
;STORE_ROB       Position Roboter speichern
;STORE_FRAES     Position Fraese speichern
;SYNC
;LOAD_ROB        Roboter Teach- In Datei von PC laden
;LOAD_FRAES      Fraese- Teach- In Datei von PC laden
;LOAD            Beide Teach- In Dateien von PC laden
;SAVE_ROB        Roboter Teach- In Datei auf PC speichern
;SAVE_FRAES      Fraese- Teach- In Datei auf PC speichern

;------------------------------------------------------------------------------
        SEGMENT CODE
;---------------------------------------------------------------------
; Funktion : Initialisieren der Error- und Warningeinspruenge fuer die
;            Fraese.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

INIT_FRS:
        JB P3.5,INIT_FRAES_START
        LJMP INIT_FRAES_ENDE
INIT_FRAES_START:
        PUSH_ALL
;        MOV B,#5                                ; Msg. Frs
;        MOV AR2,#MemFrs                         ; meine Adr.
;        MOV AR3,#GetFrsError                    ; Fehlereinsprung
;        MOV AR4,#GetFrsWarning                  ; Warnungeinsprung
;        post_message1 Frs,SetMasterAdress       ; Ready-Anfordrung
        post_message2 #Frs,#SetMasterAdress,#MemFrs,#GetFrsError,#GetFrsWarning
        POP_ALL
INIT_FRAES_ENDE:
        RET

;---------------------------------------------------------------------
; Funktion : Initialisieren der Teach- In Einheit
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

INIT_TEACH:
        CLR READY
        CLR CRC
        LCALL INIT_MEM
        MOV T_Sync_Counter,#0
        MOV Frs_Ref_x,#0
        MOV Frs_Ref_x+1,#0
        MOV Frs_Ref_y,#0
        MOV Frs_Ref_y+1,#0
        MOV Frs_Ref_z,#0
        MOV Frs_Ref_z+1,#0
        MOV Frs_Ref_Tiefe,#0
        MOV Frs_Ref_Tiefe+1,#0
        CLR Ref_Flag
        CLR Tiefe_Flag
        RET

;---------------------------------------------------------------------
; Funktion : Deinitialisieren der Teach- In Einheit
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

DEINIT_TEACH:
        CLR READY
        CLR CRC
        LCALL DEINIT_MEM

        RET

;---------------------------------------------------------------------
; Funktion : Speicher der Teach- In Einheit loeschen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

CLEAR_TEACH:

        RET

;---------------------------------------------------------------------
; Funktion : Teach- In Einheit zuruecksetzen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

RESET_TEACH:
        LCALL RESET_MEM
        SETB READY

        RET

;---------------------------------------------------------------------
; Funktion : Roboter Teach- In Datei von PC laden
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

LOAD_ROB:
        JB P3.5,LOAD_ROB_ENDE
        CLR READY
        LCD 40H,"Roboter- Teachin- Datei wird geladen.   "
        LCALL RD_MEM_PC
LOAD_ROB_ENDE:
        RET

;---------------------------------------------------------------------
; Funktion : Fraese- Teach- In Datei von PC laden
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

LOAD_FRAES:
        JNB P3.5,LOAD_FRAES_ENDE
        CLR READY
        LCD 40H,"Fraese- Teachin- Datei wird geladen.    "
        LCALL RD_MEM_PC
LOAD_FRAES_ENDE:
        RET

;---------------------------------------------------------------------
; Funktion : Roboter Teach- In Datei auf PC speichern
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

SAVE_ROB:
        JB READY,SAVE_ROB_START
        JNB P3.5,SAVE_ROB_START
        LJMP SAVE_ROB_ENDE
SAVE_ROB_START:
        LCD 40H,"Roboter- Teachin- Datei wird gespeichert"
        LCALL WR_MEM_PC
        JNC SAVE_ROB_OK
        LCD 40H,"FEHLER bei Speichern.                   "
        RET

SAVE_ROB_OK:
        LCD 40H,"Datei gespeichert.                      "
        RET

SAVE_ROB_ENDE:
        RET

;---------------------------------------------------------------------
; Funktion : Fraese- Teach- In Datei auf PC speichern
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

SAVE_FRAES:
        JB READY,SAVE_FRAES_START
        JB P3.5,SAVE_FRAES_START
        LJMP SAVE_FRAES_ENDE
SAVE_FRAES_START:
        LCALL WR_MEM_PC
        JNC SAVE_FRAES_OK
        LCD 40H,"FEHLER bei Speichern.                   "
        RET

SAVE_FRAES_OK:
        LCD 40H,"Datei gespeichert.                      "
        RET

SAVE_FRAES_ENDE:
        RET

;---------------------------------------------------------------------
; Funktion : Position des Roboters speichern. 1. Teil: Msg- Header
;            in MEM speichern und ersten Teil der Position vom Roboter
;            anfordern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

STORE_ROB:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JNB READY,STORE_ROB_ENDE                ; Fertig und Roboter ?
        JB P3.5,STORE_ROB_ENDE

        MOV B,#4                                ; Msg. an Roboter um Pos.
        MOV R2,#MemRob                          ; Empfaenger angeben
        MOV R3,#RobPos1                         ; Msg.-Nr. Empfaenger angeben
        post_message1 Rob,GibPos1                ; zu erfragen

STORE_ROB_ENDE:
        CLR READY                               ; Teach- In nicht bereit
        POP AR0
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Position des Roboters speichern. 2. Teil: 1. Teil Position
;            in MEM speichern und zweiten Teil der Position vom Roboter
;            anfordern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

FIRST_FROM_ROB:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JB READY,FIRST_ROB_ENDE                 ; NICHT Fertig und Roboter ?
        JB P3.5,FIRST_ROB_ENDE
        MOV A,#8                                ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.-Laenge)
        ERROR 0
        MOV A,#Rob                              ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Modulnr.)
        ERROR 0
        MOV A,#MoveAPos1                        ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.- Nr.)
        ERROR 0
        MOV A,R2                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 High)
        ERROR 0
        MOV A,R3                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 Low)
        ERROR 0
        MOV A,R4                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 High)
        ERROR 0
        MOV A,R5                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 Low)
        ERROR 0
        MOV A,R6                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 High)
        ERROR 0
        MOV A,R7                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 Low)
        ERROR 0

        MOV B,#4                                ; Msg. an Roboter um Pos.
        MOV R2,#MemRob                          ; Empfaenger angeben
        MOV R3,#RobPos2                         ; Msg.-Nr. Empfaenger angeben
        post_message1 Rob,GibPos2               ; zu erfragen

FIRST_ROB_ENDE:
        POP AR0
        POP ACC
        POP PSW

        RET

;---------------------------------------------------------------------
; Funktion : Position des Roboters speichern. 3. Teil: 2. Teil Position
;            in MEM speichern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

SECOND_FROM_ROB:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JB READY,SECOND_ROB_ENDE                ; NICHT Fertig und Roboter ?
        JB P3.5,SECOND_ROB_ENDE
        MOV A,#8                                ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.-Laenge)
        ERROR 0
        MOV A,#Rob                              ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Modulnr.)
        ERROR 0
        MOV A,#MoveAPos2                        ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.- Nr.)
        ERROR 0
        MOV A,R2                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 High)
        ERROR 0
        MOV A,R3                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 Low)
        ERROR 0
        MOV A,R4                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 High)
        ERROR 0
        MOV A,R5                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 Low)
        ERROR 0
        MOV A,R6                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 High)
        ERROR 0
        MOV A,R7                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 Low)
        ERROR 0

SECOND_ROB_ENDE:
        POP AR0
        POP ACC
        POP PSW
        SETB READY                               ; Teach- In wieder bereit
        RET

;---------------------------------------------------------------------
; Funktion : Position der Fraese speichern. 1. Teil: Msg- Header
;            in MEM speichern und ersten Teil der Position von Fraese
;            anfordern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

STORE_FRAES:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JNB READY,STORE_FRS_ENDE                ; Fertig und Fraese?
        JNB P3.5,STORE_FRS_ENDE

        MOV B,#4                                ; Msg. an Roboter um Pos.
        MOV R2,#MemFrs                          ; Empfaenger angeben
        MOV R3,#FrsPos1                         ; Msg.-Nr. Empfaenger angeben
        post_message1 Frs,GibFrsPos1            ; zu erfragen

STORE_FRS_ENDE:
        CLR READY                               ; Teach- In nicht bereit
        POP AR0
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Position der Fraese speichern. 2. Teil: 1. Teil Position
;            in MEM speichern und zweiten Teil der Position von Fraese
;            anfordern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

FIRST_FROM_FRS:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JB READY,FIRST_FRS_ENDE                 ; NICHT Fertig und Roboter ?
        JNB P3.5,FIRST_FRS_ENDE
        MOV A,#8                                ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.-Laenge)
        ERROR 0
        MOV A,#Frs                              ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Modulnr.)
        ERROR 0
        MOV A,#MoveAPos                         ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.- Nr.)
        ERROR 0
        MOV A,R2                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 High)
        ERROR 0
        MOV A,R3                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 Low)
        ERROR 0
        MOV A,R4                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 High)
        ERROR 0
        MOV A,R5                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 Low)
        ERROR 0
        MOV A,R6                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 High)
        ERROR 0
        MOV A,R7                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 Low)
        ERROR 0

        MOV B,#4                                ; Msg. an Roboter um Pos.
        MOV R2,#MemFrs                          ; Empfaenger angeben
        MOV R3,#FrsPos2                         ; Msg.-Nr. Empfaenger angeben
        post_message1 Frs,GibFrsPos2            ; zu erfragen

FIRST_FRS_ENDE:
        POP AR0
        POP ACC
        POP PSW

        RET

;---------------------------------------------------------------------
; Funktion : Position der Fraese speichern. 3. Teil: 2. Teil Position
;            in MEM speichern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

SECOND_FROM_FRS:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JB READY,SECOND_FRS_ENDE                ; NICHT Fertig und Roboter ?
        JNB P3.5,SECOND_FRS_ENDE

        MOV A,#4                                ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.-Laenge)
        ERROR 0
        MOV A,#Frs                              ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Modulnr.)
        ERROR 0
        MOV A,#MoveV                            ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.- Nr.)
        ERROR 0
        MOV A,R2                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 High)
        ERROR 0
        MOV A,R3                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 Low)
        ERROR 0

        MOV A,#3                                ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.-Laenge)
        ERROR 0
        MOV A,#Frs                              ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Modulnr.)
        ERROR 0
        MOV A,#Drill                            ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.- Nr.)
        ERROR 0
        MOV A,R4                                ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 High)
        ERROR 0

SECOND_FRS_ENDE:
        POP AR0
        POP ACC
        POP PSW
        SETB READY                               ; Teach- In wieder bereit
        RET

;---------------------------------------------------------------------
; Funktion :
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;


        RET

;=====================================================================
;        END
;---------------------------------------------------------------------

