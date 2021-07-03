;******************************************************************************
;*                                                                            *
;*  Includedatei fuer SECMAIN.ASM                                             *
;*  liefert low-level-Routinen fuer SecMain                                   *
;*  Version hier fuer WD1003-kompatible Kontroller:                           *
;*  MFM, RLL, ESDI, AT-Bus                                                    *
;*                                                                            *
;*  Historie: 28.12.1994 herueberkopiert aus Hauptmodul                       *
;*            30.12.1994 LowLevelIdent                                        *
;*            19. 1.1995 Workaround fuer LCS6220                              *
;******************************************************************************

                section wd1003at

Base1           equ     170h            ; Basisadresse Task-File
Base2           equ     370h            ; Basisadresse Floppy-Teil
Task_Data       equ     Base1+0         ; Datentransferregister (R/W)
Task_Error      equ     Base1+1         ; genauerer Fehlercode (R)
Task_PreComp    equ     Base1+1         ; erster Zylinder Praekomp. (/4, nur W)
Task_SecCnt     equ     Base1+2         ; Zahl zu transferierender Sektoren (R/W)
Task_SecNum     equ     Base1+3         ; Startsektor (R/W)
Task_CylLo      equ     Base1+4         ; Startzylinder Bit 0..7 (R/W)
Task_CylHi      equ     Base1+5         ; Startzylinder Bit 8..n (R/W)
Task_DrHead     equ     Base1+6         ; Laufwerk/Startkopf (R/W)
Task_Status     equ     Base1+7         ; Status Laufwerk & Controller (R)
Task_Command    equ     Base1+7         ; Kommando Controller (W)
Task_FDiskReg   equ     Base2+6         ; Bit 3=1: >8 Koepfe

Cmd_Restore     equ     10h             ; Kommando: Rekalibrieren
Cmd_Seek        equ     70h             ; Kommando: Zylinder anfahren
Cmd_Read        equ     20h             ; Kommando: Sektoren lesen
Cmd_Write       equ     30h             ; Kommando: Sektoren schreiben
Cmd_Format      equ     50h             ; Kommando: Spur formatieren
Cmd_Verify      equ     40h             ; Kommando: Sektoren auf Lesbarkeit pruefen
Cmd_Diagnose    equ     90h             ; Kommando: Selbsttest
Cmd_SetParams   equ     91h             ; Kommando: Laufwerksparameter setzen

                proc    WriteParams

                mov     [axsave],ax
                mov     [cxsave],cx
                PrChar  ' '
                mov     ax,bx
                mov     cl,5
                call    WriteDec
                PrChar  ' '
                mov     al,byte ptr[axsave+1]
                mov     ah,0
                mov     cl,2
                call    WriteDec
                PrChar  ' '
                mov     al,byte ptr[cxsave+1]
                mov     ah,0
                mov     cl,2
                call    WriteDec
                PrChar  ' '
                mov     al,byte ptr[cxsave]
                mov     ah,0
                mov     cl,2
                call    WriteDec
                PrChar  ' '
                mov     ax,es
                mov     cl,4
                call    WriteHex
                PrChar  ':'
                mov     ax,bp
                mov     cl,4
                call    WriteHex
                mov     ax,[axsave]
                mov     cx,[cxsave]
                ret

cxsave          dw      ?
axsave          dw      ?

                endp

;******************************************************************************
;* Workaround fuer LCS6220: Wird direkt nach dem Einschalten ein Seek ausge-  *
;* fuehrt, gibt der Kontroller faelschlicherweise Daten aus und blockiert alle*
;* weiteren Kommandos.  Diese Routine raeumt einfach den Puffer leer...       *
;******************************************************************************

                proc    ClearBuffer

                push    dx              ; Register retten
                push    ax

RdLoop:         mov     dx,Task_Status  ; Bit 3 noch gesetzt ?
                in      al,dx
                btst    al,3
                jz      RdLoopEnd       ; nein --> fertig
                mov     dx,Task_Data
                in      ax,dx
                jmp     RdLoop
RdLoopEnd:
                pop     ax              ; Register zurueck
                pop     dx

                ret

                endp

;******************************************************************************
;* Interleave-Tabelle berechnen                                               *
;*            In    :   AL = Sektorzahl                                       *
;*                      AH = Interleave                                       *
;*                      DH = Bad-Flag                                         *
;******************************************************************************

                proc    SetInterleaveBuffer

                pusha                   ; Register retten
                push    es

                push    ax              ; Sektorpuffer initialisieren
                mov     ax,ds
                mov     es,ax
                sub     ax,ax
                lea     di,[SectorBuffer]
                mov     cx,SecSize/2
                cld
                rep     stosw
                pop     ax

                sub     di,di           ; DI=Adresse in Puffer=(phys. Sektor-1)*2
                mov     dl,dh           ; DL = Bad-Flag
                mov     dh,1            ; DH=log. Sektornummer
                mov     cl,al           ; CX=Schleifenzaehler
                mov     ch,0
                mov     bl,al           ; Sektorzahl*2 nach BX
                mov     bh,0
                add     bx,bx
                mov     si,ax           ; Interleave*2 nach SI
                shr     si,8
                add     si,si
InterLoop:      cmp     byte ptr SectorBuffer[di],0 ; Eintrag frei ?
                je      Inter_FreeFound ; ja, beenden
                add     di,2            ; nein, linear weitersuchen
                cmp     di,bx
                jb      InterLoop
                mov     di,0            ; Wrap-Around beruecksichtigen
                jmp     InterLoop
Inter_FreeFound:mov     word ptr SectorBuffer[di],dx ; Sektor einschreiben
                add     di,si           ; Interleave-Sprung dazu
                cmp     di,bx           ; Modulo Sektorzahl
                jb      Inter_NoWrap
                sub     di,bx
Inter_NoWrap:   inc     dh              ; naechster log. Sektor
                loop    InterLoop

                pop     es              ; Register zurueck
                popa

                ret

                endp

;******************************************************************************
;* Laufwerk und Sonderwerte einprogrammieren                                  *
;*              In  :   AL = Laufwerk                                         *
;*                      AH = Kopf                                             *
;******************************************************************************

                proc    SetDriveEnv

                push    di              ; Register retten
                push    dx
                mov     dx,ax           ; Laufwerk/Kopf retten

                call    GetPTabAdr      ; Tabellenadresse holen

                mov     al,dl           ; Laufwerk und Kopf zusammenbauen
                shl     al,4
                or      al,dh
                or      al,0a0h
                mov     dx,Task_DrHead
                out     dx,al
                mov     ax,[di+DrPar_PrComp] ; Startzylinder Praekompensation
                shr     ax,2
                mov     dl,Lo(Task_PreComp)
                out     dx,al
                mov     al,[di+DrPar_CByte] ; Wert fuer Fixed Disk Register
                mov     dx,Task_FDiskReg
                out     dx,al
                call    WaitBusy

                clc                     ; Ende ohne Fehler
                pop     dx
                pop     di
                ret

                endp

;******************************************************************************
;* Zylinder- und Sektorparameter an Kontroller ausgeben                       *
;*              In  :   BX = Startzylinder                                    *
;*                      CL = Sektorzahl                                       *
;*                      CH = Startsektor                                      *
;******************************************************************************

                proc    SetTransParams

                push    dx              ; Register retten

                mov     dx,Task_CylLo   ; Startzylinder programmieren
                mov     al,bl
                out     dx,al
                mov     dx,Task_CylHi ;***
                mov     al,bh
                out     dx,al
                mov     dx,Task_SecNum ; Startsektor... ;***
                mov     al,ch
                out     dx,al
                mov     dx,Task_SecCnt ; ...und Sektorzahl ;***
                mov     al,cl
                out     dx,al

                pop     dx             ; Register zurueck
                ret

                endp

;******************************************************************************
;* warten, bis Controller bereit oder Fehler                                  *
;*              Out :   AL = letzter Status                                   *
;******************************************************************************

                proc    WaitBusy

                push    dx              ; Register retten
                mov     dx,Task_Status  ; auf Statusregister
Loop:           in      al,dx           ; Status lesen
                btst    al,7            ; Bit 7 noch gesetzt ?
                jnz     Loop            ; ja--> weiter pollen
                pop     dx              ; Register zurueck
                ret

                endp

;******************************************************************************
;* warten, bis Laufwerk bereit                                                *
;*              Out :   AL = letzter Status                                   *
;******************************************************************************

                proc    WaitDrive

                push    dx              ; Register retten
                mov     dx,Task_Status  ; auf Statusregister
Loop:           in      al,dx           ; Status lesen
                btst    al,7            ; Bit 7 = 0 ? ( Controller Busy )
                jnz     Loop
                btst    al,6            ; Bit 6 = 1 ? ( Drive not Ready )
                jz      Loop
                btst    al,4            ; Bit 4 = 1 ? ( Seek not complete )
                jz      Loop
                pop     dx
                ret

                endp

;******************************************************************************
;* warten, bis Datentransfer erforderlich                                     *
;*              Out :   AL = letzter Status                                   *
;*                      C = 1, falls Fehler                                   *
;******************************************************************************

                proc    WaitData

                push    dx              ; Register retten
                mov     dx,Task_Status  ; auf Statusregister
Loop:           in      al,dx           ; Status lesen
                btst    al,7            ; Bit 7 = 0 ?
                jnz     Loop
                btst    al,3            ; Bit 3 = 1 ?
                jz      Loop
                pop     dx              ; Register zurueck
                ret

                endp

;******************************************************************************
;* Status bilden                                                              *
;*              Out : C+AX = Status                                           * 
;******************************************************************************

                proc    BuildError

                push    dx              ; Register retten

                mov     dx,Task_Status  ; Statusregister lesen
                in      al,dx
                mov     ah,al
                btst    ah,0            ; Fehlerflag gesetzt ?
                clc
                jz      End             ; kein Fehler
                
                mov     dx,Task_Error   ; ja: Error-Register lesen ;***
                in      al,dx
                stc

End:            pop     dx              ; Register zurueck
                ret

                endp

;******************************************************************************
;* Begráungsmeldung ausgeben:                                                *
;******************************************************************************

                globproc LowLevelIdent

                push    ax              ; Register retten

                PrMsg   IdentMsg

                pop     ax

                ret

IdentMsg        db      "Low-Level-Routinen f",UUML,"r WD1003-WAH und kompatible Controller",CR,LF,'$'

                endp

;******************************************************************************
;* Controller-Diagnose:                                                       *
;*              Out :  AL = Diagnosecode                                      *
;******************************************************************************

                globproc ContDiag

                push    cx              ; Register retten
                push    dx

                mov     dx,Task_Status  ; das erste Mal mit Timeout warten
                sub     cx,cx
BWait:          in      al,dx
                btst    al,7            ; auf NOT BUSY warten
                loopnz  BWait           ; oder bis 64K Durchlaeufe durch
                or      cx,cx           ; Timeout ?
                jne     NTOut
                mov     al,Diag_Timeout ; ja: Fehlercode setzen...
                jmp     End             ; ...und Feierabend

NTOut:          mov     al,CMD_Diagnose ; Selbsttest starten
                mov     dl,Lo(Task_Command)
                out     dx,al
                call    WaitBusy        ; auf Fertigstellung warten
                mov     dl,Lo(Task_Error) ; Ergebnis laden
                in      al,dx

End:            pop     dx              ; Register zurueck
                pop     cx
                ret

                endp

;******************************************************************************
;* Dem Kontroller die Laufwerksgeometrie mitteilen                            *
;*              In  :   AL = Laufwerk                                         *
;*              Out :   C  = 1-->Fehler                                       *
;******************************************************************************

                globproc SetDriveParams

                push    di              ; Register retten
                push    dx
                mov     dl,al           ; Laufwerk retten

                call    GetPTabAdr      ; Adresse Parametertabelle holen

                call    WaitBusy        ; Kontroller muss frei sein

                mov     al,dl           ; Kopfzahl/Laufwerk vorbesetzen
                shl     al,4
                mov     ah,[di+DrPar_Heads]
                dec     ah              ; Maximalnummer anstelle Gesamtzahl
                or      al,ah
                or      al,0a0h
                mov     dx,Task_DrHead
                out     dx,al
                mov     dl,Lo(Task_SecCnt) ; Sektorzahl setzen
                mov     al,[di+DrPar_NSecs]
                out     dx,al

                mov     dl,Lo(Task_Command) ; Parameter uebertragen
                mov     al,Cmd_SetParams
                out     dx,al

                call    WaitBusy        ; auf Fertigstellung warten

                clc                     ; Ende ohne Fehler
                pop     dx
                pop     di
                ret

                endp

;******************************************************************************
;* Laufwerk rekalibrieren, gleichzeitig Test, ob vorhanden                    *
;*              In  :  AL = Laufwerk                                          *
;*              Out :  C + AX = Status                                        *
;******************************************************************************

                globproc Recalibrate

                push    cx              ; Register retten
                push    dx

                mov     cx,ax           ; Laufwerk retten
                call    WaitBusy        ; warten, bis Controller frei

                mov     dx,Task_DrHead  ; Laufwerk eintragen
                mov     al,cl
                shl     al,4
                add     al,0a0h
                out     dx,al

                mov     dl,Lo(Task_Status) ; Laufwerk muss jetzt bereit sein,
                in      al,dx           ; da sich einige Kontroller sonst im
                and     al,50h          ; folgenden aufhaengen, falls
                cmp     al,50h          ; keine Platte angeschlossen ist.
                stc                     ; falls nicht bereit, Fehler simulieren
                mov     al,4            ; "Aborted Command"
                jne     TotEnde
                mov     al,0
                mov     dl,Lo(Task_CylLo) ; erstmal auf die sanfte Tour:
                out     dx,al           ; Spur 0 anfahren
                mov     dl,Lo(Task_CylHi)
                out     dx,al
                mov     dl,Lo(Task_Command)
                mov     al,Cmd_Seek
                out     dx,al
                call    WaitBusy
                call    BuildError
                jnc     Ende            ; wenn OK: fertig

                call    ClearBuffer     ; falls sich der Longshine verheddert...
                mov     dl,Lo(Task_Command) ; 2. Anlauf: echtes Restore
                mov     al,Cmd_Restore
                out     dx,al

                call    WaitBusy        ; auf Controller warten

Ende:           call    BuildError      ; Status einlesen
TotEnde:
                pop     dx              ; Register zurueck
                pop     cx
                ret

                endp

;******************************************************************************
;* Sektor(en) lesen                                                           *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Startkopf                                         *
;*                     BX = Startzylinder                                     *
;*                     CL = Sektorzahl                                        *
;*                     CH = Startsektor                                       *
;*                     ES:DI = Zeiger auf Datenpuffer                         *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc ReadSectors

                push    si              ; Register sichern
                push    dx
                push    bp

                if      debug
                 PrChar  'R'
                 mov     bp,di
                 call    WriteParams
                endif

                sub     bp,bp           ; Fehlerzaehler auf 0

Retry:          push    ax              ; Parameter sichern
                push    bx
                push    cx
                push    di

                mov     si,ax           ; Laufwerk/Kopf retten
                call    WaitBusy        ; warten, bis Ruhe im Wald

                mov     ax,si
                call    SetDriveEnv     ; Laufwerk jetzt schon setzen, damit
                                        ; korr. Ready-Signal abgefragt wird
                call    WaitDrive       ; bis Laufwerk bereit

                call    SetTransParams  ; restliche Parameter ausgeben

                mov     ch,0            ; Sektorzahl nach SI
                mov     si,cx
                mov     dx,Task_Command     ; Kommando triggern
                mov     al,Cmd_Read     
                out     dx,al

                mov     dx,Task_Data    ; Vorbereitung fuer INSW
                cld
Loop:           call    WaitBusy        ; auf gelesenen Sektor warten
                btst    al,0            ; Fehler ?
                jnz     Again           ; -->neu aufsetzen
                call    WaitData        
                btst    al,0
                jnz     Again
                call    WaitDrive
                btst    al,0
                jnz     Again
                mov     cx,SecSize/2    ; Daten transferieren
                rep     insw            ; bagger, schaufel
                dec     si              ; naechster Sektor
                jnz     Loop

End:            pop     di              ; Parameter nicht mehr gebraucht
                pop     cx
                pop     bx
                pop     ax
Term:           if      debug
                 PrChar  CR
                 PrChar  LF
                endif
                call    BuildError
                pop     bp
                pop     dx
                pop     si

                ret

Again:          inc     bp              ; Fehlerzaehler rauf
                cmp     bp,MaxRetry     ; zu oft aufgetreten ?
                jae     End

                pop     di              ; nein: Parameter vom Stack
                pop     cx
                pop     bx
                pop     ax
                mov     si,ax           ; Laufwerk retten
                call    Recalibrate     ; auf Spur 0 zurueck
                jc      Term            ; bei erneutem Fehler Abbruch
                mov     ax,si
                call    SetDriveParams  ; Parameter neu initialisieren
                mov     ax,si
                jmp     Retry           ; neuer Versuch

 
              endp

;******************************************************************************
;* Sektor(en) verifizieren                                                    *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Startkopf                                         *
;*                     BX = Startzylinder                                     *
;*                     CL = Sektorzahl                                        *
;*                     CH = Startsektor                                       *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc VeriSectors

                push    si              ; Register sichern
                push    dx
                push    bp

                if      debug
                 PrChar  'V'
                 mov     bp,0
                 call    WriteParams
                endif

                sub     bp,bp           ; Fehlerzaehler auf 0

Retry:          push    ax              ; Parameter sichern
                push    bx
                push    cx

                mov     si,ax           ; Laufwerk/Kopf retten
                call    WaitBusy        ; warten, bis Ruhe im Wald

                mov     ax,si
                call    SetDriveEnv     ; Laufwerk jetzt schon setzen, damit
                                        ; korr. Ready-Signal abgefragt wird
                call    WaitDrive       ; bis Laufwerk bereit

                call    SetTransParams  ; restliche Parameter ausgeben

                mov     dx,Task_Command ; Kommando triggern
                mov     al,Cmd_Verify
                out     dx,al

                call    WaitBusy        ; auf Fertigstellung warten
                mov     cx,16           ; einige Kontroller brauchen
DelStat:        loop    DelStat         ; etwas fuer das Fehlerflag
                mov     dx,Task_Status
                in      al,dx
                btst    al,0            ; Fehler ?
                jnz     Again           ; -->neu aufsetzen
                call    WaitDrive
                btst    al,0
                jnz     Again

Ende:           pop     cx              ; Parameter nicht mehr gebraucht
                pop     bx
                pop     ax
Term:           if      debug
                 PrChar  CR
                 PrChar  LF
                endif
                call    BuildError
                pop     bp
                pop     dx
                pop     si

                ret

Again:          inc     bp             ; Fehlerzaehler rauf
                cmp     bp,MaxRetry    ; zu oft aufgetreten ?
                jae     Ende

                pop     cx              ; nein: Parameter vom Stack
                pop     bx
                pop     ax
                mov     si,ax           ; Laufwerk retten
                call    Recalibrate     ; auf Spur 0 zurueck
                jc      Term            ; bei erneutem Fehler Abbruch
                mov     ax,si
                call    SetDriveParams  ; Parameter neu initialisieren
                mov     ax,si
                jmp     Retry           ; neuer Versuch
                mov     ax,si
                endp

;******************************************************************************
;* Sektor(en) schreiben                                                       *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Startkopf                                         *
;*                     BX = Startzylinder                                     *
;*                     CL = Sektorzahl                                        *
;*                     CH = Startsektor                                       *
;*                     ES:SI = Zeiger auf Datenpuffer                         *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc WriteSectors

                push    di              ; Register sichern
                push    dx
                push    bp

                if      debug
                 PrChar  'W'
                 mov     bp,si
                 call    WriteParams
                endif

                xor     bp,bp           ; Fehlerzaehler auf 0

Retry:          push    ax              ; Parameter sichern
                push    bx
                push    cx
                push    si

                mov     di,ax           ; Laufwerk/Kopf retten
                call    WaitBusy        ; warten, bis Ruhe im Wald

                mov     ax,di
                call    SetDriveEnv     ; Laufwerk jetzt schon setzen, damit
                                        ; korr. Ready-Signal abgefragt wird
                call    WaitDrive       ; bis Laufwerk bereit

                call    SetTransParams  ; restliche Parameter ausgeben

                mov     ch,0            ; Sektorzahl nach DI
                mov     di,cx
                mov     dx,Task_Command ; Kommando triggern
                mov     al,Cmd_Write
                out     dx,al

                mov     dx,Task_Data    ; Vorbereitung fuer OUTSW
                cld
Loop:           call    WaitBusy        ; auf Datenbereitschaft warten
                btst    al,0            ; Fehler ?
                jnz     Again           ; ja-->neu aufsetzen
                call    WaitData
                btst    al,0
                jnz     Again
                call    WaitDrive
                btst    al,0
                jnz     Again
                mov     cx,SecSize/2    ; Daten transferieren
                seges
                rep     outsw           ; bagger, schaufel
                call    WaitBusy        ; warten, bis Transfer fertig
                btst    al,0
                jnz     Again
                dec     di              ; naechster Sektor
                jnz     Loop

End:            pop     si              ; Parameter nicht mehr gebraucht
                pop     cx
                pop     bx
                pop     ax
Term:           if      debug
                 PrChar  CR
                 PrChar  LF
                endif
                call    BuildError
                pop     bp
                pop     dx
                pop     di

                ret

Again:          inc     bp              ; Fehlerzaehler rauf
                cmp     bp,MaxRetry     ; zu oft aufgetreten ?
                jae     End

                pop     si              ; nein: Parameter vom Stack
                pop     cx
                pop     bx
                pop     ax
                mov     di,ax           ; Laufwerk retten
                call    Recalibrate     ; auf Spur 0 zurueck
                jc      Term            ; bei erneutem Fehler Abbruch
                mov     ax,di
                call    SetDriveParams  ; Parameter neu initialisieren
                mov     ax,di
                jmp     Retry           ; neuer Versuch

                endp

;******************************************************************************
;* Laufwerk formatieren                                                       *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Interleave                                        *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc FormatUnit

                push    bx
                push    cx
                push    dx
                push    si
                push    di
                push    bp

                mov     bx,ax           ; Interleave retten
                PrMsg   ESCMsg
                mov     ax,bx
                call    GetPTabAdr      ; Parametertabelle->DI
                mov     ax,bx
                mov     dh,0            ; gute Spuren schreiben
                mov     al,[di+DrPar_NSecs]
                call    SetInterleaveBuffer ; Tabelle berechnen
                mov     ax,bx
                call    Recalibrate     ; Kontroller reinitialisieren
                jc      Fin
                mov     ax,bx
                mov     bp,[di+DrPar_Cyls] ; Zylinderzaehler in BP (abwaerts)
                dec     bp
                mov     dl,al           ; Laufwerk in DL
                cld
CylLoop:        mov     dh,0            ; Kopf in dh
HeadLoop:       call    WaitBusy        ; warten, bis WD1003 frei
                call    WriteCoords     ; Bildschirmausgabe
                mov     ax,dx           ; Laufwerk+Kopf progr.
                call    SetDriveEnv
                mov     bx,bp           ; Zylinder+Sektor progr.
                mov     cl,[di+DrPar_NSecs]
                mov     ch,1
                call    SetTransParams
                mov     bx,dx
                mov     dx,Task_Command
                mov     al,Cmd_Format
                out     dx,al
                call    WaitData        ; Sektortabelle schicken
                mov     cx,SecSize/2
                mov     dx,Task_Data
                lea     si,[SectorBuffer]
                rep     outsw
                call    WaitBusy        ; warten, bis Kontroller fertig
                shr     al,1            ; Fehlerbit in Carry laden
                jnc     GoOn
                PrMsg   ErrorMsg        ; falls Fehler, Meldung ausgeben
                mov     dx,bx
                call    WriteCoords
                PrChar  LF
GoOn:           mov     dx,bx           ; Laufwerk und Kopf zurueck
                call    BreakOnESC      ; will der Benutzer abbrechen ?
                jc      UserTerm        ; ja, Abbruch
                inc     dh              ; naechster Kopf
                cmp     dh,[di+DrPar_Heads]
                jb      HeadLoop
                dec     bp              ; naechster Zylinder
                jns     CylLoop
TermLoop:       mov     al,dl           ; damit die Seek-Rate wieder stimmt
                call    Recalibrate

Fin:            push    ax              ; Fehlerstatus halten
                pushf
                PrChar  LF
                popf                    ; Fehlerstatus zurueck
                pop     ax
                pop     bp
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                ret

UserTerm:       mov     al,dl           ; Abbruch durch Benutzer: noch schnell
                call    Recalibrate     ; rekalibrieren
                jc      Fin             ; Fehler dabei ?
                stc                     ; Ansonsten Sonderfehlercode
                mov     al,DErr_UserTerm
                jmp     Fin

WriteCoords:    push    ax              ; Kopf/Zylinder ausgeben
                push    cx

                PrMsg   CylMsg
                mov     ax,bp
                mov     cl,6
                call    WriteDec
                PrMsg   HeadMsg
                mov     al,dh
                mov     ah,0
                mov     cl,3
                call    WriteDec
                PrChar  CR

                pop     cx
                pop     ax
                ret

ESCMsg:         db      "Abbruch mit <ESC>",CR,LF,'$'
CylMsg:         db      "Zylinder $"
HeadMsg:        db      ", Kopf $"
ErrorMsg:       db      "Formatierfehler auf $"

                endp

;******************************************************************************
;* Spur formatieren                                                           *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Kopf                                              *
;*                     BX = Zylinder                                          *
;*                     CL = Interleave                                        *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc FormatTrack

                push    bx              ; Register retten
                push    cx
                push    dx
                push    si
                push    di
                push    bp

                mov     bp,ax           ; Laufwerk & Kopf retten
                call    Recalibrate     ; Laufwerk sicherheitshalber rekalibrieren
                mov     ax,bp
                call    GetPTabAdr      ; Sektortabelle aufbauen
                mov     dh,0            ; fehlerhafte Sektoren schreiben
                mov     ah,cl           ; Interleave vorgeben
                mov     al,[di+DrPar_NSecs]
                call    SetInterleaveBuffer
                mov     ax,bp           ; Laufwerk und Kopf zurueck
                call    SetDriveEnv     ; in Kontroller einprogrammieren
                mov     cl,[di+DrPar_NSecs] ; Sektor & Zylinder einschreiben
                mov     ch,1
                call    SetTransParams
                mov     dx,Task_Command ; Kommando schicken
                mov     al,Cmd_Format
                out     dx,al
                call    WaitData        ; Sektortabelle schicken
                mov     cx,SecSize/2
                mov     dx,Task_Data
                lea     si,[SectorBuffer]
                rep     outsw
                call    WaitBusy        ; warten, bis Kontroller fertig
                jc      Fin             ; Abbruch bei Fehler
                mov     ax,bp           ; Laufwerk nochmal rekalibrieren
                call    Recalibrate     ; damit Steprate stimmt

Fin:            pop     bp
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                ret

                endp

;******************************************************************************
;* Spur als defekt markieren                                                  *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Kopf                                              *
;*                     BX = Zylinder                                          *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc MarkBad

                push    bx      ; Register retten
                push    cx
                push    dx
                push    si
                push    di
                push    bp

                mov     bp,ax   ; Laufwerk & Kopf retten
                call    Recalibrate ; Laufwerk sicherheitshalber rekalibrieren
                mov     ax,bp
                call    GetPTabAdr ;Sektortabelle aufbauen
                mov     dh,80h  ; fehlerhafte Sektoren schreiben
                mov     ah,3    ; Interleave ist ziemlich egal...
                mov     al,[di+DrPar_NSecs]
                call    SetInterleaveBuffer
                mov     ax,bp   ; Laufwerk und Kopf zurueck
                call    SetDriveEnv ; in Kontroller einprogrammieren
                mov     cl,[di+DrPar_NSecs] ; Sektor& Zylinder einschreiben
                mov     ch,1
                call    SetTransParams
                mov     dx,Task_Command ; Kommando schicken
                mov     al,Cmd_Format
                out     dx,al
                call    WaitData        ; Sektortabelle schicken
                mov     cx,SecSize/2
                mov     dx,Task_Data
                lea     si,[SectorBuffer]
                rep     outsw
                call    WaitBusy        ; warten, bis Kontroller fertig
                jc      Fin             ; Abbruch bei Fehler
                mov     ax,bp           ; Laufwerk nochmal rekalibrieren
                call    Recalibrate     ; damit Steprate stimmt

Fin:            pop     bp
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                ret

                endp

                endsection
