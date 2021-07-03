;******************************************************************************
;*                                                                            *
;* SECDRIVE - Treiber fuer einen 2. HD-Kontroller im PC                       *
;*                                                                            *
;* Historie:  12. 8.1993  Grundsteinlegung, Definitionen                      *
;*            16. 8.1993  Dispatcher                                          *
;*                        Basislesefunktionen                                 *
;*            17. 8.1993  Partitionsinformationen zusammenkratzen             *
;*            18. 8.1993  Laufwerksparameter initialisieren                   *
;*            19. 8.1993  Zylinder/Sektorregister setzen                      *
;*                        Partitiossektorbaum durchgehen                      *
;*            24. 8.1993  BPB aufbauen                                        *
;*            25. 8.1993  Parameteruebersetzung				      *
;*                        Einlesen                                            *
;*                        Sektoren schreiben                                  *
;*            26. 8.1993  Fehlerbehandlung                                    *
;*                        Verify                                              *
;*                        1. Restore-Versuch mit Seek                         *
;*             7. 9.1993  Versuch Version 1.39 mit Proc's                     *
;*            28. 9.1993  etwas gekÅrzt                                       *
;*            27.12.1994  leichte Korrekturen im Restore                      *
;*            28.12.1994  Trennung Low-Level-Routinen begonnen                *
;*            19. 1.1995  Fehlermeldungen im Klartext                         *
;*                                                                            *
;******************************************************************************

;******************************************************************************
;* globale Definitionen                                                       *
;******************************************************************************

; A C H T U N G : Mono.SYS mu· fÅr Debugging geladen sein !!!!

debug           equ     0
debug2          equ     0

                include bitfuncs.inc

                cpu     80186           ; WD 1003 fordert min. 80286

Diag_NoError    equ     01h             ; Selbstdiagnosecodes: kein Fehler
Diag_ContError  equ     02h             ; Controller-Fehler
Diag_SBufError  equ     03h             ; Sektorpuffer defekt
Diag_ECCError   equ     04h             ; Fehlerkorrektor defekt
Diag_ProcError  equ     05h             ; Steuerprozessor defekt
Diag_Timeout    equ     06h             ; Controller antwortet nicht

ParTab		struct
BFlag		 db	?		; Partitionseintrag: Partition aktiv ?
FHead		 db	?		; Startkopf
FSecCyl		 dw	?		; Startzylinder/sektor
Type		 db	?		; Partitionstyp
LHead		 db	?		; Endkopf
LSecCyl  	 dw     ?		; Endzylinder/sektor
LinSec   	 dd     ?		; Anzahl Sektoren
NSecs    	 dd     ?		; linearer Startsektor
		endstruct

DErr_WrProtect  equ     00h             ; Treiberfehlercodes: Schreibschutz
DErr_InvUnit    equ     01h             ; unbekannte Geraetenummer
DErr_NotReady   equ     02h             ; Laufwerk nicht bereit
DErr_Unknown    equ     03h             ; Unbekannes Treiberkommando
DErr_CRCError   equ     04h             ; Pruefsummenfehler
DErr_InvBlock   equ     05h             ; ungueltiger Request-Header
DErr_TrkNotFnd  equ     06h             ; Spur nicht gefunden
DErr_InvMedia   equ     07h             ; Unbekanntes Traegerformat
DErr_SecNotFnd  equ     08h             ; Sektor nicht gefunden
DErr_PaperEnd   equ     09h             ; Papierende im Drucker
DErr_WrError    equ     0ah             ; allg. Schreibfehler
DErr_RdError    equ     0bh             ; allg. Schreibfehler
DErr_GenFail    equ     0ch             ; allg. Fehler
DErr_InvChange  equ     0fh             ; unerlaubter Diskettenwechsel

DErr_UserTerm   equ     0ffh            ; Fehlercode Abbruch durch Benutzer

SecSize         equ     512             ; Sektorgroesse in Bytes
MaxPDrives      equ     2               ; Maximalzahl physikalischer Laufwerke
MaxDrives       equ     10              ; Maximalzahl verwaltbarer Laufwerke
MaxParts	equ	4		; Maximalzahl Partitionen in einem Sektor
MaxRetry        equ     2               ; max. 2 Lese/Schreibversuche
StackSize       equ     512             ; etwas mehr wg. Rekursion

DrPar_Offset    equ     1aeh            ; Offset Parametertabelle im Part.-Sektor
ParTab_Offset	equ	1beh		; Offset Partitionstabelle "   "     "
ParSecID_Offset	equ	1feh		; Offset Partitionssektorflag (55aa)
BPBOfs          equ     11              ; Offset BPB im Bootsektor

INT_DOS         equ     21h             ; DOS-Funktionsaufruf
INT_Mono        equ     60h             ; Haken zum VT100-Treiber
INT_Clock       equ     1ah             ; BIOS-Uhreneinstieg
INT_Keyboard    equ     16h             ; BIOS-Tastatureinstieg

DOS_WrString    equ     9               ; DOS-Funktionen
DOS_WrChar      equ     6
DOS_RdString    equ     10
DOS_RdChar      equ     8

HD_ID           equ     0f8h            ; Media-ID fuer Festplatten

CR              equ     13
LF              equ     10
BEL             equ     7
ESC             equ     27
AUML		equ	84h
OUML		equ	94h
UUML		equ	81h
SZLIG		equ	0e1h

;******************************************************************************
; Makros                                                                      *
;******************************************************************************

;jmp             macro adr
;                !jmp long adr
;                endm

beep            macro
                push    ax
                mov     ax,0e07h
                int     10h
                pop     ax
                endm

PrMsg           macro   Adr             ; Meldung ausgeben
                push    dx              ; Register retten
                lea     dx,[Adr]
                mov     ah,DOS_WrString
                int     INT_DOS
                pop     dx              ; Register zurueck
                endm

PrChar          macro   Zeichen         ; Zeichen ausgeben
                push    dx              ; Register retten
                push    ax
                mov     dl,Zeichen
                mov     ah,DOS_WrChar
                int     INT_DOS
                pop     ax
                pop     dx              ; Register zurueck
                endm

;------------------------------------------------------------------------------

btst            macro   op,bit          ; ein einzelnes Bit testen
		test	op,(1<<bit)
		endm

;------------------------------------------------------------------------------

ljnz            macro   adr             ; lange Spruenge
                jz      Next
                jmp     adr
Next:
                endm

ljne            macro   adr
                je      Next
                jmp     adr
Next:
                endm

ljc             macro   adr
                jnc     Next
                jmp     adr
Next:
                endm

lje             macro   adr
                jne     Next
                jmp     adr
Next:
                endm

ljz             macro   adr
                jnz     Next
                jmp     adr
Next:
                endm

ljb             macro   adr
                jnb     Next
                jmp     adr
Next:
                endm

;------------------------------------------------------------------------------

proc            macro   name
                section name
                public  name:parent
name            label   $
                endm

globproc        macro   name
                section name
                public  name
name            label   $
                endm

endp            macro
                endsection
                endm

;------------------------------------------------------------------------------
; Fehlermakro

JmpOnError      macro   Drv,Adr
                jnc     GoOn            ; kein Fehler, weitermachen
                mov     ah,Drv
                call    WrErrorCode     ; Fehler-->ausgeben...
                jmp     Adr             ; ...Abbruch
GoOn:
                endm

;******************************************************************************
;* Treiberkopf                                                                *
;******************************************************************************


              assume  cs:code,ds:nothing,ss:nothing,es:nothing

                org     0

DriverHead:     dd      -1              ; Zeiger auf Nachfolger
                dw      0000100000000010b ; Attribut
;                       ^   ^         ^
;                       |   |         +- kann 32-Bit-Setornummern verarbeiten
;                       |   +----------- kennt Close/Open/Removeable
;                       +--------------- Blocktreiber
                dw      StrategyProc    ; Zeiger auf Strategieroutine
                dw      InterruptProc   ; Zeiger auf eigentlichen Treibercode
NrOfVols:       db      0               ; Zahl log. Laufwerke
                db      "SECDRIV"       ; bei Blocktreibern unbenutzt

;******************************************************************************
;* residente Daten                                                            *
;******************************************************************************

Rh_Ptr          dd      ?               ; Speicher fuer Request-Header

JmpTable        dw      Init            ; Sprungtabelle: Initialisierung
                dw      MediaCheck      ; Medium gewechselt ?
                dw      BuildBPB        ; Parameterblock laden
                dw      IOCTLRead       ; Steuerdaten vom Treiber
                dw      Read            ; Daten lesen
                dw      ND_Read         ; Lesen, ohne Pufferstatus zu Ñndern
                dw      InputStatus     ; Daten im Eingabepuffer ?
                dw      InputFlush      ; Eingabepuffer loeschen
                dw      Write           ; Daten schreiben
                dw      Write_Verify    ; Daten mit Prueflesen schreiben
                dw      OutputStat      ; Ausgabepuffer leer ?
                dw      OutputFlush     ; Ausgabepuffer loeschen
                dw      IOCTLWrite      ; Steuerdaten zum Treiber
                dw      DeviceOpen      ; DOS hat eine Datei darauf geoeffnet
                dw      DeviceClose     ; DOS hat eine Datei darauf geschlossen
                dw      Removeable      ; Ist Datentraeger wechselbar ?
                dw      OutputTillBusy  ; Ausgabe, bis Puffer voll
                dw      GenIOCTL        ; genormtes IOCTL
                dw      GetLogical      ; Laufwerkszuordnung lesen
                dw      SetLogical      ; Laufwerkszuordnung setzen
                dw      IOCTLQuery      ; Abfrage, ob GenIOCTL unterstuetzt

SectorBuffer:   db      SecSize dup (?) ; Sektorpuffer fuer Treiber selber

                db      StackSize dup (?) ; Treiberstack
DriverStack:

BPBSize         equ     36
DrTab		struct			; Laufwerkstabelle:
StartHead	 db     ?		;  Startkopf
StartCyl	 dw	?		;  Startzylinder
StartSec	 db	?		;  Startsektor
LinStart	 dd	?		;  lin. Startsektor
SecCnt		 dd	?		;  Gesamtsektorzahl
Drive		 db	?		;  Laufwerk
BPB		 db	BPBSize dup (?)	;  BPB
		endstruct

DrTab           db      DrTab_Len*MaxDrives dup (?)
DrTab_BPBs      dd      2*MaxDrives dup (?)
DrCnt           db      0               ; Anzahl gefundener Laufwerke
DrOfs           db      0               ; erster freier Laufwerksbuchstabe

DrPar		struct			; Plattenparametersatz:
Cyls		 dw	?		;  Zylinderzahl
Heads		 db	?		;  Kopfzahl
RedWr		 dw	?		;  Startzylinder reduzierter Schreibstrom
PrComp		 dw	?		;  Startzylinder Praekompensation
ECCLen		 db	?		;  max. korrigierbarer Fehlerburst (Bits)
CByte		 db	?		;  Wert fuers Plattensteuerregister
TOut		 db	?		;  genereller Timeout
FTOut		 db	?		;  Timeout Formatierung
CTOut		 db	?		;  Timeout fuer Pruefung
LZone		 dw	?		;  Landezylinder
NSecs		 db	?		;  Sektorzahl
Dummy		 db	?		;  unbenutzt
		endstruct

DrPars          db      DrPar_Len*MaxPDrives dup (0)

;******************************************************************************
;* Strategieroutine                                                           *
;******************************************************************************

StrategyProc:   mov     word ptr [Rh_Ptr],bx ; Zeiger speichern
                mov     word ptr [Rh_Ptr+2],es
                retf

;******************************************************************************
;* Treiberdispatcher                                                          *
;******************************************************************************

Rh		struct
Size		 db	?		; gemeinsame Headerteile: Laenge Block
Unit		 db	?		; angesprochenes Laufwerk
Func		 db	?		; Treibersubfunktion
Status		 dw	?		; Ergebnis
Resvd		 db	8 dup (?)	; unbenutzt
		endstruct

InterruptProc:  pusha                   ; alle Register retten

                cli                     ; Stack umschalten
                mov     si,ss           ; alten zwischenspeichern
                mov     di,sp
                mov     ax,cs           ; neuen setzen
                mov     ss,ax
                lea     sp,[DriverStack]
                push    di              ; alten auf neuem (!) speichern
                push    si
                sti

                mov     ax,cs           ; DS auf Treibersegment
                mov     ds,ax
                assume  ds:code

                les     bx,[Rh_Ptr]     ; Zeiger laden
                mov     word ptr es:[bx+Rh_Status],0 ; Status loeschen

                mov     al,es:[bx+Rh_Func] ; Subfunktion ausrechnen
                if      debug
                 call   PrByte
                endif
                mov     ah,0
                add     ax,ax
                mov     si,ax
                jmp     JmpTable[si]

                jmp     Init

;******************************************************************************
;* gemeinsames Ende                                                           *
;******************************************************************************

StateError:     btst    al,1            ; Bit 1: Spur 0 nicht gefunden
                jz      StateError_N1
                mov     al,DErr_TrkNotFnd
                jmp     StateError_End
StateError_N1:  btst    al,2            ; Bit 2: abgebrochenes Kommando
                jz      StateError_N2
                btst    ah,5            ; Bit S5: Schreibfehler
                jz      StateError_N21
                mov     al,DErr_WrError
                jmp     StateError_End
StateError_N21: btst    ah,4            ; Bit S4: Positionierung nicht vollstaendig
                jnz     StateError_N22
                mov     al,DErr_TrkNotFnd
                jmp     StateError_End
StateError_N22: btst    ah,6            ; Bit S6: Laufwerk nicht bereit
                jnz     StateError_N23
                mov     al,DErr_NotReady
                jmp     StateError_End
StateError_N23: mov     al,DErr_GenFail ; Notnagel 1
                jmp     StateError_End
StateError_N2:  test    al,11h          ; Bit 0/4: Sektor nicht gefunden
                jz      StateError_N3
                mov     al,DErr_SecNotFnd
                jmp     StateError_End
StateError_N3:  btst    al,6            ; Bit 6: Pruefsummenfehler
                jz      StateError_N4
                mov     al,DErr_CRCError
                jmp     StateError_End
StateError_N4:  mov     al,DErr_GenFail ; Notnagel 2
StateError_End: les     bx,[Rh_Ptr]     ; Code einspeichern
                mov     es:[bx+Rh_Status],al
                jmp     Error

Unknown:        les     bx,[Rh_Ptr]
                mov     byte ptr es:[bx+Rh_Status],DErr_Unknown ; unbek. Funktion

Error:          or      byte ptr es:[bx+Rh_Status+1],80h ; Fehler-Flag setzen
                jmp     Done

Busy:           les     bx,[Rh_Ptr]
                or      byte ptr es:[bx+Rh_Status+1],2  ; Busy-Flag setzen

Done:           les     bx,[Rh_Ptr]
                or      byte ptr es:[bx+Rh_Status+1],1  ; Done-Flag setzen

                if      debug
                 call   NxtLine
                endif

                cli                     ; Stack zurueckschalten
                pop     si
                pop     di              ; alten in SI:DI laden
                mov     sp,di           ; einschreiben
                mov     ss,si
                sti

                popa                    ; Register zurueck
                retf

;******************************************************************************
;* Debugginghilfe                                                             *
;******************************************************************************

                if      debug||debug2

HexTab          db      "0123456789ABCDEF"

PrByte:         push    es              ; Register retten
                push    di
                push    bx
                push    ax

                lea     bx,[HexTab]


                db      0d4h,10h        ; AAM 16
                push    ax
                mov     al,ah
                xlat
                mov     ah,0eh
                int     10h
                pop     ax
                xlat
                mov     ah,0eh
                int     10h

                pop     ax              ; Register zurueck
                pop     bx
                pop     di
                pop     es
                ret

PrWord:         xchg    ah,al           ; Hi-Byte
                call    PrByte
                xchg    ah,al
                call    PrByte
                ret

PrChar:         push    ax
                mov     ah,0eh
                int     10h
                pop     ax
                ret

NxtLine:        push    ax              ; Register retten
                push    bx
                push    dx

                mov     ax,0e0dh
                int     10h
                mov     ax,0e0ah
                int     10h

                pop     dx              ; Register zurueck
                pop     bx
                pop     ax

                endif

;******************************************************************************
;* residente Subfunktionen                                                    *
;******************************************************************************

;******************************************************************************
;* eine logische Laufwerksnummer ueberpruefen                                 *
;*              In  :   AL = Laufwerk                                         *
;*              Out :   C  = 1, falls Fehler                                  *
;******************************************************************************

                proc    ChkDrive

                cmp     al,[DrCnt]      ; C=1, falls < (d.h. OK)
                cmc                     ; Deshalb noch einmal drehen
                jnc     OK              ; C=0, alles in Butter
                les     bx,[Rh_Ptr]     ; ansonsten Fehlerstatus setzen
                mov     byte ptr es:[bx+Rh_Status],DErr_InvUnit
OK:             ret

                endp

;******************************************************************************
;* Adresse der phys. Laufwerkstabelle errechnen                               *
;*              In  :   AL = Laufwerk                                         *
;*              Out :   DI = Adresse                                          *
;******************************************************************************

                proc    GetPTabAdr

                mov     ah,DrPar_Len	; relative Adresse berechnen
                mul     ah
                lea     di,[DrPars]     ; Offset dazu
                add     di,ax
                ret

                endp

;******************************************************************************
;* Adresse eines Partitionsdeskriptors errechnen                              *
;*              In  :   AL = log. Laufwerk                                    *
;*              Out :   DI = Adresse                                          *
;******************************************************************************

                proc    GetTabAdr

                mov     ah,DrTab_Len    ; relative Adresse berechnen
                mul     ah
                lea     di,[DrTab]      ; Offset dazu
                add     di,ax
                ret

                endp

;******************************************************************************
;* logische Parameter in physikalische uebersetzen                            *
;*              In  :   BL = log. Laufwerk                                    *
;*                      DX:AX = relative Sektornummer			      *
;*              Out :   AL = phys. Laufwerk                                   *
;*                      AH = Kopf                                             *
;*                      BX = Zylinder                                         *
;*                      CH = Sektor                                           *
;******************************************************************************

                proc    TranslateParams

                push    di              ; Register retten

		xchg	bx,ax		; Adresse Parametertabelle holen
		call	GetTabAdr	

		add	bx,[di+DrTab_LinStart]   ; in absolute Sektornummer
		adc	dx,[di+DrTab_LinStart+2] ; umrechnen
		mov	al,[di+DrTab_Drive]      ; phys. Laufwerksnummer holen
		push	ax		; bis zum Ende retten
		
		call	GetPTabAdr	; von dieser phys. Platte die Tabelle holen
		mov	ax,bx		; Sektor# wieder nach DX:AX
		mov	bl,[di+DrPar_NSecs] ; Sektorzahl auf 16 Bit
		mov	bh,0		; aufblasen
		div	bx
		mov	ch,dl		; Modulo-Rest ist Sektornummer auf
		inc	ch		; Spur: Vorsicht, Numerierung ab 1 !!!
		sub	dx,dx		; wieder auf 32 Bit erweitern
		mov	bl,[di+DrPar_Heads] ; Kopfnummer herausfummeln
		div	bx
		mov	bx,ax		; Quotient ist Zylinder
		pop	ax		; Laufwerk zurueck
		mov	ah,dl		; Rest ist Kopf

		pop	di		; Register zurueck
		ret

                endp

;******************************************************************************
;* Einbindung Low-Level-Routinen                                              *
;******************************************************************************

; definiert werden muessen:

; LowLevelIdent:  Meldung ueber unterstuetzte Hardware ausgeben
; ContDiag:       Kontroller-Selbsttest durchfuehren
;                 Ergebniskode in AL
; Recalibrate:    Laufwerk [AL] auf Zylinder 0 fahren
;                 Fehlerflag in C, Fehlerkode in AX
; SetDriveParams: dem Kontroller die Geometrie fuer Laufwerk [AL] einbleuen
;                 Fehlerflag in C
; ReadSectors:    von Laufwerk [AL] ab Zylinder [BX], Kopf [AH], Sektor [CH]
;                 [CL] Sektoren in Puffer ab ES:DI lesen
;                 Fehlerflag in C, Fehlerkode in AX
; WriteSectors:   auf Laufwerk [AL] ab Zylinder [BX], Kopf [AH], Sektor [CH]
;                 [CL] Sektoren von Puffer ab ES:SI schreiben
;                 Fehlerflag in C, Fehlerkode in AX
; VeriSectors:    auf Laufwerk [AL] ab Zylinder [BX], Kopf [AH], Sektor [CH]
;                 [CL] Sektoren verifizieren
;                 Fehlerflag in C, Fehlerkode in AX
; FormatUnit:     Laufwerk [AL] mit Interleave [AH] formatieren, Fehlerkode
;                 in AX
; FormatTrack:    Zylinder [BX], Kopf [AH] auf Laufwerk [AL] mit Interleave
;                 [CL] formatieren, Fehlerkode in AX
; MarkBad:        Zylinder [BX], Kopf [AH] auf Laufwerk [AL] als defekt
;                 markieren, Fehlerkode in AX

                include "lowlevel.inc"

;******************************************************************************
;* Bootsektor eines log. Laufwerkes lesen                                     *
;*              In  :   AL = Laufwerksnummer                                  *
;*              Out :   C+AX = Fehlerstatus                                   *
;******************************************************************************

                proc    ReadBootSec

                push    es              ; Register retten
                push    bx
                push    cx
                push    di

                call    GetTabAdr       ; Eintrag in Laufwerkstabelle ermitteln
                mov     al,[di+DrTab_Drive] ; davon ersten Sektor lesen
                mov     ah,[di+DrTab_StartHead]
                mov     bx,[di+DrTab_StartCyl]
                mov     cl,1
                mov     ch,[di+DrTab_StartSec]
                mov     di,cs
                mov     es,di
                lea     di,[SectorBuffer]
                call    ReadSectors

                pop     di              ; Register zurueck
                pop     cx
                pop     bx
                pop     es
                ret

                endp

;******************************************************************************
;* Funktion 1: Test, ob Medium gewechselt                                     *
;******************************************************************************

                proc    MediaCheck

Rh_MediaID      equ     Rh_Resvd+8      ; erwartetes Media-ID
Rh_Return       equ     Rh_MediaID+1    ; Ergebnis-Flag
Rh_VolName      equ     Rh_Return+1     ; Adresse alter Laufwerksname

                cmp     byte ptr es:[bx+Rh_MediaID],HD_ID ; gueltige ID ?
                je      OK
                mov     byte ptr es:[bx+Rh_Status],DErr_InvMedia ; nein...
                jmp     Error
OK:             mov     byte ptr es:[bx+Rh_Return],1 ; nie gewechselt
                jmp     Done

                endp

;******************************************************************************
;* Funktion 2: BPB aufbauen                                                   *
;******************************************************************************

                proc    BuildBPB

Rh2		struct
		 db	Rh_Len dup (?)
MediaID          db	?		; erwartetes Media-ID
FATSector	 dd	?		; Pufferadresse 1. FAT-Sektor
BPBAddress	 dd	?		; Adresse neuer BPB
		endstruct

                mov     al,es:[bx+Rh_Unit]
		call    ChkDrive        ; Laufwerksnummer gueltig ?
                ljc     Error           ; nein-->Fehler & Ende

                call    ReadBootSec     ; Bootsektor lesen
                ljc     StateError      ; bei Fehlern Ende

                les     bx,[Rh_Ptr]        ; Zeiger neu laden
		mov	al,es:[bx+Rh_Unit] ; Tabellenadresse aufbauen
		call	GetTabAdr
		lea	di,[di+DrTab_BPB]  ; DI auf BPB-Speicher
                mov     es:[bx+Rh2_BPBAddress],di ; BPB-Zeiger abspeichern
                mov     es:[bx+Rh2_BPBAddress+2],cs
		
                mov     si,cs              ; BPB umkopieren
                mov     es,si
		lea	si,[SectorBuffer+BPBOfs]
		cld
		mov	cx,BPBSize
		rep	movsb

                jmp     Done

                endp

;******************************************************************************

IOCTLRead:      jmp     Unknown

;******************************************************************************
;* Funktion 4: Sektoren lesen                                                 *
;******************************************************************************

Rh4		struct
		 db	Rh_len dup (?)
MediaID		 db	?		; Media-ID Laufwerk
BufOfs		 dw	?		; Adresse Datenpuffer
BufSeg		 dw	?
NSecs		 dw	?		; Anzahl zu lesender Bloecke
FirstSec	 dw	?		; Startsektor bzw. $FFFF fuer 32-Bit-Nummern
VolID		 dd	?		; Adresse Laufwerksname
LFirstSec	 dw	?		; lange Startsektornummer
HFirstSec	 dw	?
		endstruct

Read:           mov	al,es:[bx+Rh_Unit] ; Laufwerksnummer pruefen
		call	ChkDrive
		ljc	Error

                mov     ch,al                   ; Laufwerksnummer retten
                mov     ax,es:[bx+Rh4_FirstSec] ; Sektor holen (BIGDOS beachten)
                sub     dx,dx
                cmp     ax,-1
                jne     Read_SmallSec
                mov     ax,es:[bx+Rh4_LFirstSec]
                mov     dx,es:[bx+Rh4_HFirstSec]
Read_SmallSec:  mov     cl,es:[bx+Rh4_NSecs]    ; Sektorzahl laden (muss <=128 sein)
                les     di,es:[bx+Rh4_BufOfs]   ; Zieladresse laden
                mov     bl,ch                   ; Laufwerksnummer nach BL

                if      debug
                 push   ax
                 push   cx
                 mov    cx,ax
                 mov    al,' '
                 call   PrChar
                 mov    al,bl           ; Laufwerksnummer
                 call   PrByte
                 mov    al,' '
                 call   PrChar
                 mov    ax,dx           ; Startsektor
                 call   PrWord
                 mov    ax,cx
                 call   PrWord
                 mov    al,' '
                 call   PrChar
                 pop    cx
                 mov    al,cl           ; Sektorzahl
                 call   PrByte
                 mov    al,' '
                 call   PrChar
                 mov    ax,es           ; Startadresse
                 call   PrWord
                 mov    al,':'
                 call   PrChar
                 mov    ax,di
                 call   PrWord
                 pop    ax
                endif

                call    TranslateParams         ; umrechnen lassen
                call    ReadSectors             ; der eigentliche Lesevorgang

                ljc     StateError              ; bei Fehlern...
                jmp     Done                    ; ansonsten o.k.

;******************************************************************************

ND_Read:        jmp     Unknown

InputStatus:    jmp     Unknown

InputFlush:     jmp     Unknown

;******************************************************************************
;* Funktion 8: Sektoren schreiben                                             *
;******************************************************************************

Rh8		struct
		 db	Rh_len dup (?)
MediaID		 db	?		; Media-ID Laufwerk
BufOfs		 dw	?		; Adresse Datenpuffer
BufSeg		 dw	?
NSecs		 dw	?		; Anzahl zu lesender Bloecke
FirstSec	 dw	?		; Startsektor bzw. $FFFF fuer 32-Bit-Nummern
VolID		 dd	?		; Adresse Laufwerksname
LFirstSec	 dw	?		; lange Startsektornummer
HFirstSec	 dw	?
		endstruct

DoWrite:        if      debug2
                 mov    al,es:[bx+Rh_Unit]
                 call   PrByte
                 mov    al,' '
                 call   PrChar
                 mov    ax,es:[bx+Rh8_FirstSec]
                 call   PrWord
                 mov    al,' '
                 mov    ax,es:[bx+Rh8_HFirstSec]
                 call   PrWord
                 mov    ax,es:[bx+Rh8_LFirstSec]
                 call   PrWord
                 call   NxtLine
                endif

                mov     al,es:[bx+Rh_Unit]
                mov     ch,al                   ; Laufwerksnummer retten
                mov     ax,es:[bx+Rh8_FirstSec] ; Sektor holen (BIGDOS beachten)
                sub     dx,dx
                cmp     ax,-1
                jne     DWrite_SmallSec
                mov     ax,es:[bx+Rh8_LFirstSec]
                mov     dx,es:[bx+Rh8_HFirstSec]
DWrite_SmallSec:mov     cl,es:[bx+Rh8_NSecs]    ; Sektorzahl laden (muss <=128 sein)
                les     si,es:[bx+Rh8_BufOfs]   ; Zieladresse laden
                mov     bl,ch                   ; Laufwerksnummer nach BL

                if      debug
                 push   ax
                 push   cx
                 mov    cx,ax
                 mov    al,' '
                 call   PrChar
                 mov    al,bl           ; Laufwerksnummer
                 call   PrByte
                 mov    al,' '
                 call   PrChar
                 mov    ax,dx           ; Startsektor
                 call   PrWord
                 mov    ax,cx
                 call   PrWord
                 mov    al,' '
                 call   PrChar
                 pop    cx
                 mov    al,cl           ; Sektorzahl
                 call   PrByte
                 mov    al,' '
                 call   PrChar
                 mov    ax,es           ; Startadresse
                 call   PrWord
                 mov    al,':'
                 call   PrChar
                 mov    ax,si
                 call   PrWord
                 pop    ax
                endif

                call    TranslateParams         ; umrechnen lassen
                call    WriteSectors            ; der eigentliche Lesevorgang

                ret

Write:          mov     al,es:[bx+Rh_Unit] ; Laufwerksnummer pruefen
		call	ChkDrive
		ljc	Error

                call    DoWrite

                ljc     StateError              ; bei Fehlern...
                jmp     Done                    ; ansonsten o.k.


;******************************************************************************
;* Funktion 9: Sektoren schreiben mit Ueberpruefung                           *
;******************************************************************************

Rh9		struct
		 db	Rh_len dup (?)
MediaID		 db     ?		; Media-ID Laufwerk
BufOfs		 dw     ?		; Adresse Datenpuffer
BufSeg		 dw     ?
NSecs		 dw     ?		; Anzahl zu lesender Bloecke
FirstSec	 dw     ?		; Startsektor bzw. $FFFF fuer 32-Bit-Nummern
VolID		 dd     ?		; Adresse Laufwerksname
LFirstSec	 dw     ?		; lange Startsektornummer
HFirstSec	 dw     ?
		endstruct

Write_Verify:   mov     al,es:[bx+Rh_Unit] ; Laufwerksnummer pruefen
		call	ChkDrive
		ljc	Error

                call    DoWrite         ; schreiben

                ljc     StateError      ; bei Fehlern vorher abbrechen

                les     bx,[Rh_Ptr]     ; Parameter nochmal fuer Verify laden
                mov     al,es:[bx+Rh_Unit]
                mov     ch,al
                mov     ax,es:[bx+Rh9_FirstSec]
                sub     dx,dx
                cmp     ax,-1
                jne     VWrite_SmallSec
                mov     ax,es:[bx+Rh9_LFirstSec]
                mov     dx,es:[bx+Rh9_HFirstSec]
VWrite_SmallSec:mov     cl,es:[bx+Rh9_NSecs]
                mov     bl,ch

                call    TranslateParams ; nochmal umrechen...
                call    VeriSectors     ; und prueflesen

                jmp     Done            ; alles gut gegangen

;******************************************************************************

OutputStat:     jmp     Unknown

OutputFlush:    jmp     Unknown

IOCTLWrite:     jmp     Unknown

;******************************************************************************
;* kein Device wechselbar, Oeffnen/Schliessen interessiert nicht              *
;******************************************************************************

DeviceOpen:     jmp     Done

DeviceClose:    jmp     Done

Removeable:     jmp     Done

;******************************************************************************

OutputTillBusy: jmp     Unknown

GenIOCTL:       jmp     Unknown

GetLogical:     jmp     Unknown

SetLogical:     jmp     Unknown

IOCTLQuery:     jmp     Unknown

;******************************************************************************
;* Funktion 0: Initialisierung                                                *
;******************************************************************************

                include "secparam.inc"

Rh0		struct
		 db	Rh_len dup (?)
Units		 db     ?		; Zahl bedienter Laufwerke
EndOfs		 dw     ?		; Endadresse Offset
EndSeg		 dw     ?		; Endadresse Segment
ParamOfs	 dw     ?		; Parameter Offsetadresse
ParamSeg	 dw     ?		; Parameter Segmentadresse
FirstDrive	 db     ?		; erstes freies Laufwerk
MsgFlag		 db     ?		; Flag, ob DOS Fehler ausgeben darf
		endstruct

Init:           PrMsg   HelloMsg        ; Meldung ausgeben
                call    LowLevelIdent   ; Startmeldung des Low-Level-Treibers

                mov     byte ptr es:[bx+Rh0_Units],0 ; noch keine Laufwerke

                mov     al,es:[bx+Rh0_FirstDrive] ; Startlaufwerk retten
                mov     [DrOfs],al

                mov     ax,cs           ; ES auf gem. Segment
                mov     es,ax

; Schritt 1: Controller pruefen

                PrMsg   DiagMsg0
                call    ContDiag        ; Diagnose ausfuehren
                sub     al,Diag_NoError
                cmp     al,6            ; ausserhalb ?
                jae     Diag_Over
                add     al,al           ; Meldung ausrechnen
                mov     ah,0
                mov     si,ax
                mov     dx,DiagMsgTable[si]
                mov     ah,9
                int     INT_DOS
                or      si,si           ; fehlerfrei ?
                ljnz    Init_Err        ; Nein, Fehler
                jmp     Init_ChkDrives  ; Ja, weiter zum Laufwerkstest

Diag_Over:      push    ax
                PrMsg   UndefDiagMsg    ; undefinierter Fehlercode
                pop     ax
                add     al,Diag_NoError ; Meldung rueckkorrigieren
                db      0d4h,10h        ; AAM 16
                add     ax,'00'
                push    ax
                mov     al,ah
                mov     ah,14
                int     10h
                pop     ax
                mov     ah,14
                int     10h
                PrChar  CR
                PrChar  LF
                jmp     Init_Err


; Schritt 2: Laufwerke testen

; Menueaufruf?

Init_ChkDrives: mov     ax,40h          ; CTRL gedrueckt ?
                mov     es,ax
                btst    byte ptr es:[17h],2
                jz      Init_Menu
                call    DiskMenu

; Schritt 2a: Laufwerk rekalibrieren

Init_Menu:      mov     al,[MomDrive]
                call    Recalibrate
                ljc     Init_NextDrive  ; Fehler: Laufwerk ueberspringen

; Schritt 2b: Masterpartitionssektor lesen

ReadMaster:     mov     al,[MomDrive]
		mov	ah,0		; Kopf...
		sub	bx,bx		; ...Zylinder...
		mov	cx,0101h	; ...ein Sektor ab Sektor 1
                mov     di,ds
                mov     es,di
                lea     di,[SectorBuffer] ; in den Sektorpuffer
		call	ReadSectors
                JmpOnError [MomDrive],Init_NextDrive ; Fehler ?

; Schritt 2c: Laufwerksparameter initialisieren

                lea     si,[SectorBuffer+DrPar_Offset] ; Quelladresse im Sektor
                mov     al,[MomDrive]   ; Zieladresse ausrechnen
                call    GetPTabAdr
                mov     cx,DrPar_Len
                cld
                rep     movsb

                sub     di,DrPar_Len    ; Laufwerk nicht initialisiert ?
                cmp     word ptr[di+DrPar_Cyls],0
                je      DoQuery
                cmp     byte ptr[di+DrPar_Heads],0
                je      DoQuery
                cmp     byte ptr[di+DrPar_NSecs],0
                jne     NoQuery
DoQuery:        mov     al,[MomDrive]   ; wenn ja, dann nachfragen
                mov     ah,1            ; Rueckschreiben hier erlaubt
                call    QueryParams
                or      al,al           ; =0-->Laufwerk ignorieren
                jz      Init_NextDrive
                dec     al              ; =1-->nochmal lesen
                jz      ReadMaster      ; ansonsten weitermachen

NoQuery:        mov     al,[MomDrive]   ; Laufwerksparameter ausgeben...
                call    PrintPDrive
                mov     al,[MomDrive]   ; ...und dem Controller einbleuen
                call    SetDriveParams
                JmpOnError [MomDrive],Init_NextDrive
                mov     al,[MomDrive]
                call    Recalibrate
                JmpOnError [MomDrive],Init_NextDrive

; Schritt 2d: durch die Partitionssektoren hangeln

		mov	al,[MomDrive]	; Laufwerk : momentanes
		cbw			; Kopf : 0
		push	ax
		sub	ax,ax
		push	ax		; Zylinder : 0
		inc	ax		; Sektor : 1
		push	ax
		dec	ax
		push	ax		; lin. Sektornummer 0
		push	ax
		call	ScanParts

Init_NextDrive: inc     [MomDrive]      ; Zaehler weitersetzen
		cmp	[MomDrive],MaxPDrives
                ljb     Init_ChkDrives

                cmp     [DrCnt],0       ; keine Partitionen gefunden ?
                jne     Init_PDrives
                PrMsg   ErrMsgNoDrives  ; ja: meckern
                jmp     Init_Err

Init_PDrives:   PrMsg   LDriveMsg
                mov     [MomDrive],0    ; Parameter der Partitionen ausgeben
                lea     bp,[DrTab_BPBs] ; und BPB-Tabelle aufbauen
                cld

Init_PLDrives:  mov     al,[MomDrive]
                call    PrintLDrive

                mov     al,[MomDrive]   ; Bootsdektor lesen
                call    ReadBootSec
                lea     si,[SectorBuffer+BPBOfs] ; BPB rauskopieren
                mov     al,[MomDrive]
                call    GetTabAdr
                lea     di,[di+DrTab_BPB]
                mov     ax,cs
                mov     es,ax
                mov     ds:[bp],di      ; Adresse nebenbei ablegen
                add     bp,2
                mov     cx,BPBSize
                rep     movsb

                inc     [MomDrive]
                mov     al,[MomDrive]
                cmp     al,[DrCnt]
                jb      Init_PLDrives

                PrChar  LF              ; sieht besser aus...

                les     bx,[Rh_Ptr]     ; Zeiger auf BPB-Zeiger einschreiben
                lea     ax,[DrTab_BPBs]
                mov     es:[bx+Rh0_ParamOfs],ax
                mov     es:[bx+Rh0_ParamSeg],cs
                jmp     Init_OK         ; Initialisierung erfolgeich zu Ende

Init_Err:       PrMsg   WKeyMsg
                xor     ah,ah           ; damit Meldung lesbar bleibt
                int     16h
                sub     ax,ax           ; Treiber aus Speicher entfernen
                jmp     Init_End

Init_OK:        mov     al,[DrCnt]      ; Laufwerkszahl holen
                les     bx,[Rh_Ptr]
                mov     es:[bx+Rh0_Units],al ; im Request Header eintragen
                mov     [NrOfVols],al   ; im Treiberkopf eintragen
                lea     ax,[Init]       ; residenten Teil installieren

Init_End:       les     bx,[Rh_Ptr]
                mov     es:[bx+Rh0_EndOfs],ax ; Endadresse setzen
                mov     es:[bx+Rh0_EndSeg],cs

                jmp     Done

;******************************************************************************
;* transiente Unterroutinen                                                   *
;******************************************************************************

;******************************************************************************
;* Partitionsbaum durchgehen                                                  *
;*              In  :   dw Kopf/Laufwerk                                      *
;*                      dw Zylinder                                           *
;*			dw Sektor					      *
;*			dd lineare Nummer des Sektors			      *
;******************************************************************************

ScParts_DrHd	equ	12		; Parameteradressen auf Stack
ScParts_Cyl	equ	10
ScParts_Sec	equ	8
ScParts_LinSec	equ	4
ScParts_ParTab  equ     0-(MaxParts*ParTab_Len)   ; Kopie Partitionstabelle
ScParts_LocSize equ     0-ScParts_ParTab          ; belegter Stack

ScanParts:      enter	ScParts_LocSize,0

; Partitionssektor lesen

		mov	ax,[bp+ScParts_DrHd]
		mov	bx,[bp+ScParts_Cyl]
		mov	ch,[bp+ScParts_Sec]
		mov	cl,1
		mov	di,cs
		mov	es,di
		lea	di,[SectorBuffer]
                call    ReadSectors
                JmpOnError [MomDrive],ScanParts_End

; Partitionssektorkennung o.k. ?

		cmp	word ptr SectorBuffer[ParSecID_Offset],0aa55h
                ljne    ScanParts_End

; Partitionstabelle auslesen

		lea	si,[SectorBuffer+ParTab_Offset] ; Quelladresse
		mov	di,ss		; Zieladresse auf Stack
		mov	es,di
                lea     di,[bp+ScParts_ParTab]
		mov	cx,MaxParts*ParTab_Len  ; Laenge
		cld
		rep	movsb

; Partitionstabelle durchgehen

                mov     si,ScParts_ParTab       ; vorne anfangen
		mov	cx,MaxParts		; alle durchgehen
ScanParts_Scan:	push	cx

		mov	al,[bp+si+ParTab_Type]	; Typ der Partition lesen
                lea     bx,[AccPartTypes-1]     ; auf Suchtabelle
ScanParts_LAcc: inc     bx                      ; einen Eintrag weiter
                cmp     byte ptr [bx],0         ; Tabellenende ?
		je	ScanParts_Next		; ja-->war nix
		cmp	al,[bx]			; gefunden ?
		jne	ScanParts_LAcc		;

		mov	bx,[bp+si+ParTab_LinSec] ; linearen Startsektor ausrechnen
		mov	cx,[bp+si+ParTab_LinSec+2]
                add     bx,[bp+ScParts_LinSec]  ; in CX:BX
                adc     cx,[bp+ScParts_LinSec+2]

		cmp	al,5			; extended partition ?
		jne	ScanParts_Enter

		push	si			; ja: Zeiger fuer Rekursion retten
		mov	al,[bp+ScParts_DrHd]	; Laufwerk & Kopf zusammenbauen
		mov	ah,[bp+si+ParTab_FHead]
		push	ax
		mov	ax,[bp+si+ParTab_FSecCyl] ; Zylinder ausfiltern
                xchg    ah,al
		shr	ah,6
		push	ax
                mov     al,[bp+si+ParTab_FSecCyl] ; Sektor ausfiltern
		and	ax,63
		push	ax
		push	cx
		push	bx
		call	ScanParts
		pop	si			; Zeiger zurueck
		jmp	ScanParts_Next

ScanParts_Enter:mov	al,[DrCnt]		; Partition in Tabelle eintragen
		call	GetTabAdr		; dazu Adresse neuen Eintrags holen
		cld
		mov	ax,cs			; Ziel im Segment
		mov	es,ax
		mov	al,[bp+si+ParTab_FHead] ; Kopf kopieren
		stosb	
		mov	ax,[bp+si+ParTab_FSecCyl] ; Zylinder kopieren
                xchg    ah,al
		shr	ah,6
		stosw
                mov     al,[bp+si+ParTab_FSecCyl] ; Sektor kopieren
		and	al,63
		stosb
		mov	ax,bx			; linearen Startsektor kopieren
		stosw
		mov	ax,cx
		stosw
		mov	ax,[bp+si+ParTab_NSecs]	; Sektorzahl kopieren
		stosw
		mov	ax,[bp+si+ParTab_NSecs+2]
		stosw
		mov	al,[bp+ScParts_DrHd]	; Laufwerksnummer kopieren
		stosb
		inc	[DrCnt]			; ein log. Laufwerk mehr

ScanParts_Next:	add	si,ParTab_Len		; auf naechste Partition
		pop	cx
                dec     cx
                ljnz    ScanParts_Scan

ScanParts_End:  leave
		ret	10

;******************************************************************************
;* Daten eines physikalischen Laufwerks ausgeben                              *
;*              In  :   AL = Laufwerk                                         *
;******************************************************************************

                proc    PrintPDrive

                push    cx              ; Register retten
                push    dx
                push    di

                cbw                     ; AH loeschen
                push    ax              ; Laufwerk ausgeben
                PrMsg   PDriveMsg1
                pop     ax
                push    ax
                inc     ax
                mov     cl,1
                call    WriteDec
                PrMsg   PDriveMsg2

                pop     ax              ; Adresse Laufwerkstabelle berechnen
                call    GetPTabAdr

                mov     ax,[di+DrPar_Cyls] ; Zylinder ausgeben
                mov     cl,5
                call    WriteDec
                PrMsg   PDriveMsg3

                mov     al,[di+DrPar_Heads] ; Koepfe ausgeben
                mov     ah,0
                mov     cl,3
                call    WriteDec
                PrMsg   PDriveMsg4

                mov     al,[di+DrPar_NSecs] ; Sektoren ausgeben
                mov     ah,0
                mov     cl,4
                call    WriteDec
                PrMsg   PDriveMsg5

                mov     al,[di+DrPar_Heads] ; Gesamtsektorzahl berechnen
                mul     byte ptr [di+DrPar_NSecs]
                mul     word ptr [di+DrPar_Cyls]
		call	WriteMBytes
                PrMsg   PDriveMsg6

                pop     di              ; Register zurueck
                pop     dx
                pop     cx
                ret

                endp

;******************************************************************************
;* Daten eines logischen Laufwerks ausgeben                                   *
;*              In  :   AL = Laufwerk                                         *
;******************************************************************************

                proc    PrintLDrive

                push    cx              ; Register retten
                push    dx
                push    di

                mov     dx,ax           ; Laufwerk retten
                push    dx
                mov     cx,3            ; ausgeben
                call    WriteSpc
                add     dl,[DrOfs]
                add     dl,'A'
                mov     ah,DOS_WrChar
                int     INT_DOS
                PrChar  ':'

                pop     ax              ; Tabelle holen
                call    GetTabAdr

                mov     al,[di+DrTab_Drive] ; Laufwerk ausgeben...
                inc     al
                cbw
                mov     cl,9
                call    WriteDec

                mov     ax,[di+DrTab_StartCyl] ; ...Zylinder...
                mov     cl,10
                call    WriteDec

                mov     al,[di+DrTab_StartHead] ; ...Kopf...
                cbw
                mov     cl,7
                call    WriteDec

                mov     al,[di+DrTab_StartSec] ; ...Sektor...
                cbw
                mov     cl,8
                call    WriteDec

                mov     cx,2
                call    WriteSpc
                mov     ax,[di+DrTab_SecCnt] ; ...Groesse
                mov     dx,[di+DrTab_SecCnt+2]
                call    WriteMBytes

                PrMsg   PDriveMsg6      ; Meldung wiederverwertet...

                pop     di              ; Register zurueck
                pop     dx
                pop     cx
                ret

                endp

;******************************************************************************
;* Fehlercode eines Laufwerks ausgeben                                        *
;*              In :    AL = Fehlercode                                       *
;*                      AH = Laufwerksnummer (0,1...)                         *
;******************************************************************************

                proc    WrErrorCode

                push    bx              ; Register retten
                push    cx
                push    dx

                add     ah,'1'          ; LW-Nummer in ASCII umrechnen...
                mov     [DrvErrorMsg2],ah ; ...und einschreiben
                mov     ch,al           ; Kode sichern
                PrMsg   DrvErrorMsg
                mov     cl,7            ; bei Bit 0 anfangen
ErrLoop:        rol     ch,1            ; fagl. Bit in Carry
                jnc     NoErrBit
                mov     bl,cl           ; Bit gefunden: Index ausrechnen
                mov     bh,0
                add     bx,bx
                mov     dx,[bx+Pointers]
                mov     ah,DOS_WrString
                int     INT_DOS
NoErrBit:       dec     cl              ; naechstes Bit
                jnz     ErrLoop

                pop     dx              ; Register zurueck
                pop     cx
                pop     bx

                ret

DrvErrorMsg:    db      "Fehler auf Festplatte "
DrvErrorMsg2:   db      "0:",CR,LF,'$'

Pointers        dw      Msg0,Msg1,Msg2,Msg3,Msg4,Msg5,Msg6,Msg7
Msg0            db      "  Adre",SZLIG,"marke nicht gefunden",CR,LF,'$'
Msg1            db      "  Spur 0 nicht gefunden",CR,LF,'$'
Msg2            db      "  Kommandoabbruch",CR,LF,'$'
Msg3            db      "$"
Msg4            db      "  Sektor nicht gefunden",CR,LF,'$'
Msg5            db      "$"
Msg6            db      "  Datenfehler",CR,LF,'$'
Msg7            db      "  Sektor als defekt markiert",CR,LF,'$'

                endp

;******************************************************************************
;* Sektorenzahl als MBytes ausgeben                                           *
;*              In:     DX:AX = Sektorzahl                                    *
;******************************************************************************

                proc    WriteMBytes

SecsPerMByte    equ     (2^20)/SecSize

                push    cx              ; Register retten
		push	dx

                add     ax,SecsPerMByte/20 ; wg. Rundung
                adc     dx,0

                mov     cx,SecsPerMByte ; durch 2048 teilen = MByte
                div     cx
                push    dx              ; Nachkommastellen retten
                mov     cl,6
                call    WriteDec

                PrChar  '.'             ; Nachkommastelle
                pop     ax              ; holen
                cwd
                mov     cx,SecsPerMByte/10 ; Sektoren pro 100 KByte
		div	cx
		mov	cl,1		; ausgeben
		call	WriteDec

		pop	dx		; Register zurueck
		pop	cx
		ret

                endp

;******************************************************************************
;* transiente Daten                                                           *
;******************************************************************************

HelloMsg:       db      CR,LF,"SekundÑrlaufwerkstreiber V0.4",CR,LF,'$'

ErrMsgNoDrives: db      CR,LF,"Fehler: keine Partitionen gefunden",CR,LF,'$'

DiagMsg0:       db      CR,LF,"Controller-Selbsttest: ",'$'
DiagMsg1:       db      "OK",CR,LF,'$'
DiagMsg2:       db      "Controller fehlerhaft",CR,LF,'$'
DiagMsg3:       db      "Sektorpuffer defekt",CR,LF,'$'
DiagMsg4:       db      "Fehlerkorrektur defekt",CR,LF,'$'
DiagMsg5:       db      "Steuerprozessor defekt",CR,LF,'$'
DiagMsg6:       db      "Timeout",CR,LF,'$'
DiagMsgTable    dw      DiagMsg1,DiagMsg2,DiagMsg3,DiagMsg4,DiagMsg5,DiagMsg6
UndefDiagMsg    db      "Unbekannter Fehlercode #$"
WKeyMsg:        db      "Weiter mit beliebiger Taste...",CR,LF,'$'

PDriveMsg1:     db      "Festplatte $"
PDriveMsg2:     db      " :$"
PDriveMsg3:     db      " Zylinder,$"
PDriveMsg4:     db      " K",OUML,"pfe,$"
PDriveMsg5:     db      " Sektoren,$"
PDriveMsg6:     db      " MByte",CR,LF,'$'

LDriveMsg:      db      CR,LF,"vorhandene Partitionen:",CR,LF
                db      "Laufwerk  Platte  Zylinder  Kopf"
                db      "  Sektor      Kapazit",AUML,"t",CR,LF,'$'

AccPartTypes	db	1		; akzeptierte Partitionstypen: DOS 2.x FAT12
                db      4               ; DOS 3.x FAT16
		db	5		; DOS 3.3 extended
                db      6               ; DOS 3.31 >32 MByte
		db	0		; Tabellenende

MomDrive	db	0		; momentan gescanntes Laufwerk

                end
