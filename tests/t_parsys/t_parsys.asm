                page            60
                cpu             68000

                include         parsys.inc

;-----------------------------------------------------------------------------
; Die Exceptionvektoren:

                supmode         on

		org		$00000000	; Die Vektoren
		dc.l		0		; Adresse vom Stack (Dummy)
		dc.l		Start	        ; erster Start
		dc.l		ex_vec2		; Busfehler
		dc.l		ex_vec3		; Adressfehler
		dc.l		ex_vec4		; Illegaler Befehl
		dc.l		ex_vec5		; Division durch Null
		dc.l		ex_vec6		; Befehl CHK
		dc.l		ex_vec7		; Befehl TRAPV
		dc.l		ex_vec8         ; Privilegverletzung
		dc.l	        StepProc	; Ablaufverfolgung
		dc.l            ex_vec10        ; Line-A --> Gleitkomma
		dc.l		S_LineF		; Line-F --> 68881-Emulator
		dc.l		ex_vec12	; Reserviert
                dc.l            ex_vec13        ; Koprozessor-Protokollfehler
                dc.l            ex_vec14        ; illegaler FRESTORE-Frame
		dc.l		ex_vec15	; nicht initialisierter Unterbrechungsvektor
		dc.l		ex_vec16        ; Reserviert
		dc.l		ex_vec17        ; Reserviert
		dc.l		ex_vec18        ; Reserviert
		dc.l		ex_vec19        ; Reserviert
		dc.l		ex_vec20        ; Reserviert
		dc.l		ex_vec21        ; Reserviert
		dc.l		ex_vec22        ; Reserviert
		dc.l		ex_vec23        ; Reserviert
		dc.l		ex_vec24        ; Unechte Unterbrechung
		dc.l		ex_vec25        ; autovektoriell 1
		dc.l		ex_vec26        ; autovektoriell 2
		dc.l		ex_vec27        ; autovektoriell 3
		dc.l		ex_vec28        ; autovektoriell 4
		dc.l		ex_vec29        ; autovektoriell 5
		dc.l		ex_vec30        ; autovektoriell 6
		dc.l		ex_vec31        ; autovektoriell 7
		dc.l		PcSysCall	; Trap #0 --> PC-Kommunikation
		dc.l		ex_vec33        ; Trap #1
		dc.l		ex_vec34        ; Trap #2
		dc.l		ex_vec35        ; Trap #3
		dc.l		ex_vec36        ; Trap #4
		dc.l		ex_vec37        ; Trap #5
		dc.l		ex_vec38        ; Trap #6
		dc.l		ex_vec39        ; Trap #7
		dc.l		ex_vec40        ; Trap #8
		dc.l		ex_vec41        ; Trap #9
		dc.l		ex_vec42        ; Trap #10
		dc.l		ex_vec43        ; Trap #11
		dc.l		ex_vec44        ; Trap #12
                dc.l            S_LibFun        ; Trap #13 --> Libraryverwaltung
		dc.l		S_StepTgl	; Trap #14 --> Trace an/aus
		dc.l		S_ProgEnd	; Trap #15 --> Programmende
                dc.l            ex_vec48        ; BSUN in FPU gesetzt
                dc.l            ex_vec49        ; FPU inexaktes Ergebnis
                dc.l            ex_vec50        ; FPU Division durch 0
                dc.l            ex_vec51        ; FPU Unterlauf
                dc.l            ex_vec52        ; FPU Operandenfehler
                dc.l            ex_vec53        ; FPU Ueberlauf
                dc.l            ex_vec54        ; FPU signaling NAN
                dc.l            ex_vec55        ; reserviert
                dc.l            ex_vec56        ; MMU Konfigurationsfehler
                dc.l            ex_vec57        ; MMU Illegale Operation
                dc.l            ex_vec58        ; MMU Zugriffsfehler
                                                ; Vektoren 59..255 frei

;----------------------------------------------------------------------------
; Installationssequenz:

                org     $800
start:
                clr.w   S_Latch.w       ; Port nullen

                and.b   #$fc,S_MemEnd+3.w ; Speichergroesse auf Lang-
                                        ; wortadresse ausrichten
                move.l  S_MemEnd.w,a7   ; SSP setzen

                lea     -256(a7),a0     ; SSP-Anfang in A0
                move.l  a0,S_SSPEnd.w   ; sichern

                lea     S_End.w,a1      ; Codelaenge berechnen
                lea     S_Start.w,a2
                sub.l   a2,a1           ; A1=Laenge Systemcode
                moveq   #4,d0           ; auf mehrfaches von 4 aus-
                sub.w   a1,d0           ; richten
                and.l   #3,d0
                add.l   d0,a1

                sub.l   a1,a0           ; Start des Systemcodes rechnen
                move.l  a0,S_SysStart.w ; sichern
                move.l  a0,$4.w         ; =Programmstart

                move.l  a1,d0           ; Systemcode umkopieren
                lsr.l   #2,d0           ; =Zahl Langworttransfers
                subq.w  #1,d0           ; wg. DBRA
S_SysCopy:      move.l  (a2)+,(a0)+
                dbra    d0,S_SysCopy

                sub.l   a2,a0           ; Verschiebedifferenz rechnen
                move.l  a0,d1

                lea     8.w,a1          ; alle Vektoren relozieren
                moveq   #45,d0
S_RelVec:       add.l   d1,(a1)+
                dbra    d0,S_RelVec

                move.l  S_SysStart.w,a1 ; obere Speichergrenze in USP...
                move    a1,usp
                move.l  a1,S_FreeMemEnd.w ; und Variable

                move.l  #-1,S_LibStart.w; Librarykette leer

                lea     S_floatlib.w,a0 ; passende FloatLib installieren
                btst    #0,S_Latch+1.w  ; 68881 vorhanden ?
                bne.s   S_NoCo81
                lea     S_float81lib.w,a0 ; ja-->andere Library
S_NoCo81:       moveq   #LibCtrlInstall,d0 ; einschreiben
                trap    #TrapLibCtrl

                moveq   #LibCtrlGetAdr,d0 ; Adresse holen
                lea     S_LibName.w,a0
                trap    #TrapLibCtrl
                move.l  d0,S_LibAdr.w

                move.l  4*4.w,a4        ; Exceptionvektoren 4 und 11 retten
                move.l  11*4.w,a5
                move.l  sp,a6           ; SP retten
                move.l  #S_NoCPU,4*4.w  ; neue Exceptionhandler einschreiben
                move.l  #S_NoMMU,11*4.w
                moveq   #is68008,d1     ; Prozessorcode loeschen

                cpu     68030           ; fuer zus. Befehle

                ori     #1,ccr          ; 68008 ausfiltern
                moveq   #is68000,d1

                movec   vbr,d0          ; geht erst ab 68010
                moveq   #is68010,d1

                extb    d0              ; geht erst ab 68020
                moveq   #is68020,d1

                cpu     68000           ; nicht mehr gebraucht
                fpu     on              ; dafuer dies

S_NoCPU:        btst    #0,S_Latch+1.w  ; FPU vorhanden ?
                bne.s   S_NoFPU         ; nein-->
                or.w    #has68881,d1    ; ja : 68881 annehmen
                fnop                    ; FPU idle machen, damit im folgenden
                fsave   -(sp)           ; ein idle frame gespeichert wird
                cmp.b   #$18,1(sp)      ; Framelaenge=$18 fuer 68881, $38 fuer 882
                beq.s   S_NoFPU
                add.w   #(has68882-has68881),d1 ; 68882 eintragen

                fpu     off             ; FPU nicht mehr gebraucht
                pmmu    on              ; dafuer die MMU

S_NoFPU:        move.l  a4,4*4.w        ; Exception 4 zuruecksetzen
                pflusha                 ; dies versteht auch die 68030-MMU
                add.w   #hasMMU,d1      ; gefunden: Flag dazu
                move.l  #S_SmallMMU,11*4.w ; testen ob Schmalspur-MMU
                psave   -(sp)
                bra.s   S_NoMMU         ; Ergebnis 68020/68851

S_SmallMMU:     move.b  #is68030,d1     ; 68030 eintragen (nicht MOVEQ!!)
                add.w   #(intMMU-hasMMU),d1 ; Code interne MMU

S_NoMMU:        move.l  a5,11*4.w       ; Line-F-Vektor zuruecksetzen
                move.l  a6,sp           ; SP restaurieren
                move.w  d1,S_CPUNo.w    ; Ergebnis einschreiben

                trap    #TrapProgEnd

S_LibName:      dc.b    "FLOAT",0

;----------------------------------------------------------------------------
; Gleitkommalibrary, ohne 68881:

                supmode         off

                include         float.inc

;----------------------------------------------------------------------------
; Gleitkommalibrary, mit 68881:

                supmode         off

                include         float81.inc

;----------------------------------------------------------------------------
; Die Startsequenz:

                supmode         on

S_Start:        clr.w           S_Latch.w       ; Ports loeschen
                clr.l           _fadd_cnt.w     ; Zielvariablen loeschen
                clr.l           _fmul_cnt.w
                clr.l           _fdiv_cnt.w
                clr.l           _fsqrt_cnt.w

		move.l          S_MemEnd.w,d0	; SSP an Speicherende legen
		move.l		d0,$0.w		; Neben Resetvekor vermerken
		move.l		d0,a7		; SSP setzen
                move.l          S_FreeMemEnd.w,a0 ; USP liegt am Speicherende
		move		a0,usp

		andi		#$dfff,sr	; In Usermodus schalten
                jmp             Start.w         ; zum Programmstart

;----------------------------------------------------------------------------
; Die Ausnahmebehandlungsprozeduren:

ex_vec2:
		move.w  	#2,S_ExVec.w
		bra.l		ex_handle
ex_vec3:
		move.w  	#3,S_ExVec.w
		bra.l		ex_handle
ex_vec4:
		move.w  	#4,S_ExVec.w
		bra.l		ex_handle
ex_vec5:
		move.w  	#5,S_ExVec.w
		bra.l		ex_handle
ex_vec6:
		move.w  	#6,S_ExVec.w
		bra.l		ex_handle
ex_vec7:
		move.w  	#7,S_ExVec.w
		bra.l		ex_handle
ex_vec8:
		move.w  	#8,S_ExVec.w
		bra.l		ex_handle
ex_vec10:
		move.w  	#10,S_ExVec.w
		bra.l		ex_handle
ex_vec11:
		move.w		#0,S_Control+S_Response.w ; FPU resetten
		move.w  	#11,S_ExVec.w
		bra.l		ex_handle
ex_vec12:
		move.w  	#12,S_ExVec.w
		bra.l		ex_handle
ex_vec13:
		move.w  	#13,S_ExVec.w
		bra.l		ex_handle
ex_vec14:
		move.w  	#14,S_ExVec.w
		bra.l		ex_handle
ex_vec15:
		move.w  	#15,S_ExVec.w
		bra.l		ex_handle
ex_vec16:
		move.w  	#16,S_ExVec.w
		bra.l		ex_handle
ex_vec17:
		move.w  	#17,S_ExVec.w
		bra.l		ex_handle
ex_vec18:
		move.w  	#18,S_ExVec.w
		bra.l		ex_handle
ex_vec19:
		move.w  	#19,S_ExVec.w
		bra.l		ex_handle
ex_vec20:
		move.w  	#20,S_ExVec.w
		bra.l		ex_handle
ex_vec21:
		move.w  	#21,S_ExVec.w
		bra.l		ex_handle
ex_vec22:
		move.w  	#22,S_ExVec.w
		bra.l		ex_handle
ex_vec23:
		move.w  	#23,S_ExVec.w
		bra.l		ex_handle
ex_vec24:
		move.w  	#24,S_ExVec.w
		bra.l		ex_handle
ex_vec25:
		move.w  	#25,S_ExVec.w
		bra.l		ex_handle
ex_vec26:
		move.w  	#26,S_ExVec.w
		bra.l		ex_handle
ex_vec27:
		move.w  	#27,S_ExVec.w
                bra.l           ex_handle
ex_vec28:
		move.w  	#28,S_ExVec.w
                bra.l           ex_handle
ex_vec29:
		move.w  	#29,S_ExVec.w
                bra.l           ex_handle
ex_vec30:
		move.w  	#30,S_ExVec.w
                bra.l           ex_handle
ex_vec31:
		move.w  	#31,S_ExVec.w
                bra.l           ex_handle
ex_vec33:
		move.w  	#33,S_ExVec.w
                bra.l           ex_handle
ex_vec34:
		move.w  	#34,S_ExVec.w
                bra.l           ex_handle
ex_vec35:
		move.w  	#35,S_ExVec.w
                bra.l           ex_handle
ex_vec36:
		move.w  	#36,S_ExVec.w
                bra.l           ex_handle
ex_vec37:
		move.w  	#37,S_ExVec.w
                bra.l           ex_handle
ex_vec38:
		move.w  	#38,S_ExVec.w
                bra.l           ex_handle
ex_vec39:
		move.w  	#39,S_ExVec.w
                bra.l           ex_handle
ex_vec40:
		move.w  	#40,S_ExVec.w
                bra.l           ex_handle
ex_vec41:
		move.w  	#41,S_ExVec.w
                bra.l           ex_handle
ex_vec42:
		move.w  	#42,S_ExVec.w
                bra.l           ex_handle
ex_vec43:
		move.w  	#43,S_ExVec.w
                bra.l           ex_handle
ex_vec44:
                move.w          #44,S_ExVec.w
                bra.l           ex_handle
ex_vec48:
                move.w          #48,S_ExVec.w
                bra.l           ex_handle
ex_vec49:
                move.w          #49,S_ExVec.w
                bra.l           ex_handle
ex_vec50:
                move.w          #50,S_ExVec.w
                bra.l           ex_handle
ex_vec51:
                move.w          #51,S_ExVec.w
                bra.l           ex_handle
ex_vec52:
                move.w          #52,S_ExVec.w
                bra.l           ex_handle
ex_vec53:
                move.w          #53,S_ExVec.w
                bra.l           ex_handle
ex_vec54:
                move.w          #54,S_ExVec.w
                bra.l           ex_handle
ex_vec55:
                move.w          #55,S_ExVec.w
                bra.l           ex_handle
ex_vec56:
                move.w          #56,S_ExVec.w
                bra.l           ex_handle
ex_vec57:
                move.w          #57,S_ExVec.w
                bra.l           ex_handle
ex_vec58:
                move.w          #58,S_ExVec.w

ex_handle:
		movem.l		d0-d7/a0-a7,S_RegSave.w ; Wert der Register abspeichern
		move		usp,a0
		move.l		a0,S_RegSave+64.w
		lea		S_Latch.w,a0
		move.w		S_ExVec.w,d0	; Vektornr. holen
		move.b		d0,1(a0)	; Fehlernr. ausgeben
ex_infinite:
		move.b		d0,d1		; Die LED n-mal blinken lassen						; Die LED n-mal blinken lassen
ex_blink:
		bset		#0,(a0)		; LED an
                bsr.s           ex_wait
		bclr		#0,(a0) 	; LED aus
		bsr.s		ex_wait
		subq.b		#1,d1
		bne.s		ex_blink
		move.b          #$05,d1		; eine Pause einlegen
ex_pause:
		bsr.s		ex_wait
		subq.b		#1,d1
		bne.s		ex_pause
		bra.s		ex_handle	; und alles von vorne

ex_wait:
		move.l		d0,-(a7)   	; Register retten
		move.l		#$50000,d0	; ungefaehr 1/2 Sekunde
ex_wloop:                          		; Register herunterzaehlen
		subq.l		#1,d0
		bne.s		ex_wloop
		move.l		(a7)+,d0	; D0 wieder zurueck
		rts

;----------------------------------------------------------------------------
; Einzelschrittverfolgung:

StepProc:
		clr.b		S_Latch+1.w
		movem.l		d0-d7/a0-a7,S_RegSave.w ; Register retten
		move		usp,a0
		move.l		a0,S_RegSave+64.w
		move.l		$4.w,S_ResVecSave.w ; Resetvektor sichern
		lea		S_Restart(pc),a0    ; am Punkt S_StepBack wacht
		move.l		a0,$4.w		    ; der PcPar wieder auf
		move.b		#9,S_Latch+1.w      ; ParMon-Aufruf ausgeben
                stop            #$2000              ; hier geht es nur mit einem
                                                    ; Reset weiter

;----------------------------------------------------------------------------
; Routinen zur Kommunikation mit dem PC  (Trap 0)

PcSysCall:
		clr.b		S_Latch+1.w
		movem.l		d0-d7/a0-a7,S_RegSave.w ; Register retten
		move		usp,a0
		move.l		a0,S_RegSave+64.w
		move.l		$4.w,S_ResVecSave.w ; Resetvektor sichern
		lea		S_Restart(pc),a0    ; am Punkt S_Restart wacht
		move.l		a0,$4.w		    ; der PcPar wieder auf
		move.b		#$40,S_Latch+1.w    ; PC-Aufruf ausgeben
                stop            #$2000              ; hier geht es nur mit einem

S_Restart:
		clr.b		S_Latch+1.w	    ; Systemanfrage loeschen
		move.l		S_ResVecSave.w,$4.w ; Resetvektor zurueck
		move.l		S_RegSave+64.w,a0
		move		a0,usp
		movem.l		S_RegSave.w,d0-d7/a0-a7 ; Register zurueckholen
		rte				        ; das war's

;----------------------------------------------------------------------------
; Libraryverwaltung :   Trap #13
;
; Struktur einer Library :
;
; Adresse 0:    Laenge in Bytes  (1 <= Laenge <= 256k)
; Adresse 4:    Dummyzeiger =-1 (vom System verwendet)  \
; Adresse 8:    Libraryname als ASCIIZ-String           | kopierter Block
; Adresse n:    Sprungtabelle                           |
; Adresse m:    Librarycode, private Daten              /
;
; der gesamte Librarycode muss lageunabhaengig geschrieben sein !
;
; definierte Unterfunktionen:
;
;     D0.L=0 : Library installieren
;     D0.L=1 : Libraryzeiger holen
;
;----------------------------------------------------------------------------

; Subfunktion 0: Library von Adresse 0 installieren:
; Eingabe: A0=Startadresse der Library
; Ausgabe: keine

S_LibFun:       movem.l d1-d2/a1-a2,-(a7)  ; Register sichern
                tst.l   d0              ; ist es Funktion 0 ?
                bne.s   S_LibFun1       ; nein-->bei Funktion 1 weitertesten

                move.l  (a0),d0         ; Laenge Library holen
                addq.l  #3,d0           ; auf Doppelworte aufrunden
                and.b   #$fc,d0
                moveq   #1,d1
                cmp.l   #$40000,d0      ; Maximalgroesse ueberschritten ?
                bge.l   S_LibErr        ; ja-->Ende mit Fehler Nr.1

                move    usp,a1          ; Userstack holen
                move.l  S_FreeMemEnd.w,d2 ; mom. belegte Stackmenge berechnen
                sub.l   a1,d2
                move.l  a1,a2           ; neue Untergrenze in A2 rechnen
                sub.l   d0,a2
                moveq   #2,d1
                cmp.l   #$800,a2        ; unter abs. Untergrenze gesunken ?
                ble.l   S_LibErr        ; ja-->Ende mit Fehler Nr.2

                move    a2,usp          ; neuen Userstack einschreiben
                lsr.l   #1,d2           ; Stackgroesse in Worten
                bra.s   S_LibStckEnd    ; damit Ende, falls kein Stack belegt

S_LibStckCpy:   move.w  (a1)+,(a2)+     ; Userstack umkopieren
S_LibStckEnd:   dbra    d2,S_LibStckCpy

                move.l  S_FreeMemEnd.w,a1 ; Startadresse der Library rechnen
                sub.l   d0,a1           ; =altes Speicherende-Laenge
                addq.l  #4,a0           ; Quellzeiger weitersetzen
                move.l  S_LibStart.w,d1 ; bisheriges Ende der Kette holen
                move.l  d1,(a0)         ; in neue Library eintragen
                move.l  a1,S_FreeMemEnd.w ; Speichergrenze heruntersetzen
                move.l  a1,S_LibStart.w ; neuen Kettenanfang eintragen

                lsr.l   #2,d0           ; Laenge in Doppelworte umrechnen
                subq.w  #1,d0           ; wg. DBRA

S_LibInstLoop:  move.l  (a0)+,(a1)+     ; Library umkopieren
                dbra    d0,S_LibInstLoop

                bra.l   S_LibOK         ; Ende ohne Fehler

; Subfunktion 1: Library finden, deren Name ab (A0) als ASCIIZ steht:
; Eingabe: A0=Startadresse des ASCIIZ-Strings
; Ausgabe: D0=Startadresse der Sprungtabelle

S_LibFun1:      subq.l  #1,d0           ; ist es Funktion 1 ?
                bne.s   S_LibFun2       ; nein-->bei Funktion 2 weitertesten

                move.l  S_LibStart.w,a2 ; Wurzelzeiger der Kette holen

S_LibGetLoop:   moveq   #3,d1           ; Kettenende erreicht ?
                move.l  a2,d0
                addq.l  #1,d0           ; wird durch -1 angezeigt
                beq.l   S_LibErr        ; ja-->Ende mit Fehler

                move.l  a0,d0           ; Startadresse Vergleichsstring retten
                lea     4(a2),a1        ; A1 zeigt auf zu testenden Namen
S_LibGetComp:   cmpm.b  (a0)+,(a1)+     ; ein Zeichen vergleichen
                bne.s   S_LibGetNext    ; ungleich-->weiter in Kette
                tst.b   -1(a0)          ; War das das Ende ?
                beq.s   S_LibGetFnd     ; ja-->Heureka!
                bra.s   S_LibGetComp    ; ansonsten naechstes Zeichen vergleichen

S_LibGetNext:   move.l  (a2),a2         ; weiter auf Nachfolger in Kette
                move.l  d0,a0           ; A0 auf Referenzstringanfang
                bra.s   S_LibGetLoop

S_LibGetFnd:    move.l  a1,d0           ; Libraryadresse gerade machen
                addq.l  #1,d0
                bclr    #0,d0

                bra.l   S_LibOK         ; Ende ohne Fehler

S_LibFun2:      moveq   #127,d1         ; unbekannte Funktion:
                bra.l   S_LibErr

S_LibErr:       move.l  d1,d0           ; Fehlercode in D0 holen
                movem.l (a7)+,d1-d2/a1-a2  ; Register zurueck
                or.b    #1,1(a7)        ; Carry setzen
                rte

S_LibOK:        movem.l (a7)+,d1-d2/a1-a2  ; Register zurueck
                and.b   #$fe,1(a7)      ; Carry loeschen
                rte

;----------------------------------------------------------------------------
; Tracemode ein/aus:

S_StepTgl:
		andi		#$7fff,sr	; bitte hier kein Trace!
		bclr		#7,(a7)		; altes T-Flag loeschen
		btst		#0,1(a7)
		bne.s		S_StepOn        ; C=1-->Tracemodus an
		rte				; C=0-->fertig
S_StepOn:
		bset		#7,(a7)		; T-Flag setzen
		rte

;----------------------------------------------------------------------------
; Programmende (Trap 15)

S_ProgEnd:      lea		S_Start(pc),a0	; Startvektor zurueck
		move.l		a0,4.w
		move.b		#$ff,S_Latch+1.w ; "Ich bin fertig"
                stop            #$2000          ; und Ende

;----------------------------------------------------------------------------
; Line-F-Exception

S_Response	equ		$fffffe00	; In a6 (Coprozessor-Register)
S_Control	equ		$02		; Alle weiteren Register relativ
S_Save		equ		$04		; zu "Response"
S_Restore	equ		$06
S_Command	equ		$0a		; in a5
S_Condition	equ		$0e
S_Operand	equ		$10		; in a4
S_Reg_Selec	equ		$14
S_Ins_Add	equ		$18

                supmode         on

S_LineF:	btst		#0,S_Latch+1.w	; Ist ein Koprozessor vorhanden ?
		bne		ex_vec11	; nein-->normaler Line-F

		movem.l		d0-d7/a0-a6,S_RegSave.w	; Register retten
		move.l		usp,a0		; USP retten
		move.l		a0,S_RegSave+60.w ; (geht nur ueber Umweg)
		lea		S_Response.w,a6	; #response nach A6
		lea		S_Command(a6),a5 ; #command nach A5
		lea		S_Operand(a6),a6 ; #operand nach A4
		lea		S_RegSave.w,a3	; #dregs nach A3
		move.l		2(a7),a0	; PC nach A0
		move.w		(a0),d1		; Kommando nach D1
S_again:					; Einsprung fuer weitere FPU-Befehle
		and.w		#%0000000111000000,d1 ; Spezialteil ausmaskieren
		bne		S_spezial	; Ein Bit gesetzt-->Spezialbefehl
		move.w		2(a0),d1	; zweiten Befehlsteil in D1 merken
		move.w		d1,(a5)		; Befehl in FPU schr. (A5==#command)
S_do_ca:					; Einsprung fuer weitere Nachfragen an FPU
		move.w		(a6),d0		; Response lesen
		btst		#12,d0		; Erstes Modusbit testen
		bne		S_rw_1x		; ==1 --> springen
		btst		#11,d0		; Zweites Modusbit testen
                beq.s           S_rw_00         ; ==0 --> springen
; ----- %xxx01, Null-Primitive/Transfer Single CPU Register
		btst		#10,d0		; Register uebertragen ?
		bne.s		S_rw_sngl	; Ja--> Transfer Single CPU-Register
		btst		#15,d0		; CA (Come Again) gesetzt ?
		bne.s		S_do_ca		; Ja--> weiter fragen, sonst fertig
		addq.l		#4,a0		; A0 um reine Befehlslaenge weiter
						; ( alles andere wurde in calc_add erledigt )
		move.w		(a0),d1		; erstes Befehlswort holen
		move.w		d1,d0		; und nach D0
		and.w		#$f000,d0	; Wieder COP-Befehl ?
		eor.w		#$f000,d0
		beq.s		S_again		; Ja-->direkt weitermachen
		move.l		a0,2(a7)	; Neuen PC eintragen
		move.l		S_RegSave+60.w,a0 ; USP wiederherstellen
		move.l		a0,usp		; (geht nur ueber Umweg)
		movem.l		(a3),d0-a6	; Register wiederherstellen
		rte				; Trap beenden
S_rw_sngl:
		and.w		#%1110000,d1	; Registernummer ausmaskieren ( nur Dn )
		lsr.w		#2,d1		; D1=Nummer*4
		move.l		0(a3,d1.w),(a4)	; Register uebertragen (a4==#operand, a3==#dregs)
		bra.s		S_do_ca		; danach kommt immer noch etwas
;-----%xxx00, Transfer multiple coprocessor Reg.
S_rw_00:
		bsr		S_calc_add	; Operandenadresse nach A1 holen
		move.w		S_Reg_Selec(a6),d4 ; Registerliste nach D4 holen
		btst		#13,d0		; Dr-Bit testen
		beq.s		S_w_00		; ==0--> Daten in FPU schreiben
		btst		#12,d0		; Predekrementmodus ?
		beq.s		S_r_pred	; ==0--> Ja,springen
		moveq		#7,d0		; Schleifenzaehler fuer 8 Bits
S_11:
		lsl.w		#1,d4		; Ein Bit ins Carry
		bcc.s		S_21		; nur bei Bit==1 etwas machen
		move.l		(a4),(a1)+	; 1 (A4==#operand)
		move.l		(a4),(a1)+	; 2
		move.l		(a4),(a1)+	; 3 Langworte fuer jedes Register
S_21:
		dbra		d0,S_11		; Fuer alle 8 Bits
                bra.s           S_do_ca         ; Nochmal FPU befragen
S_r_Pred:
		moveq		#7,d0           ; Schleifenzaehler fuer 8 Bits
S_12:
		lsl.w		#1,d4		; Ein Bit ins Carry
		bcc.s		S_22		; nur bei Bit=1 etwas machen
		move.l		(a4),(a1)+	; 1 (A4==#operand)
		move.l		(a4),(a1)+	; 2
		move.l		(a4),(a1)+	; 3 Langworte fuer jedes Register
		suba.w		#24,a1		; Dekrement durchfuehren
S_22:
		dbra		d0,S_12		; Fuer alle 8 Bits
		adda.w		#12,a1		; A1 wieder auf letztes Register
		move.l		a1,(a2)		; A1 als Registerinhalt abspeichern
		bra		S_do_ca		; Nochmal FPU befragen
S_w_00:
		move.w		(a0),d0		; erstes Befehlswort holen
		and.b		#%111000,d0	; Adressierungsart maskieren
		cmp.b		#%011000,d0	; Gleich (An)+ ?
		beq.s		S_w_Post	; Ja-->Postinkrementiermodus
		moveq		#7,d0		; Schleifenzaehler fuer 8 Bits
S_13:
		lsl.w		#1,d4		; Ein Bit ins Carry
		bcc.s		S_23		; Nur bei Bit==1 etwas machen
		move.l		(a1)+,(a4)	; 1 (A4==#operand)
		move.l		(a1)+,(a4)	; 2
		move.l		(a1)+,(a4)	; 3 Langworte fuer jedes Register
S_23:
		dbra		d0,S_13		; Fuer alle 8 Bits
		bra		S_do_ca		; Nochmal FPU befragen
S_w_Post:
		suba.w		#12,a1		; Inkrement von calc_add aufheben
		moveq		#7,d0		; Schleifenzaehler fuer 8 Bits
S_14:
		lsl.w		#1,d4		; Ein Bit ins Carry
		bcc.s		S_24		; nur bei Bit==1 etwas machen
		move.l		(a1)+,(a4)	; 1 (A4==#operand)
		move.l		(a1)+,(a4)	; 2
		move.l		(a1)+,(a4)	; 3 Langworte fuer jedes Register
S_24:
		dbra		d0,S_14		; Fuer alle 8 Bits
		move.l		a1,(a2)		; A1 als Registerinhalt abspeichern
		bra		S_do_ca		; Nochmal FPU befragen

S_rw_1x:
		btst		#11,d0		; zweites Modusbit testen
		bne.s		S_rw_11		; ==1 --> springen (Trap,Error)
		btst		#13,d0		; DR-Bit testen
		beq.s		S_w_10		; ==0 --> Daten an FPU schreiben
;----- %xx110, evaluate effective adress and transfer data
		bsr		S_calc_add	; Operandenadresse berechnen
						; A1=Operandenadresse, d1.l=Operandenl„nge
		cmp.w		#2,d1		; Laenge-2
		ble.s		S_r_bw		; <=2 --> Wort-oder-Byteoperand
S_r_11:
		move.l		(a4),(a1)+	; ein Langwort lesen (A4==#operand)
		subq.l		#4,d1		; und runterzaehlen
		bgt.s		S_r_11		; >0 --> weiter uebertragen
		bra		S_do_ca		; Nochmal FPU befragen
S_r_bw:
		btst		#0,d1		; Byte ?
		bne.s		S_r_byte	; Ja!
		move.w		(a4),(a1)	; Wort-Operand lesen (A4==#operand)
		bra		S_do_ca		; Nochmal FPU befragen
S_r_byte:
		move.b		(a4),(a1)	; Byte-Operand lesen (A4==#operand)
                bra.l           S_do_ca         ; Nochmal FPU befragen

;----- %xx101, evaluate effective adress and transfer data
S_w_10:
		bsr		S_calc_add	; Operandenadresse berechnen
						; A1=Operandenadresse, d1.l=Operandenl„nge
		cmp.w		#2,d1		; Laenge-2
		ble.s		S_w_bw		; <=2 --> Wort-oder-Byteoperand
S_w_11:
		move.l		(a1)+,(a4)	; ein Langwort lesen (A4==#operand)
		subq.l		#4,d1		; und runterzaehlen
		bgt.s		S_w_11		; >0 --> weiter uebertragen
		bra		S_do_ca		; Nochmal FPU befragen
S_w_bw:
		btst		#0,d1		; Byte ?
		bne.s		S_w_byte	; Ja!
		move.w		(a1),(a4)	; Wort-Operand lesen (A4==#operand)
		bra		S_do_ca		; Nochmal FPU befragen
S_w_byte:
		move.b		(a1),(a4)	; Byte-Operand lesen (A4==#operand)
                bra.l           S_do_ca         ; Nochmal FPU befragen

;----- %xxx11, take pre-instruction exception
S_rw_11:
		bra		ex_vec11	; Error-Handler anspringen
;		( hier koennte man eine genauere Fehleranalyse machen )

S_spezial:                                      ; Sprungbefehle etc.
		cmp.w		#%001000000,d1	; FScc,FDBcc oder FTRAPcc
		beq.s		S_s_trap
		cmp.w		#%010000000,d1	; Branch mit 16-Bit-Offset
                beq.l           S_s_br16
		cmp.w		#%011000000,d1	; Branch mit 32-Bit-Offset
                beq.l           S_s_br32
		bra		ex_vec11	; FSAVE/FRESTORE nicht unterstuetzt
S_s_trap:
		move.w		(a0),d0		; Erstes Befehlswort nach D0
		move.w		d0,d1		; und nach D1 retten
		and.w		#%111000,d0	; Wichtige Bits ausmaskieren
		cmp.w		#%001000,d0	; FDBcc ?
		beq.s		S_s_fdbcc	; Ja-->springen
		cmp.w		#%111000,d0	; FTRAP ?
		beq		ex_vec11	; Ja-->Fehler (nicht unterstuetzt)
						; sonst FScc
		move.w		2(a0),S_condition(a6) ; Bedingung an FPU schicken
		moveq		#1,d0		; Operandenlaenge=1 (fuer calc_add)
		bsr		S_calc_add	; Operandenadresse berechnen
S_15:
		move.w		(a6),d0		; Response lesen
		btst		#8,d0		; IA-Bit testen
		beq.s		S_25		; ==0 --> fertig
		and.w		#%1100000000000,d0 ; Bits 11 und 12 ausmaskieren
		eor.w		#%1100000000000,d0 ; Beide gesetzt ?
		bne.s		S_15		; Nicht beide==1 --> warten
		bra		ex_vec11	; Sonst ist Exception aufgetreten
S_25:
		btst		#0,d0		; Antwortbit testen
		sne		(a1)		; Je nach Bit setzen/loeschen
		bra		S_do_ca		; Nochmal FPU befragen
S_s_fdbcc:
		move.w		2(a0),S_condition(a6) ; Bedingung an FPU schicken
		and.w		#%111,d1	; Registernummer maskieren (D1=(A0))
		lsl.w		#2,d1		; D1=Nummer*4
		lea		0(a3,d1.w),a1	; A1 enthaelt Adresse des Datenreg.
		move.l		(a1),d1		; Dn holen
		subq.w		#1,d1		; Dn=Dn-1
		move.l		d1,(a1)		; Dn zurueckschreiben
		move.l		a0,a2           ; alten PC nach A2 holen
		addq.l		#2,a0		; PC 2 weiter ( fuer "nicht springen")
S_16:
		move.w		(a6),d0		; Response lesen
		btst		#8,d0		; IA-Bit testen
		beq.s		S_26		; ==0 --> fertig
		and.w		#%1100000000000,d0 ; Bits 11 und 12 ausmaskieren
		eor.w		#%1100000000000,d0 ; Beide gesetzt ?
		bne.s		S_16		; Nicht beide==1 --> warten
		bra		ex_vec11	; Sonst ist Exception aufgetreten
S_26:
		btst		#0,d0		; Antwortbit testen
		bne		S_do_ca		; True-->das war's schon
		adda.w		2(a2),a2	; 16-Bit-Sprungdist. add. (A2=PC)
		addq.w		#1,d1		; Dn=-1 ?
		beq		S_do_ca		; Ja-->kein Sprung (Schleifenende)
		move.l		a2,a0		; Sonst "Sprung" (neuen PC laden)
		bra		S_do_ca		; nochmal FPU befragen
S_s_br16:
		move.w		(a0),S_Condition(a6) ; Bedingung an FPU schicken
S_17:
		move.w		(a6),d0		; Response lesen
		btst		#8,d0		; IA-Bit testen
		beq.s		S_27		; ==0 --> fertig
		and.w		#%1100000000000,d0 ; Bits 11 und 12 ausmaskieren
		eor.w		#%1100000000000,d0 ; Beide gesetzt ?
		bne.s		S_17		; Nicht beide==1 --> warten
		bra		ex_vec11	; Sonst ist Exception aufgetreten
S_27:
		btst		#0,d0		; Antwortbit testen
		beq		S_do_ca		; False--> das war's schon
		adda.w		2(a0),a0	; 16-Bit-Sprungdistanz addieren
		subq.l		#2,a0		; Ein Wort zurueck ( weil spaeter
		; noch 4 addiert wird und und nur 2 addiert werden muesste )
		bra		S_do_ca		; Nochmal FPU befragen
S_s_br32:
		move.w		(a0),S_Condition(a6) ; Bedingung an FPU schicken
S_18:
		move.w		(a6),d0		; Response lesen
		btst		#8,d0		; IA-Bit testen
		beq.s		S_28	; ==0 --> fertig
		and.w		#%1100000000000,d0 ; Bits 11 und 12 ausmaskieren
		eor.w		#%1100000000000,d0 ; Beide gesetzt ?
		bne.s		S_18	; Nicht beide==1 --> warten
		bra		ex_vec11	; Sonst ist Exception aufgetreten
S_28:
		addq.l          #2,a0		; Befehl ist 3 Worte lang
						; (jetzt : (A0)=Distanz)
		btst		#0,d0		; Antwortbit testen
		beq		S_do_ca		; False--> das war's schon
		adda.l		(a0),a0		; 32-Bit-Sprungdistanz addieren
		subq.l		#4,a0		; Zwei Worte zurueck ( weil spaeter
		; noch 4 addiert wird, 2 wurden schon addiert )
		bra		S_do_ca		; Nochmal FPU befragen
S_calc_add:
		; Operandenadresse berechnen. A0 muss die Adresse des Line-F-
		; Befehls enthalten, D0 im unteren Byte die Operandenlaenge.
		; die zu berechnende Adresse wird in A1 abgelegt. A0 wird
		; um die Laenge der zusaetzlichen Daten erhaelt.
		; Zusaetzlich wird in D1 die Laenge des Operanden zurueckge-
		; geben (in Bytes, als Langwort). D2,D3,A3 werden zerstoert.
		; Bei den Adressierungsarten -(An),(An)+ steht in A2 ein
		; Zeiger auf die Stelle, in der der Inhalt des Adressregisters
		; gisters An steht (wird fuer FMOVEM gebraucht).

		clr.l		d1		; Laenge als Langwort loeschen
		move.b		d0,d1		; und Byte umkopieren
		move.w		(a0),d2		; erstes Befehlswort nach D2
		move.w		d2,d3		; und D3 retten
		and.w		#%111000,d3	; Adressierungsart ausmaskieren
		lsr.w		#1,d3		; D3=Adressierungsart*4 (Langworte)
                lea             S_cs_tab(pc),a1 ; Sprungtabellenadresse nach A1
		move.l		0(a1,d3.w),a1	; Adresse der Routine nach A1
		jmp		(a1)		; und Routine anspringen
S_c_drd:	; %000  Data Register Direct:			    Dn
S_c_ard:	; %001  Address Register Direct:        	    An
		lea		(a3),a1		; A1 auf Registerfeld
		and.w		#%1111,d2	; Registernummer ausmaskieren
;		( und ein Bit vom Modus, 0 fuer Daten-,1 fuer Adressregister )
		lsl.w		#2,d2		; D2="Registernummer"*4 (+Modusbit)
		addq.w		#4,d2		; +4 (fuer Operandenlaenge)
		sub.w		d1,d2		; Wahre Laenge abziehen
		adda.w		d2,a1		; Offset auf Registerfeldanfang add.
		rts
S_c_ari:	; %010  Address Register indirect:		    (An)
		and.w		#%111,d2	; Registernummer ausmaskieren
		lsl.w		#2,d2		; D2=Registernummer*4
		move.l		32(a3,d2.w),a1	; Adresse nach A1
		rts
S_c_arpo:	; %011	Adressregister indirect with Postincrement: (An)+
		and.w		#%111,d2	; Registernummer ausmaskieren
		lsl.w		#2,d2		; D2=Registernummer*4
		lea		32(a3,d2.w),a2	; Adresse Adressregister nach A2
		move.l		(a2),a1		; Adresse (Inhalt A.-Reg.) nach A1
		btst		#0,d1		; D1 ungerade ? (Byteoperand)
		bne.s		S_29		; Ja-->Spezialbehandlung
S_19:
		add.l		d1,(a2)		; Inkrement durchfuehren
		rts
S_29:
		cmp.w		#4*7,d2		; Ist A7 gemeint ?
		bne.s		S_19		; nein-->normal vorgehen
		addq.l		#2,(a2)		; Sonst (bei Byte) 2 addieren,
		rts				; damit Stack gerade bleibt!
S_c_arpr:	; %100  Adressregister Indirect with Predekrement:  -(An)
		and.w		#%111,d2	; Registernummer ausmaskieren
		lsl.w		#2,d2		; D2=Registernummer*4
		lea		32(a3,d2.w),a2	; Adresse des Adressreg. nach A2
		btst		#0,d1		; D1 ungerade? (Byteoperand)
		bne.s		S_210		; Ja-->Spezialbehandlung
S_110:
		sub.l		d1,(a2)		; Dekrement durchfuehren
		move.l		(a2),a1		; Adresse (Inhalt des A.-Reg) nach A1
		rts
S_210:
		cmp.w		#4*7,d2		; Ist A7 gemeint?
		bne.s		S_110		; nein-->normal vorgehen
		subq.l		#2,(a2)		; Sonst (bei Byte) 2 addieren,
						; damit Stack gerade bleibt !
		move.l		(a2),a1		; Adresse (Inhalt des A.-Reg) nach A1
		rts
S_c_ar16:	; %101  Addressregister Indirect with Displacement: d16(An)
		and.w		#%111,d2	; Registernummer ausmaskieren
		lsl.w		#2,d2		; D2=Registernummer*4
		move.l		32(a3,d2.w),a1	; Adresse nach A1
		move.w		4(a0),d2	; 3.Befehlswort nach D2 (Offset)
		adda.w		d2,a1		; Offset auf Adresse addieren
		addq.l		#2,a0		; A0 ein Wort (d16) weiter
		rts
S_c_ar08:	; %110  Addressregister Indirect with Index : 	    d8(An,Xn)
		and.w		#%111,d2	; Registernummer ausmaskieren
		lsl.w		#2,d2		; D2=Registernummer*4
		move.l		32(a3,d2.w),a1	; Adresse nach A1
		move.w		4(a0),d2	; 3.Befehlswort nach D2 (Byte-Offset)
		move.w		d2,d3		; und nach D3
		and.w		#$ff,d3		; Byte ausmaskieren (Byte-Offset)
		adda.w		d3,a1		; Offset auf Adresse addieren
		btst		#11,d2		; 1=long; 0=word
		bne.s		S_c_ar81
		and.w		#%1111000000000000,d2 ; Nummer von Dn und Modusbit
		lsr.w		#5,d2		; maskieren
		lsr.w		#5,d2		; D2=Registernummer*4 (und modusbit)
		adda.w		2(a3,d2.w),a1	; 16-Bit-Index auf A1 addieren
		addq.l		#2,a0		; A0 ein Wort (Kram & d8) weiter
		rts
S_c_ar81:
		and.w		#%1111000000000000,d2 ; Nummer von Dn und Modusbit
		lsr.w		#5,d2		; maskieren
		lsr.w		#5,d2		; D2=Registernummer*4 (und modusbit)
		adda.w		0(a3,d2.w),a1	; 32-Bit-Index auf A1 addieren
		addq.l		#2,a0		; A0 ein Wort (Kram & d8) weiter
		rts
S_c_pc:		; %111  absolut short/long, PC-relativ (ohne/mit Index) \ oder direkt
		btst		#2,d2		; Immidiate ?
		bne.s		S_immi		; <>0 --> Ja!
		btst		#1,d2		; PC-relativ ?
		bne.s		S_pc_rel	; <>0 --> Ja!
		btst		#0,d2		; Long ?
		bne.s		S_c_long	; <>0 --> Ja!
						; sonst short
		move.w		4(a0),d2	; Wortadresse holen
		ext.l		d2		; Auf Langwort erweitern
		move.l		d2,a1		; und als Operandenadresse merken
		addq.l		#2,a0		; A0 ein Wort (Short-A.) weiter
		rts
S_c_long:
		move.l		4(a0),a1	; Langwortadresse holen
		addq.l		#4,a0		; A0 zwei Worte (Long-A.) weiter
		rts
S_immi:
		move.l		a0,a1		; Befehlsadresse nach A1
		add.l		d1,a0		; A0 ueber Operand hinwegsetzen
		rts
S_pc_rel:
		btst		#0,d2		; mit Index ?
		bne.s		S_pc_idx	; <>0 --> Ja!
		move.l		a0,a1		; PC nach A1
		adda.w		4(a0),a1	; Offset addieren
		addq.l		#4,a1		; +4 fuer Laenge des FPU-Befehls
		addq.l		#2,a0		; A0 zwei (16-Bit-Offset) weiter
		rts
S_pc_idx:
		move.l		a0,a1		; PC nach A1
		clr.w		d2		; Oberes Byte loeschen
		move.b		5(a0),d2	; Offset nach D2
		adda.w		d2,a1		; und addieren
		addq.l		#4,a1		; +4 fuer Laenge des FPU-Befehls
		move.b		4(a0),d2	; D2=Registernummer*16 und Modusbit
						; ( high-Byte ist noch 0 )
		btst		#3,d2		; Long-Bit testen
		bne.s		S_pc_i_l	; <>0 -->Long-Index
		and.b		#%11110000,d2	; Registerinformation ausblenden
		lsr.w		#2,d2		; D2=Registernummer*4 (und Modusbit)
		adda.w		2(a3,d2.w),a1	; Word-Index addieren
		addq.l		#2,a0		; A0 zwei (8-Bit-Offset & Kram) weiter
		rts
S_pc_i_l:
		and.b		#%11110000,d2	; Restinformation ausblenden
		lsr.w		#2,d2		; D2=Registernummer*4 (und Modusbit)
		adda.l		0(a3,d2.w),a1	; Long-Index addieren
		addq.l		#2,a0		; A0 zwei (8-Bit-Offset & Kram) weiter
		rts				; Ende von S_calc_add

S_cs_tab:
		dc.l		S_c_drd,S_c_ard,S_c_ari,S_c_arpo ; Sprungtabelle fuer
		dc.l		S_c_arpr,S_c_ar16,S_c_ar08,S_c_pc ; Adressierungsarten
S_End:

