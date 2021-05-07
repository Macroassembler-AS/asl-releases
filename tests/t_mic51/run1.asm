;
; Aufgabe Nr.: Speichermodul fuer uP- Praktikum II
; Autor: Joerg Vollandt
; erstellt am : 01.07.1994
; letzte Aenderung am : 02.08.1994
; Bemerkung :
;
; Dateiname : run1.asm
;

;---------------------------------------------------------------------
Definitionen

ENDE_MARKE      EQU     0FFH
SYNC_MARKE      EQU     0FEH
READY_MARKE     EQU     0FDH
DRILL_MARKE     EQU     0FCH

PenUp           EQU     000H
PenDown         EQU     0FFH
d_z             EQU     200             ; Schritte fuer Auf-/Ab
Queue_Const     EQU     10              ; je Befehle ein GibReady an Frs

;---------------------------------------------------------------------
        SEGMENT CODE
;---------------------------------------------------------------------
; Funktion : Initialisierung
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

INIT_RUN:
        CLR RUNNINGBIT
        CLR Sync_Waiting
        CLR Ready_Waiting
        CLR Drilling
        CLR Drill_down
        CLR FrsWarning
        CLR PAUSE
        CLR SingleStep
        CLR Break
        MOV R_Sync_Counter,#0
        MOV Queue_Counter,#Queue_Const
        RET

;---------------------------------------------------------------------
; Funktion : Runmodul liesst ein Fkt.-Byte oder eine komplette Msg.
;            aus dem Speicher und schickt diese ins Netz.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

RUN_MODUL:
        PUSH_ALL
        JNB Break,RUN1
        LJMP RUN_ENDE
RUN1:   LCALL GET_ELEMENT_MEM                   ; Anzahl Bytes
        JNC RUN4                                ; Speicher leer
        LJMP RUN_LEER
RUN4:   CJNE A,#ENDE_MARKE,RUN5                 ; Ende -MARKE im Speicher
        GET_16 RD_POINTER
        DEC_DPTR
        SET_16 RD_POINTER
        LJMP RUN_ENDE                           ; erkannt
RUN5:   CJNE A,#SYNC_MARKE,RUN6                 ; Sync -MARKE im Speicher
        LJMP RUN_SYNC                           ; erkannt
RUN6:   CJNE A,#READY_MARKE,RUN7                ; Ready -MARKE im Speicher
        LJMP RUN_READY                          ; erkannt
RUN7:   CJNE A,#DRILL_MARKE,RUN8                ; Drill -MARKE im Speicher
        LJMP RUN_DRILL                          ; erkannt
RUN8:
        LCD 40h,"Ablauf der Teachin- Datei.              "
        PUSH ACC
        MOV B,A
        USING 1                                 ; Msg.
        MOV R0,#AR0                             ; aus Speicher nach Bank 1
        USING 0
RUN_Next_Byte:
        LCALL GET_ELEMENT_MEM                   ; Bytes aus Speicher
        MOV @R0,A                               ; holen
        INC R0
        DJNZ B,RUN_Next_Byte

        POP B
        PUSH PSW
        CLR RS1                                 ; Bank 1
        SETB RS0
        MOV DPTR,#ModulNetAdr_Tab
        MOV A,R0
        MOVC A,@A+DPTR
        SEND_NET                                ; Msg senden
        POP PSW                                 ; alte Bank
        POP_ALL
        JNB SingleStep,RUN_Next_Ende
        PUSH_ALL
        LJMP RUN_READY
RUN_Next_Ende:
        RET                                     ; fertig !


RUN_READY:
        LCD 40h,"Warten bis Geraet fertig.               "
        JNB P3.5,RUN_READY_FRS
        post_message2 #Frs,#GibReady,#MemFrs,#GetFrsReady,#0 ; Ready-Anforderung
        LJMP RUN_READY_WEITER                   ; schicken
RUN_READY_FRS:
        post_message2 #Rob,#RobGibReady,#MemRob,#GetRobReady
RUN_READY_WEITER:
        SETB Ready_Waiting
        POP_ALL
        RET


RUN_SYNC:
        LCD 40h,"Warten auf Synchronisationspunkt Nr.: "
        LCALL GET_ELEMENT_MEM                   ; Sync_Counter aus Speicher
        LCALL A_LCD
        JB P3.5,RUN_SYNC_FRS
        post_message2 #MemFrs,#GetFrsSync,A     ; Sync.-Meldung an Partner
        LJMP RUN_SYNC0                          ; schicken
RUN_SYNC_FRS:
        post_message2 #MemRob,#GetRobSync,A
RUN_SYNC0:
        MOV B,A
        MOV A,R_Sync_Counter
        CJNE A,B,RUN_SYNC1
RUN_SYNC1: JNC RUN_SYNC_ENDE
        SETB Sync_Waiting
RUN_SYNC_ENDE:
        POP_ALL
        RET


RUN_DRILL:
        JNB P3.5,RUN_DRILL_ROB
        LJMP RUN_DRILL_FRS
RUN_DRILL_ROB:
        LCD 40h,"Roboter kann nicht fraesen! Abbruch.    "
        CLR RUNNINGBIT
        POP_ALL
        LCALL INIT_TEACH
        LCALL RESET_TEACH
        RET

RUN_DRILL_FRS:
        LCD 40h,"Fraesdatei wird abgearbeitet.           "
        SETB Drilling
        LCALL GET_ELEMENT_MEM                          ; Fraestiefe aus Speicher
        MOV Frs_Ref_Tiefe,A
        LCALL GET_ELEMENT_MEM
        MOV Frs_Ref_Tiefe+1,A
        post_message2 #Frs,#FrsVelocityDraw,#fast       ; schnelle Bewegung
        post_message2 #Frs,#GoPieceRefPos               ; Werkstueckreferenz
        post_message2 #Frs,#MoveRZ,#(d_z/256),#(d_z#256) ; Pen up
        post_message2 #Frs,#DRILL,#on                   ; Motor an
        clr DRILL_DOWN
        SETB DRILL_DOWN

        MOV A,MY_SLAVE_ADR
        SWAP A                                         ; *16
        ADD A,#MSG_PC_RESET+Msg_Frs_Datei
        MOV R0,A
        MOV A,#PC_Slave_Adr                            ; Datei oeffnen
        MOV B,#8
        SEND_NET
        MOV A,MY_SLAVE_ADR
        SWAP A                                         ; *16
        ADD A,#MSG_PC_GET+Msg_Frs_Datei
        MOV R0,A
        MOV A,#PC_Slave_Adr                            ;
        MOV B,#8
        MOV DPTR,#0
        SET_16 BLOCKNR                                 ; Blocknr.=0 setzen
        MOV R1,DPL                                     ; Blocknr.
        MOV R2,DPH
        SEND_NET                                       ; 0. Block laden
        POP_ALL
        RET


RUN_LEER:
        LCD 40h,"Speicherinhalt defeckt.                 "
        CLR RUNNINGBIT
        POP_ALL
        LCALL INIT_TEACH
        LCALL RESET_TEACH
        RET


RUN_ENDE:
        LCD 40h,"Ablauf beendet.                         "
        GET_16 NEXT_FREE
        DEC_DPTR
        SET_16 NEXT_FREE                        ; Ende- MARKE entfernen
        CLR RUNNINGBIT                          ; Ablaufbeenden
        MOV A,R_Sync_Counter
        MOV T_Sync_Counter,A
        GET_16 RD_POINTER
        SET_16 NEXT_FREE
        POP_ALL
        CLR Break
        RET

;---------------------------------------------------------------------
; Funktion : Start des Runmoduls. Schreibt die Endemarke in den Speicher,
;            setzt den Speicher zurueck und setzt das Bit RUNNINBIT.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

START_RUNNING:
        JB READY,START_R
        LJMP START_NICHT_BEREIT
START_R: PUSH ACC
        MOV A,#ENDE_MARKE
        LCALL PUT_ELEMENT_MEM
        POP ACC
        LCALL RESET_MEM
        SETB RUNNINGBIT
        CLR Sync_Waiting
        CLR Ready_Waiting
        MOV R_Sync_Counter,#0
        RET

START_NICHT_BEREIT:
        LCD 40H,"Modul nicht bereit fuer Ablauf.         "
        RET

 ;---------------------------------------------------------------------
; Funktion : Piece-Ref-Position der Fraese speichern. 1. Teil: Position
;            anforndern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

STORE_PIECE_REF:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JNB READY,STORE_P_REF_ENDE              ; Fertig und Fraese?
        JNB P3.5,STORE_P_REF_ENDE

;        MOV B,#4                                ; Msg. an Frs um Pos.
;        MOV R2,#MemFrs                          ; Empfaenger angeben
;        MOV R3,#FrsPieceRef                     ; Msg.-Nr. Empfaenger angeben
;        post_message1 Frs,GibFrsPos1            ; zu erfragen
        post_message2 #Frs,#GibFrsPos1,#MemFrs,#FrsPieceRef

STORE_P_REF_ENDE:
        CLR READY                               ; Teach- In nicht bereit
        POP AR0
        POP ACC
        POP PSW
        RET

 ;---------------------------------------------------------------------
; Funktion : Piece-Ref-Position der Fraese speichern. 2. Teil: Position
;            in x,y,z speichern, Ref_Flag setzen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

PIECE_REF_FROM_FRS:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JNB READY,P_REF_FRS_DO                    ; NICHT Fertig und Roboter ?
        JB P3.5,P_REF_FRS_DO
        LJMP P_REF_FRS_ENDE
P_REF_FRS_DO:
        MOV Frs_Ref_x,R2                        ; Position speichern
        MOV Frs_Ref_x+1,R3                      ; Position speichern
        MOV Frs_Ref_y,R4                        ; Position speichern
        MOV Frs_Ref_y+1,R5                      ; Position speichern
        MOV Frs_Ref_z,R6                        ; Position speichern
        MOV Frs_Ref_z+1,R7                      ; Position speichern

P_REF_FRS_ENDE:
        POP AR0
        POP ACC
        POP PSW
        SETB READY                               ; Teach- In wieder bereit
        SETB Ref_Flag
        RET

 ;---------------------------------------------------------------------
; Funktion : Fraestiefe der Fraese speichern. 1. Teil: Position
;            anforndern.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

STORE_TIEFE:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JNB READY,STORE_TIEFE_ENDE              ; Fertig und Fraese?
        JNB P3.5,STORE_TIEFE_ENDE

;        MOV B,#4                                ; Msg. an Frs um Pos.
;        MOV R2,#MemFrs                          ; Empfaenger angeben
;        MOV R3,#FrsTiefe                        ; Msg.-Nr. Empfaenger angeben
;        post_message1 Frs,GibFrsPos1            ; zu erfragen
        post_message2 #Frs,#GibFrsPos1,#MemFrs,#FrsTiefe

STORE_TIEFE_ENDE:
        CLR READY                               ; Teach- In nicht bereit
        POP AR0
        POP ACC
        POP PSW
        RET

 ;---------------------------------------------------------------------
; Funktion : Fraestiefe der Fraese speichern. 2. Teil: Tiefe berechnen
;            und in Frs_Ref_Tiefe speichern, Tiefe_Flag setzen.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register : -
; Stackbedarf :
; Zeitbedarf :
;

TIEFE_FROM_FRS:
        PUSH PSW
        PUSH ACC
        PUSH AR0
        JNB READY,TIEFE_FRS_DO                  ; NICHT Fertig und Roboter ?
        JB P3.5,TIEFE_FRS_DO
        LJMP TIEFE_FRS_ENDE
TIEFE_FRS_DO:
;        MOV A,AR7                               ; Fraestiefe berechnen
;        CLR C                                   ; und speichern
;        SUBB A,Frs_Ref_Tiefe+1
        MOV Frs_Ref_Tiefe+1,AR7
;  7
;        MOV A,AR6
;        SUBB A,Frs_Ref_Tiefe
        MOV Frs_Ref_Tiefe,AR6
TIEFE_FRS_ENDE:
        POP AR0
        POP ACC
        POP PSW
        SETB READY                               ; Teach- In wieder bereit
        SETB Tiefe_Flag
        RET

;---------------------------------------------------------------------
; Funktion : Flags abfragen, nur wenn Ref_Flag und Tiefe_Flag low weiter.
;            SetPieceRef Msg. in Speicher ablegen, Drill Marke ablegen,
;            Tiefe ablegen.
; Aufrrameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

STORE_DRILL:
        PUSH PSW
        PUSH ACC
        JNB Ref_Flag,STORE_DRILL_ERROR          ; PieceRefPos und Tiefe
        JNB Tiefe_Flag,STORE_DRILL_ERROR        ; definiert
        LJMP STORE_DRILL_OK

STORE_DRILL_ERROR:
        LCD 40h,"Fehler: RefPos/ Tiefe nicht definiert.  "
        POP ACC
        POP PSW
        RET

STORE_DRILL_OK:
        MOV A,#8                                ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.-Laenge)
        ERROR 0
        MOV A,#Frs                              ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Modulnr.)
        ERROR 0
        MOV A,#SetPieceRef                      ; Msg- Header speichern
        LCALL PUT_ELEMENT_MEM                   ; (Msg.- Nr.)
        ERROR 0
        MOV A,Frs_Ref_x                         ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 High)
        ERROR 0
        MOV A,Frs_Ref_x+1                       ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 1 Low)
        ERROR 0
        MOV A,Frs_Ref_y                         ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 High)
        ERROR 0
        MOV A,Frs_Ref_y+1                       ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 2 Low)
        ERROR 0
        MOV A,Frs_Ref_z                         ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 High)
        ERROR 0
        MOV A,Frs_Ref_z+1                       ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 Low)
        ERROR 0

        MOV A,#DRILL_MARKE
        LCALL PUT_ELEMENT_MEM

        MOV A,Frs_Ref_Tiefe                     ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 High)
        ERROR 0
        MOV A,Frs_Ref_Tiefe+1                   ; Position speichern
        LCALL PUT_ELEMENT_MEM                   ; (Parameter 3 Low)
        ERROR 0

        POP ACC
        POP PSW
        RET

;---------------------------------------------------------------------
; Funktion : Speichert die Ready-MARKE im Speicher
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

STORE_READY:
        PUSH ACC
        MOV A,#READY_MARKE
        LCALL PUT_ELEMENT_MEM
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : Speichert das Sync.-MARKE und den Sync.-Counter im Speicher
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

STORE_SYNC:
        PUSH ACC
        MOV A,#SYNC_MARKE
        LCALL PUT_ELEMENT_MEM
        INC T_Sync_Counter                ; Sync_Counter +1
        MOV A,T_Sync_Counter
        LCALL PUT_ELEMENT_MEM
        LCD 29,"Sync.-Nr."
        LCALL A_LCD
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : Ready-Msg. erhalten und bearbeiten.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

GET_READY_MSG:
        LCD 40H,"                                        "
        CLR Ready_Waiting
        RET

;---------------------------------------------------------------------
; Funktion : Sync.-Msg. erhalten und bearbeiten. Bricht Running ab
;            wenn von Partner Sync.-Nr. 0 kommt. Wenn eigenes Modul
;            noch nicht getstartet wird Sync.-Msg. mit #0 an Partner
;            geschickt.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

GET_SYNC_MSG:
        JNB RunningBit,G_S_1
        LJMP G_S_2
G_S_1:  PUSH B
        MOV B,#3                                ; Msg.
        MOV R2,#0                               ; mit #0
        JB P3.5,G_S_FRS
        post_message1 MemFrs,GetFrsSync         ; Sync.-Meldung an Partner
        POP B
        RET                                     ; schicken
G_S_FRS: post_message1 MemRob,GetRobSync
        POP B
        RET

G_S_2:  PUSH PSW
        PUSH ACC
        INC R_Sync_Counter
        MOV A,R2                        ; Sync_Counter aus Msg. holen
        CJNE A,#0,G_S_KEIN_ABBRUCH        ; Abbruch bei #0
        LJMP G_S_ABBRUCH
G_S_KEIN_ABBRUCH:
        CJNE A,R_Sync_Counter,G_S_ERROR   ; Fehler wenn ungleich
        CLR Sync_Waiting
        POP ACC
        POP PSW
        RET

G_S_ABBRUCH:
        LCD 40h,"Partner nicht bereit. Abbruch.          "
        CLR RUNNINGBIT
        POP ACC
        POP PSW
        LCALL INIT_TEACH
        LCALL RESET_TEACH
        RET

G_S_ERROR:
        LCD 40h,"Synchronisationsfehler.                 "
        CLR RUNNINGBIT
        POP ACC
        POP PSW
        LCALL INIT_TEACH
        LCALL RESET_TEACH
        RET

;---------------------------------------------------------------------
; Funktion : Testen ob Queue- Warnung von Frs gekommen ist. Wenn ja,
;            GibReady an die Fraese schicken und warten bis kommt.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

queue_test      macro

Q_T_Main_Loop:
        LCALL MAIN_EVENT_LOOP
        JB PAUSE,Q_T_Main_Loop                  ; Pause aktiv
        JB READY_WAITING,Q_T_Main_Loop          ; warten auf Ready von Frs
        LCALL READ_STATUS
        JB ACC.1,Q_T_Main_Loop

        endm

;---------------------------------------------------------------------
; Funktion : Daten aus der Fraesdatei vom PC empfangen, konvertieren
;            und an die Fraese schicken.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;


GET_WORKFR_FROM_PC:
        PUSH_ALL
        JB RUNNINGBIT,GET_WORKFR_FROM_PC_START
        LJMP GET_WORKFR_FROM_PC_ABBRUCH
GET_WORKFR_FROM_PC_START:
        MOV A,R1                                ; testen ob Dateiende
        CJNE A,#0ffh,GET_WORKFR_FROM_PC_NEXT
        MOV A,R2
        CJNE A,#0ffh,GET_WORKFR_FROM_PC_NEXT
        LJMP GET_WORKFR_FROM_PC_ENDE

GET_WORKFR_FROM_PC_NEXT:                        ; naechste Msg. von PC
        PUSH_ALL                                ; konvertieren und an Frs
        queue_test
        MOV A,AR7                               ; schicken
        CJNE A,#PenDown,G_W_PenUp               ; Auf- oder Abbewegung?
        ljmp G_W_PenDown
G_W_PenUp:
        jb drill_down,G_W_Make_Up
        ljmp G_W_Pen_Ready                      ; ist schon oben
G_W_Make_Up:
        post_message2 #Frs,#MoveZ,Frs_Ref_z,Frs_Ref_z+1 ; Pen aus Werkstueck ziehen
        queue_test
        post_message2 #Frs,#FrsVelocityDraw,#fast ; schnelle Bewegung
        queue_test
        post_message2 #Frs,#MoveRZ,#(d_z/256),#(d_z#256) ; Pen up
        clr DRILL_DOWN
        queue_test
        LJMP G_W_Pen_Ready                      ; fertig

G_W_PenDown:
        jnb drill_down,G_W_Make_Down
        ljmp G_W_Pen_Ready                       ; ist schon unten
G_W_Make_Down:
        post_message2 #Frs,#MoveRZ,#(((-d_z) & 0ffffh) / 256),#(((-d_z) & 0ffffh) # 256) ; Ab
        queue_test
        post_message2 #Frs,#FrsVelocityDraw,#slow ; langsame Bewegung
        queue_test
        post_message2 #Frs,#MoveZ,Frs_Ref_Tiefe,Frs_Ref_Tiefe+1 ; Pen aus Werkstueck ziehen
        queue_test
        SETB DRILL_DOWN
G_W_Pen_Ready:                                  ; Pen fertig !

        POP_ALL
        post_message2 #Frs,#MovePR,AR4,AR3,AR6,AR5,#0,#0 ; rel. Bewegung
        queue_test

;        DJNZ Queue_Counter,G_W_Send_no_Ready
;        MOV Queue_Counter,#Queue_Const
;        PUSH_ALL
;        post_message2 #Frs,#GibReady,#MemFrs,#GetFrsReady,#0
;        POP_ALL

G_W_Send_no_Ready:

        GET_16 BLOCKNR
        INC DPTR
        SET_16 BLOCKNR                          ; Blocknr.=+1 setzen
        MOV R1,DPL                              ; Blocknr.
        MOV R2,DPH
        MOV A,MY_SLAVE_ADR
        SWAP A                                  ; *16
        ADD A,#MSG_PC_GET+Msg_Frs_Datei
        MOV R0,A
        MOV A,#PC_Slave_Adr
        MOV B,#8
        SEND_NET                                ; naechsten Block anfordern
        POP_ALL
        RET

GET_WORKFR_FROM_PC_ENDE:
        post_message2 #Frs,#DRILL,#off          ;
        CLR Drilling

        POP_ALL
        LCD 40h,"Fraesdatei fertig.                      "
        RET

GET_WORKFR_FROM_PC_ABBRUCH:
        CLR Drilling
        POP_ALL
        RET

;---------------------------------------------------------------------
; Funktion : Warnung von Frs erhalten und verarbeiten.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

Get_Warning_From_Frs:
        LCD 40h,"Warnung Fraese:                         "
        PUSH ACC
        PUSH PSW
        PUSH B
        PUSH_DPTR

        MOV A,AR2
        IFMAKE QUEUE_WARNING,SETB FrsWarning    ; Queue Fraese laeuf ueber

        MOV DPTR,#ParamStr_Tab                  ; Tabellenbasis laden
GWFF_NEXT:
        MOV A,#0
        MOVC A,@A+DPTR                          ; Wert laden
        PUSH ACC                                ; retten
        MOV A,#1
        MOVC A,@A+DPTR                          ; Stringlaenge laden
        MOV B,A
        POP ACC                                 ; Wert mit empfangenem
        CJNE A,AR2,GWFF_LOOP                    ; vergleichen
        LJMP GWFF_AUSGABE
GWFF_LOOP:
        MOV A,B                                 ; Stringlaende auf Tabellen-
        ADD A,DPL                               ; zeiger addieren
        MOV DPL,A
        MOV A,#0
        ADDC A,DPH
        MOV DPH,A
        LJMP GWFF_NEXT

GWFF_AUSGABE:
        MOV A,#50h                              ; LCD- Position
        LCALL LCD_SET_DD_RAM_ADDRESS
        INC DPTR
        INC DPTR
        LCALL LCD_WRITE_STRING                  ; Meldung ausgeben
        MOV A,#0h                               ; LCD- Position
        LCALL LCD_SET_DD_RAM_ADDRESS

        POP_DPTR
        POP B
        POP PSW
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion : Fehler von Frs erhalten und verarbeiten.
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

Get_Error_From_Frs:
        LCD 40h,"Fehler Fraese:                          "
        PUSH ACC
        PUSH PSW
        PUSH B
        PUSH_DPTR
        MOV DPTR,#ParamStr_Tab                  ; Tabellenbasis laden
GEFF_NEXT:
        MOV A,#0
        MOVC A,@A+DPTR                          ; Wert laden
        PUSH ACC
        MOV A,#1
        MOVC A,@A+DPTR                          ; Stringlaenge laden
        MOV B,A
        POP ACC                                 ; Wert mit empfangenem
        CJNE A,AR2,GEFF_LOOP                    ; vergleichen
        LJMP GEFF_AUSGABE
GEFF_LOOP:
        MOV A,B                                 ; Stringlaende auf Tabellen-
        ADD A,DPL                               ; zeiger addieren
        MOV DPL,A
        MOV A,#0
        ADDC A,DPH
        MOV DPH,A
        LJMP GEFF_NEXT

GEFF_AUSGABE:
        MOV A,#4Fh                              ; LCD- Position
        LCALL LCD_SET_DD_RAM_ADDRESS
        INC DPTR
        INC DPTR
        LCALL LCD_WRITE_STRING                  ; Meldung ausgeben
        MOV A,#0h                               ; LCD- Position
        LCALL LCD_SET_DD_RAM_ADDRESS

        POP_DPTR
        POP B
        POP PSW
        POP ACC
        RET

;---------------------------------------------------------------------
; Funktion :
; Aufrufparameter : -
; Ruechgabeparameter : -
; Veraenderte Register :
; Stackbedarf :
; Zeitbedarf :
;

;=====================================================================
;        END
;---------------------------------------------------------------------



