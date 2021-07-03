;*****************************************************************************
;* Sekundaerlaufwerkstreiber, Modul SecParam                                 *
;* liefert Laufwerksparameter, falls fehlend im Bootsektor                   *
;* stellt das ganze Formatiermenue zur Verfuegung                            *
;*****************************************************************************

                section SecParam

;*****************************************************************************
;* gemeinsam benutzte Meldungen                                              *
;*****************************************************************************

TrackMsg        db      "Spur (0..Spurzahl-1) : $"
HeadMsg         db      "Kopf (0..Kopfzahl-1) : $"
ConfirmMsg      db      "Sind Sie sicher ?$"
InterleaveMsg   db      "Interleave (1..Sektorzahl-1) : $"

;*****************************************************************************
;* Routine BreakOnESC                                                        *
;* schaut nach, ob ESC gedrueckt wurde                                       *
;* Ausgabe:     C=1, falls ja                                                *
;*****************************************************************************

                GlobProc BreakOnESC

                push    ax              ; Register retten
                mov     ah,1            ; Tastaturpuffer antesten
                int     INT_Keyboard
                clc                     ; Annahme nicht gedrueckt
                jz      Ende
                mov     ah,0            ; Zeichen da: dies abholen
                int     INT_Keyboard
                cmp     al,ESC          ; ist es ESC ?
                clc                     ; Annahme nein
                jne     Ende
                stc                     ; jawoll!
Ende:           pop     ax              ; Register zurueck
                ret

                endp

;*****************************************************************************
;* Routine YesNo                                                             *
;* fragt nach Ja oder Nein                                                   *
;* Ausgabe:     C=0, falls ja                                                *
;*****************************************************************************

                proc    YesNo

                push    ax              ; Register retten

QueryLoop:      mov     ah,DOS_RdChar   ; ein Zeichen lesen
                int     INT_DOS

                cmp     al,'a'          ; Kleinbuchstaben ?
                jb      Upper
                cmp     al,'z'
                ja      Upper
                add     al,'A'
                sub     al,'a'
Upper:
                cmp     al,'J'          ; akzeptierte Zeichen fuer Ja
                clc
                je      YNDone
                cmp     al,'Y'
                clc
                je      YNDone
                cmp     al,'N'          ; akzeptierte Zeichen fuer Nein
                stc
                je      YNDone

                PrChar  BEL             ; alles andere anmeckern
                jmp     QueryLoop

YNDone:         PrChar  al              ; Zeichen als Echo ausgeben
                PrChar  CR              ; Zeilenvorschub
                PrChar  LF
                pop     ax              ; Register zurueck

                ret

                endp

;*****************************************************************************
;* Routine ReadNumber                                                        *
;* liest einen Wert von 0..n in AX ein                                       *
;* Eingabe:     AX = Maximalwert                                             *
;*              DI = Zeiger auf Meldung                                      *
;* Ausgabe:     AX = eingelesene Zahl                                        *
;*****************************************************************************

                proc    ReadNumber

                push    bx              ; Register retten
                push    cx
                push    dx
                push    si

                mov     si,ax           ; Maximalwert retten
InLoop:         mov     dx,di           ; Meldung ausgeben
                mov     ah,DOS_WrString
                int     INT_DOS
                lea     dx,[KeyBuffer]  ; Zahl als String einlesen
                mov     ah,DOS_RdString
                int     INT_DOS
                PrChar  CR
                PrChar  LF
                mov     ax,0            ; jetzt Zeichen verarbeiten
                mov     cl,[KeyBuffer+1]
                mov     ch,0
                lea     bx,[KeyBuffer+2]
                jcxz    InvFormat       ; Nullschleife abfangen
ConvLoop:       mov     dx,10           ; bisheriges Ergebnis hochmultiplizieren
                mul     dx
                mov     dl,[bx]         ; ein Zeichen holen
                inc     bx
                sub     dl,'0'          ; ASCII-Offset abziehen
                jc      InvFormat       ; bei Formatfehler abbrechen
                cmp     dl,9            ; nur 0..9 erlaubt
                ja      InvFormat
                add     al,dl           ; dazuaddieren
                adc     ah,0
                loop    ConvLoop
                jmp     ChkRange        ; fertig: weiter zur Bereichspruefung
InvFormat:      PrMsg   InvMsg          ; wenn fehlerhaft, meckern
                jmp     InLoop          ; und nochmal versuchen
ChkRange:       cmp     ax,si           ; ausserhalb Bereich ?
                jbe     OK
                PrMsg   OverMsg         ; ja: meckern...
                jmp     InLoop          ; ...und auf ein neues

OK:             pop     si              ; Register zurueck
                pop     dx
                pop     cx
                pop     bx

                ret

KeyBufferLen    equ     30              ; 30 Zeichen sollten fuer Zahlen reichen...
KeyBuffer       db      KeyBufferLen    ; Maimallaenge fuer DOS
                db      0               ; effektive Laenge Eingabe
                db      KeyBufferLen dup (0) ; Platz fuer Eingabe
InvMsg          db      "Ung",UUML,"ltiges Zahlenformat",CR,LF,'$'
OverMsg         db      "Bereichs",UUML,"berschreitung",CR,LF,'$'

                endp

;******************************************************************************
;* eine Anzahl Leerzeichen ausgeben                                           *
;*              In  :   CX = Anzahl                                           *
;******************************************************************************

                globproc WriteSpc

                push    dx              ; Register retten

                jcxz    NULL            ; Nullschleife abfangen
Loop:           mov     ah,DOS_WrChar
                mov     dl,' '
                int     INT_DOS
                loop    Loop

NULL:           pop     dx              ; Register zurueck
                ret

                endp

;******************************************************************************
;* vorzeichenlose Zahl dezimal ausgeben                                       *
;*              In  :   AX = Zahl                                             *
;*                      CL = min. Stellenzahl                                 *
;******************************************************************************

                globproc WriteDec

                push    di              ; Register retten
                push    cx
                push    dx

                mov     ch,0            ; CH zaehlt effektive Zeichenzahl
InLoop:         sub     dx,dx           ; Stellendivision
                mov     di,10           ; gewuenschtes Zahlensystem
                div     di
                mov     di,ax           ; war es vorher 0 ?
                or      di,dx           ; (wenn Quotient & Rest 0)
                jnz     NZero           ; nein-->normal ausgeben
                or      ch,ch           ; noch erste Stelle ?
                jz      NZero           ; dann auf jeden Fall 0 ausgeben
                mov     dl,0f0h         ; ansonsten Leerzeichen fuer leading 0
NZero:          push    dx              ; Zeichen speichern...
                inc     ch              ; ...und mitzaehlen
                cmp     ch,cl           ; Mindestzahl ausgegeben ?
                jb      InLoop          ; nein-->weiter
                or      ax,ax           ; ansonsten: 0 ?
                jnz     InLoop          ; nein, weitermachen

                shr     cx,8            ; effektive Zahl nach CX
OLoop:          pop     dx              ; ein Zeichen runterholen
                add     dl,'0'          ; in ASCII konvertieren
                mov     ah,DOS_WrChar   ; ueber DOS ausgeben
                int     INT_DOS
                loop    OLoop

                pop     dx
                pop     cx
                pop     di              ; Register zurueck
                ret

                endp

;******************************************************************************
;* vorzeichenlose Zahl hexadezimal ausgeben                                   *
;*              In  :   AX = Zahl                                             *
;*                      CL = min. Stellenzahl                                 *
;******************************************************************************

                globproc WriteHex

                push    di              ; Register retten
                push    cx
                push    dx

                mov     ch,0            ; CH zaehlt effektive Zeichenzahl
InLoop:         sub     dx,dx           ; Stellendivision
                mov     di,16           ; gewuenschtes Zahlensystem
                div     di
                mov     di,ax           ; war es vorher 0 ?
                or      di,dx           ; (wenn Quotient & Rest 0)
                jnz     NZero           ; nein-->normal ausgeben
                or      ch,ch           ; noch erste Stelle ?
                jz      NZero           ; dann auf jeden Fall 0 ausgeben
                mov     dl,0f0h         ; ansonsten Leerzeichen fr leading 0
NZero:          push    dx              ; Zeichen speichern...
                inc     ch              ; ...und mitzaehlen
                cmp     ch,cl           ; Mindestzahl ausgegeben ?
                jb      InLoop          ; nein-->weiter
                or      ax,ax           ; ansonsten: 0 ?
                jnz     InLoop          ; nein, weitermachen

                shr     cx,8            ; effektive Zahl nach CX
OLoop:          pop     dx              ; ein Zeichen runterholen
                add     dl,'0'          ; in ASCII konvertieren
                cmp     dl,'9'
                jbe     NoHex
                add     dl,7
NoHex:          mov     ah,DOS_WrChar   ; ueber DOS ausgeben
                int     INT_DOS
                loop    OLoop

                pop     dx
                pop     cx
                pop     di              ; Register zurueck
                ret

                endp

;*****************************************************************************
;* Routine GeometryDefined - stellt fest, ob Geometrie fuer ei Laufwerk defi-*
;* niert ist                                                                 *
;* Eingabe:     AL = Laufwerksnummer                                         *
;* Ausgabe:     C  = 0, falls OK                                             *
;*****************************************************************************

                proc    GeometryDefined

                push    di              ; Register retten
                call    GetPTabAdr      ; Tabellenadresse bilden
                cmp     word ptr[di+DrPar_Cyls],0 ; Zylinderzahl 0 ?
                stc
                je      Fin
                cmp     byte ptr[di+DrPar_Heads],0 ; Kopfzahl 0 ?
                stc
                je      Fin
                cmp     byte ptr[di+DrPar_NSecs],0 ; Sektorzahl 0 ?
                stc
                je      Fin
                clc                     ; alles OK
Fin:            pop     di              ; Register zurueck
                ret

                endp

;*****************************************************************************
;* Routine QueryRedefine - fragt nach, ob Geometrie neu definert werden soll *
;* Eingabe:     AL = Laufwerk                                                *
;* Ausgabe:     C = 0, falls ja                                              *
;*****************************************************************************

                proc    QueryRedefine

                add     al,'1'          ; Laufwerksnummer in ASCII umrechnen
                mov     [UndefMsg2],al  ; in Meldung einschreiben
                PrMsg   UndefMsg
                PrMsg   DoQueryMsg      ; nachfragen, ob Definition erwuenscht
                call    YesNo
                ret

UndefMsg        db      "Geometrie f",UUML,"r Laufwerk "
UndefMsg2       db      "  undefiniert.",CR,LF,"$"
DoQueryMsg      db      "Geometrie neu definieren ? $"

                endp

;*****************************************************************************
;* Routine ReadGeomety - liest Laufwerksgeometrie vom Benutzer ein           *
;* Eingabe:     AL = phys. Laufwerksnummer                                   *
;*****************************************************************************

                proc    ReadGeometry

                push    ax              ; Register retten
                push    si
                push    di

                call    GetPTabAdr      ; Zeiger auf Parametertabelle holen
                mov     si,di

                lea     di,[CylInpMsg]  ; Zylinderzahl erfragen
                mov     ax,1024
                call    ReadNumber
                mov     [si+DrPar_Cyls],ax

                lea     di,[HeadInpMsg] ; Kopfzahl erfragen
                mov     ax,16
                call    ReadNumber
                mov     [si+DrPar_Heads],al

                lea     di,[RWCInpMsg]  ; RWC-Zylinder erfragen
                mov     ax,65535
                call    ReadNumber
                mov     [si+DrPar_RedWr],ax

                lea     di,[PreInpMsg]  ; Praekompensations-Zylinder erfragen
                mov     ax,65535
                call    ReadNumber
                mov     [si+DrPar_PrComp],ax

                lea     di,[ECCInpMsg]  ; ECC-Laenge erfragen
                mov     ax,11
                call    ReadNumber
                mov     [si+DrPar_ECCLen],ax

                mov     al,[si+DrPar_Heads] ; Steuerbyte Bit 3=1, falls Platte
                dec     al
                and     al,8            ; mehr als 8 Koepfe hat
                mov     [si+DrPar_CByte],al

                mov     al,0            ; Timeouts unbenutzt
                mov     [si+DrPar_TOut],al
                mov     [si+DrPar_FTOut],al
                mov     [si+DrPar_CTOut],al

                mov     ax,[si+DrPar_Cyls] ; Parkzylinder=Zylinderzahl+1
                inc     ax
                mov     [si+DrPar_LZone],ax

                lea     di,[SecInpMsg]  ; Sektorzahl erfragen
                mov     ax,255
                call    ReadNumber
                mov     [si+DrPar_NSecs],al

                pop     di              ; Register zurueck
                pop     si
                pop     ax
                ret

CylInpMsg       db      "Anzahl Zylinder (max. 1024) : $"
HeadInpMsg      db      "Anzahl K",OUML,"pfe (max. 16) : $"
RWCInpMsg       db      "Startzylinder f",UUML,"r reduzierten Schreibstrom (max. 65535) : $"
PreInpMsg       db      "Startzylinder f",UUML,"r Pr",AUML,"kompensation (max. 65535) : $"
ECCInpMsg       db      "max. ECC-Fehlerburstl",AUML,"nge (5 oder 11) : $"
SecInpMsg       db      "Sektorzahl (max. 255) : $"

                endp

;*****************************************************************************
;* Routine WriteGeoToDisk - schreibt Laufwerksgeometrie auf Platte           *
;* Eingabe:     AL = phys. Laufwerksnummer                                   *
;* Ausgabe:     C+AX = Fehlerstatus                                          *
;*****************************************************************************

                proc    WriteGeoToDisk

                push    bx              ; Register retten
                push    cx
                push    dx
                push    si
                push    di
                push    es

                mov     dx,ds           ; alles im folgenden im Datensegment
                mov     es,dx

                mov     dl,al           ; Laufwerksnummer retten
                mov     ah,0            ; Kopf 0,
                sub     bx,bx           ; Spur 0,
                mov     cx,0101h        ; Sektor 1 lesen
                lea     di,[SectorBuffer]
                call    ReadSectors
                jc      GeoError        ; Abbruch bei Fehler

                mov     al,dl           ; Geometrietabelle adressieren
                call    GetPTabAdr
                mov     si,di           ; selbige in MBR einschreiben
                lea     di,[SectorBuffer+DrPar_Offset]
                mov     cx,DrPar_Len/2
                cld
                rep     movsw

                mov     al,dl           ; jetzt den ganzen Kram zurueckschreiben
                mov     ah,0
                sub     bx,bx
                mov     cx,0101h
                lea     si,[SectorBuffer]
                call    WriteSectors
                jc      GeoError

Fin:            pop     es              ; Register zurueck
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                ret

GeoError:       mov     ah,bl           ; Fehlerausgabe
                call    WrErrorCode
                jmp     Fin

                endp

;*****************************************************************************
;* Routine QueryParams                                                       *
;* Eingabe:     AL = Laufwerksnummer                                         *
;*              AH = 1, falls Rueckschreiben erlaubt                         *
;* Ausgabe:     AL = 0: Laufwerk vergessen                                   *
;*              AL = 1: Mastersektor neu lesen                               *
;*              AL = 2: Tabelle nur transient eingetragen, kein Neulesen     *
;*****************************************************************************

                globproc QueryParams

                push    bx              ; Register retten
                push    cx
                push    dx
                push    si
                push    di
                push    es

                mov     bx,ax           ; Laufwerksnummer retten
                call    QueryRedefine   ; nachfragen, ob Neudefinition erwuenscht
                mov     al,0            ; Abbruch bei Nein
                ljc     Terminate

                mov     al,bl           ; Geometrie einlesen
                call    ReadGeometry

                shr     bh,1            ; Rueckschreiben erlaubt ?
                cmc                     ; C=1-->verboten
                jc      NoWriteBack

                PrMsg   WriteBackMsg    ; nachfragen, ob Rueckschreiben erwuenscht
                call    YesNo
NoWriteBack:    mov     al,2            ; Neulesen bei Nein verhindern
                jc      Terminate

                mov     al,bl
                call    WriteGeoToDisk
                jc      WrError

                mov     al,1            ; Ergebnis: MBR-Lesen wiederholen
                jmp     Terminate
WrError:        mov     al,0            ; Schreibfehler: Laufwerk ignorieren

Terminate:      pop     es              ; Register zurueck
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx

                ret

WriteBackMsg    db      "Parametersatz zur",UUML,"ckschreiben ? $"

                endp

;****************************************************************************
;* Laufwerksnummer einlesen                                                 *
;* Ausgabe:     AL = Laufwerksnummer                                        *
;****************************************************************************

                proc    InputDrive

                push    dx              ; Register retten

RdLoop:         PrMsg   PromptMsg       ; Anfrage ausgeben
                mov     ah,DOS_RdChar   ; ein Zeichen holen
                int     INT_DOS
                mov     dl,al           ; Zeichen retten
                PrChar  dl              ; Laufwerk als Echo zurueckgeben
                PrChar  CR
                PrChar  LF
                sub     dl,'1'          ; nur 1 oder 2 erlaubt
                jc      RdLoop
                cmp     dl,MaxPDrives
                jae     RdLoop
                mov     al,dl           ; Laufwerk in AL zurueckgeben

                pop     dx              ; Register zurueck
                ret

PromptMsg       db      "Laufwerksnummer (1 oder 2) : $"

                endp

;****************************************************************************
;* Men fr Plattenfunktionen                                               *
;****************************************************************************

                proc    SelectDisk

                push    ax              ; Register sichern
                push    bx
                push    cx
                push    dx
                push    si
                push    di
                push    es

                mov     dh,ah           ; Rueckschreibeflag sichern
                call    InputDrive
                mov     dl,al           ; Laufwerksnummer sichern

                mov     al,dl           ; Geometrie noch undefiniert ?
                call    GeometryDefined
                jnc     IsOK            ; Nein, alles in Butter

                mov     al,dl           ; nicht definiert: versuchen,
                mov     ah,0            ; Geometrie vom MBR zu lesen
                mov     bx,ds
                mov     es,bx
                sub     bx,bx
                mov     cx,0101h
                lea     di,[SectorBuffer]
                call    ReadSectors
                jnc     CopyTable       ; kein Fehler->Tabelle auslesen
                mov     dh,0            ; wenn Fehler, nie zurueckschreiben
                jmp     ReadManual      ; und manuell probieren
CopyTable:      lea     si,[SectorBuffer+DrPar_Offset]
                mov     al,dl
                call    GetPTabAdr
                mov     cx,DrPar_Len/2
                cld
                rep     movsw
                mov     al,dl           ; Geometrie jetzt da ?
                call    GeometryDefined
                jnc     IsOK            ; falls ja, Ende

ReadManual:     mov     al,dl           ; fragen, ob Redefinition erwuenscht
                call    QueryRedefine
                jc      NotOK           ; falls nein, Abbruch
                mov     al,dl           ; ansonsten einlesen
                call    ReadGeometry
                shr     dh,1            ; zurueckschreiben ?
                jnc     IsOK
                mov     al,dl           ; ja...
                call    WriteGeoToDisk  ; Fehler ignorieren

IsOK:           mov     [MomDrive],dl   ; Laufwerk akzeptiert

NotOK:          pop     es              ; Register zurueck
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                pop     ax

                ret

                endp

;----------------------------------------------------------------------------

                proc    ChangeGeometry

                cmp     [MomDrive],-1   ; Laufwerk ueberhaupt schon definiert ?
                jne     DriveDefined
                call    InputDrive      ; nein: lesen & einschreiben
                mov     [MomDrive],al
DriveDefined:   mov     al,[MomDrive]   ; neue Geometrie einlesen
                call    ReadGeometry
                mov     al,[MomDrive]
                call    WriteGeoToDisk  ; die auch gleich zu schreiben versuchen

                ret

                endp

;----------------------------------------------------------------------------

                proc    VerifyDisk

                pusha                   ; restlos alles...

                mov     al,[MomDrive]   ; erstmal sicherstellen, dass der
                call    SetDriveParams  ; Kontroller die Geometrie kennt

                mov     al,[MomDrive]   ; die Geometrie brauchen wir auch
                call    GetPTabAdr

                PrMsg   ESCMsg
                sub     bp,bp           ; Fehlerzaehler
                mov     si,bp           ; Zylinderzaehler
                mov     dx,bp           ; Folgefehlerzaehler
CylLoop:        mov     dl,0            ; Kopfzaehler
HeadLoop:       PrMsg   CylMsg          ; zu testende Spur ausgeben
                mov     ax,si
                mov     cl,4
                call    WriteDec
                PrMsg   HeadMsg
                mov     al,dl
                mov     ah,0
                mov     cl,2
                call    WriteDec
                PrChar  CR

                mov     al,[MomDrive]   ; eine Spur testen
                mov     ah,dl
                mov     bx,si
                mov     cl,[di+DrPar_NSecs]
                mov     ch,1
                call    VeriSectors

                jnc     NoError         ; evtl. Fehlerbehandlung
                push    ax
                PrChar  LF
                pop     ax
                mov     ah,[MomDrive]
                call    WrErrorCode
                inc     bp              ; Fehlerzaehler rauf
                inc     dh
                test    dh,7            ; alle 8 Fehler in Reihe nachfragen
                jnz     NextTrack
                PrMsg   GoOnMsg
                call    YesNo
                jc      Terminate
                jmp     NextTrack
NoError:        mov     dh,0            ; eine Spur gut->Folgenzaehler loeschen
NextTrack:      call    BreakONESC      ; Abbruch ?
                jc      Terminate
                inc     dl              ; naechster Kopf
                cmp     dl,[di+DrPar_Heads]
                jb      HeadLoop
                inc     si              ; naechster Zylinder
                cmp     si,[di+DrPar_Cyls]
                ljb     CylLoop

Terminate:      mov     ax,bp           ; Fehlerzahl ausgeben
                mov     cl,5
                call    WriteDec
                PrMsg   ErrorMsg

                popa                    ; Register zurueck

                ret

EscMsg          db      "Verifizieren..",CR,LF,"Abbruch mit <ESC>",CR,LF,'$'
CylMsg          db      "Zylinder $"
HeadMsg         db      ", Kopf $"
GoOnMsg         db      "Test fortsetzen? $"
ErrorMsg:       db      " Fehler gefunden     ",CR,LF,'$'

                endp

;----------------------------------------------------------------------------

                proc    FormatDisk

                push    ax              ; Register retten
                push    bx
                push    cx
                push    di
                push    es

                mov     al,[MomDrive]   ; erstmal sicherstellen, dass der
                call    SetDriveParams  ; Kontroller die Geometrie kennt

                mov     al,[MomDrive]
                call    GetPTabAdr
InterLoop:      mov     al,[di+DrPar_NSecs] ; Maximum=Sektorzahl-1
                dec     al
                mov     ah,0
                lea     di,[InterleaveMsg] ; Interleave erfragen
                call    ReadNumber
                or      ax,ax           ; Null wollen wir nicht
                jz      InterLoop
                mov     bl,al           ; Interleave retten
                PrMsg   ConfirmMsg      ; sicherheitshalber nachfragen
                call    YesNo
                jc      Fin
                PrMsg   NewLine
                PrMsg   FormatMsg
                mov     ah,bl           ; Interleave zurueck
                mov     al,[MomDrive]
                call    FormatUnit
                jc      FormatError     ; Fehler beim Formatieren ?

NoFormatError:  PrMsg   WriteMsg
                lea     di,[SectorBuffer] ; MBR erzeugen
                cld
                mov     cx,SecSize/2-1  ; letztes Wort anders!
                mov     ax,ds
                mov     es,ax
                sub     ax,ax           ; prinzipiell erstmal alles Nullen
                rep     stosw
                mov     word ptr[di],0aa55h ; Gueltigkeitsflag am Ende
                mov     al,[MomDrive]   ; Geometrietabelle eintragen
                call    GetPTabAdr
                mov     si,di
                lea     di,[SectorBuffer+DrPar_Offset]
                mov     cx,DrPar_Len/2
                rep     movsw
                mov     al,[MomDrive]   ; Sektor schreiben
                mov     ah,0            ; MBR auf Kopf 0,
                mov     bx,0            ; Spur 0,
                mov     cx,0101h        ; Sektor 1
                lea     si,[SectorBuffer]
                call    WriteSectors
                jc      FormatError     ; Fehler beim Schreiben ?

Fin:            pop     es              ; Register zurueck
                pop     di
                pop     cx
                pop     bx
                pop     ax
                ret

FormatError:    cmp     al,DErr_UserTerm ; Abbruch durch Benutzer ?
                je      Fin             ; dann nicht meckern
                push    ax              ; Fehlercode retten
                pushf
                PrMsg   NewLine
                popf
                pop     ax
                mov     ah,[MomDrive]   ; Fehlermeldung ausgeben
                call    WrErrorCode
                jmp     Fin

FormatMsg        db     "Formatieren...",CR,LF,'$'
WriteMsg         db     "MBR schreiben...",CR,LF,'$'

                endp

;----------------------------------------------------------------------------

                proc    BadTrack

                push    bx
                push    si
                push    di

                mov     al,[MomDrive]   ; Zeiger auf Geometrietabelle
                call    GetPTabAdr      ; holen
                mov     si,di

                lea     di,[TrackMsg]   ; Spurnummer abfragen
                mov     ax,[si+DrPar_Cyls]
                dec     ax
                call    ReadNumber
                mov     bx,ax
                lea     di,[HeadMsg]    ; Kopfnummer abfragen
                mov     ax,[si+DrPar_Heads]
                dec     ax
                call    ReadNumber
                mov     ah,al
                push    ax              ; sicherheitshalber noch bestaetigen
                PrMsg   ConfirmMsg
                call    YesNo
                pop     ax
                jc      NoError
                mov     al,[MomDrive]   ; Spur markieren
                call    MarkBad      
                jnc     NoError
                push    ax              ; Fehlercode retten
                pushf
                PrMsg   NewLine
                popf
                pop     ax
                mov     ah,[MomDrive]
                call    WrErrorCode
NoError:
                pop     di
                pop     si
                pop     bx

                ret

                endp

;----------------------------------------------------------------------------

                proc    FormTrack

                push    bx
                push    si
                push    di

                mov     al,[MomDrive]   ; Zeiger auf Geometrietabelle
                call    GetPTabAdr      ; holen
                mov     si,di

                lea     di,[TrackMsg]   ; Spurnummer abfragen
                mov     ax,[si+DrPar_Cyls]
                dec     ax
                call    ReadNumber
                mov     bx,ax
                lea     di,[HeadMsg]    ; Kopfnummer abfragen
                mov     ax,[si+DrPar_Heads]
                dec     ax
                call    ReadNumber
                mov     ah,al
                push    ax              ; Kopf retten
InterLoop:      mov     al,[si+DrPar_NSecs] ; Interleave-Maximum=Sektorzahl-1
                dec     al
                mov     ah,0
                lea     di,[InterleaveMsg] ; Interleave erfragen
                call    ReadNumber
                or      ax,ax           ; Null wollen wir nicht
                jz      InterLoop
                mov     cl,al           ; Interleave passend ablegen
                PrMsg   ConfirmMsg      ; nochmal nachfragen
                call    YesNo
                pop     ax
                jc      NoError
                mov     al,[MomDrive]   ; Kopf zurueck
                call    FormatTrack
                jnc     NoError
                push    ax              ; Fehlercode retten
                pushf
                PrMsg   NewLine
                popf
                pop     ax
                mov     ah,[MomDrive]
                call    WrErrorCode
NoError:
                pop     di
                pop     si
                pop     bx

                ret

                endp

;----------------------------------------------------------------------------

; packt eine Sektorkoordinate ins BIOS-Format
; -->Zylinder=BX, Kopf=AH, Sektor=AL
; <--Zylinder/Sektor=BX, Kopf=AH

                proc    PackCoordinate

                shl     bh,6
                or      bh,al
                xchg    bl,bh
                ret

                endp

                proc    UnpackCoordinate

                xchg    bh,bl           ; Zylinderbytes in richtige Reihenfolge
                mov     al,bh           ; Sektor ausmaskieren
                and     al,00111111b
                shr     bh,6            ; Zylinder korrigieren

                ret

                endp

; berechnet aus einer Sektorkoordinate die lineare Sektornummer
; -->Zylinder/Sektor=BX, Kopf=AH, Geometriezeiger=SI
; <--Sektornummer=DX/AX

                proc    LinearizeCoordinate

                push    bx
                push    cx

                mov     cx,ax           ; Kopf retten
                mov     al,bh           ; Zylinder rekonstruieren
                mov     ah,bl
                shr     ah,6
                mov     dl,[si+DrPar_Heads]
                mov     dh,0
                mul     dx              ; = Anzahl Spuren bis Zylinderbeginn
                add     al,ch           ; Startkopf dazuaddieren
                adc     ah,0            ; bisher hoffentlich nur 16 Bit...
                mov     dl,[si+DrPar_NSecs]
                mov     dh,0
                mul     dx              ; = Anzahl Spuren bis Spurbeginn
                and     bl,3fh          ; letztendlich Sektor-1 dazu
                dec     bl
                add     al,bl
                adc     ah,0
                adc     dx,0

                pop     cx
                pop     bx

                ret

                endp

                proc    MakePart

                push    bx
                push    cx
                push    dx
                push    si
                push    di
                push    es

                PrMsg   ConfirmMsg      ; sind wir sicher ?
                call    YesNo
                ljc     End

                mov     al,[MomDrive]   ; Laufwerk rekalibrieren
                call    Recalibrate
                ljc     PartError

                mov     al,[MomDrive]   ; alten MBR auslesen
                mov     ah,0
                mov     bx,0
                mov     cx,0101h
                mov     si,ds
                mov     es,si
                lea     di,[SectorBuffer]
                call    ReadSectors
                ljc     PartError

                mov     al,[MomDrive]   ; Plattengeometrie holen
                call    GetPTabAdr
                mov     si,di

                lea     di,[SectorBuffer+ParTab_Offset] ; an erste Tabelle schreiben
                mov     byte ptr [di+ParTab_BFlag],80h ; Partition aktiv
                cmp     byte ptr[si+DrPar_Heads],1 ; nur ein Kopf ?
                ja      MoreHeads
                mov     bx,1            ; ja: Start auf Zyl. 1, Kopf 0
                mov     ah,0
                jmp     WriteStart
MoreHeads:      sub     bx,bx           ; nein: Start auf Zyl. 0, Kopf 1
                mov     ah,1
WriteStart:     mov     al,01h          ; Startsektor immer 1
                call    PackCoordinate
                mov     [di+ParTab_FHead],ah
                mov     [di+ParTab_FSecCyl],bx
                call    LinearizeCoordinate ; linearen Start schreiben
                mov     [di+ParTab_LinSec],ax
                mov     [di+ParTab_LinSec+2],dx
                push    dx
                push    ax
                mov     bx,[si+DrPar_Cyls] ; Ende: Zylinder n-2, Kopf n-1, Sektor n
                sub     bx,2
                mov     ah,[si+DrPar_Heads]
                dec     ah
                mov     al,[si+DrPar_NSecs]
                call    PackCoordinate
                mov     [di+ParTab_LHead],ah
                mov     [di+ParTab_LSecCyl],bx
                call    LinearizeCoordinate ; Sektorzahl berechnen
                pop     bx              ; dazu Start abziehen
                sub     ax,bx
                pop     bx
                sbb     dx,bx
                add     ax,1            ; !!! Laenge=Stop-Start+1
                adc     dx,0
                mov     [di+ParTab_NSecs],ax
                mov     [di+ParTab_NSecs+2],dx
                or      dx,dx            ; falls >64K Sektoren,
                jz      NoBigDOS        ; eine BigDOS-Partition
                mov     bl,6
                jmp     TypeFound
NoBigDOS:       cmp     ax,32679        ; ab 32680 Sektoren 16-Bit-FAT
                jb      NoFAT16
                mov     bl,04
                jmp     TypeFound
NoFAT16:        mov     bl,1            ; kleine 12-Bit-Partition
TypeFound:      mov     [di+ParTab_Type],bl
                add     di,ParTab_Len   ; die anderen 3 Partitionen loeschen
                mov     cx,3*(ParTab_Len/2)
                sub     ax,ax
                cld
                rep     stosw

                mov     al,[MomDrive]   ; neuen MBR schreiben
                mov     ah,0
                mov     bx,0
                mov     cx,0101h
                lea     si,[SectorBuffer]
                call    WriteSectors
                ljc     PartError

End:            pop     es
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx

                ret

PartError:      push    ax              ; Fehlercode retten
                pushf
                PrMsg   NewLine
                popf
                pop     ax
                mov     ah,[MomDrive]   ; Fehlermeldung ausgeben
                call    WrErrorCode
                jmp     End

ConfirmMsg:     db      "ACHTUNG! Alle bisherigen Partitionen auf der",CR,LF
                db      "Festplatte werden gel",OUML,"scht! Fortfahren? $"

                endp

;----------------------------------------------------------------------------

                proc    FormatPart

                pusha

                mov     ax,ds           ; wir arbeiten nur im Datensegment
                mov     es,ax

                PrMsg   ConfirmMsg      ; vorher nachfragen
                call    YesNo
                ljc     Ende

                mov     al,[MomDrive]   ; Laufwerk rekalibrieren
                call    Recalibrate
                ljc     LFormError

; Schritt 1: MBR lesen

                mov     al,[MomDrive]   ; MBR auslesen, um Partitions-
                mov     ah,0            ; daten zu bekommen
                mov     bx,0
                mov     cx,0101h
                lea     di,[SectorBuffer]
                call    ReadSectors
                ljc     LFormError

; Schritt 2: Partitionsdaten in BPB kopieren

                lea     di,[SectorBuffer+ParTab_Offset] ; auf Partitionsdaten
                mov     al,[di+ParTab_Type] ; muss primaere Partition sein
                cmp     al,1            ; DOS 2.x FAT12?
                je      ParTypeOK
                cmp     al,4            ; DOS 3.x FAT16?
                je      ParTypeOK
                cmp     al,6            ; DOS 4.x BIGDOS?
                je      ParTypeOK
                PrMsg   InvParTypeMsg   ; nichts dergleichen: Abbruch
                jmp     Ende
ParTypeOK:      mov     word ptr[BPB_SysID],'AF' ; FAT-Kennung in BPB eintragen
                mov     word ptr[BPB_SysID+2],'1T' ; FAT12/FAT16
                mov     word ptr[BPB_SysID+5],'  '
                mov     ah,'2'          ; Annahme FAT12
                cmp     al,1
                je      ParIsFAT12      ; wenn Typ=1, OK
                mov     ah,'6'          ; ansonsten FAT16
ParIsFAT12:     mov     byte ptr[BPB_SysID+4],ah
                mov     ax,[di+ParTab_NSecs] ; Sektorzahl in BPB schreiben
                mov     dx,[di+ParTab_NSecs+2]
                mov     word ptr[BPB_NumSecs32],ax
                mov     word ptr[BPB_NumSecs32+2],dx
                or      dx,dx           ; falls < 64K Sektoren,
                jz      ParIsNotBig     ; Groesse auch unten eintragen,
                sub     ax,ax           ; ansonsten 0
ParIsNotBig:    mov     [BPB_NumSecs16],ax
                mov     ax,word ptr[di+ParTab_LinSec] ; Startsektor umkopieren
                mov     dx,word ptr[di+ParTab_LinSec+2]
                mov     word ptr[BPB_LinStart],ax
                mov     word ptr[BPB_LinStart+2],dx

; Schritt 3: Partitionsdaten in Partitionstabelle kopieren, damit wir die
; linearen Schreib/Lesefunktionen nutzen koennen

                mov     [DrCnt],1       ; nur ein Laufwerk belegt
                mov     ah,[di+ParTab_FHead] ; Startkoordinate ablegen
                mov     bx,[di+ParTab_FSecCyl]
                call    UnpackCoordinate
                mov     [DrTab+DrTab_StartHead],ah
                mov     word ptr [DrTab+DrTab_StartCyl],bx
                mov     [DrTab+DrTab_StartSec],al
                mov     ax,[di+ParTab_LinSec]
                mov     word ptr [DrTab+DrTab_LinStart],ax
                mov     ax,[di+ParTab_LinSec+2]
                mov     word ptr [DrTab+DrTab_LinStart+2],ax
                mov     ax,[di+ParTab_NSecs]
                mov     word ptr [DrTab+DrTab_SecCnt],ax
                mov     ax,[di+ParTab_NSecs+2]
                mov     word ptr [DrTab+DrTab_SecCnt+2],ax
                mov     al,[MomDrive]   ; Laufwerk einschreiben
                mov     [DrTab+DrTab_Drive],al
                mov     byte ptr[DrTab+DrTab_BPB],0 ; kein BPB

; Schritt 4: konstante Felder in BPB eintragen

                mov     [BPB_SecSize],SecSize ; Sektorgroesse konstant 512 Byte
                mov     [BPB_ResvdSecs],1 ; nur Bootsektor reserviert
                mov     [BPB_NumFATs],2 ; 2 FATs ist DOS-Standard
                mov     [BPB_MediaID],0f8h ; Media-Byte fuer Platten konstant
                mov     al,[MomDrive]
                call    GetPTabAdr
                mov     ah,0
                mov     al,[di+DrPar_NSecs]; Plattenzylinder und -koepfe
                mov     [BPB_SecsPerTrk],ax
                mov     al,[di+DrPar_Heads]
                mov     [BPB_Heads],ax
                mov     al,[MomDrive]   ; Plattennummer+80h
                add     al,80h
                mov     [BPB_PhysNo],ax
                mov     [BPB_ExtID],29h ; Erkennung, dass erw. BPB gueltig
                mov     ah,0            ; Seriennummer=Uhrzeit
                int     INT_Clock
                mov     word ptr[BPB_SerialNo],cx
                mov     word ptr[BPB_SerialNo+2],dx
                lea     di,[BPB_Name]   ; Name ist leer
                mov     cx,11
                mov     al,' '
                cld
                rep     stosb

; Schritt 5: einige Sachen vom Anwender erfragen

DirEntLoop:     mov     ax,1024         ; mehr ist wohl kaum sinnvoll
                lea     di,[DirEntriesMsg]
                call    ReadNumber
                cmp     ax,SecSize/32   ; weniger als ein Sektor ergibt
                jb      DirEntLoop      ; keinen Sinn
                mov     [BPB_DirLen],ax ; Anzahl in BPB eintragen
                mov     dx,0            ; Directory-Sektorzahl berechnen
                mov     bx,SecSize/32
                div     bx
                or      dx,dx           ; ggfs. aufrunden
                jz      DirLenEven
                inc     ax
DirLenEven:     mov     [DirLen],ax

; Schritt 6: Clusterlaenge berechnen

                mov     ax,word ptr[BPB_NumSecs32]  ; # Sektoren in Datenfeld
                mov     dx,word ptr[BPB_NumSecs32+2] ; und FATs berechnen
                sub     ax,[BPB_ResvdSecs]
                sbb     dx,0
                sub     ax,[DirLen]
                sbb     dx,0
                mov     bl,1 ; Annahme: moeglichst wenig Sektoren pro Cluster
ClusterLoop:    or      dx,dx           ; wenn noch mehr als 64K Cluster,
                jnz     ClusterNxtLoop  ; auf jeden Fall zu groá
                cmp     ax,4080         ; bei weniger als 4K Clustern
                jb      ClusterEndLoop  ; auf jeden Fall OK
                cmp     [BPB_SysID+4],'2' ; sonst bei FAT12
                je      ClusterNxtLoop  ; auf jeden Fall zu viel
                cmp     ax,65510        ; bei FAT16 Vergleich auf 64K
                jb      ClusterEndLoop
ClusterNxtLoop: shl     bl,1            ; zu viel: Cluster verdoppeln
                js      ClusterEndLoop  ; bei 128 Sektoren/Cluster abbrechen
                shr     dx,1            ; Clusterzahl halbiert sich
                rcr     ax,1
                jmp     ClusterLoop
ClusterEndLoop: mov     [BPB_SecsPerClust],bl ; Ergebnis einschreiben
                add     ax,2            ; Dummy-Eintraege in FAT
                adc     dx,0
                mov     bx,341          ; Anzahl FAT-Sektoren berechnen
                cmp     [BPB_SysID+4],'2'
                jz      Cluster12
                mov     bx,256
Cluster12:      div     bx
                or      dx,dx           ; Sektorzahl aufrunden
                jz      FATLenEven
                inc     ax
FATLenEven:     mov     [BPB_SecsPerFAT],ax ; Anzahl FAT-Sektoren einschreiben

; Schritt 7: Bootsektor aufbauen

                PrMsg   WrBootSectMsg
                lea     di,[SectorBuffer]
                cld
                mov     al,0ebh         ; Dummy-Sprung einschreiben
                stosb
                mov     al,0feh
                stosb
                mov     al,90h
                stosb
                mov     ax,'ES'         ; OEM-ID einschreiben
                stosw
                mov     ax,'DC'
                stosw
                mov     ax,'IR'
                stosw
                mov     ax,'EV'
                stosw
                lea     si,[BPBBuffer]  ; BPB einschreiben
                mov     cx,BPB_Length
                rep     movsb
                mov     cx,SectorBuffer+SecSize ; Rest vom Bootsektor nullen
                sub     cx,di
                mov     al,0
                rep     stosb
                mov     ax,0            ; Bootsektor ist log. Sektor 0
                mov     dx,ax
                mov     bl,0            ; Partition 0
                call    TranslateParams
                mov     cl,1            ; nur ein Sektor
                lea     si,[SectorBuffer]
                call    WriteSectors
                ljc     LFormError

; Schritt 8: Directory & FATs ausnullen

                lea     di,[SectorBuffer] ; Sektorpuffer wird benutzt
                mov     cx,SecSize/2
                cld
                sub     ax,ax
                rep     stosw
                PrMsg   WrFATMsg
                mov     ax,[BPB_ResvdSecs] ; Startsektor FATs holen
                sub     dx,dx
                lea     si,[SectorBuffer]
                mov     cx,[BPB_SecsPerFAT]
                add     cx,cx           ; beide FATs nullen
FATZeroLoop:    push    ax              ; Sektornummer und -zahl retten
                push    cx
                push    dx
                mov     bl,0
                call    TranslateParams
                mov     cl,1
                call    WriteSectors
                pop     dx
                pop     cx
                pop     ax
                ljc     LFormError
                add     ax,1            ; naechster Sektor
                adc     dx,0
                loop    FATZeroLoop
                push    ax              ; !!! PrMsg zerstoert AX-Register
                PrMsg   WrDirMsg
                pop     ax
                mov     cx,[DirLen]     ; dito fuer Directory
DirZeroLoop:    push    ax              
                push    cx
                push    dx
                mov     bl,0
                call    TranslateParams
                mov     cl,1
                call    WriteSectors
                pop     dx
                pop     cx
                pop     ax
                ljc     LFormError
                add     ax,1            ; naechster Sektor
                adc     dx,0
                loop    DirZeroLoop

; Schritt 9: Sektoren testen und FAT initialisieren

                mov     ax,[BPB_ResvdSecs] ; Datensektorbeginn berechnen
                sub     dx,dx
                mov     [FAT1Pos],ax    ; Beginn 1. FAT hinter Bootsektor
                mov     [FAT1Pos+2],dx
                add     ax,[BPB_SecsPerFAT] ; Beginn 2. FAT hinter 1. FAT
                adc     dx,0
                mov     [FAT2Pos],ax
                mov     [FAT2Pos+2],dx
                add     ax,[BPB_SecsPerFAT]
                adc     dx,0
                add     ax,[DirLen]     ; Datenbeginn hinter Directory
                adc     dx,0
                mov     [DataPos],ax    ; diesen Startsektor retten
                mov     [DataPos+2],dx

                mov     ax,word ptr[BPB_NumSecs32] ; Anzahl Cluster berechnen
                mov     dx,word ptr[BPB_NumSecs32+2]
                sub     ax,[DataPos]
                sbb     dx,[DataPos+2]
                mov     bl,[BPB_SecsPerClust]      ; / SecsPerCluster
                mov     bh,0
                div     bx
                mov     [ClusterCnt],ax

                call    ClearFATBuffer  ; erste Elemente in FAT schreiben
                mov     ah,0ffh
                mov     al,[BPB_MediaID]
                call    WriteFAT
                mov     al,0ffh
                call    WriteFAT
                PrMsg   ESCMsg
                mov     ax,[DataPos]    ; Schleifenvorbereitung
                mov     dx,[DataPos+2]
                mov     cx,[ClusterCnt]
VerifyLoop:     push    ax              ; Zaehler retten
                mov     bp,cx
                push    dx
                mov     bl,0            ; immer Laufwerk 0
                call    TranslateParams ; Cluster testlesen
                mov     cl,[BPB_SecsPerClust]
                test    bp,15           ; nur alle 16 Cluster schreiben
                jnz     NoWriteVeri
                push    ax              ; Clusternummer ausgeben
                push    cx
                PrMsg   ClusterMsg
                mov     ax,[ClusterCnt]
                sub     ax,bp
                add     ax,2            ; erster Datencluster hat Nummer 2
                mov     cl,5
                call    WriteDec
                PrChar  CR
                pop     cx
                pop     ax
NoWriteVeri:    call    VeriSectors
                mov     ax,0            ; Annahme OK (SUB wuerde C loeschen...)
                jnc     Verify_OK
                mov     ax,0fff7h
Verify_OK:      call    WriteFAT
                pop     dx              ; Zaehler zurueck
                mov     cx,bp
                pop     ax
                add     al,[BPB_SecsPerClust]
                adc     ah,0
                adc     dx,0
                call    BreakOnESC      ; Abbruch durch Benutzer ?
                jc      Ende
                loop    VerifyLoop
                cmp     [FATBufferFill],0 ; Puffer rausschreiben
                je      NoFATFlush
                call    FlushFATBuffer
NoFATFlush:

Ende:           PrMsg   NewLine
                mov     [DrCnt],0       ; sonst kommt jemand ins Schleudern...
                popa
                ret

LFormError:     push    ax              ; Fehlercode retten
                pushf
                PrMsg   NewLine
                popf
                pop     ax
                mov     ah,[MomDrive]   ; Fehlermeldung ausgeben
                call    WrErrorCode
                jmp     Ende

WriteFAT:       push    bx              ; einen FAT-Eintrag schreiben
                mov     bx,ax
                call    WriteFATNibble  ; Bit 0..3 schreiben
                mov     al,bl           ; Bit 4..7 schreiben
                shr     al,4
                call    WriteFATNIbble
                mov     al,bh           ; Bit 8..11 schreiben
                call    WriteFATNibble
                cmp     [BPB_SysID+4],'2' ; evtl. Bit 12..15 schreiben
                je      WriteFAT12
                mov     al,bh
                shr     al,4
                call    WriteFATNibble
WriteFAT12:     pop     bx
                ret

WriteFATNibble: push    bx
                and     al,15           ; Bit 0..3 ausmaskieren
                jz      SkipWriteNibble ; Nullen brauchen wir nicht schreiben
                mov     [MustFlushFAT],1 ; vermerken, dass Puffer nicht genullt ist
                mov     bx,[FATBufferFill]  ; Bit 1.. enthalten Adresse
                shr     bx,1
                jnc     WriteFATLower   ; oben oder unten schreiben
                shl     al,4
WriteFATLower:  or      FATBuffer[bx],al
SkipWriteNibble:inc     [FATBufferFill]
                cmp     [FATBufferFill],SecSize*2 ; Sektor voll ?
                jne     WriteFAT_NFlush
                call    FlushFATBuffer
WriteFAT_NFlush:pop     bx
                ret

FlushFATBuffer: push    bx
                push    cx
                push    dx
                push    si
                cmp     [MustFlushFAT],0 ; nix zu tun ?
                je      SkipFlushDisk
                mov     ax,[FAT1Pos]    ; erste FAT schreiben
                mov     dx,[FAT1Pos+2]
                mov     bl,0
                call    TranslateParams
                mov     cl,1
                lea     si,[FATBuffer]
                call    WriteSectors
                mov     ax,[FAT2Pos]    ; zweite FAT schreiben
                mov     dx,[FAT2Pos+2]
                mov     bl,0
                call    TranslateParams
                mov     cl,1
                lea     si,[FATBuffer]
                call    WriteSectors
SkipFlushDisk:  call    ClearFATBuffer  ; Zeiger wieder auf 0
                add     [FAT1Pos],1     ; FAT-Sektornummern weiterzaehlen
                adc     [FAT1Pos+2],0
                add     [FAT2Pos],1
                adc     [FAT2Pos+2],0
                pop     si
                pop     dx
                pop     cx
                pop     bx
                ret


ClearFATBuffer: push    cx
                push    di
                cld
                lea     di,[FATBuffer]
                mov     cx,SecSize/2
                sub     ax,ax
                rep     stosw
                pop     di
                pop     cx
                mov     [FATBufferFill],ax
                mov     [MustFlushFAT],al
                ret

ConfirmMsg      db      "ACHTUNG! Alle Daten gehen verloren! Fortfahren? $"
InvParTypeMsg   db      CR,LF,"Partition 1 hat ung",UUML,"ltigen Typ oder ist undefiniert!",CR,LF,'$'
DirEntriesMsg   db      "Anzahl Eintr",AUML,"ge im Wurzelverzeichnis (16..1024) : $"
WrBootSectMsg   db      "Schreibe Boot-Sektor...",CR,LF,'$'
WrFATMsg        db      "Initialisiere FATs...",CR,LF,'$'
WrDirMsg        db      "Initialisiere Wurzelverzeichnis...",CR,LF,'$'
ESCMsg          db      "Abbruch mit <ESC>",CR,LF,'$'
ClusterMsg      db      "Teste Cluster $"

DirLen          dw      ?               ; # Directory-Sektoren
ClusterCnt      dw      ?
FAT1Pos         dw      2 dup (?)       ; speichern Sektorzaehler waehrend Test
FAT2Pos         dw      2 dup (?)
DataPos         dw      2 dup (?)

BPBBuffer:                              ; Zwischenspeicher fuer BPB
BPB_SecSize     dw      ?               ; Sektorgroesse in Bytes
BPB_SecsPerClust db     ?               ; Sektoren pro Cluster
BPB_ResvdSecs   dw      ?               ; reservierte Sektoren (Bootsektor)
BPB_NumFATs     db      ?               ; Anzahl FATs
BPB_DirLen      dw      ?               ; Anzahl Eintraege im Directory
BPB_NumSecs16   dw      ?               ; Anzahl Sektoren, falls <32 MByte
BPB_MediaID     db      ?               ; Media-Erkennungsbyte
BPB_SecsPerFAT  dw      ?               ; Sektoren pro FAT
BPB_SecsPerTrk  dw      ?               ; Sektoren pro Spur
BPB_Heads       dw      ?               ; Anzahl Koepfe
BPB_LinStart    dd      ?               ; linearer Startsektor auf Laufwerk
BPB_NumSecs32   dd      ?               ; Anzahl Sektoren, falls >=32 MByte
BPB_PhysNo      dw      ?               ; physikalische Laufwerks-Nummer
BPB_ExtID       db      ?               ; Erkennung, daá es ein erweiterter Boot-Record ist
BPB_SerialNo    dd      ?               ; Seriennummer
BPB_Name        db      11 dup (?)      ; Name
BPB_SysID       db      7 dup (?)       ; Systemerkennung
BPB_Length      equ     $-BPBBuffer     ; Laenge BPB

FATBuffer       db      SecSize dup (?) ; Puffer, um FATs aufzubauen
MustFlushFAT    db      ?               ; Flag, ob FAT-Puffer geschrieben werden muá
FATBufferFill   dw      ?               ; Anzahl Nibbles in FAT-Puffer

                endp

;----------------------------------------------------------------------------

                globproc DiskMenu

                push    ax              ; Register retten
                push    cx
                push    di

                mov     [spsave],sp     ; Stack umschalten
                mov     [sssave],ss
                mov     ax,cs
                mov     ss,ax
                lea     sp,[Stack]

MenuLoop:       mov     al,[MomDrive]   ; Festplatten-Nr. in Kopfmeldung einschreiben
                add     al,'1'
                cmp     al,'0'
                jnz     DrivePresent    ; falls Laufwerk noch undefiniert, - ausgeben
                mov     al,'-'
DrivePresent:   mov     [MenuMsg_Drv],al
                PrMsg   MenuMsg
                mov     al,[MomDrive]
                cmp     al,-1           ; falls <>-1, Geometrie ausgeben
                je      NoDrivePresent
                call    GetPTabAdr      ; dazu Tabelle holen
                mov     ax,[di+DrPar_Cyls]
                mov     cl,5
                call    WriteDec
                PrMsg   CylMsg
                mov     al,[di+DrPar_Heads]
                mov     ah,0
                mov     cl,3
                call    WriteDec
                PrMsg   HeadMsg
                mov     al,[di+DrPar_NSecs]
                mov     ah,0
                mov     cl,3
                call    WriteDec
                PrMsg   SecMsg
NoDrivePresent:
                PrMsg   MenuList
                mov     ah,DOS_RdChar
                int     INT_DOS
                push    ax
                PrChar  al
                PrMsg   TwoLines
                pop     ax

                cmp     al,'0'          ; 0 = Menue verlassen
                lje     MenuEnd

                cmp     al,'1'          ; 1 = Platte wechseln
                jne     NoSelectDisk
                mov     ah,1            ; Rueckschreiben erlaubt
                call    SelectDisk
                jmp     MenuLoop
NoSelectDisk:
                cmp     al,'2'          ; 2 = Geometrie wechseln
                jne     NoChangeGeometry
                call    ChangeGeometry
                jmp     MenuLoop
NoChangeGeometry:
                cmp     [MomDrive],-1   ; fuer alles weitere muá Platte gesetzt sein
                jne     DiskIsSelected
                push    ax
                shl     ax,8            ; Annahme: Geometrie nicht zurueckschreiben
                cmp     ah,'3'
                je      NoWriteBack
                inc     al              ; fuer alles ausser low-level schon
NoWriteBack:    call    SelectDisk      ; implizit Laufwerk erfragen
                PrMsg   NewLine
                pop     ax
                cmp     [MomDrive],-1
                lje     MenuLoop        ; wenn immer noch nicht gesetzt, Abbruch
DiskIsSelected:
                cmp     al,'3'          ; 3 = Platte low-leveln
                jne     NoFormatDisk
                call    FormatDisk
                jmp     MenuLoop
NoFormatDisk:
                cmp     al,'4'          ; 4 = Spur formatieren
                jne     NoFormTrack
                call    FormTrack
                jmp     MenuLoop
NoFormTrack:
                cmp     al,'5'          ; 5 = Platte prueflesen
                jne     NoBadTrack
                call    BadTrack
                jmp     MenuLoop
NoBadTrack:
                cmp     al,'6'          ; 6 = Platte verifizieren
                jne     NoVerifyDisk
                call    VerifyDisk
                jmp     MenuLoop
NoVerifyDisk:
                cmp     al,'7'          ; 7 = Partition anlegen
                jne     NoMakePart
                call    MakePart
                jmp     MenuLoop
NoMakePart:
                cmp     al,'8'          ; 8 = Partition formatieren
                jne     NoFormatPart
                call    FormatPart
                jmp     MenuLoop
NoFormatPart:   PrChar  BEL             ; alle anderen Angaben anmeckern
                jmp     MenuLoop
MenuEnd:        mov     ss,[sssave]     ; Stack zurueck
                mov     sp,[spsave]

                pop     di
                pop     cx
                pop     ax              ; Register zurueck

                ret

MenuMsg         db      CR,LF,"SECDRIVE Men",UUML,"      Platte:"
MenuMsg_Drv     db      '-',CR,LF,'$'
CylMsg          db      " Zylinder,$"
HeadMsg         db      " K",OUML,"pfe,$"
SecMsg          db      " Sektoren",CR,LF,'$'
MenuList        db      CR,LF
                db      "1 = Platte wechseln",CR,LF
                db      "2 = Geometrie neu definieren",CR,LF
                db      "3 = Platte formatieren",CR,LF
                db      "4 = Spur formatieren",CR,LF
                db      "5 = Defekte Spuren markieren",CR,LF
                db      "6 = Platte verifizieren",CR,LF
                db      "7 = Partition erstellen",CR,LF
                db      "8 = Partition log. formatieren",CR,LF
                db      "------------------------",CR,LF
                db      "0 = Men",UUML," verlassen",CR,LF,'$'
spsave          dw      ?
sssave          dw      ?

                db      1024 dup (?)    ; Programmstack
Stack:

                endp

MomDrive        db      -1
TwoLines:       db      CR,LF
NewLine:        db      CR,LF,'$'

                endsection
