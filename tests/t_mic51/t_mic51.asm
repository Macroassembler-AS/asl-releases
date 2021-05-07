;Program:               TEACH-IN EINHEIT
;
;Autor:                 Laurent Savary
;
;Datum:                 9.5.94
;
;letze Aenderung:       23.6.94
;
;
;Dateiname: TEACHIN.ASM
;
;------------------------------------------------------------------------------

Init_Vektor     macro Vektor,Routine

;        mov Vektor+0,#(Routine/256)             ;der Interruptvektor wird
;        mov Vektor+1,#(Routine#256)             ;ins interne RAM geladen

        push acc
        push dph
        push dpl
        mov dptr,#Vektor
        mov a,#00000010b                ; LJMP
        movx @dptr,a
        inc dptr
        mov a,#(Routine/256)
        movx @dptr,a
        inc dptr
        mov a,#(Routine#256)
        movx @dptr,a
        pop dpl
        pop dph
        pop acc

                endm

DEC_DPTR        MACRO
                INC DPL
                DJNZ DPL,DEC_DPTR1
                DEC DPH
DEC_DPTR1:      DEC DPL
                ENDM


INC_R0R1        MACRO
                INC     R0
                INC     R0
                DJNZ R0,INC_R0R1_End
                INC     R1
INC_R0R1_End:
                ENDM


DEC_R0R1:       MACRO
                INC R0
                DJNZ R0,DEC_R0R1_End
                DEC R1
DEC_R0R1_End:   DEC R0
                ENDM

;---------------------------------------------------------------------

post_Message macro
        push acc
SEND_NET1: LCALL READ_STATUS
        JB ACC.1,SEND_NET1
        pop acc
        LCALL SEND_MESSAGE
        endm

Take_Message macro
        push acc
RECEIVE_NET1: LCALL READ_STATUS
        JNB ACC.0,RECEIVE_NET1
        pop acc
        LCALL READ_MESSAGE
        endm

;------------------------------------------------------------------------------

        cpu 80515
        include stddef51.inc
        include net_lcd.inc
        include defint.inc
        include defgequ.inc
        include defmacro.inc

        USING   0

;------------------------ Konstanten ------------------------------------------

Ass_Keyboard_Only       EQU     False           ; Wenn True, TI-Einheit lauft nur mit
                                                ; der Tastatur (kein Tastenfeld)

Last_Code       EQU     0FFFFh  ; Endeadressen
Last_Bit_Data   EQU     07Fh    ; der Speicher-
Last_Data       EQU     07Fh    ; bereiche
Last_IData      EQU     0FFh    ; bestimmen
Last_XData      EQU     0FFFFh

Kb_Max_Length   EQU     40                      ; Hoechstlaenge der anzuzeigenden Strings
TI_Sample_Valid_Time    EQU     30              ; Gueltige Druckdauer in ms

ASCII_Space     EQU     32                      ;
ASCII_Left      EQU     19                      ;
ASCII_Right     EQU     4                       ;
ASCII_Up        EQU     5                       ;
ASCII_Down      EQU     24                      ; ASCII-Code fuer die
ASCII_CR        EQU     13                      ; Tastatur
ASCII_Esc       EQU     27                      ;
ASCII_DEL       EQU     127                     ;
ASCII_BkSpc     EQU     8                       ;
ASCII_LWord     EQU     1                       ;
ASCII_RWord     EQU     6                       ;
ASCII_Search    EQU     12                      ;

KSS_Off         EQU     0
KSS_Mod         EQU     1
KSS_Mod_Top     EQU     2
KSS_Mod_Bot     EQU     3
KSS_Msg         EQU     4
KSS_Msg_Top     EQU     5
KSS_Msg_Bot     EQU     6
KSS_Inc_Mod     EQU     7
KSS_No_Choice   EQU     8

R0_Bk1          EQU     08h
R1_Bk1          EQU     09h
R2_Bk1          EQU     0Ah
R3_Bk1          EQU     0Bh
R4_Bk1          EQU     0Ch
R5_Bk1          EQU     0Dh
R6_Bk1          EQU     0Eh
R7_Bk1          EQU     0Fh

;------------------------------------------------------------------------------

        segment data
        org Data_Start

Save_P4         DB      ?               ;
Save_P5         DB      ?               ;
Old_P4          DB      ?               ;
Old_P5          DB      ?               ;
Temp_P4         DB      ?               ; benoetigte Variablen fuer
Temp_P5         DB      ?               ; Teach_In_Sampler
TI_On_P4        DB      ?               ;
TI_Off_P4       DB      ?               ;
TI_On_P5        DB      ?               ;
TI_Off_P5       DB      ?               ;
TI_Sample_Counter DB    ?               ;

Text_Dec_Status DB      ?
Kb_Char_Buffer  DB      ?
Kb_Str_Pointer  DB      ?
Kb_Cursor       DB      ?
ASCII_Low_Byte  DB      ?
ASCII_High_Byte DB      ?
Rcv_Msg_Length  DB      ?
My_Slave_Adr    DB      ?               ; Physikalische Adresse dieses Moduls

Kb_Search_Status DB     ?               ;
Kb_Search_DPL   DB      ?               ;
Kb_Search_DPH   DB      ?               ; Benoetigte Variablen fuer
KS_Actual_Word  DB      ?               ; Keyboard Search
KS_Actual_Module DB     ?               ;
KS_Cursor       DB      ?               ;

Stat_Code       DB      ?               ;
Stat_Address    DB      ?               ;
Stat_Num_Param  DB      ?               ; Benoetigte Variablen fuer
Stat_Picture    DB      ?               ; Text_Decoder
Stat_Module     DB      ?               ;
Stat_Length     DB      ?               ;

                if $ > Last_Data
                  then fatal "Data-Bereichgrenze ueberschritten"
                endif


;------------------------------------------------------------------------------

        segment xdata
        org X_Data_Start

Kb_Str_Buffer   DB      Kb_Max_Length dup (?)   ; Text Buffer (fuer die Tastatur)
Token_Str       DB      Kb_Max_Length dup (?)   ; Ergebnis von Get_Token
Net_Rcv_Str     DB      Kb_Max_Length dup (?)   ; Empfangene Message vom Teach-In Modul

                 if $ > Last_XData
                  then fatal "XData-Bereichgrenze ueberschritten"
                endif

;------------------------------------------------------------------------------

        segment idata
        org I_Data_Start

Msg_Registers   DB      8  dup (?)      ; Register-Buffer fur die zu sendenden Messages

                 if $ > Last_IData
                  then fatal "IData-Bereichgrenze ueberschritten"
                endif


;------------------------------------------------------------------------------

        segment bitdata
        org Bit_Data_Start

Kb_Str_Ready    DB      ?               ; -> Text_Decoder
Kb_Char_Ready   DB      ?               ; -> Keyb_Controller
TI_Sample_Chg_Flg DB    ?               ; -> TeachIn_Decoder
TD_Status_Ready DB      ?               ; -> LCD_Controller
TD_Send_Ready   DB      ?               ; -> Send_Manager
Receive_Ready   DB      ?               ; -> Receive_Manager
TD_Next_Flg     DB      ?               ; -> Kb_Controller
KS_Status_Ready DB      ?               ; -> LCD_Controller
KS_Active_Flg   DB      ?               ; -> KB_Search_Up / _Down
Kb_Dsp_Ready    DB      ?               ; -> LCD_Controller
Ext_Dsp_Ready   DB      ?               ; -> LCD_Controller
System_Error    DB      ?
Sys_Robot_Mode  DB      ?
Sys_Keyboard_Mode DB    ?
TID_Done_Flg    DB      ?               ; -> TeachIn_Sampler

                if $ > Last_Bit_Data
                  then fatal "Bit_Data-Bereichgrenze ueberschritten"
                endif


;------------------------------------------------------------------------------

        segment code
        org Code_Start

;====================== H A U P T P R O G R A M M =============================

        segment code

Main_Prog:      CLR     EAL                     ;alle Interrupts sperren
                MOV     SP,#Stack-1             ;Stackpointer setzen

                LCALL   Init_Data
                LCALL   Init_IData
                LCALL   Init_BitData
                LCALL   Init_XData
                LCALL   Init_Timer
                LCALL   Init_Mode
Main_Error:     JB      System_Error,Main_Error

                LCALL   Init_Net
                LCALL   LCD_Clear
                MOV     A,#1
                LCALL   LCD_Curser_OnOff
                LCALL   Init_Int
                SETB    EAL

                CLR TESTBIT
                CLR MSG
                MOV Sp_MSG_Buffer,#0
                CLR Sp_MSG
                LCALL INIT_TEACH
                LCALL INIT_RUN
                LCALL RESET_TEACH
                LCALL INIT_FRS

Main_Loop:      LCALL   Main_Manager
                LCALL   Main_Event_Loop
                SJMP    Main_Loop


;------------------------------------------------------------------------------

Adr_Table:      Create_IncTab       ModulNetAdr, "defModul.inc"
Module_Table:   Create_IncOffsTab   ModulStr,    "defModul.inc"
Symbol_Table:   Create_IncOffsTab   ParamStr,    "defParam.inc"
Stat_Table:     Create_IncOffsTab   MsgStr,      "defMsg.inc"

KOn_P4_Rob:     Create_IncKeyTab       On,  P4, TeachRob, "defKey.inc"
KOff_P4_Rob:    Create_IncKeyTab       Off, P4, TeachRob, "defKey.inc"
KOn_P5_Rob:     Create_IncKeyTab       On,  P5, TeachRob, "defKey.inc"
KOff_P5_Rob:    Create_IncKeyTab       Off, P5, TeachRob, "defKey.inc"
KOn_P4_Frs:     Create_IncKeyTab       On,  P4, TeachFrs, "defKey.inc"
KOff_P4_Frs:    Create_IncKeyTab       Off, P4, TeachFrs, "defKey.inc"
KOn_P5_Frs:     Create_IncKeyTab       On,  P5, TeachFrs, "defKey.inc"
KOff_P5_Frs:    Create_IncKeyTab       Off, P5, TeachFrs, "defKey.inc"

;--------------------------------------------------------------------------
        include t_mod1.asm              ;
;------------------------------------------------------------------------------

Main_Manager:   JNB     Kb_Char_Ready,MM_Txt_Dec
                LCALL   Keyb_Controller

MM_Txt_Dec:     JNB     Kb_Str_Ready,MM_TI_Dec
                LCALL   Text_Decoder

MM_TI_Dec:      JNB     TI_Sample_Chg_Flg,MM_Send_Mng
                LCALL   TeachIn_Decoder

MM_Send_Mng:    JNB     TD_Send_Ready,MM_Receive_Mng
                LCALL   Send_Manager

MM_Receive_Mng: JNB     Receive_Ready,MM_LCD_Ctrl
                LCALL   Receive_Manager

MM_LCD_Ctrl:    JB      Ext_Dsp_Ready,MM_LCD_Ctrl2
                JB      Kb_Dsp_Ready,MM_LCD_Ctrl2
                JB      TD_Status_Ready,MM_LCD_Ctrl2
                JB      KS_Status_Ready,MM_LCD_Ctrl2
                SJMP    MM_End
MM_LCD_Ctrl2:   LCALL   LCD_Controller

MM_End:         RET

;--------------------------------------------------------------------------

Init_Data:
                MOV     Save_P4,#0FFh
                MOV     Save_P5,#0FFh
                MOV     Old_P4,#0FFh
                MOV     Old_P5,#0FFh
                MOV     Temp_P4,#0FFh
                MOV     Temp_P5,#0FFh
                MOV     TI_On_P4,#00
                MOV     TI_Off_P4,#00
                MOV     TI_On_P5,#00
                MOV     TI_Off_P5,#00
                MOV     TI_Sample_Counter,#00
                MOV     Rcv_Msg_Length,#00
                MOV     My_Slave_Adr,#00

                MOV     Text_Dec_Status,#00
                MOV     Kb_Char_Buffer,#00
                MOV     Kb_Str_Pointer,#00
                MOV     Kb_Cursor,#00
                MOV     ASCII_Low_Byte,#00
                MOV     ASCII_High_Byte,#00

                MOV     Kb_Search_DPL,#00
                MOV     Kb_Search_DPH,#00
                MOV     KS_Actual_Word,#00
                MOV     KS_Actual_Module,#00
                MOV     KS_Cursor,#00
                MOV     Kb_Search_Status,#00

                MOV     Stat_Code,#00
                MOV     Stat_Address,#00
                MOV     Stat_Num_Param,#00
                MOV     Stat_Picture,#00
                MOV     Stat_Module,#00
                MOV     Stat_Length,#00
                RET


Init_IData:     LCALL   Clr_Msg_Buffer
                RET


Init_XData:     PUSH    DPL
                PUSH    DPH
                PUSH    Acc

                MOV     DPTR,#Kb_Str_Buffer
                MOV     A,#ASCII_Space
                LCALL   Clear_Str
                MOV     DPTR,#Token_Str
                MOV     A,#StringEnde
                LCALL   Clear_Str

                POP     Acc
                POP     DPH
                POP     DPL
                RET


Init_BitData:   CLR     Kb_Str_Ready
                CLR     Kb_Char_Ready
                CLR     TI_Sample_Chg_Flg
                CLR     TD_Status_Ready
                CLR     TD_Send_Ready
                CLR     Receive_Ready
                SETB    TD_Next_Flg
                CLR     KS_Active_Flg
                CLR     KS_Status_Ready
                CLR     Kb_Dsp_Ready
                CLR     Ext_Dsp_Ready
                CLR     System_Error
                CLR     Sys_Robot_Mode
                CLR     Sys_Keyboard_Mode
                CLR     TID_Done_Flg
                RET

;--------------------------------------------------------------------------
; Routine            : Init_Mode
; Parameter          : -
; Rueckgabeparameter : Sys_Robot_Mode, Sys_Keyboard_Mode,System_Error
;
; entscheidet, ob das Programm im TI-Roboter, TI-Fraese oder TI-Tastatur
; laufen muss.                    (SRM /SKM)  (/SRM /SKM)    (SKM)


Init_Mode:      PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                CLR     System_Error

        if Ass_Keyboard_Only
                SJMP    IM_Keyboard
        elseif
                CLR     Sys_Robot_Mode
                CLR     Sys_Keyboard_Mode
                LCALL   LCD_Clear
                MOV     DPTR,#Screen_Title
                LCALL   LCD_Write_String
                JNB     P3.5,IM_Robot
                JB      P3.4,IM_Error
                MOV     DPTR,#Screen_Drill
                SJMP    IM_End
        endif

IM_Robot:       JNB     P3.4,IM_Keyboard
                SETB    Sys_Robot_Mode
                MOV     DPTR,#Screen_Robot
                SJMP    IM_End

IM_Keyboard:    SETB    Sys_Keyboard_Mode
                MOV     DPTR,#Screen_Key
                SJMP    IM_End

IM_Error:       SETB    System_Error
                MOV     DPTR,#Screen_Error

IM_End:         LCALL   LCD_Write_String
                LCALL   Wait_2s

                POP     DPH
                POP     DPL
                POP     PSW
                RET

Screen_Title:   DB      "****       TEACH-IN UNIT v1.0       ****",00
Screen_Drill:   DB      "               Drill mode",00
Screen_Robot:   DB      "               Robot mode",00
Screen_Key:     DB      "             Keyboard  mode",00
Screen_Error:   DB      "   ERROR : Incorrect micro-controller",00

;--------------------------------------------------------------------------

Init_Int:       Init_Vektor     INT0_VEKTOR,Keyb_Sampler
                Init_Interrupt  INT0_VEKTOR,Falling_Edge,Param_On
                Init_Vektor     ICT0_VEKTOR,TeachIn_Sampler
                Init_Interrupt  ICT0_VEKTOR,Nope,Param_On
                RET

;--------------------------------------------------------------------------


Init_Net:       PUSH    Acc
                PUSH    DPL
                PUSH    DPH

                MOV     DPTR,#MESSAGE_INTERRUPT         ;Receive_Sampler
                LCALL   Set_CallBack_Adress
                MOV     DPTR,#Adr_Table
                JB      Sys_Keyboard_Mode,Init_Net_Key
                JNB     Sys_Robot_Mode,Init_Net_Frs

Init_Net_Rob:   MOV     A,#TeachRob
                SJMP    Init_Net_End

Init_Net_Frs:   MOV     A,#TeachFrs
                SJMP    Init_Net_End

Init_Net_Key:   MOV     A,#TeachKey

Init_Net_End:   MOVC    A,@A+DPTR
                MOV     My_Slave_Adr,A
                LCALL   Set_My_Adress

                POP     DPH
                POP     DPL
                POP     Acc
                RET

;--------------------------------------------------------------------------

Init_Timer:     MOV     TH0,#00
                MOV     TL0,#00
                MOV     TMOD,#00110001b         ; T1 : Off, T0 : 16 Bit Timer
                SETB    TR0                     ; T0 einschalten
                RET

;--------------------------------------------------------------------------
; Routine            : LCD_Controller
; Parameter          : Ext_Dsp_Ready   -> DPTR
;                      TD_Status_Ready -> Text_Dec_Status
;                      KS_Status_Ready -> Kb_Search_Status
; Rueckgabeparameter : -
; wenn Ext_Dsp_Ready gesetzt wird, zeigt den mit DPTR gezeigten String
; auf den Bildschirm an.
; wenn TD_Status_Ready (bzw KS_Status_Ready) gesetzt wird, zeigt die
; entsprechende Meldung von Text_Dec_Status (bzw Kb_Search_Status)
;

LCD_Controller: PUSH    PSW
                PUSH    Acc
                PUSH    AR0
                PUSH    AR1
                PUSH    DPL
                PUSH    DPH
                LCALL   LCD_Home

                JNB     Ext_Dsp_Ready,LCD_Str_Buffer
                CLR     Ext_Dsp_Ready
                MOV     A,#40h
                LCALL   LCD_Set_DD_RAM_Address
                MOV     R1,#Kb_Max_Length

LCD_Ext_Loop:   MOVX    A,@DPTR
                LCALL   LCD_Write_Char
                INC     DPTR
                DJNZ    R1,LCD_Ext_Loop
                LCALL   LCD_Home

LCD_Str_Buffer: JNB     Kb_Dsp_Ready,LCD_TD_Status
                CLR     Kb_Dsp_Ready
                MOV     DPTR,#Kb_Str_Buffer
                MOV     R1,#Kb_Max_Length

LCD_Str_Loop:   MOVX    A,@DPTR
                LCALL   LCD_Write_Char
                INC     DPTR
                DJNZ    R1,LCD_Str_Loop

LCD_TD_Status:  JNB     TD_Status_Ready,LCD_KS_Status
                CLR     TD_Status_Ready

                MOV     A,#40
                LCALL   LCD_Set_DD_RAM_Address
                MOV     DPTR,#LCD_TD_Table
                MOV     R0,Text_Dec_Status
                CJNE    R0,#00,LCD_TD_Loop
                SJMP    LCD_TD_Cont

LCD_TD_Loop:    MOV     A,#41
                ADD     A,DPL
                MOV     DPL,A
                MOV     A,DPH
                ADDC    A,#00
                MOV     DPH,A
                DJNZ    R0,LCD_TD_Loop

LCD_TD_Cont:    LCALL   LCD_Write_String

LCD_KS_Status:  JNB     KS_Status_Ready,LCD_End
                CLR     KS_Status_Ready

                MOV     A,#40
                LCALL   LCD_Set_DD_RAM_Address
                MOV     DPTR,#LCD_KS_Table
                MOV     R0,Kb_Search_Status
                CJNE    R0,#00,LCD_KS_Loop
                SJMP    LCD_KS_Cont

LCD_KS_Loop:    MOV     A,#41
                ADD     A,DPL
                MOV     DPL,A
                MOV     A,DPH
                ADDC    A,#00
                MOV     DPH,A
                DJNZ    R0,LCD_KS_Loop

LCD_KS_Cont:    LCALL   LCD_Write_String

LCD_End:        MOV     A,Kb_Cursor
                LCALL   LCD_Set_DD_RAM_Address

                POP     DPH
                POP     DPL
                POP     AR1
                POP     AR0
                POP     Acc
                POP     PSW
                RET

LCD_TD_Table:   DB      "Edit OK : Sending                       ",00
                DB      "Edit ERROR : message waited             ",00
                DB      "Edit ERROR : incorrect message          ",00
                DB      "Edit ERROR : more parameters waited     ",00
                DB      "Edit ERROR : parameter range            ",00
                DB      "Edit ERROR : module name waited         ",00
                DB      "Edit ERROR : incorrect module name      ",00
                DB      "Edit WARNING : too many parameters      ",00

LCD_KS_Table:   DB      "Text editing                            ",00
                DB      "Search : Choose a module                ",00
                DB      "Search : Choose a module, reached TOP   ",00
                DB      "Search : Choose a module, reached BOTT. ",00
                DB      "Search : Choose a message               ",00
                DB      "Search : Choose a message, reached TOP  ",00
                DB      "Search : Choose a message, reached BOTT.",00
                DB      "Search : Incorrect module               ",00
                DB      "Search : No choice available            ",00

;--------------------------------------------------------------------------

Keyb_Sampler:
                MOV     Kb_Char_Buffer,P1
                SETB    Kb_Char_Ready
                RETI

;--------------------------------------------------------------------------
; Routine            : Keyb_Controller
; Parameter          : Kb_Char_Ready            ; (Buchstabe im Buffer)
;                      Kb_Char_Buffer           ; (zu verarbeitender Buchstabe)
;                      Kb_Cursor                ; (Cursorposition auf LCD)
;                      KS_Active_Flg            ; (Keyb. Search Modus)
;                      Kb_Str_Buffer            ; (Text Buffer f. Tastatur)
;                      TD_Next_Flg              ; Text_Decoder ist fertig
; Rueckgabeparameter : Kb_Cursor
;                      KS_Active_Flg
;                      Kb_Str_Buffer
;                      Kb_Str_Ready             ; (->Text_Decoder : String verarbeiten)
;                      Kb_Search_Status         ; (Keyb. Search Zustand)
;                      KS_Status_Ready          ; (-> LCD : Zustand anzeigen)
;
; Verwaltet Kb_Str_Buffer nach der Tastatur-Eingaben


Keyb_Controller:
                PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                CLR     Kb_Char_Ready
                MOV     A,Kb_Char_Buffer

                JNB     TD_Next_Flg,Kb_Ctrl_UP
                CLR     TD_Next_Flg
Kb_Ctrl_Test1:  CJNE    A,#ASCII_Left,Kb_Ctrl_Test2
                SJMP    Kb_Ctrl_Old
Kb_Ctrl_Test2:  CJNE    A,#ASCII_Right,Kb_Ctrl_Test3
                SJMP    Kb_Ctrl_Old
Kb_Ctrl_Test3:  CJNE    A,#ASCII_BkSpc,Kb_Ctrl_Test4
                SJMP    Kb_Ctrl_Old
Kb_Ctrl_Test4:  CJNE    A,#ASCII_CR,Kb_Ctrl_New


Kb_Ctrl_Old:    MOV     Kb_Search_Status,#KSS_Off
                SETB    KS_Status_Ready
                LJMP    Kb_Ctrl_Up

Kb_Ctrl_New:    PUSH    Acc
                MOV     A,#ASCII_Space
                MOV     DPTR,#Kb_Str_Buffer
                LCALL   Clear_Str
                MOV     Kb_Cursor,#00
                MOV     Kb_Search_Status,#KSS_Off
                SETB    KS_Status_Ready
                POP     Acc

Kb_Ctrl_UP:     CJNE    A,#ASCII_Up,Kb_Ctrl_DOWN
                LCALL   Kb_Search_Up
                LJMP    Kb_Ctrl_End

Kb_Ctrl_DOWN:   CJNE    A,#ASCII_Down,Kb_Ctrl_RET
                LCALL   Kb_Search_Down
                LJMP    Kb_Ctrl_End

Kb_Ctrl_RET:    CLR     KS_Active_Flg
                CJNE    A,#ASCII_CR,Kb_Ctrl_LEFT
                SETB    Kb_Str_Ready
                LJMP    Kb_Ctrl_End

Kb_Ctrl_LEFT:   CJNE    A,#ASCII_Left,Kb_Ctrl_RIGHT
                LCALL   Exe_LEFT
                LJMP    Kb_Ctrl_End

Kb_Ctrl_RIGHT:  CJNE    A,#ASCII_Right,Kb_Ctrl_DEL
                LCALL   Exe_RIGHT
                LJMP    Kb_Ctrl_End

Kb_Ctrl_DEL:    CJNE    A,#ASCII_Del,Kb_Ctrl_BKSPC
                LCALL   Exe_DEL
                LJMP    Kb_Ctrl_End

Kb_Ctrl_BKSPC:  CJNE    A,#ASCII_BkSpc,Kb_Ctrl_ESC
                LCALL   Exe_LEFT
                LCALL   Exe_DEL
                LJMP    Kb_Ctrl_End

Kb_Ctrl_ESC:    CJNE    A,#ASCII_Esc,Kb_Ctrl_Alpha
                MOV     DPTR,#Kb_Str_Buffer
                MOV     A,#ASCII_Space
                LCALL   Clear_Str
                MOV     Kb_Cursor,#00
                LJMP    Kb_Ctrl_End

Kb_Ctrl_Alpha:  LCALL   Exe_Set_Actual_Letter
                MOV     A,Kb_Char_Buffer
                MOVX    @DPTR,A
                LCALL   Exe_RIGHT

Kb_Ctrl_End:    SETB    Kb_Dsp_Ready
                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------

Exe_Set_Actual_Letter:                          ; laedt in DPTR die externale Adresse
                PUSH    Acc                     ; des vom Kb_Cursor gezeigten Zeichens
                PUSH    PSW
                MOV     A,Kb_Cursor
                MOV     DPTR,#Kb_Str_Buffer
                ADD     A,DPL
                MOV     DPL,A
                JNC     ESAL_End
                INC     DPH
ESAL_End:       POP     PSW
                POP     Acc
                RET

Exe_LEFT:       PUSH    AR1                     ; dekr. Kb_Cursor
                DEC     Kb_Cursor
                MOV     R1,Kb_Cursor
                CJNE    R1,#0FFh,Exe_LEFT_End
                MOV     Kb_Cursor,#00
Exe_LEFT_End:   POP     AR1
                RET

Exe_RIGHT:      PUSH    Acc                     ; inkr. Kb_Cursor
                PUSH    PSW
                INC     Kb_Cursor
                CLR     C
                MOV     A,#Kb_Max_Length-1
                SUBB    A,Kb_Cursor
                JNC     Exe_RIGHT_End
                MOV     Kb_Cursor,#Kb_Max_Length-1
Exe_RIGHT_End:  POP     PSW
                POP     Acc
                RET

Exe_DEL:        PUSH    Acc                     ; loescht aktuelles Zeichen
                LCALL   Exe_Set_Actual_Letter
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Kb_Init_Search
; Parameter          : Kb_Str_Buffer
;                      Kb_Cursor
; Rueckgabeparameter : KS_Status_Ready
;                      Kb_Search_Status
;                      KS_Active_Flg
;                      KS_Actual_Module
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;                      KS_Cursor
;
; Sucht, auf welchem Wort der Cursor sich befindet und zeigt das erste ent-
; sprechende Element aus der Search-Menu-Tabelle (zB : wenn Cursor auf Message,
; sucht die erste Message fuer das schon eingetragene Modul).


Kb_Init_Search: PUSH    Acc
                PUSH    B
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                LCALL   Kb_Valid_Word
                MOV     A,KS_Actual_Word
                CJNE    A,#1,KIS_Msg                    ; Cursor auf 1. Wort -> Modul

                MOV     Kb_Search_DPL,#(Module_Table # 256)
                MOV     Kb_Search_DPH,#(Module_Table / 256)
                LCALL   Search_Next_Module
                JC      KIS_No_Choice                   ; springt wenn noch kein Modul in der Tabelle
                MOV     DPTR,#Kb_Str_Buffer             ;
                MOV     A,#ASCII_Space                  ; loescht Kb_Str_Buffer
                LCALL   Clear_Str                       ;
                MOV     Kb_Cursor,#00                   ;
                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                INC     DPTR
                INC     DPTR
                MOV     Kb_Search_Status,#KSS_Mod
                SJMP    KIS_Found

KIS_Msg:        CJNE    A,#2,KIS_No_Choice
                MOV     Kb_Str_Pointer,#00              ; Cursor auf 2. Wort -> Message
                LCALL   Get_Token
                LCALL   Is_Token_Module
                JNC     KIS_Mod_Corr
                MOV     Kb_Search_Status,#KSS_Inc_Mod   ; erstes Wort kein Korrektes Modul
                SJMP    KIS_End

KIS_Mod_Corr:   MOV     Kb_Cursor,Kb_Str_Pointer
                LCALL   Exe_Right                       ; ein Leerzeichen nach dem 1. Wort lassen
                MOV     KS_Actual_Module,A              ; Modulnummer abspeichern
                MOV     Kb_Search_DPL,#(Stat_Table # 256)
                MOV     Kb_Search_DPH,#(Stat_Table / 256)
                LCALL   Search_Next_Msg
                JC      KIS_No_Choice                   ; existiert Message fuer dieses Modul ?
                MOV     DPTR,#Kb_Str_Buffer             ; Ja -> Mess. in Textbuffer kopieren
                MOV     A,#ASCII_Space
                MOV     B,Kb_Cursor
                LCALL   Clear_Pos_Str                   ; Kb_Str_Buffer ab der Cursorposition loeschen
                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                INC     DPTR
                INC     DPTR
                INC     DPTR
                MOV     Kb_Search_Status,#KSS_Msg
                SJMP    KIS_Found

KIS_No_Choice:  MOV     Kb_Search_Status,#KSS_No_Choice ; Nein -> Fehlermeldung
                SJMP    KIS_End

KIS_Found:      MOV     KS_Cursor,Kb_Cursor             ; kopiert das gefundene Element
                LCALL   Copy_Pos_Buffer                 ; in Kb_Str_Buffer ab KS_Cursor
                SETB    KS_Active_Flg                   ;

KIS_End:        SETB    KS_Status_Ready
                POP     DPH
                POP     DPL
                POP     PSW
                POP     B
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Kb_Valid_Word
; Parameter          : Kb_Str_Buffer
;                      Kb_Cursor
; Rueckgabeparameter : KS_Actual_Word
;
; Sucht auf welchem Wort der Cursor sich befindet


Kb_Valid_Word:  PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                MOV     DPTR,#Kb_Str_Buffer
                MOV     Kb_Str_Pointer,#00
                MOV     KS_Actual_Word,#00

KVW_Loop:       LCALL   Get_Token
                INC     KS_Actual_Word
                CLR     C
                MOV     A,Kb_Str_Pointer
                SUBB    A,Kb_Cursor             ; wenn Kb_Str_Pointer > Kb_Cursor
                JC      KVW_Loop                ; hat man das richtige Wort gefunden

                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Kb_Search_Up
; Parameter          : KS_Active_Flg
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;                      KS_Cursor
; Rueckgabeparameter : KS_Active_Flg
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;
; Legt fest, ob die Search-Routinen initialisiert werden muessen, ob das vorhergehende
; Modul, oder Message, abgeholt werden muss.


Kb_Search_Up:   PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                JB      KS_Active_Flg,KSU_Choose        ; Schon im Search-Modus ?
                LCALL   Kb_Init_Search                  ; Nein -> Initialisierung
                SJMP    KSU_End

KSU_Choose:     MOV     A,KS_Actual_Word                ; Ja -> auf welchem Wort liegt der Cursor ?
                CJNE    A,#1,KSU_Msg
                LCALL   Search_Prev_Module              ; 1. Wort -> sucht Modul
                JC      KSU_Mod_Top                     ; gefunden ?
                MOV     Kb_Search_Status,#KSS_Mod       ; Ja -> DPTR retten
                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                INC     DPTR
                INC     DPTR
                SJMP    KSU_Show

KSU_Mod_Top:    MOV     Kb_Search_Status,#KSS_Mod_Top   ; Nein -> Top of list
                SJMP    KSU_End

KSU_Msg:        LCALL   Search_Prev_Msg                 ; 2. Wort -> sucht Message
                JC      KSU_Msg_Top                     ; gefunden ?
                MOV     Kb_Search_Status,#KSS_Msg       ; Ja -> DPTR retten
                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                INC     DPTR
                INC     DPTR
                INC     DPTR
                SJMP    KSU_Show

KSU_Msg_Top:    MOV     Kb_Search_Status,#KSS_Msg_Top   ; Nein -> Top of list
                SJMP    KSU_End

KSU_Show:       MOV     Kb_Cursor,KS_Cursor             ; gefundenes Wort in Textbuffer
                PUSH    DPL                             ; kopieren
                PUSH    DPH
                MOV     DPTR,#Kb_Str_Buffer
                MOV     A,#ASCII_Space
                MOV     B,Kb_Cursor
                LCALL   Clear_Pos_Str
                POP     DPH
                POP     DPL
                LCALL   Copy_Pos_Buffer

KSU_End:        SETB    KS_Status_Ready
                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Kb_Search_Down
; Parameter          : KS_Active_Flg
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;                      KS_Cursor
; Rueckgabeparameter : KS_Active_Flg
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;
; Legt fest, ob die Search-Routinen initialisiert werden muessen, ob das naechste
; Modul, oder Message, abgeholt werden muss.


Kb_Search_Down: PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                JB      KS_Active_Flg,KSD_Choose        ; schon im Search Modus ?
                LCALL   Kb_Init_Search                  ; Nein -> Initialisierung
                SJMP    KSD_End

KSD_Choose:     MOV     A,KS_Actual_Word                ; Ja -> welches Wort ?
                CJNE    A,#1,KSD_Msg
                LCALL   Search_Next_Module              ; 1. Wort -> sucht naechstes Modul
                JC      KSD_Mod_Bot                     ; gefunden ?
                MOV     Kb_Search_Status,#KSS_Mod       ; Ja -> DPTR retten
                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                INC     DPTR
                INC     DPTR
                SJMP    KSD_Show

KSD_Mod_Bot:    MOV     Kb_Search_Status,#KSS_Mod_Bot   ; Nein -> bottom of list
                SJMP    KSD_End

KSD_Msg:        LCALL   Search_Next_Msg                 ; 2. Wort -> sucht naechste Message
                JC      KSD_Msg_Bot                     ; gefunden ?
                MOV     Kb_Search_Status,#KSS_Msg       ; Ja -> DPTR retten
                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                INC     DPTR
                INC     DPTR
                INC     DPTR
                SJMP    KSD_Show

KSD_Msg_Bot:    MOV     Kb_Search_Status,#KSS_Msg_Bot   ; Nein -> bottom of list
                SJMP    KSD_End

KSD_Show:       MOV     Kb_Cursor,KS_Cursor             ; gefundenes Wort in Textbuffer
                PUSH    DPL                             ; kopieren
                PUSH    DPH
                MOV     DPTR,#Kb_Str_Buffer
                MOV     A,#ASCII_Space
                MOV     B,Kb_Cursor
                LCALL   Clear_Pos_Str
                POP     DPH
                POP     DPL
                LCALL   Copy_Pos_Buffer

KSD_End:        SETB    KS_Status_Ready
                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Search_Next_Module
; Parameter          : Kb_Search_DPL
;                      Kb_Search_DPH
; Rueckgabeparameter : C  (gesetzt -> nicht gefunden)
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;
; Sucht naechstes Modul ab aktueller Position in der Tabelle (durch KB_Search_DPL
; (DPH) gespeichert)


Search_Next_Module:
                PUSH    Acc
                PUSH    DPL
                PUSH    DPH

                MOV     DPL,Kb_Search_DPL       ; aktuelle Pos. in DPTR laden
                MOV     DPH,Kb_Search_DPH
                MOV     A,#1
                MOVC    A,@A+DPTR
                ADD     A,DPL                   ; DPTR mit Offset addieren
                MOV     DPL,A
                JNC     SNMod_Cont
                INC     DPH
SNMod_Cont:     CLR     C
                CLR     A
                MOVC    A,@A+DPTR
                XRL     A,#TableEnd
                JZ      SNMod_End               ; Ende der Tabelle ?
                MOV     Kb_Search_DPL,DPL       ; Nein -> DPTR retten
                MOV     Kb_Search_DPH,DPH
                CPL     C

SNMod_End:      CPL     C                       ; Ja -> nicht gefunden
                POP     DPH
                POP     DPL
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Search_Prev_Module
; Parameter          : Kb_Search_DPL
;                      Kb_Search_DPH
; Rueckgabeparameter : C  (gesetzt -> nicht gefunden)
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;
; Sucht vorhergehendes Modul ab aktueller Position in der Tabelle (durch KB_Search_DPL
; (DPH) gespeichert). Analog zu Search_Next_Module


Search_Prev_Module:
                PUSH    Acc
                PUSH    AR1
                PUSH    DPL
                PUSH    DPH

                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH
                DEC_DPTR
                CLR     A
                MOVC    A,@A+DPTR
                INC     DPTR
                CLR     C
                MOV     R1,A
                MOV     A,DPL
                SUBB    A,R1
                MOV     DPL,A
                JNC     SPMod_Cont
                DEC     DPH
SPMod_Cont:     CLR     C
                CLR     A
                MOVC    A,@A+DPTR
                XRL     A,#TableAnf
                JZ      SPMod_End
                MOV     Kb_Search_DPL,DPL
                MOV     Kb_Search_DPH,DPH
                CPL     C

SPMod_End:      CPL     C
                POP     DPH
                POP     DPL
                POP     AR1
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Search_Next_Msg
; Parameter          : Kb_Search_DPL
;                      Kb_Search_DPH
;                      KS_Actual_Module
; Rueckgabeparameter : C  (gesetzt -> nicht gefunden)
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;
; Sucht naechste Message ab aktueller Position in der Tabelle (durch KB_Search_DPL
; (DPH) gespeichert), die KS_Actual_Module entspricht


Search_Next_Msg:
                PUSH    Acc
                PUSH    DPL
                PUSH    DPH

                MOV     DPL,Kb_Search_DPL       ; aktuelle Pos. in DPTR laden
                MOV     DPH,Kb_Search_DPH

SNMsg_Loop:     MOV     A,#1
                MOVC    A,@A+DPTR
                ADD     A,DPL                   ; DPTR und Offset addieren
                MOV     DPL,A
                JNC     SNMsg_Cont
                INC     DPH
SNMsg_Cont:     CLR     C
                CLR     A
                MOVC    A,@A+DPTR
                CJNE    A,#TableEnd,SNMsg_Mod   ; Ende der Tabelle ?
                SETB    C                       ; Ja -> nicht gefunden
                SJMP    SNMsg_End
SNMsg_Mod:      CJNE    A,KS_Actual_Module,SNMsg_Loop ; Nein -> Modulnummer korrekt ?
                                                      ;         Nein -> sucht weiter
                MOV     Kb_Search_DPL,DPL             ;         Ja -> DPTR retten
                MOV     Kb_Search_DPH,DPH
                CLR     C

SNMsg_End:      POP     DPH
                POP     DPL
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Search_Prev_Msg
; Parameter          : Kb_Search_DPL
;                      Kb_Search_DPH
;                      KS_Actual_Module
; Rueckgabeparameter : C  (gesetzt -> nicht gefunden)
;                      Kb_Search_DPL
;                      Kb_Search_DPH
;
; Sucht vorhergehende Message ab aktueller Position in der Tabelle (durch KB_Search_DPL
; (DPH) gespeichert), die KS_Actual_Module entspricht. Analog zu Search_Next_Msg


Search_Prev_Msg:
                PUSH    Acc
                PUSH    DPL
                PUSH    DPH
                PUSH    AR1

                MOV     DPL,Kb_Search_DPL
                MOV     DPH,Kb_Search_DPH

SPMsg_Loop:     DEC_DPTR
                CLR     A
                MOVC    A,@A+DPTR
                INC     DPTR
                CLR     C
                MOV     R1,A
                MOV     A,DPL
                SUBB    A,R1
                MOV     DPL,A
                JNC     SPMsg_Cont
                DEC     DPH
SPMsg_Cont:     CLR     C
                CLR     A
                MOVC    A,@A+DPTR
                CJNE    A,#TableAnf,SPMsg_Mod
                SETB    C
                SJMP    SPMsg_End
SPMsg_Mod:      CJNE    A,KS_Actual_Module,SPMsg_Loop
                MOV     Kb_Search_DPL,DPL
                MOV     Kb_Search_DPH,DPH
                CLR     C

SPMsg_End:      POP     AR1
                POP     DPH
                POP     DPL
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine            : Text_Decoder
; Parameter          : Kb_Str_Buffer
; Rueckgabeparameter : Msg_Registers
;                      Text_Dec_Status
;                      TD_Status_Ready
;                      TD_Send_Ready
;                      Stat_Module
;                      Stat_Num_Param
;                      Stat_Picture
;                      Stat_Length
;                      Stat_Code
;
; Interpretiert den im Kb_Str_Buffer liegenden Text und legt die entsprechenden
; Werte in Msg_Registers und Stat_ Variablen ab. Wenn korrekter Text, setzt das
; TD_Send_Ready-Flag (ready to send).


Text_Decoder:   PUSH    AR0
                PUSH    AR1
                PUSH    AR2
                PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH

                LCALL   Clr_Msg_Buffer
                MOV     Stat_Length,#02

                CLR     Kb_Str_Ready
                MOV     Kb_Str_Pointer,#00
                LCALL   Get_Token               ; sucht 1. Wort
                JNC     TD_Module               ; gefunden ?
                MOV     Text_Dec_Status,#5      ; Nein -> Fehler (fehlendes Modul)
                LJMP    TD_End

TD_Module:      LCALL   Is_Token_Module         ; Ja -> ist das Modul korrekt ?
                JNC     TD_Statement
                MOV     Text_Dec_Status,#6      ; Nein -> Fehler (inkorrektes Modul)
                LJMP    TD_End

TD_Statement:   MOV     Stat_Module,A           ; Ja -> Modulnummer abspeichern
                LCALL   Get_Token               ; sucht 2. Wort
                JNC     TD_Stat_Cont            ; gefunden ?
                MOV     Text_Dec_Status,#1      ; Nein -> Fehler (fehlende Message)
                LJMP    TD_End

TD_Stat_Cont:   MOV     R0,#(Token_Str # 256)   ; Ja -> sucht Message in der Tabelle
                MOV     R1,#(Token_Str / 256)
                MOV     DPTR,#Stat_Table

TD_Stat_Loop:   CLR     A                       ;
                MOVC    A,@A+DPTR               ;
                CJNE    A,#TableEnd,TD_Stat_Cont2
                MOV     Text_Dec_Status,#2      ; nur die Messages der Tabelle, die
                LJMP    TD_End                  ; das aktuelle Modul entsprechen, muessen
                                                ; betrachtet werden
TD_Stat_Cont2:  XRL     A,Stat_Module           ;
                JZ      TD_Stat_Check           ;

TD_Stat_Next:   MOV     A,#01                   ;
                MOVC    A,@A+DPTR               ;
                ADD     A,DPL                   ; sucht naechste Message in der
                MOV     DPL,A                   ; Tabelle
                JNC     TD_Stat_Loop            ;
                INC     DPH                     ;
                SJMP    TD_Stat_Loop            ;

TD_Stat_Check:  INC     DPTR
                INC     DPTR
                CLR     A
                MOVC    A,@A+DPTR
                MOV     Stat_Code,A
                INC     DPTR

                LCALL   Compare_Str             ; Text und Message in der Tabelle
                JNC     TD_Parameters           ; vergleichen
                DEC_DPTR
                DEC_DPTR
                DEC_DPTR
                SJMP    TD_Stat_Next            ; nicht gleich -> next one !

TD_Parameters:  LCALL   Jump_Blank_Str          ; gleich -> Parameter dekodieren
                MOV     R0,#Msg_Registers
                MOV     @R0,Stat_Module

                INC     R0
                MOV     @R0,Stat_Code

                INC     DPTR
                CLR     A
                MOVC    A,@A+DPTR
                MOV     Stat_Num_Param,A
                MOV     R1,A
                JZ      TD_Send

                INC     DPTR
                CLR     A
                MOVC    A,@A+DPTR
                MOV     Stat_Picture,A
                INC     R0

TD_Par_Loop:    LCALL   Get_Token
                JNC     TD_Par_Symbol
                MOV     Text_Dec_Status,#3
                LJMP    TD_End

TD_Par_Symbol:  CLR     C
                LCALL   Is_Token_Symbol
                JC      TD_Par_Digit
                MOV     ASCII_Low_Byte,A
                MOV     ASCII_High_Byte,#00
                SJMP    TD_Par_Load

TD_Par_Digit:   CLR     C
                LCALL   ASCII_To_Bin
                JNC     TD_Par_Load
                MOV     Text_Dec_Status,#4
                SJMP    TD_End

TD_Par_Load:    MOV     A,Stat_Picture
                JB      Acc.0,TD_Par_Single
                MOV     @R0,ASCII_High_Byte
                MOV     ASCII_High_Byte,#00
                INC     R0
                INC     Stat_Length

TD_Par_Single:  MOV     R2,ASCII_High_Byte
                CJNE    R2,#00,TD_Par_Error
                MOV     @R0,ASCII_Low_Byte
                INC     R0
                INC     Stat_Length
                RR      A
                MOV     Stat_Picture,A
                DJNZ    R1,TD_Par_Loop

TD_Send:        MOV     Text_Dec_Status,#0
                SETB    TD_Send_Ready
                LCALL   Get_Token
                JC      TD_End
                MOV     Text_Dec_Status,#7
                SJMP    TD_End

TD_Par_Error:   MOV     Text_Dec_Status,#4

TD_End:         SETB    TD_Status_Ready
                SETB    TD_Next_Flg
                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                POP     AR2
                POP     AR1
                POP     AR0
                RET

;--------------------------------------------------------------------------

Get_Token:      PUSH    Acc
                PUSH    P2
                PUSH    DPL
                PUSH    DPH
                PUSH    AR0
                PUSH    AR1
                PUSH    AR2

                MOV     DPTR,#Token_Str
                CLR     A
                LCALL   Clear_Str
                MOV     DPTR,#Kb_Str_Buffer
                MOV     A,#Kb_Max_Length        ;
                CLR     C                       ;
                SUBB    A,Kb_Str_Pointer        ;
                JNZ     GT_Cont                 ; R2 = Anzahl der noch
                SETB    C                       ; zuverarbeitenden
                SJMP    GT_End                  ; Buchstaben
                                                ;
GT_Cont:        MOV     R2,A                    ;
                MOV     A,DPL
                ADD     A,Kb_Str_Pointer
                MOV     DPL,A
                JNC     GT_Blank_Loop
                INC     DPH

GT_Blank_Loop:  MOVX    A,@DPTR
                CJNE    A,#ASCII_Space,GT_Text
                INC     DPTR
                INC     Kb_Str_Pointer
                DJNZ    R2,GT_Blank_Loop
                SETB    C
                SJMP    GT_End

GT_Text:        MOV     R0,#(Token_Str # 256)
                MOV     R1,#(Token_Str / 256)

GT_Text_Loop:   MOVX    A,@DPTR
                CJNE    A,#ASCII_Space,GT_Text_Add
                CLR     C
                SJMP    GT_End

GT_Text_Add:    LCALL   UpCase
                MOV     P2,R1
                MOVX    @R0,A
                INC     Kb_Str_Pointer
                INC_R0R1
                INC     DPTR
                DJNZ    R2,GT_Text_Loop
                CLR     C

GT_End:         POP     AR2
                POP     AR1
                POP     AR0
                POP     DPH
                POP     DPL
                POP     P2
                POP     Acc
                RET

;--------------------------------------------------------------------------

Compare_Str:    IRP     Source,Acc,P2,DPL,DPH,AR0,AR1,AR2,AR3
                PUSH    Source
                ENDM

                CLR     C
                MOV     R2,#Kb_Max_Length

                CLR     A
                MOVC    A,@A+DPTR
                CJNE    A,#StringEnde,Comp_Loop
                SJMP    Comp_False

Comp_Loop:      MOV     R3,A
                MOV     P2,R1
                MOVX    A,@R0
                XRL     A,R3
                JNZ     Comp_False
                MOV     A,R3
                JZ      Comp_End
                INC     DPTR
                INC_R0R1
                CLR     A
                MOVC    A,@A+DPTR
                DJNZ    R2,Comp_Loop
                CPL     C

Comp_False:     CPL     C

Comp_End:       IRP     Target,AR3,AR2,AR1,AR0,DPH,DPL,P2,Acc
                POP     Target
                ENDM
                RET

;--------------------------------------------------------------------------
TeachIn_Sampler:
                PUSH    Acc
                PUSH    PSW
                PUSH    AR1
                MOV     TH0,#0FCh

                MOV     Temp_P4,P4
                MOV     Temp_P5,P5
                MOV     A,Temp_P4
                XRL     A,Save_P4
                JNZ     TI_Smp_Edge
                MOV     A,Temp_P5
                XRL     A,Save_P5
                JZ      TI_Smp_Inc

TI_Smp_Edge:    MOV     TI_Sample_Counter,#00
                MOV     Save_P4,Temp_P4
                MOV     Save_P5,Temp_P5
                SJMP    TI_Smp_End

TI_Smp_Inc:     INC     TI_Sample_Counter
                MOV     A,TI_Sample_Counter
                CJNE    A,#TI_Sample_Valid_Time,TI_Smp_End
                MOV     TI_Sample_Counter,#00
                MOV     A,Old_P4
                XRL     A,Save_P4
                JNZ     TI_Smp_Change
                MOV     A,Old_P5
                XRL     A,Save_P5
                JZ      TI_Smp_End

TI_Smp_Change:  SETB    TI_Sample_Chg_Flg
                JNB     TID_Done_Flg,TISC_No_Init
                CLR     TID_Done_Flg
                MOV     TI_On_P4,#00
                MOV     TI_Off_P4,#00
                MOV     TI_On_P5,#00
                MOV     TI_Off_P5,#00

TISC_No_Init:   MOV     A,Old_P4
                XRL     A,Save_P4
                MOV     R1,A                    ; R1 = Save_P4
                MOV     A,Save_P4
                CPL     A
                ANL     A,R1
                ORL     A,TI_On_P4
                MOV     TI_On_P4,A

                MOV     A,Save_P4
                ANL     A,R1
                MOV     TI_Off_P4,A
                MOV     A,Old_P5
                XRL     A,Save_P5
                MOV     R1,A
                MOV     A,Save_P5
                CPL     A
                ANL     A,R1
                MOV     TI_On_P5,A
                MOV     A,Save_P5
                ANL     A,R1
                MOV     TI_Off_P5,A

                MOV     Old_P4,Save_P4
                MOV     Old_P5,Save_P5

TI_Smp_End:     POP     AR1
                POP     PSW
                POP     Acc
                RETI

;--------------------------------------------------------------------------

TeachIn_Decoder:
                PUSH    Acc
                PUSH    DPL
                PUSH    DPH

                CLR     TI_Sample_Chg_Flg
                MOV     A,TI_On_P4
                JZ      TID_Table2
                JB      Sys_Robot_Mode,TID_T1_Rob
                MOV     DPTR,#KOn_P4_Frs
                LCALL   TID_Main
                SJMP    TID_Table2

TID_T1_Rob:     MOV     DPTR,#KOn_P4_Rob
                LCALL   TID_Main

TID_Table2:     MOV     A,TI_Off_P4
                JZ      TID_Table3
                JB      Sys_Robot_Mode,TID_T2_Rob
                MOV     DPTR,#KOff_P4_Frs
                LCALL   TID_Main
                SJMP    TID_Table3

TID_T2_Rob:     MOV     DPTR,#KOff_P4_Rob
                LCALL   TID_Main

TID_Table3:     MOV     A,TI_On_P5
                JZ      TID_Table4
                JB      Sys_Robot_Mode,TID_T3_Rob
                MOV     DPTR,#KOn_P5_Frs
                LCALL   TID_Main
                SJMP    TID_Table4

TID_T3_Rob:     MOV     DPTR,#KOn_P5_Rob
                LCALL   TID_Main

TID_Table4:     MOV     A,TI_Off_P5
                JZ      TID_End
                JB      Sys_Robot_Mode,TID_T4_Rob
                MOV     DPTR,#KOff_P5_Frs
                LCALL   TID_Main
                SJMP    TID_End

TID_T4_Rob:     MOV     DPTR,#KOff_P5_Rob
                LCALL   TID_Main

TID_End:        SETB    TID_Done_Flg

                POP     DPH
                POP     DPL
                POP     Acc
                RET

;--------------------------------------------------------------------------

TID_Main:       PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH
                PUSH    AR0
                PUSH    AR1

                MOV     R1,#8
TID_Main_Loop:  CLR     C
                RRC     A
                JNC     TID_Main_Next

                PUSH    Acc
                MOV     A,#1
                MOVC    A,@A+DPTR
                MOV     R0,A
                POP     Acc
                CJNE    R0,#StringEnde,TID_Main_Msg
                SJMP    TID_Main_Next

TID_Main_Msg:   PUSH    DPL
                PUSH    DPH
                PUSH    Acc
                MOV     DPTR,#Kb_Str_Buffer
                MOV     A,#ASCII_Space
                LCALL   Clear_Str
                POP     Acc
                POP     DPH
                POP     DPL

                INC     DPTR
                MOV     Kb_Cursor,#00
                LCALL   Copy_Pos_Buffer
                SETB    Kb_Str_Ready
                SETB    Kb_Dsp_Ready
                CLR     KS_Active_Flg
                DEC_DPTR
                LCALL   Main_Manager

TID_Main_Next:  PUSH    Acc
                CLR     A
                MOVC    A,@A+DPTR
                ADD     A,DPL
                MOV     DPL,A
                JNC     TIDM_Next_Cont
                INC     DPH

TIDM_Next_Cont: POP     Acc
                DJNZ    R1,TID_Main_Loop

                POP     AR1
                POP     AR0
                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------

Send_Manager:   PUSH    Acc
                PUSH    B
                PUSH    DPL
                PUSH    DPH
                PUSH    AR0
                PUSH    PSW

                CLR     TD_Send_Ready

Send_Mng_Load:  MOV     R0,#Msg_Registers
                MOV     R0_Bk1,@R0
                MOV     A,@R0                   ; logische Adresse
                INC     R0
                MOV     R1_Bk1,@R0
                INC     R0
                MOV     R2_Bk1,@R0
                INC     R0
                MOV     R3_Bk1,@R0
                INC     R0
                MOV     R4_Bk1,@R0
                INC     R0
                MOV     R5_Bk1,@R0
                INC     R0
                MOV     R6_Bk1,@R0
                INC     R0
                MOV     R7_Bk1,@R0

                MOV     DPTR,#Adr_Table
                MOVC    A,@A+DPTR
                MOV     B,Stat_Length
                SETB    RS0
                CLR     RS1

                Post_Message

                POP     PSW
                POP     AR0
                POP     DPH
                POP     DPL
                POP     B
                POP     Acc
                RET
;--------------------------------------------------------------------------

Receive_Sampler:
                lcall MESSAGE_INTERRUPT
;                SETB    Receive_Ready
                RET


Receive_Manager:
                PUSH    Acc
                PUSH    B
                PUSH    PSW

                CLR     Receive_Ready
                SETB    RS0
                CLR     RS1

                Take_Message
                CLR     RS0
                MOV     Rcv_Msg_Length,A
                MOV     DPTR,#Net_Rcv_Str
                MOV     A,#ASCII_Space
                LCALL   Clear_Str

                MOV     A,R1_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                INC     DPTR

                MOV     A,R2_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                INC     DPTR

                MOV     A,R3_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                INC     DPTR

                MOV     A,R4_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                INC     DPTR

                MOV     A,R5_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                INC     DPTR

                MOV     A,R6_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A
                INC     DPTR

                MOV     A,R7_Bk1
                LCALL   Bin_To_ASCII
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,B
                MOVX    @DPTR,A
                INC     DPTR
                MOV     A,#ASCII_Space
                MOVX    @DPTR,A

                MOV     DPTR,#Net_Rcv_Str
                SETB    Ext_Dsp_Ready

                POP     PSW
                POP     B
                POP     Acc
                RET

;=============================== Tools ====================================

Is_Token_Symbol:
                PUSH    AR0
                PUSH    AR1
                PUSH    AR2
                PUSH    AR3
                PUSH    DPL
                PUSH    DPH

                CLR     C
                MOV     DPTR,#Symbol_Table
                MOV     R0,#(Token_Str # 256)
                MOV     R1,#(Token_Str / 256)

Is_Symb_Loop:   CLR     A
                MOVC    A,@A+DPTR
                MOV     R3,A            ; Symbolwert
                XRL     A,#TableEnd
                JZ      Is_Symb_Not_Found

                INC     DPTR
                CLR     A
                MOVC    A,@A+DPTR
                MOV     R2,A            ; Offset

                INC     DPTR
                LCALL   Compare_Str
                JNC     Is_Symb_Found

                DEC_DPTR
                DEC_DPTR
                MOV     A,DPL
                ADD     A,R2
                MOV     DPL,A
                JNC     Is_Symb_Loop
                INC     DPH
                SJMP    Is_Symb_Loop

Is_Symb_Found:  MOV     A,R3
                CLR     C
                SJMP    Is_Symb_End

Is_Symb_Not_Found:
                SETB    C

Is_Symb_End:    POP     DPH
                POP     DPL
                POP     AR3
                POP     AR2
                POP     AR1
                POP     AR0
                RET

;--------------------------------------------------------------------------

Is_Token_Module:
                PUSH    AR0
                PUSH    AR1
                PUSH    AR2
                PUSH    AR3
                PUSH    DPL
                PUSH    DPH

                CLR     C
                MOV     DPTR,#Module_Table
                MOV     R0,#(Token_Str # 256)
                MOV     R1,#(Token_Str / 256)

Is_Mod_Loop:    CLR     A
                MOVC    A,@A+DPTR
                MOV     R3,A            ; Modulname
                XRL     A,#TableEnd
                JZ      Is_Mod_Not_Found

                INC     DPTR
                CLR     A
                MOVC    A,@A+DPTR
                MOV     R2,A            ; Offset

                INC     DPTR
                LCALL   Compare_Str
                JNC     Is_Mod_Found

                DEC_DPTR
                DEC_DPTR
                MOV     A,DPL
                ADD     A,R2
                MOV     DPL,A
                JNC     Is_Mod_Loop
                INC     DPH
                SJMP    Is_Mod_Loop

Is_Mod_Found:   MOV     A,R3
                CLR     C
                SJMP    Is_Mod_End

Is_Mod_Not_Found:
                SETB    C

Is_Mod_End:     POP     DPH
                POP     DPL
                POP     AR3
                POP     AR2
                POP     AR1
                POP     AR0
                RET

;--------------------------------------------------------------------------

Bin_To_ASCII:   PUSH    AR0
                PUSH    DPL
                PUSH    DPH

                MOV     DPTR,#BTA_Table
                MOV     B,#16
                DIV     AB
                MOVC    A,@A+DPTR
                MOV     R0,A
                MOV     A,B
                MOVC    A,@A+DPTR
                MOV     B,A
                MOV     A,R0

                POP     DPH
                POP     DPL
                POP     AR0
                RET

BTA_Table:      DB      "0123456789ABCDEF"

;--------------------------------------------------------------------------

ASCII_To_Bin:   IRP     Source,Acc,P2,DPL,DPH,AR0,AR1,AR2,AR3
                PUSH    Source
                ENDM

                MOV     R0,#(Token_Str # 256)
                MOV     R1,#(Token_Str / 256)
                MOV     DPTR,#ATB_Table
                MOV     ASCII_Low_Byte,#00
                MOV     ASCII_High_Byte,#00
                MOV     R2,#00

ATB_Search:     INC_R0R1
                INC     R2
                MOV     P2,R1
                MOVX    A,@R0
                JNZ     ATB_Search

                DEC_R0R1

ATB_Loop:       CLR     C
                MOV     P2,R1
                MOVX    A,@R0
                LCALL   Is_Digit
                JC      ATB_Not_Digit
                MOV     R3,A
                JZ      ATB_Next

ATB_Add_Loop:   CLR     A
                MOVC    A,@A+DPTR
                CJNE    A,#0FFh,ATB_Add_Cont
                SJMP    ATB_False
ATB_Add_Cont:   ADD     A,ASCII_Low_Byte
                MOV     ASCII_Low_Byte,A
                MOV     A,#01
                MOVC    A,@A+DPTR
                ADDC    A,ASCII_High_Byte
                JC      ATB_End
                MOV     ASCII_High_Byte,A
                DJNZ    R3,ATB_Add_Loop

ATB_Next:       INC     DPTR
                INC     DPTR
                DEC_R0R1
                DJNZ    R2,ATB_Loop

                CLR     C                               ;
                MOV     A,ASCII_High_Byte               ; Overflow (+) ?
                MOV     C,Acc.7                         ;
                SJMP    ATB_End                         ;

ATB_Not_Digit:  CJNE    A,#45,ATB_False
                CJNE    R2,#1,ATB_False
                CLR     C
                CLR     A
                SUBB    A,ASCII_Low_Byte
                MOV     ASCII_Low_Byte,A
                CLR     A
                SUBB    A,ASCII_High_Byte
                MOV     ASCII_High_Byte,A

                CLR     C                               ;
                MOV     A,ASCII_High_Byte               ;
                MOV     C,Acc.7                         ; Overflow (-) ?
                CPL     C                               ;
                SJMP    ATB_End                         ;

ATB_False:      SETB    C

ATB_End:        IRP     Target,AR3,AR2,AR1,AR0,DPH,DPL,P2,Acc
                POP     Target
                ENDM
                RET

ATB_Table:      DB      001h,000h
                DB      00Ah,000h
                DB      064h,000h
                DB      0E8h,003h
                DB      010h,027h
                DB      0FFh

;--------------------------------------------------------------------------

Jump_Blank_Str:
                PUSH    Acc

JB_Loop:        MOV     A,#00
                MOVC    A,@A+DPTR
                JZ      JB_End
                INC     DPTR
                SJMP    JB_Loop

JB_End:         POP     Acc
                RET

;--------------------------------------------------------------------------
;Routine :      Clear_Str
;Parameter:     A (Loeschzeichen)
;               DPTR (zu loeschender String)

Clear_Str:      PUSH    DPL
                PUSH    DPH
                PUSH    AR1
                MOV     R1,#Kb_Max_Length
Clear_Str_Loop: MOVX    @DPTR,A
                INC     DPTR
                DJNZ    R1,Clear_Str_Loop
                POP     AR1
                POP     DPH
                POP     DPL
                RET

;--------------------------------------------------------------------------
;Routine :      Clear_Pos_Str  (loescht einen String von Startposition bis Ende)
;Parameter:     DPTR    (zu loeschender String)
;               A       (Loeschzeichen)
;               B       (Startposition)


Clear_Pos_Str:  PUSH    Acc
                PUSH    PSW
                PUSH    DPL
                PUSH    DPH
                PUSH    AR1

                MOV     R1,B
                CJNE    R1,#Kb_Max_Length,CPS_Cont
                SJMP    CPS_End
CPS_Cont:       PUSH    Acc
                MOV     A,B
                ADD     A,DPL
                MOV     DPL,A
                JNC     CPS_Cont2
                INC     DPH

CPS_Cont2:      CLR     C
                MOV     A,#Kb_Max_Length
                SUBB    A,B
                MOV     R1,A
                POP     Acc
                JC      CPS_End
CPS_Loop:       MOVX    @DPTR,A
                INC     DPTR
                DJNZ    R1,CPS_Loop

CPS_End:        POP     AR1
                POP     DPH
                POP     DPL
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------
; Routine :     Copy_Pos_Buffer  (kopiert einen String in Kb_Str_Buffer
;                                 ab Kb_Cursor; dieser zeigt dann nach
;                                 dem letzten Zeichen des Strings)
; Parameter:    DPTR (zu kopierender String)


Copy_Pos_Buffer:
                PUSH    Acc
                PUSH    PSW
                PUSH    P2
                PUSH    DPL
                PUSH    DPH
                PUSH    AR0
                PUSH    AR1

                MOV     R0,#(Kb_Str_Buffer # 256)
                MOV     R1,#(Kb_Str_Buffer / 256)
                MOV     A,R0
                ADD     A,Kb_Cursor
                MOV     R0,A
                JNC     CPB_Loop
                INC     R1

CPB_Loop:       MOV     A,Kb_Cursor
                CJNE    A,#Kb_Max_Length,CPB_Loop_Cont
                DEC     Kb_Cursor
                SJMP    CPB_End

CPB_Loop_Cont:  CLR     A
                MOVC    A,@A+DPTR
                JZ      CPB_End
                MOV     P2,R1
                MOVX    @R0,A
                INC     DPTR
                INC     Kb_Cursor
                INC_R0R1
                SJMP    CPB_Loop

CPB_End:        POP     AR1
                POP     AR0
                POP     DPH
                POP     DPL
                POP     P2
                POP     PSW
                POP     Acc
                RET

;--------------------------------------------------------------------------

UpCase:         PUSH    PSW
                PUSH    AR0

                MOV     R0,A
                CLR     C
                SUBB    A,#97
                JC      UpCase_Rest
                MOV     A,#122
                SUBB    A,R0
                JC      UpCase_Rest
                MOV     A,R0
                SUBB    A,#32
                SJMP    UpCase_End
UpCase_Rest:    MOV     A,R0

UpCase_End:     POP     AR0
                POP     PSW
                RET

;--------------------------------------------------------------------------

Is_Digit:       PUSH    AR0

                CLR     C
                MOV     R0,A
                SUBB    A,#48
                JC      Is_Digit_Rest
                MOV     A,#57
                SUBB    A,R0
                JC      Is_Digit_Rest
                MOV     A,R0
                SUBB    A,#48
                SJMP    Is_Digit_End

Is_Digit_Rest:  MOV     A,R0

Is_Digit_End:   POP     AR0
                RET

;--------------------------------------------------------------------------

Wait_2s:        PUSH    AR0
                PUSH    AR1
                PUSH    AR2

                MOV     R2,#12
Wait_Loop2:     MOV     R1,#250
Wait_Loop1:     MOV     R0,#250
Wait_Loop0:     DJNZ    R0,Wait_Loop0
                DJNZ    R1,Wait_Loop1
                DJNZ    R2,Wait_Loop2

                POP     AR2
                POP     AR1
                POP     AR0
                RET

;--------------------------------------------------------------------------

Clr_Msg_Buffer: PUSH    AR0
                PUSH    AR1

                MOV     R1,#8
                MOV     R0,#Msg_Registers
Clr_Msg_Loop:   MOV     @R0,#00
                INC     R0
                DJNZ    R1,Clr_Msg_Loop

                POP     AR1
                POP     AR0
                RET

;------------------------------------------------------------------------------

;Stackarea in idata nach oben nur durch Prozessorram begrenzt!!
;Dieses Segment muá IMMER als letztes stehen!!!

        segment idata

Stack:          db ?                    ;ab hier liegt der Stack

;------------------------------------------------------------------------------

                end
