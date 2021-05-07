
; Aufgabe Nr.: Teach- In Einheit fuer uP- Praktikum II
;              --- Link- Modul ---
; Autor: Joerg Vollandt
; erstellt am : 13.06.1994
; letzte Aenderung am : 02.08.1994
; Bemerkung :
;
; Dateiname : t_mod1.asm
;
;=====================================================================
        SEGMENT CODE
        USING 0

        include makro1.asm
        include tast1.asm
        include mem1.asm
        include teach1.asm
        include run1.asm
        include debug.asm
;=====================================================================
; Definitionen der Funktionen der Teach- In Einheit

Adr_InitRobTeach   EQU    INIT_TEACH       ; Initialisieren der Teach- In Einheit
Adr_InitFrsTeach   EQU    INIT_TEACH       ; Initialisieren der Teach- In Einheit
Adr_DeinitRobTeach EQU     DEINIT_TEACH    ; Deinitialisieren der Teach- In Einheit
Adr_DeinitFrsTeach EQU     DEINIT_TEACH    ; Deinitialisieren der Teach- In Einheit
Adr_ClearRobTeach  EQU     CLEAR_TEACH     ; Speicher loeschen
Adr_ClearFrsTeach  EQU     CLEAR_TEACH     ; Speicher loeschen
Adr_ResetRobTeach  EQU     RESET_TEACH     ; Speicher zum lesen zuruecksetzen
Adr_ResetFrsTeach  EQU     RESET_TEACH     ; Speicher zum lesen zuruecksetzen
Adr_StoreRobPos    EQU     STORE_ROB       ; Position Roboter speichern
Adr_StoreFrsPos    EQU     STORE_FRAES     ; Position Fraese speichern
Adr_StoreRobSync   EQU     STORE_SYNC      ; Synchronisation speichern
Adr_StoreFrsSync   EQU     STORE_SYNC      ; Synchronisation speichern
Adr_StoreRobReady  EQU     STORE_READY     ; Warten auf Geraet speichern
Adr_StoreFrsReady  EQU     STORE_READY     ; Warten auf Geraet speichern
Adr_StoreFrsPieceRef EQU   STORE_PIECE_REF ; Werkstueck Nullpkt. festlegen
Adr_StoreFrsTiefe  EQU     STORE_TIEFE     ; Fraestiefe festlegen
Adr_StoreFrsDrill  EQU     STORE_DRILL     ; Fraesdatei bearbeiten
Adr_GetRobSync     EQU     GET_SYNC_MSG    ; Synchronisation empfangen
Adr_GetFrsSync     EQU     GET_SYNC_MSG    ; Synchronisation empfangen
Adr_GetRobReady    EQU     GET_READY_MSG   ; Ready empfangen
Adr_GetFrsReady    EQU     GET_READY_MSG   ; Ready empfangen
Adr_LoadRob        EQU     LOAD_ROB        ; Roboter Teach- In Datei von PC laden
Adr_LoadFrs        EQU     LOAD_FRAES      ; Fraese- Teach- In Datei von PC laden
Adr_SaveRob        EQU     SAVE_ROB        ; Roboter Teach- In Datei auf PC speichern
Adr_SaveFrs        EQU     SAVE_FRAES      ; Fraese- Teach- In Datei auf PC speichern

Adr_RobPos1        EQU     FIRST_FROM_ROB  ; Position von Roboter 1. Teil
Adr_RobPos2        EQU     SECOND_FROM_ROB ; Position von Roboter 2. Teil
Adr_FrsPos1        EQU     FIRST_FROM_FRS  ; Position von Fraese 1. Teil
Adr_FrsPos2        EQU     SECOND_FROM_FRS ; Position von Fraese 2. Teil
Adr_FrsPieceRef    EQU     PIECE_REF_FROM_FRS ; Position von Fraese
Adr_FrsTiefe       EQU     TIEFE_FROM_FRS  ; Position von Fraese

Adr_DebugRob       EQU     DEBUG_MEM       ; Position von Roboter 2. Teil
Adr_DebugFrs       EQU     DEBUG_MEM       ; Position von Roboter 2. Teil
Adr_StartRobRun    EQU     START_RUNNING   ; Runmanager starten
Adr_StartFrsRun    EQU     START_RUNNING   ; Runmanager starten

Adr_GetFrsError    EQU     Get_Error_from_frs   ;
Adr_GetFrsWarning  EQU     Get_Warning_from_frs ;


MemRob_MsgCall_Tab:
        include defMsg.inc
MemRob_MsgCall_Tend:

MemFrs_MsgCall_Tab:
        include defMsg.inc
MemFrs_MsgCall_Tend:

;------------------------------------------------------------------------------
; Speicherdefinitionen


        SEGMENT BITDATA

MSG             DB      ?
Sp_MSG          DB      ?
READY           DB      ?
CRC             DB      ?

TESTBIT         DB      ?
RUNNINGBIT      DB      ?
Sync_Waiting    DB      ?
Ready_Waiting   DB      ?
Drilling        DB      ?
Drill_down      DB      ?
PAUSE           DB      ?
FrsWarning      DB      ?
SingleStep      DB      ?
Break           DB      ?

Ref_Flag        DB      ?
Tiefe_Flag      DB      ?

        SEGMENT DATA

Sp_MSG_Buffer     DB      ?
T_Sync_Counter    DB      ?
R_Sync_Counter    DB      ?
Queue_Counter     DB      ?

Frs_Ref_x         DW      ?
Frs_Ref_y         DW      ?
Frs_Ref_z         DW      ?
Frs_Ref_Tiefe     DW      ?

;---------------------------------------------------------------------
        SEGMENT CODE
;---------------------------------------------------------------------
; Funktion : CALL_BACK- Fkt. wird nach Empfang einer Message
;            aufgerufen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

MESSAGE_BIT             BIT     ACC.0   ; Message Bits
SPECIAL_MESSAGE_BIT     BIT     ACC.2
START_BIT               BIT     ACC.0   ; Special- Message Bits
STOP_BIT                BIT     ACC.1
RESET_BIT               BIT     ACC.3
PAUSE_BIT               BIT     ACC.2
AUX1_BIT                BIT     ACC.4
AUX2_BIT                BIT     ACC.5

MESSAGE_INTERRUPT:
        PUSH ACC
        LCALL READ_STATUS
        JNB SPECIAL_MESSAGE_BIT,MESSAGE_INTERRUPT1
        LCALL READ_SPECIAL_MESSAGE              ; Special_Message lesen
        MOV Sp_MSG_Buffer,A                     ; und retten
        SETB Sp_MSG
        POP ACC
        RET

MESSAGE_INTERRUPT1:
        JNB MESSAGE_BIT,MESSAGE_INTERRUPT2
        SETB MSG                                ; Normale Msg.empfangen
MESSAGE_INTERRUPT2:
        POP ACC
        RET


;---------------------------------------------------------------------
; Funktion : Message- Scheduler fuer Speichermodul.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;
; ****************************************************************************
; R0    Empf„nger (logische Adresse)
; R1    Message
; R2 - R7 Parameter
; ****************************************************************************

Message_Handler MACRO   Modul

        push PSW
        push ACC
        push DPH
        push DPL

        mov  DPTR,#Msg_Hndl_Ret        ; Ruecksprungadresse vom indirekten
        push DPL                        ; Jump ergibt indirekten Call
        push DPH
        mov  DPTR,#Modul_MsgCall_Tab
        mov  A,AR1
        clr  C
        rlc  A
        mov  AR1,A
        jnc  No_inc
        inc DPH
No_inc: movc A,@A+DPTR
        push ACC
        inc DPTR
        mov  A,AR1
        movc A,@A+DPTR
        push ACC
        ret                             ; indireckter Sprung

Msg_Hndl_Ret:
        pop  DPL
        pop  DPH
Msg_Ha_Exit:
        pop  ACC
        pop  PSW

        ENDM

;---------------------------------------------------------------------
; Funktion : Message- Scheduler fuer PC- Messages.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

MEM_SCHEDULER:
        PUSH PSW
        PUSH ACC
        CLR MSG
        MOV A,R0
        IFCALL  02h,GET_FROM_PC         ; TI-Datei von PC an Roboter
        IFCALL  03h,GET_FROM_PC         ; TI-Datei von PC an Fraese
        IFCALL  01h,GET_WORKFR_FROM_PC  ; Fraesdatei von PC
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Message auf die Module verteilen
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

Dispatch_Msg:
        PUSH PSW
        PUSH ACC
        MOV A,R0
        CJNE A,#10h,Dis_Msg0            ; Msg.-Nr. <=10h sind von PC
Dis_Msg0: JC Dis_Msg01                  ; und werden von MEM_SCHEDULER
        LJMP Dis_Msg02                  ; bearbeitet
Dis_Msg01:
        LCALL MEM_SCHEDULER
        LJMP Dis_Msg_Ret

Dis_Msg02:
        cjne A,#TeachRob,Dis_Msg10
        LJMP Dis_Msg11
Dis_Msg10: LJMP Dis_Msg2
Dis_Msg11:
        ifdef TeachRob_MsgCall_Tab
          Message_Handler TeachRob
        endif
        ljmp Dis_Msg_Ret

Dis_Msg2: cjne A,#TeachFrs,Dis_Msg20
        LJMP Dis_Msg21
Dis_Msg20: LJMP Dis_Msg3
Dis_Msg21:
        ifdef TeachFrs_MsgCall_Tab
          Message_Handler TeachFrs
        endif
        ljmp Dis_Msg_Ret

Dis_Msg3: cjne A,#Rob,Dis_Msg30
        LJMP Dis_Msg31
Dis_Msg30: LJMP Dis_Msg4
Dis_Msg31:
        ifdef Rob_MsgCall_Tab
          Message_Handler Rob
        endif
        ljmp Dis_Msg_Ret

Dis_Msg4: cjne A,#Frs,Dis_Msg40
        LJMP Dis_Msg41
Dis_Msg40: LJMP Dis_Msg5
Dis_Msg41:
        ifdef Frs_MsgCall_Tab
          Message_Handler Frs
        endif
        ljmp Dis_Msg_Ret

Dis_Msg5: cjne A,#MemFrs,Dis_Msg50
        LJMP Dis_Msg51
Dis_Msg50: LJMP Dis_Msg6
Dis_Msg51:
        ifdef MemFrs_MsgCall_Tab
          Message_Handler MemFrs
        endif
        ljmp Dis_Msg_Ret

Dis_Msg6: cjne A,#MemRob,Dis_Msg60
        LJMP Dis_Msg61
Dis_Msg60: LJMP Dis_Msg7
Dis_Msg61:
        ifdef MemRob_MsgCall_Tab
          Message_Handler MemRob
        endif
        ljmp Dis_Msg_Ret

Dis_Msg7:

Dis_Msg_Ret:
        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : START-Routine
; Aufrufparameter : Wird durch die globale Message "START" ausgeloesst
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
GLOBAL_START:
        ;LCD 40H,"START-Routine                           "
        LCALL START_RUNNING
        RET

;---------------------------------------------------------------------
; Funktion : NOTAUS-Routine
; Aufrufparameter : Wird durch die globale Message "STOP" ausgeloesst
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
GLOBAL_NOTAUS:
        LCD 40H,"NOTAUS!!! Abbruch.                      "
        CLR RUNNINGBIT
        LCALL INIT_TEACH
        LCALL RESET_TEACH
        RET

;---------------------------------------------------------------------
; Funktion : RESET-Routine
; Aufrufparameter : Wird durch die globale Message "RESET" ausgeloesst
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
GLOBAL_RESET:
        LCD 40H,"Teachin- u. Runmanager initialisiert.   "
        LCALL INIT_TEACH
        LCALL INIT_RUN
        LCALL RESET_TEACH
        LCALL INIT_FRS
        CLR TESTBIT

        RET

;---------------------------------------------------------------------
; Funktion : PAUSE-Routine
; Aufrufparameter : Wird durch die globale Message "PAUSE" ausgeloesst
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
GLOBAL_PAUSE:
        JB RUNNINGBIT,GLOBAL_PAUSE_1
        LJMP GLOBAL_PAUSE_ENDE
GLOBAL_PAUSE_1:
        CPL PAUSE
        JNB PAUSE,GLOBAL_PAUSE_AUS
        LCD 40H,"Pausemodus. Weiter mit <PAUSE>.         "
        RET
GLOBAL_PAUSE_AUS:
        LCD 40H,"Pausemodus aufgehoben.                  "
        RET
GLOBAL_PAUSE_ENDE:
        RET

;---------------------------------------------------------------------
; Funktion : AUX1-Routine
; Aufrufparameter : Wird durch die globale Message "AUX1" ausgeloesst
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
GLOBAL_AUX1:
        LCD 40H,"AUX1-Routine                            "
        SETB SingleStep
        JNB Ready_Waiting,GLOBAL_AUX1_ENDE
        SETB Break
GLOBAL_AUX1_ENDE
        RET

;---------------------------------------------------------------------
; Funktion : AUX2-Routine
; Aufrufparameter : Wird durch die globale Message "AUX2" ausgeloesst
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;
GLOBAL_AUX2:
        ;LCD 40H,"AUX2-Routine                            "
        LCD 40H,"Teachin- Datei wird gelaeden.           "
        LCALL LOAD_ROB
        LCALL LOAD_FRAES
        RET

;---------------------------------------------------------------------
; Funktion : Hauptprogramm fuer das Speichermodul
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

Main_Event_Loop:
        JNB Sp_MSG,No_Sp_Msg
        LCALL Do_Sp_Msg
        JB Sp_MSG,Main_Event_Loop
No_Sp_Msg:
        JNB MSG,No_Msg
        LCALL Do_Msg
        JB MSG,Main_Event_Loop
No_Msg:
        JNB RUNNINGBIT,No_Runnig
        LCALL Do_Runnig
No_Runnig:
        JB Sp_MSG,Main_Event_Loop
        JB MSG,Main_Event_Loop

        RET


Do_Msg: CLR MSG
        PUSH_ALL
        LCALL READ_MESSAGE
        LCALL Dispatch_Msg
        POP_ALL
        RET

Do_Sp_Msg:
        CLR Sp_MSG
        PUSH ACC
        MOV A,Sp_MSG_Buffer
SM_START: JNB START_BIT,SM_NOTAUS       ; Special- Message Fkt.
        LCALL GLOBAL_START              ; aufrufen
        POP ACC
        RET
SM_NOTAUS: JNB STOP_BIT,SM_RESET
        LCALL GLOBAL_NOTAUS
        POP ACC
        RET
SM_RESET: JNB RESET_BIT,SM_PAUSE
        LCALL GLOBAL_RESET
        POP ACC
        RET
SM_PAUSE: JNB PAUSE_BIT,SM_AUX1
        LCALL GLOBAL_PAUSE
        POP ACC
        RET
SM_AUX1: JNB AUX1_BIT,SM_AUX2
        LCALL GLOBAL_AUX1
        POP ACC
        RET
SM_AUX2: JNB AUX2_BIT,SM_ENDE
        LCALL GLOBAL_AUX2
        POP ACC
        RET
SM_ENDE: POP ACC
        RET

Do_Runnig:
        JB Drilling,Do_Drilling
        JB PAUSE,Do_Waiting
        JB Sync_Waiting,Do_Waiting
        JB Ready_Waiting,Do_Waiting
        LCALL RUN_MODUL
Do_Waiting:
        RET

Do_Drilling:
        JNB FrsWarning,No_FrsWarning            ; Queue- Warnung von Frs
        PUSH_ALL
        post_message2 #Frs,#GibReady,#MemFrs,#GetFrsReady,#0
        POP_ALL
        CLR FrsWarning
        SETB READY_WAITING
No_FrsWarning:
        RET

;=====================================================================
;        END
;---------------------------------------------------------------------

