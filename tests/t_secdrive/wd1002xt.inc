;******************************************************************************
;*                                                                            *
;*  Includedatei fuer SECMAIN.ASM                                             *
;*  liefert low-level-Routinen fuer SecMain                                   *
;*  Version hier fuer WD1002XT-kompatible Kontroller:                         *
;*  MFM, RLL (?)                                                              *
;*                                                                            *
;*  Historie: 28.12.1994                                                      *
;*            26. 3.1994 Formatierroutinen                                    *
;*             8. 4.1994 defekte Spuren markieren                             *
;*                                                                            *
;******************************************************************************

                section wd1002xt

;------------------------------------------------------------------------------
; Portadressen

Port_Base       equ     320h            ; primaere Basisadresse
Port_Data       equ     Port_Base+0     ; Datenregister (R+W)
Port_Status     equ     Port_Base+1     ; Statusregister (R)
Port_Reset      equ     Port_Base+1     ; Reset ausloesen (W)
Port_Config     equ     Port_Base+2     ; Jumper auslesen (R)
Port_Select     equ     Port_Base+2     ; Kontroller selektieren (W)
Port_IRQDRQ     equ     Port_Base+3     ; IRQ/DRQ-Leitungen freigeben (W)

;------------------------------------------------------------------------------
; Kommandocodes

Cmd_Diagnose    equ     0e4h            ; Kommando: Kontroller-Selbsttest
Cmd_GetStatus   equ     003h            ; Status letzter Operation lesen
Cmd_TestReady   equ     000h            ; Test, ob Laufwerk bereit
Cmd_Restore     equ     001h            ; Laufwerk rekalibrieren
Cmd_SetParams   equ     00ch            ; Laufwerksparameter setzen
Cmd_Seek        equ     00bh            ; Spur anfahren
Cmd_Read        equ     008h            ; Sektoren lesen
Cmd_Write       equ     00ah            ; Sektoren schreiben
Cmd_Verify      equ     005h            ; Sektoren auf Lesbarkeit pruefen
Cmd_WriteBuffer equ     00fh            ; Sektorpuffer beschreiben
Cmd_FormatDisk  equ     004h            ; Laufwerk formatieren
Cmd_FormatTrack equ     006h            ; Spur formatieren
Cmd_FormatBad   equ     007h            ; Spur als defekt markieren

;------------------------------------------------------------------------------
; I/O-Bremse

IODelay         macro
                jmp     $+2
                endm

;------------------------------------------------------------------------------
; Puffer

CmdBufSize      equ     6               ; enthaelt Kommandoblock fuer WD1002
CmdBuf          db      CmdBufSize dup (0)

StatBufSize     equ     4               ; enthaelt Statusinfo vom WD1002
StatBuf         db      StatBufSize dup (0)

GeomBufSize     equ     8               ; enthaelt Parametertabelle fuer Laufwerk
GeomBuf         db      GeomBufSize dup (0)

;******************************************************************************
;* Kommandopuffer initialisieren                                              *
;******************************************************************************

                proc    InitCmdBuf

                push    ax              ; Register retten

                sub     ax,ax           ; mit Nullen initialisieren
                mov     word ptr[CmdBuf],ax
                mov     word ptr[CmdBuf+2],ax
                mov     ah,45h          ; Retry on, 70us Steprate
                mov     word ptr[CmdBuf+4],ax

                pop     ax              ; Register zurueck

                ret

                endp

;******************************************************************************
;* einen Datenblock an den Kontroller schicken                                *
;*              In  :   ES:SI = Datenblock                                    *
;*                      CX = Anzahl Bytes                                     *
;*              Out :   C=1 bei Protokollfehler                               *
;******************************************************************************

                proc    SendBlock

                push    ax              ; Register retten
                push    cx
                push    dx
                push    si

                mov     dx,Port_Status
                jcxz    ZeroLoop        ; Nullschleife abfangen
                cld                     ; !!!
OutLoop:        in      al,dx           ; Status lesen
                btst    al,0            ; warten, bis REQ-Bit auf 1
                jz      OutLoop
                btst    al,1            ; IO-Bit muss 0 sein
                stc
                jnz     ErrEnd
                mov     dl,Lo(Port_Data); ein Byte auf Datenport ausgeben
                seges
                outsb
                mov     dl,Lo(Port_Status) ; zurueck fuer naechsten Durchlauf
                loop    OutLoop
ZeroLoop:       clc                     ; Ende ohne Fehler
ErrEnd:
                pop     si              ; Register zurueck
                pop     dx
                pop     cx
                pop     ax

                ret

                endp

;******************************************************************************
;* einen Datenblock vom Kontroller lesen                                      *
;*              In  :   ES:DI = Datenblock                                    *
;*                      CX = Anzahl Bytes                                     *
;*              Out :   C=1 bei Protokollfehler                               *
;******************************************************************************

                proc    RecvBlock

                push    ax              ; Register retten
                push    cx
                push    dx
                push    di

                mov     dx,Port_Status
                jcxz    ZeroLoop        ; Nullschleife abfangen
                cld                     ; !!!
InLoop:         in      al,dx           ; Status lesen
                btst    al,0            ; warten, bis REQ-Bit auf 1
                jz      InLoop
                btst    al,1            ; IO-Bit muss 1 sein
                stc
                jz      ErrEnd
                mov     dl,Lo(Port_Data); ein Byte von Datenport einlesen
                insb
                mov     dl,Lo(Port_Status) ; zurueck fuer naechsten Durchlauf
                loop    InLoop
ZeroLoop:       clc                     ; Ende ohne Fehler
ErrEnd:
                pop     di              ; Register zurueck
                pop     dx
                pop     cx
                pop     ax

                ret

                endp

;******************************************************************************
;* Status bilden                                                              *
;*              Out : C+AX = Status                                           * 
;******************************************************************************

                proc    BuildStatus

                push    dx              ; Register retten

                mov     dx,Port_Status  ; auf Datum warten
Loop:           in      al,dx
                btst    al,0            ; bis REQ=1
                jz      Loop
                btst    al,1            ; und IO=1
                jz      Loop
                mov     dl,Lo(Port_Data); CCB auslesen
                in      al,dx
                mov     ah,al           ; retten fuer Fehlerabfrage
                and     al,2            ; Bit 1 ausmaskieren
                clc
                ljz      End             ; wenn = 0, kein Fehler und AL=0

                push    cx              ; zusaetzliche Register retten
                push    si
                push    di
                push    es

                call    InitCmdBuf      ; ansonsten Kommando absetzen, um
                mov     [CmdBuf],Cmd_GetStatus ; Status zu lesen
                and     ah,20h          ; Status fuer korr. Laufwerk abfragen
                mov     [CmdBuf+1],ah
                mov     dx,Port_Status
WaitNBusy:      in      al,dx
                btst    al,3
                jnz     WaitNBusy
                mov     ax,ds           ; NICHT ExecCmd benutzen, da sonst
                mov     es,ax           ; Rekursion !
                lea     si,[CmdBuf]
                mov     cx,CmdBufSize
                mov     dl,Lo(Port_Select)
                out     dx,al
                call    SendBlock
                lea     di,[StatBuf]    ; 4 Statusbytes auslesen
                mov     cx,StatBufSize
                call    RecvBlock
                mov     dl,Lo(Port_Status); CCB nicht vergessen!!
Loop2:          in      al,dx
                btst    al,0            ; bis REQ=1
                jz      Loop2
                btst    al,1            ; und IO=1
                jz      Loop2
                mov     dl,Lo(Port_Data)
                in      al,dx
                mov     al,[StatBuf]    ; Fehlercode = 1.Byte,
                and     al,7fh          ; Bit 0..6
                stc                     ; Carry signalisiert Fehler
                pop     es              ; zusaetzliche Register zurueck
                pop     di
                pop     si
                pop     cx

End:            mov     ah,0            ; MSB ohne Bedeutung
                pop     dx              ; Register zurueck
                ret

                endp

;******************************************************************************
;* XT- in AT-Fehlerkode umsetzen                                              *
;* Eingabe:     AL = XT-Fehlerkode                                            *
;* Ausgabe:     C+AX = AT-Fehlerstatus                                        *
;******************************************************************************

                proc    TranslateStatus

                push    bx
                push    si

                mov     bl,al          ; alten Status sichern
                mov     bh,-1
                lea     si,[TransTable]
                cld
TransLoop:      lodsw                   ; einen Eintrag laden
                cmp     al,bh           ; Tabellenende?
                je      TransEnd
                cmp     al,bl           ; Treffer?
                jne     TransLoop       ; nein, weitersuchen
                mov     al,ah           ; uebersetzten Code laden
                cmp     al,0            ; Code fuer kein Fehler?
                clc
                je      Ende            ; ja, C=0
                jmp     TransErr        ; ansonsten C=1
TransEnd:       mov     al,04h          ; Aborted Command annehmen
TransErr:       stc                     ; Fehlerflag setzen

Ende:           pop     si              ; Register zurueck
                pop     bx

                ret

TransTable:     db      00h,00h         ; kein Fehler
                db      02h,02h         ; kein Seek Complete-Signal
                db      03h,04h         ; Write Fault
                db      04h,04h         ; Laufwerk nicht bereit
                db      06h,02h         ; Spur 0 nicht gefunden
                db      08h,02h         ; Laufwerk positioniert noch
                db      11h,40h         ; unkorrigierbarer Datenfehler
                db      12h,01h         ; Adreámarke nicht gefunden
                db      15h,10h         ; Positionierfehler
                db      18h,00h         ; korrigierbarer Fehler (ignoriert)
                db      19h,80h         ; Spur als defekt markiert
                db      -1,-1           ; Tabellenende

                endp

;******************************************************************************
;* ein Kommando ausfhren                                                     *
;*              In  :   AL = Kommando                                         *
;******************************************************************************

                proc    ExecCmd

                push    cx              ; Register retten
                push    ax
                push    dx
                push    si
                push    es

                mov     [CmdBuf],al     ; Kommandokode in Datenblock einschreiben
                mov     dx,Port_Status  ; warten, bis Kontroller frei
WaitNBusy:      in      al,dx
                btst    al,3
                jnz     WaitNBusy
                mov     dx,Port_Select  ; Kontroller selektieren
                out     dx,al
                mov     ax,ds           ; Adresse Kommandoblock
                mov     es,ax
                lea     si,[CmdBuf]
                mov     cx,CmdBufSize   ; Laenge Kommandoblock
                call    SendBlock       ; Kommandoblock abschicken

                pop     es              ; Register zurueck
                pop     si
                pop     dx
                pop     ax
                pop     cx

                ret

                endp

;******************************************************************************
;* Laufwerk und Sonderwerte in Kommandoblock einprogrammieren                 *
;*              In  :   AL = Laufwerk                                         *
;*                      AH = Kopf                                             *
;******************************************************************************

                proc    SetDriveEnv

                push    ax              ; Register retten

 
                shl     al,5            ; Laufwerksbit an Stelle 5
                or      al,ah
                mov     [CmdBuf+1],al   ; als 2. Byte im Kommandopuffer schreiben

                pop     ax              ; Register zurueck

                ret

                endp

;******************************************************************************
;* Zylinder- und Sektorparameter an Kontroller ausgeben                       *
;*              In  :   BX = Startzylinder                                    *
;*                      CL = Sektorzahl/Interleave                            *
;*                      CH = Startsektor                                      *
;******************************************************************************

                proc    SetTransParams

                push    ax              ; Register retten

                mov     [CmdBuf+3],bl   ; LSB Startzylinder
                mov     al,bh           ; MSB Startzylinder
                shl     al,6            ; in Bit 6..7 schieben
                add     al,ch           ; Sektor dazu
                dec     al              ; !!! Sektoren ab 0
                mov     [CmdBuf+2],al
                mov     [CmdBuf+4],cl   ; Sektorzahl

                pop     ax             ; Register zurueck
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

IdentMsg        db      "Low-Level-Routinen f",UUML,"r WD1002S-WX2 und kompatible Controller",CR,LF,'$'

                endp

;******************************************************************************
;* Controller-Diagnose:                                                       *
;*              Out :  AL = Diagnosecode                                      *
;******************************************************************************

                globproc ContDiag

                push    cx              ; Register retten
                push    bx
                push    dx

                sub     cx,cx
                mov     dx,Port_Status  ; auf Status
BWait:          in      al,dx
                btst    al,3            ; auf NOT BUSY warten
                loopnz  BWait           ; oder bis 64K Durchlaeufe durch
                or      cx,cx           ; Timeout ?
                jne     NTOut
                mov     al,Diag_Timeout ; ja: Fehlercode setzen...
                jmp     End             ; ...und Feierabend

NTOut:          call    InitCmdBuf      ; Kommando Selbsttest ausfuehren
                mov     al,Cmd_Diagnose
                call    ExecCmd
                call    BuildStatus     ; Status holen

                cmp     al,5            ; WD1002 definiert nur Code 0..5
                jb      DoTrans
                mov     al,7            ; "undefinierter Code"
                jmp     End
DoTrans:        lea     bx,[TransTbl]   ; ansonsten umsetzen
                xlat

End:            pop     dx              ; Register zurueck
                pop     bx
                pop     cx
                ret

TransTbl:       db      Diag_NoError    ; Code 0: kein Fehler
                db      Diag_ContError  ; Code 1: WD1010 fehlerhaft
                db      Diag_ECCError   ; Code 2: WD11C00 fehlerhaft
                db      Diag_SBufError  ; Code 3: Sektorpuffer defekt
                db      Diag_ProcError  ; Code 4: WD1015 RAM defekt
                db      Diag_ProcError  ; Code 5: WD1015 ROM defekt

 
                endp

;******************************************************************************
;* Laufwerk rekalibrieren, gleichzeitig Test, ob vorhanden                    *
;*              In  :  AL = Laufwerk                                          *
;*              Out :  C + AX = Status                                        *
;******************************************************************************

 
                globproc Recalibrate

                push    ax              ; Register retten
                push    cx

                call    InitCmdBuf      ; testen, ob Laufwerk bereit
                mov     ah,0            ; Kopf dafuer unwichtig
                call    SetDriveEnv
                mov     dl,al           ; Laufwerksnummer retten, gleichzeitig
                mov     dh,0            ; Kopf auf 0 setzen
                mov     al,Cmd_TestReady
                call    ExecCmd
                call    BuildStatus
                jc      TotEnde         ; C=1 --> Ende mit Fehler

                call    InitCmdBuf      ; sanfte Tour: Spur 0 mit Seek anfahren
                mov     ax,dx
                call    SetDriveEnv
                mov     al,0            ; Zylinder lo=0
                mov     [CmdBuf+3],al
                inc     al              ; Zylinder Hi=0, Startsektor=1
                mov     [CmdBuf+2],al
                mov     al,Cmd_Seek
                call    ExecCmd
                call    BuildStatus
                jnc     TotEnde         ; kein Fehler, alles in Butter

                call    InitCmdBuf      ; ansonsten echtes Restore versuchen
                mov     ax,dx
                call    SetDriveEnv
                mov     al,Cmd_Restore
                call    ExecCmd
                call    BuildStatus
                call    TranslateStatus

TotEnde:        pop     dx              ; Register zurueck
                pop     ax

                ret

                endp

;******************************************************************************
;* Dem Kontroller die Laufwerksgeometrie mitteilen                            *
;*              In  :   AL = Laufwerk                                         *
;*              Out :   C  = 1-->Fehler                                       *
;******************************************************************************

                globproc SetDriveParams

                push    cx              ; Register retten
                push    si
                push    es

                call    GetPTabAdr      ; Adresse Parametertabelle holen
                call    InitCmdBuf      ; Kommando anstoáen
                call    SetDriveEnv
                mov     al,Cmd_SetParams
                call    ExecCmd

 
                mov     ax,[di+DrPar_Cyls] ; Parametertabelle aufbauen
                xchg    ah,al
                mov     word ptr [GeomBuf],ax
                mov     al,[di+DrPar_Heads]
                mov     byte ptr[GeomBuf+2],al
                mov     ax,[di+DrPar_RedWr]
                xchg    ah,al
                mov     word ptr[GeomBuf+3],ax
                mov     ax,[di+DrPar_PrComp]
                xchg    ah,al
                mov     word ptr[GeomBuf+5],ax
                mov     al,[di+DrPar_ECCLen]
                mov     byte ptr[GeomBuf+7],al
                lea     si,[GeomBuf]    ; Block abschicken
                mov     cx,GeomBufSize
                mov     ax,ds
                mov     es,ax
                call    SendBlock
                call    BuildStatus
                call    TranslateStatus

                pop     es              ; Register zurueck
                pop     si
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

                push    bx              ; Register retten
                push    cx
                push    dx
                push    di
                push    es

                call    InitCmdBuf      ; Puffer initialisieren
                call    SetDriveEnv
                call    SetTransParams
                mov     al,Cmd_Read     ; Lesen triggern
                PrChar  '1'
                call    ExecCmd
                PrChar  '2'

SecLoop:        mov     dx,Port_Status  ; warten, bis Request-Bit gesetzt
RLoop:          in      al,dx
                btst    al,0
                jz      RLoop
                btst    al,2            ; Daten oder Status ?
                jnz     ErrEnd          ; wenn jetzt Status, ist etwas schief gelaufen
                push    cx              ; ansonsten Sektor auslesen
                mov     cx,SecSize
                PrChar  '3'
                call    RecvBlock
                PrChar  '4'
                pop     cx
                dec     cl
                add     di,SecSize
                jnz     RLoop           ; und naechsten Sektor verarbeiten

ErrEnd:         PrChar  '5'
                call    BuildStatus
                PrChar  '6'
                call    TranslateStatus

                pop     es              ; Register zurueck
                pop     di
                pop     dx
                pop     cx
                pop     bx

                ret

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

                push    bx              ; Register retten
                push    cx
                push    dx

                call    InitCmdBuf      ; Puffer initialisieren
                call    SetDriveEnv
                call    SetTransParams
                mov     al,Cmd_Verify   ; Verifikation triggern
                call    ExecCmd

                call    BuildStatus
                call    TranslateStatus

                pop     dx              ; Register zurueck
                pop     cx
                pop     bx

                ret

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

                push    bx              ; Register retten
                push    cx
                push    dx
                push    si
                push    es

 
                call    InitCmdBuf      ; Puffer initialisieren
                call    SetDriveEnv
                call    SetTransParams
                mov     al,Cmd_Write    ; Lesen triggern
                call    ExecCmd

SecLoop:        mov     dx,Port_Status  ; warten, bis Request-Bit gesetzt
WLoop:          in      al,dx
                btst    al,0
                jz      WLoop
                btst    al,2            ; Daten oder Status ?
                jnz     ErrEnd          ; wenn jetzt Status, ist etwas schief gelaufen
                push    cx              ; ansonsten Sektor auslesen
                mov     cx,SecSize
                call    SendBlock
                pop     cx
                dec     cl
                add     si,SecSize
                jnz     WLoop           ; und naechsten Sektor verarbeiten

ErrEnd:         call    BuildStatus
                call    TranslateStatus

                pop     es              ; Register zurueck
                pop     si
                pop     dx
                pop     cx
                pop     bx

                ret

                endp

;******************************************************************************
;* Laufwerk formatieren                                                       *
;*              In  :  AL = Laufwerk                                          *
;*                     AH = Interleave                                        *
;*              Out :  C+AX = Fehlerstatus                                    *
;******************************************************************************

                globproc FormatUnit

                push    bx              ; Register retten
                push    cx
                push    dx
                push    si
                push    di
                push    es
 
                mov     bx,ax           ; Interleave & Laufwerk retten
                
                mov     ax,ds           ; vorher noch den Sektorpuffer im
                mov     es,ax           ; Controller ausnullen
                lea     di,[SectorBuffer]
                mov     cx,SecSize/2
                sub     ax,ax
                rep     stosw
                call    InitCmdBuf
                mov     al,Cmd_WriteBuffer
                call    ExecCmd
                lea     si,[SectorBuffer]
                mov     cx,SecSize
                call    SendBlock
                call    BuildStatus
                jc      End             ; unwahrscheinlich, aber...

                call    InitCmdBuf      ; Puffer initialisieren
                mov     al,bl           ; Laufwerk wieder zurueck
                mov     ah,0            ; Startkopf ist 0
                call    SetDriveEnv
                mov     [CmdBuf+4],bh   ; Interleave einschreiben
                mov     al,Cmd_FormatDisk ; Formatieren triggern
                call    ExecCmd

ErrEnd:         call    BuildStatus
End:            call    TranslateStatus

                pop     es              ; Register zurueck
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx

                ret

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

                push    bx
                push    cx

                call    InitCmdBuf      ; Parameter einschreiben
                call    SetDriveEnv
                mov     ch,1            ; Sektorinformation muss nur gueltig sein
                call    SetTransParams
                mov     al,Cmd_FormatTrack
                call    ExecCmd
                call    BuildStatus

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

                push    bx
                push    cx

                call    InitCmdBuf      ; Parameter einschreiben
                call    SetDriveEnv
                mov     cx,0103h        ; Sektorinformation muss nur gueltig sein
                call    SetTransParams
                mov     al,Cmd_FormatBad
                call    ExecCmd
                call    BuildStatus

                pop     cx
                pop     bx
                ret

                endp

                endsection
