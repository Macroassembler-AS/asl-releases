;*                    BUFFALO
;* "Bit User's Fast Friendly Aid to Logical Operation"
;*
;* Rev 2.0 - 4/23/85 - added disassembler.
;*                     - variables now PTRn and TMPn.
;* Rev 2.1 - 4/29/85 - added byte erase to chgbyt routine.
;* Rev 2.2 - 5/16/85 - added hooks for evb board - acia
;*                       drivers, init and host routines.
;*            7/8/85  - fixed dump wraparound problem.
;*            7/10/85 - added evm board commands.
;*                     - added fill instruction.
;*            7/18/85 - added jump to EEPROM.
;* Rev 2.3 - 8/22/85 - call targco to disconnect sci from host
;*                       in reset routine for evb board.
;*            10/3/85 - modified load for download through terminal.
;* Rev 2.4 - 7/1/86  - Changed DFLOP address to fix conflicts with
;*                       EEPROM.  (was at A000)
;* Rev 2.5 - 9/8/86  - Modified to provide additional protection from
;*                       program run-away on power down.  Also fixed bugs
;*                       in MM and MOVE.  Changed to 1 stop bit from 2.
;* Rev 2.6 - 9/25/86 - Modified boot routine for variable length download
;*                       for use with 'HC11E8.
;* Rev 3.0   1/15/87 - EEPROM programming routines consolidated into WRITE.
;*                       Fill, Assem, and breakpoints will now do EEPROM.
;*                     - Added compare a to $0D to WSKIP routine.
;*            2/11/87 - Set up load to detect receiver error.
;* Rev 3.2   7/7/87  - Add disassembly to trace.
;*                     - Add entries to jump table.
;*            9/20/87 - Rewrote trace to use XIRQ, added STOPAT Command
;*            11/24/87- Write block protect reg for 'E9 version
;*                     - Modified variable length download for use
;*                          with 'E9 bootloader (XBOOT command)
;*
;*
;****************************************************
;*    Although the information contained herein,    *
;*    as well as any information provided relative  *
;*    thereto, has been carefully reviewed and is   *
;*    believed accurate, Motorola assumes no     *
;*    liability arising out of its application or   *
;*    use, neither does it convey any license under *
;*    its patent rights nor the rights of others.   *
;****************************************************

         CPU  6811

;***************
;*   EQUATES   *
;***************
RAMBS    EQU  $0000             ; start of ram
REGBS    EQU  $1000             ; start of registers
ROMBS    EQU  $E000             ; start of rom
STREE    EQU  $B600             ; start of eeprom
ENDEE    EQU  $B7FF             ; end of eeprom
PORTE    EQU  REGBS+$0A         ; port e
CFORC    EQU  REGBS+$0B         ; force output compare
TCNT     EQU  REGBS+$0E         ; timer count
TOC5     EQU  REGBS+$1E         ; oc5 reg
TCTL1    EQU  REGBS+$20         ; timer control 1
TMSK1    EQU  REGBS+$22         ; timer mask 1
TFLG1    EQU  REGBS+$23         ; timer flag 1
TMSK2    EQU  REGBS+$24         ; timer mask 2
BAUD     EQU  REGBS+$2B         ; sci baud reg
SCCR1    EQU  REGBS+$2C         ; sci control1 reg
SCCR2    EQU  REGBS+$2D         ; sci control2 reg
SCSR     EQU  REGBS+$2E         ; sci status reg
SCDAT    EQU  REGBS+$2F         ; sci data reg
BPROT    EQU  REGBS+$35         ; block protect reg
OPTION   EQU  REGBS+$39         ; option reg
COPRST   EQU  REGBS+$3A         ; cop reset reg
PPROG    EQU  REGBS+$3B         ; ee prog reg
HPRIO    EQU  REGBS+$3C         ; hprio reg
CONFIG   EQU  REGBS+$3F         ; config register
DFLOP    EQU  $4000             ; evb d flip flop
DUART    EQU  $D000             ; duart address
PORTA    EQU  DUART
PORTB    EQU  DUART+8
ACIA     EQU  $9800             ; acia address
PROMPT   EQU  '>'
BUFFLNG  EQU  35
CTLA     EQU  $01               ; exit host or assembler
CTLB     EQU  $02               ; send break to host
CTLW     EQU  $17               ; wait
CTLX     EQU  $18               ; abort
DEL      EQU  $7F               ; abort
EOT      EQU  $04               ; end of text/table
SWI      EQU  $3F

;***************
;*     RAM     *
;***************
         ORG  $33
;*** Buffalo ram space ***
         RMB  20                ; user stack area
USTACK   RMB  30                ; monitor stack area
STACK    RMB  1
REGS     RMB  9                 ; user's pc,y,x,a,b,c
SP       RMB  2                 ; user's sp
INBUFF   RMB  BUFFLNG           ; input buffer
ENDBUFF  EQU  *
COMBUFF  RMB  8                 ; command buffer
SHFTREG  RMB  2                 ; input shift register
BRKTABL  RMB  8                 ; breakpoint table
AUTOLF   RMB  1                 ; auto lf flag for i/o
IODEV    RMB  1                 ; 0=sci,  1=acia, 2=duartA, 3=duartB
EXTDEV   RMB  1                 ; 0=none, 1=acia, 2=duart,
HOSTDEV  RMB  1                 ; 0=sci,  1=acia,              3=duartB
COUNT    RMB  1                 ; # characters read
CHRCNT   RMB  1                 ; # characters output on current line
PTRMEM   RMB  2                 ; current memory location

;*** Buffalo variables - used by: ***
PTR0     RMB  2                 ; main,readbuff,incbuff,AS
PTR1     RMB  2                 ; main,BR,DU,MO,AS,EX
PTR2     RMB  2                 ; EX,DU,MO,AS
PTR3     RMB  2                 ; EX,HO,MO,AS
PTR4     RMB  2                 ; EX,AS
PTR5     RMB  2                 ; EX,AS,BOOT
PTR6     RMB  2                 ; EX,AS,BOOT
PTR7     RMB  2                 ; EX,AS
PTR8     RMB  2                 ; AS
TMP1     RMB  1                 ; main,hexbin,buffarg,termarg
TMP2     RMB  1                 ; GO,HO,AS,LOAD
TMP3     RMB  1                 ; AS,LOAD
TMP4     RMB  1                 ; TR,HO,ME,AS,LOAD
;*** Vector jump table ***
JSCI     RMB   3
JSPI     RMB   3
JPAIE    RMB   3
JPAO     RMB   3
JTOF     RMB   3
JTOC5    RMB   3
JTOC4    RMB   3
JTOC3    RMB   3
JTOC2    RMB   3
JTOC1    RMB   3
JTIC3    RMB   3
JTIC2    RMB   3
JTIC1    RMB   3
JRTI     RMB   3
JIRQ     RMB   3
JXIRQ    RMB   3
JSWI     RMB   3
JILLOP   RMB   3
JCOP     RMB   3
JCLM     RMB   3

;*****************
;*
;* ROM starts here *
;*
;*****************

        ORG  ROMBS

;*****************
;**  BUFFALO - This is where Buffalo starts
;** out of reset.  All initialization is done
;** here including determination of where the
;** user terminal is (SCI,ACIA, or DUART).
;*****************

BUFFALO  LDX  #PORTE
         BRCLR 0,X,#01,BUFISIT  ; if bit 0 of port e is 1
         JMP  $B600             ; then jump to the start of EEPROM
BUFISIT  LDAA #$93
         STAA OPTION            ; adpu, dly, irqe, cop
         LDAA #$00
         STAA TMSK2             ; timer pre = %1 for trace
         LDAA #$00
         STAA BPROT             ; clear 'E9 eeprom block protect
         LDS  #STACK            ; monitor stack pointer
         JSR  VECINIT
         LDX  #USTACK
         STX  SP                ; default user stack
         LDAA TCTL1
         ORAA #$03
         STAA TCTL1             ; force oc5 pin high for trace
         LDAA #$D0
         STAA REGS+8            ; default user ccr
         LDD  #$3F0D            ; initial command is ?
         STD  INBUFF
         JSR  BPCLR             ; clear breakpoints
         CLR  AUTOLF
         INC  AUTOLF            ; auto cr/lf = on

;* Determine type of external comm device - none, or acia *

         CLR  EXTDEV            ; default is none
         LDAA HPRIO
         ANDA #$20
         BEQ  BUFF2             ; jump if single chip mode
         LDAA #$03              ; see if external acia exists
         STAA ACIA              ; master reset
         LDAA ACIA
         ANDA #$7F              ; mask irq bit from status register
         BNE  BUFF1             ; jump if status reg not 0
         LDAA #$12
         STAA ACIA              ; turn on acia
         LDAA ACIA
         ANDA #$02
         BEQ  BUFF1             ; jump if tdre not set
         LDAA #$01
         STAA EXTDEV            ; external device is acia
         BRA  BUFF2

BUFF1    EQU  *                 ; see if duart exists
         LDAA DUART+$0C        ; read IRQ vector register
         CMPA #$0F             ; should be out of reset
         BNE  BUFF2
         LDAA #$AA
         STAA DUART+$0C         ; write irq vector register
         LDAA DUART+$0C         ; read irq vector register
         CMPA #$AA
         BNE  BUFF2
         LDAA #$02
         STAA EXTDEV            ; external device is duart A

;* Find terminal port - SCI or external. *

BUFF2    CLR  IODEV
         JSR  TARGCO            ; disconnect sci for evb board
         JSR  SIGNON            ; initialize sci
         LDAA EXTDEV
         BEQ  BUFF3             ; jump if no external device
         STAA IODEV
         JSR  SIGNON            ; initialize external device
BUFF3    CLR  IODEV
         JSR  INPUT             ; get input from sci port
         CMPA #$0D
         BEQ  BUFF4             ; jump if cr - sci is terminal port
         LDAA EXTDEV
         BEQ  BUFF3             ; jump if no external device
         STAA IODEV
         JSR  INPUT             ; get input from external device
         CMPA #$0D
         BEQ  BUFF4             ; jump if cr - terminal found ext
         BRA  BUFF3

SIGNON   JSR  INIT              ; initialize device
         LDX  #MSG1             ; buffalo message
         JSR  OUTSTRG
         RTS

;* Determine where host port should be. *

BUFF4    CLR  HOSTDEV           ; default - host = sci port
         LDAA IODEV
         CMPA #$01
         BEQ  BUFF5             ; default host if term = acia
         LDAA #$03
         STAA HOSTDEV           ;  else host is duart port b
BUFF5    EQU  *

;*****************
;**  MAIN - This module reads the user's input into
;** a buffer called INBUFF.  The first field (assumed
;** to be the command field) is then parsed into a
;** second buffer called COMBUFF.  The command table
;** is then searched for the contents of COMBUFF and
;** if found, the address of the corresponding task
;** routine is fetched from the command table.  The
;** task is then called as a subroutine so that
;** control returns back to here upon completion of
;** the task.  Buffalo expects the following format
;** for commands:
;**     <cmd>[<wsp><arg><wsp><arg>...]<cr>
;** [] implies contents optional.
;** <wsp> means whitespace character (space,comma,tab).
;** <cmd> = command string of 1-8 characters.
;** <arg> = Argument particular to the command.
;** <cr> = Carriage return signifying end of input string.
;*****************
;* Prompt user
;*do
;*   a=input();
;*   if(a==(cntlx or del)) continue;
;*   elseif(a==backspace)
;*      b--;
;*      if(b<0) b=0;
;*   else
;*      if(a==cr && buffer empty)
;*          repeat last command;
;*      else put a into buffer;
;*          check if buffer full;
;*while(a != (cr or /)

MAIN     LDS  #STACK            ; initialize sp every time
         CLR  AUTOLF
         INC  AUTOLF            ; auto cr/lf = on
         JSR  OUTCRLF
         LDAA #PROMPT           ; prompt user
         JSR  OUTPUT
         CLRB
MAIN1    JSR  INCHAR            ; read terminal
         LDX  #INBUFF
         ABX                    ; pointer into buffer
         CMPA #CTLX
         BEQ  MAIN              ; jump if cntl X
         CMPA #DEL
         BEQ  MAIN              ; jump if del
         CMPA #$08
         BNE  MAIN2             ; jump if not bckspc
         DECB
         BLT  MAIN              ; jump if buffer empty
         BRA  MAIN1
MAIN2    CMPA #$D
         BNE  MAIN3             ; jump if not cr
         TSTB
         BEQ  COMM0             ; jump if buffer empty
         STAA ,X                ; put a in buffer
         BRA  COMM0
MAIN3    STAA ,X                ; put a in buffer
         INCB
         CMPB #BUFFLNG
         BLE  MAIN4             ; jump if not long
         LDX  #MSG3             ; "long"
         JSR  OUTSTRG
         BRA  MAIN
MAIN4     CMPA #'/'
         BNE  MAIN1             ; jump if not "/"
;*         *******************

;*****************
;*  Parse out and evaluate the command field.
;*****************
;*Initialize

COMM0    EQU  *
         CLR  TMP1              ; Enable "/" command
         CLR  SHFTREG
         CLR  SHFTREG+1
         CLRB
         LDX  #INBUFF           ; ptrbuff[] = inbuff[]
         STX  PTR0
         JSR  WSKIP             ; find first char

;*while((a=readbuff) != (cr or wspace))
;*     upcase(a);
;*     buffptr[b] = a
;*     b++
;*     if (b > 8) error(too long);
;*     if(a == "/")
;*           if(enabled) mslash();
;*           else error(command?);
;*     else hexbin(a);

COMM1    EQU  *
         JSR  READBUFF          ; read from buffer
         LDX  #COMBUFF
         ABX
         JSR  UPCASE            ; convert to upper case
         STAA ,X                ; put in command buffer
         CMPA #$0D
         BEQ  SRCH              ; jump if cr
         JSR  WCHEK
         BEQ  SRCH              ; jump if wspac
         JSR  INCBUFF           ; move buffer pointer
         INCB
         CMPB #$8
         BLE  COMM2
         LDX  #MSG3             ; "long"
         JSR  OUTSTRG
         JMP  MAIN

COMM2    EQU  *
         CMPA #'/'
         BNE  COMM4             ; jump if not "/"
         TST  TMP1
         BNE  COMM3             ; jump if not enabled
         DECB
         STAB COUNT
         LDX  #MSLASH
         JMP  EXEC              ; execute "/"
COMM3    LDX  #MSG8             ; "command?"
         JSR  OUTSTRG
         JMP  MAIN
COMM4    EQU  *
         JSR  HEXBIN
         BRA  COMM1

;*****************
;*   Search tables for command.      At this point,
;* COMBUFF holds the command field to be executed,
;* and B = # of characters in the command field.
;* The command table holds the whole command name
;* but only the first n characters of the command
;* must match what is in COMBUFF where n is the
;* number of characters entered by the user.
;*****************
;*count = b;
;*ptr1 = comtabl;
;*while(ptr1[0] != end of table)
;*   ptr1 = next entry
;*   for(b=1; b=count; b++)
;*      if(ptr1[b] == combuff[b]) continue;
;*      else error(not found);
;*   execute task;
;*  return();
;*return(command not found);

SRCH    STAB COUNT              ; size of command entered
        LDX  #COMTABL           ; pointer to table
        STX  PTR1               ; pointer to next entry
SRCH1   LDX  PTR1
        LDY  #COMBUFF           ; pointer to command buffer
        LDAB 0,X
        CMPB #$FF
        BNE  SRCH2
        LDX  #MSG2              ; "command not found"
        JSR  OUTSTRG
        JMP  MAIN
SRCH2   PSHX                    ; compute next table entry
        ADDB #$3
        ABX
        STX  PTR1
        PULX
        CLRB
SRCHLP  INCB                    ; match characters loop
        LDAA 1,X                ; read table
        CMPA 0,Y                ; compare to combuff
        BNE  SRCH1              ; try next entry
        INX                     ; move pointers
        INY
        CMPB COUNT
        BLT  SRCHLP             ; loop countu1 times
        LDX  PTR1
        DEX
        DEX
        LDX  0,X                ; jump address from table
EXEC    JSR  0,X                ; call task as subroutine
        JMP  MAIN
;*
;*****************
;*   UTILITY SUBROUTINES - These routines
;* are called by any of the task routines.
;*****************
;*****************
;*  UPCASE(a) - If the contents of A is alpha,
;* returns a converted to uppercase.
;*****************
UPCASE   CMPA #'a'
         BLT  UPCASE1           ; jump if < a
         CMPA #'z'
         BGT  UPCASE1           ; jump if > z
         SUBA #$20              ; convert
UPCASE1  RTS

;*****************
;*  BPCLR() - Clear all entries in the
;* table of breakpoints.
;*****************
BPCLR    LDX  #BRKTABL
         LDAB #8
BPCLR1   CLR  0,X
         INX
         DECB
         BGT  BPCLR1            ; loop 8 times
         RTS

;*****************
;*  RPRNT1(x) - Prints name and contents of a single
;* user register. On entry X points to name of register
;* in reglist.  On exit, a=register name.
;*****************
REGLIST  FCC  "PYXABCS"         ; names
         FCB  0,2,4,6,7,8,9     ; offset
         FCB  1,1,1,0,0,0,1     ; size
RPRNT1   LDAA 0,X
         PSHA
         PSHX
         JSR  OUTPUT            ; name
         LDAA #'-'
         JSR  OUTPUT            ; dash
         LDAB 7,X               ; contents offset
         LDAA 14,X              ; bytesize
         LDX  #REGS             ; address
         ABX
         TSTA
         BEQ  RPRN2             ; jump if 1 byte
         JSR  OUT1BYT           ; 2 bytes
RPRN2    JSR  OUT1BSP
         PULX
         PULA
         RTS

;*****************
;*  RPRINT() - Print the name and contents
;* of all the user registers.
;*****************
RPRINT   PSHX
         LDX  #REGLIST
RPRI1    JSR  RPRNT1            ; print name
         INX
         CMPA #'S'              ; s is last register
         BNE  RPRI1             ; jump if not done
         PULX
         RTS

;*****************
;*   HEXBIN(a) - Convert the ASCII character in a
;* to binary and shift into shftreg.  Returns value
;* in tmp1 incremented if a is not hex.
;*****************
HEXBIN  PSHA
        PSHB
        PSHX
        JSR  UPCASE             ; convert to upper case
        CMPA #'0'
        BLT  HEXNOT             ; jump if a < $30
        CMPA #'9'
        BLE  HEXNMB             ; jump if 0-9
        CMPA #'A'
        BLT  HEXNOT             ; jump if $39> a <$41
        CMPA #'F'
        BGT  HEXNOT             ; jump if a > $46
        ADDA #$9                ; convert $A-$F
HEXNMB  ANDA #$0F               ; convert to binary
        LDX  #SHFTREG
        LDAB #4
HEXSHFT ASL  1,X                ; 2 byte shift through
        ROL  0,X                ; carry bit
        DECB
        BGT  HEXSHFT            ; shift 4 times
        ORAA 1,X
        STAA 1,X
        BRA  HEXRTS
HEXNOT  INC  TMP1               ; indicate not hex
HEXRTS  PULX
        PULB
        PULA
        RTS

;*****************
;*  BUFFARG() - Build a hex argument from the
;* contents of the input buffer. Characters are
;* converted to binary and shifted into shftreg
;* until a non-hex character is found.  On exit
;* shftreg holds the last four digits read, count
;* holds the number of digits read, ptrbuff points
;* to the first non-hex character read, and A holds
;* that first non-hex character.
;*****************
;*Initialize
;*while((a=readbuff()) not hex)
;*     hexbin(a);
;*return();

BUFFARG  CLR  TMP1              ; not hex indicator
         CLR  COUNT             ; # or digits
         CLR  SHFTREG
         CLR  SHFTREG+1
         JSR  WSKIP
BUFFLP   JSR  READBUFF          ; read char
         JSR  HEXBIN
         TST  TMP1
         BNE  BUFFRTS           ; jump if not hex
         INC  COUNT
         JSR  INCBUFF           ; move buffer pointer
         BRA  BUFFLP
BUFFRTS  RTS

;*****************
;*  TERMARG() - Build a hex argument from the
;* terminal.  Characters are converted to binary
;* and shifted into shftreg until a non-hex character
;* is found.  On exit shftreg holds the last four
;* digits read, count holds the number of digits
;* read, and A holds the first non-hex character.
;*****************
;*initialize
;*while((a=inchar()) == hex)
;*     if(a = cntlx or del)
;*           abort;
;*     else
;*           hexbin(a); countu1++;
;*return();

TERMARG  CLR  COUNT
         CLR  SHFTREG
         CLR  SHFTREG+1
TERM0    JSR  INCHAR
         CMPA #CTLX
         BEQ  TERM1             ; jump if controlx
         CMPA #DEL
         BNE  TERM2             ; jump if not delete
TERM1    JMP  MAIN              ; abort
TERM2    CLR  TMP1              ; hex indicator
         JSR  HEXBIN
         TST  TMP1
         BNE  TERM3             ; jump if not hex
         INC  COUNT
         BRA  TERM0
TERM3    RTS

;*****************
;*   CHGBYT() - If shftreg is not empty, put
;* contents of shftreg at address in X.       If X
;* is an address in EEPROM then program it.
;*****************
;*if(count != 0)
;*   (x) = a;
CHGBYT   TST  COUNT
         BEQ  CHGBYT4           ; quit if shftreg empty
         LDAA SHFTREG+1         ; get data into a
         JSR  WRITE
CHGBYT4  RTS


;*****************
;* WRITE() - This routine is used to write the
;*contents of A to the address of X.  If the
;*address is in EEPROM, it will be programmed
;*and if it is already programmed, it will be
;*byte erased first.
;******************
;*if(X is eeprom)then
;*   if(not erased) then erase;
;*   program (x) = A;
;*write (x) = A;
;*if((x) != A) error(rom);
WRITE   EQU  *
        CPX  #CONFIG
        BEQ  WRITE1             ; jump if config
        CPX  #STREE             ; start of EE
        BLO  WRITE2             ; jump if not EE
        CPX  #ENDEE             ; end of EE
        BHI  WRITE2             ; jump if not EE
WRITEE  PSHB
        LDAB 0,X
        CMPB #$FF
        PULB
        BEQ  WRITE1             ; jump if erased
        JSR  EEBYTE             ; byte erase
WRITE1  JSR  EEWRIT             ; byte program
WRITE2  STAA 0,X                ; write for non EE
        CMPA 0,X
        BEQ  WRITE3             ; jump if write ok
        PSHX
        LDX  #MSG6              ; "rom"
        JSR  OUTSTRG
        PULX
WRITE3  RTS


;*****************
;*   EEWRIT(), EEBYTE(), EEBULK() -
;* These routines are used to program and eeprom
;*locations.  eewrite programs the address in X with
;*the value in A, eebyte does a byte address at X,
;*and eebulk does a bulk of eeprom.  Whether eebulk
;*erases the config or not depends on the address it
;*receives in X.
;****************
EEWRIT  EQU  *                  ; program one byte at x
        PSHB
        LDAB #$02
        STAB PPROG
        STAA 0,X
        LDAB #$03
        BRA  EEPROG
;***
EEBYTE  EQU  *                  ; byte erase address x
        PSHB
        LDAB #$16
        STAB PPROG
        LDAB #$FF
        STAB 0,X
        LDAB #$17
        BRA  EEPROG
;***
EEBULK  EQU  *                  ; bulk erase eeprom
        PSHB
        LDAB #$06
        STAB PPROG
        LDAB #$FF
        STAB 0,X                ; erase config or not
        LDAB #$07               ; depends on X addr
EEPROG  BNE  ACL1
        CLRB                    ; fail safe
ACL1    STAB PPROG
        PULB
;***
DLY10MS EQU  *                  ; delay 10ms at E = 2MHz
        PSHX
        LDX  #$0D06
DLYLP   DEX
        BNE  DLYLP
        PULX
        CLR  PPROG
        RTS


;*****************
;*  READBUFF() -  Read the character in INBUFF
;* pointed at by ptrbuff into A.  Returns ptrbuff
;* unchanged.
;*****************
READBUFF PSHX
         LDX  PTR0
         LDAA 0,X
         PULX
         RTS

;*****************
;*  INCBUFF(), DECBUFF() - Increment or decrement
;* ptrbuff.
;*****************
INCBUFF  PSHX
         LDX  PTR0
         INX
         BRA  INCDEC
DECBUFF  PSHX
         LDX  PTR0
         DEX
INCDEC   STX  PTR0
         PULX
         RTS

;*****************
;*  WSKIP() - Read from the INBUFF until a
;* non whitespace (space, comma, tab) character
;* is found.  Returns ptrbuff pointing to the
;* first non-whitespace character and a holds
;* that character.  WSKIP also compares a to
;* $0D (CR) and cond codes indicating the
;* results of that compare.
;*****************
WSKIP    JSR  READBUFF          ; read character
         JSR  WCHEK
         BNE  WSKIP1            ; jump if not wspc
         JSR  INCBUFF           ; move pointer
         BRA  WSKIP             ; loop
WSKIP1   CMPA #$0D
         RTS

;*****************
;*  WCHEK(a) - Returns z=1 if a holds a
;* whitespace character, else z=0.
;*****************
WCHEK    CMPA #$2C              ; comma
         BEQ  WCHEK1
         CMPA #$20              ; space
         BEQ  WCHEK1
         CMPA #$09              ; tab
WCHEK1   RTS

;*****************
;*   DCHEK(a) - Returns Z=1 if a = whitespace
;* or carriage return.  Else returns z=0.
;*****************
DCHEK   JSR  WCHEK
        BEQ  DCHEK1             ; jump if whitespace
        CMPA #$0D
DCHEK1  RTS

;*****************
;*  CHKABRT() - Checks for a control x or delete
;* from the terminal.  If found, the stack is
;* reset and the control is transferred to main.
;* Note that this is an abnormal termination.
;*   If the input from the terminal is a control W
;* then this routine keeps waiting until any other
;* character is read.
;*****************
;*a=input();
;*if(a=cntl w) wait until any other key;
;*if(a = cntl x or del) abort;

CHKABRT  JSR  INPUT
         BEQ  CHK4              ; jump if no input
         CMPA #CTLW
         BNE  CHK2              ; jump in not cntlw
CHKABRT1 JSR  INPUT
         BEQ  CHKABRT1          ; jump if no input
CHK2     CMPA #DEL
         BEQ  CHK3              ; jump if delete
         CMPA #CTLX
         BEQ  CHK3              ; jump if control x
         CMPA #CTLA
         BNE  CHK4              ; jump not control a
CHK3     JMP  MAIN              ; abort
CHK4     RTS                    ; return

;***********************
;*  HOSTCO - connect sci to host for evb board.
;*  TARGCO - connect sci to target for evb board.
;***********************
HOSTCO   PSHA
         LDAA #$01
         STAA DFLOP             ; send 1 to d-flop
         PULA
         RTS

TARGCO   PSHA
         LDAA #$00
         STAA DFLOP             ; send 0 to d-flop
         PULA
         RTS

;*
;**********
;*
;*     VECINIT - This routine checks for
;*         vectors in the RAM table.  All
;*         uninitialized vectors are programmed
;*         to JMP STOPIT
;*
;**********
;*
VECINIT  LDX  #JSCI             ; Point to First RAM Vector
         LDY  #STOPIT           ; Pointer to STOPIT routine
         LDD  #$7E03            ; A=JMP opcode; B=offset
VECLOOP  CMPA 0,X
         BEQ  VECNEXT           ; If vector already in
         STAA 0,X               ; install JMP
         STY  1,X               ; to STOPIT routine
VECNEXT  ABX                    ; Add 3 to point at next vector
         CPX  #JCLM+3           ; Done?
         BNE  VECLOOP           ; If not, continue loop
         RTS
;*
STOPIT   LDAA #$50              ; Stop-enable; IRQ, XIRQ-Off
         TAP
         STOP                   ; You are lost!  Shut down
         JMP  STOPIT            ; In case continue by XIRQ

;**********
;*
;*   I/O MODULE
;*     Communications with the outside world.
;* 3 I/O routines (INIT, INPUT, and OUTPUT) call
;* drivers specified by IODEV (0=SCI, 1=ACIA,
;* 2=DUARTA, 3=DUARTB).
;*
;**********
;*   INIT() - Initialize device specified by iodev.
;*********
;*
INIT     EQU  *
         PSHA                   ; save registers
         PSHX
         LDAA IODEV
         CMPA #$00
         BNE  INIT1             ; jump not sci
         JSR  ONSCI             ; initialize sci
         BRA  INIT4
INIT1    CMPA #$01
         BNE  INIT2             ; jump not acia
         JSR  ONACIA            ; initialize acia
         BRA  INIT4
INIT2    LDX  #PORTA
         CMPA #$02
         BEQ  INIT3             ; jump duart a
         LDX  #PORTB
INIT3    JSR  ONUART            ; initialize duart
INIT4    PULX                   ; restore registers
         PULA
         RTS

;**********
;*  INPUT() - Read device. Returns a=char or 0.
;*    This routine also disarms the cop.
;**********
INPUT    EQU  *
         PSHX
         LDAA #$55              ; reset cop
         STAA COPRST
         LDAA #$AA
         STAA COPRST
         LDAA IODEV
         BNE  INPUT1            ; jump not sci
         JSR  INSCI             ; read sci
         BRA  INPUT4
INPUT1   CMPA #$01
         BNE  INPUT2            ; jump not acia
         JSR  INACIA            ; read acia
         BRA  INPUT4
INPUT2   LDX  #PORTA
         CMPA #$02
         BEQ  INPUT3            ; jump if duart a
         LDX  #PORTB
INPUT3   JSR  INUART            ; read uart
INPUT4   PULX
         RTS

;**********
;*   OUTPUT() - Output character in A.
;* chrcnt indicates the current column on the
;*output display.  It is incremented every time
;*a character is outputted, and cleared whenever
;*the subroutine outcrlf is called.
;**********

OUTPUT   EQU  *
         PSHA                   ; save registers
         PSHB
         PSHX
         LDAB IODEV
         BNE  OUTPUT1           ; jump not sci
         JSR  OUTSCI            ; write sci
         BRA  OUTPUT4
OUTPUT1  CMPB #$01
         BNE  OUTPUT2           ; jump not acia
         JSR  OUTACIA           ; write acia
         BRA  OUTPUT4
OUTPUT2  LDX  #PORTA
         CMPB #$02
         BEQ  OUTPUT3           ; jump if duart a
         LDX  #PORTB
OUTPUT3  JSR  OUTUART           ; write uart
OUTPUT4  PULX
         PULB
         PULA
         INC  CHRCNT            ; increment column count
         RTS

;**********
;*   ONUART(port) - Initialize a duart port.
;* Sets duart to internal clock, divide by 16,
;* 8 data + 1 stop bits.
;**********

ONUART   LDAA #$22
         STAA 2,X               ; reset receiver
         LDAA #$38
         STAA 2,X               ; reset transmitter
         LDAA #$40
         STAA 2,X               ; reset error status
         LDAA #$10
         STAA 2,X               ; reset pointer
         LDAA #$00
         STAA DUART+4           ; clock source
         LDAA #$00
         STAA DUART+5           ; interrupt mask
         LDAA #$13
         STAA 0,X               ; 8 data, no parity
         LDAA #$07
         STAA 0,X               ; 1 stop bits
         LDAA #$BB              ; baud rate (9600)
         STAA 1,X               ; tx and rcv baud rate
         LDAA #$05
         STAA 2,X               ; enable tx and rcv
         RTS

;**********
;*   INUART(port) - Check duart for any input.
;**********
INUART    LDAA 1,X              ; read status
         ANDA #$01              ; check rxrdy
         BEQ  INUART1           ; jump if no data
         LDAA 3,X               ; read data
         ANDA #$7F              ; mask parity
INUART1  RTS

;**********
;*   OUTUART(port) - Output the character in a.
;*         if autolf=1, transmits cr or lf as crlf.
;**********
OUTUART  TST  AUTOLF
         BEQ  OUTUART2          ; jump if no autolf
         BSR  OUTUART2
         CMPA #$0D
         BNE  OUTUART1
         LDAA #$0A              ; if cr, output lf
         BRA  OUTUART2
OUTUART1 CMPA #$0A
         BNE  OUTUART3
         LDAA #$0D              ; if lf, output cr
OUTUART2 LDAB 1,X               ; check status
         ANDB #$4
         BEQ  OUTUART2          ; loop until tdre=1
         ANDA #$7F              ; mask parity
         STAA 3,X               ; send character
OUTUART3 RTS

;**********
;*   ONSCI() - Initialize the SCI for 9600
;*                   baud at 8 MHz Extal.
;**********
ONSCI    LDAA #$30
         STAA BAUD              ; baud register
         LDAA #$00
         STAA SCCR1
         LDAA #$0C
         STAA SCCR2             ; enable
         RTS

;**********
;*   INSCI() - Read from SCI.  Return a=char or 0.
;**********
INSCI    LDAA SCSR              ; read status reg
         ANDA #$20              ; check rdrf
         BEQ  INSCI1            ; jump if no data
         LDAA SCDAT             ; read data
         ANDA #$7F              ; mask parity
INSCI1   RTS

;**********
;*  OUTSCI() - Output A to sci. IF autolf = 1,
;*                 cr and lf sent as crlf.
;**********
OUTSCI   TST  AUTOLF
         BEQ  OUTSCI2           ; jump if autolf=0
         BSR  OUTSCI2
         CMPA #$0D
         BNE  OUTSCI1
         LDAA #$0A              ; if cr, send lf
         BRA  OUTSCI2
OUTSCI1  CMPA #$0A
         BNE  OUTSCI3
         LDAA #$0D              ; if lf, send cr
OUTSCI2  LDAB SCSR              ; read status
         BITB #$80
         BEQ  OUTSCI2           ; loop until tdre=1
         ANDA #$7F              ; mask parity
         STAA SCDAT             ; send character
OUTSCI3  RTS

;**********
;*   ONACIA - Initialize the ACIA for
;* 8 data bits, 1 stop bit, divide by 64 clock.
;**********
ONACIA   LDX  #ACIA
         LDAA #$03
         STAA 0,X               ; master reset
         LDAA #$16
         STAA 0,X               ; setup
         RTS

;**********
;*   INACIA - Read from the ACIA, Return a=char or 0.
;* Tmp3 is used to flag overrun or framing error.
;**********
INACIA   LDX  #ACIA
         LDAA 0,X               ; read status register
         PSHA
         ANDA #$30              ; check ov, fe
         PULA
         BEQ  INACIA1           ; jump - no error
         LDAA #$01
         STAA TMP3              ; flag receiver error
         BRA  INACIA2           ; read data to clear status
INACIA1  ANDA #$01              ; check rdrf
         BEQ  INACIA3           ; jump if no data
INACIA2  LDAA 1,X               ; read data
         ANDA #$7F              ; mask parity
INACIA3  RTS

;**********
;*  OUTACIA - Output A to acia. IF autolf = 1,
;*                 cr or lf sent as crlf.
;**********
OUTACIA  BSR  OUTACIA3          ; output char
         TST  AUTOLF
         BEQ  OUTACIA2          ; jump no autolf
         CMPA #$0D
         BNE  OUTACIA1
         LDAA #$0A
         BSR  OUTACIA3          ; if cr, output lf
         BRA  OUTACIA2
OUTACIA1 CMPA #$0A
         BNE  OUTACIA2
         LDAA #$0D
         BSR  OUTACIA3          ; if lf, output cr
OUTACIA2 RTS

OUTACIA3 LDX  #ACIA
         LDAB 0,X
         BITB #$2
         BEQ  OUTACIA3          ; loop until tdre
         ANDA #$7F              ; mask parity
         STAA 1,X               ; output
         RTS
;*
;*         Space for modifying OUTACIA routine
;*
         FDB  $FFFF,$FFFF,$FFFF,$FFFF
;*******************************
;*** I/O UTILITY SUBROUTINES ***
;***These subroutines perform the neccesary
;* data I/O operations.
;* OUTLHLF-Convert left 4 bits of A from binary
;*             to ASCII and output.
;* OUTRHLF-Convert right 4 bits of A from binary
;*             to ASCII and output.
;* OUT1BYT-Convert byte addresed by X from binary
;*            to ASCII and output.
;* OUT1BSP-Convert byte addressed by X from binary
;*            to ASCII and output followed by a space.
;* OUT2BSP-Convert 2 bytes addressed by X from binary
;*             to ASCII and  output followed by a space.
;* OUTSPAC-Output a space.
;*
;* OUTCRLF-Output a line feed and carriage return.
;*
;* OUTSTRG-Output the string of ASCII bytes addressed
;*             by X until $04.
;* OUTA-Output the ASCII character in A.
;*
;* TABTO-Output spaces until column 20 is reached.
;*
;* INCHAR-Input to A and echo one character.  Loops
;*             until character read.
;*         *******************
;
;**********
;*  OUTRHLF(), OUTLHLF(), OUTA()
;*Convert A from binary to ASCII and output.
;*Contents of A are destroyed..
;**********
OUTLHLF  LSRA                   ; shift data to right
         LSRA
         LSRA
         LSRA
OUTRHLF  ANDA #$0F              ; mask top half
         ADDA #$30              ; convert to ascii
         CMPA #$39
         BLE  OUTA              ; jump if 0-9
         ADDA #$07              ; convert to hex A-F
OUTA     JSR  OUTPUT            ; output character
         RTS

;**********
;*  OUT1BYT(x) - Convert the byte at X to two
;* ASCII characters and output. Return X pointing
;* to next byte.
;**********
OUT1BYT  PSHA
         LDAA 0,X               ; get data in a
         PSHA                   ; save copy
         BSR  OUTLHLF           ; output left half
         PULA                   ; retrieve copy
         BSR  OUTRHLF           ; output right half
         PULA
         INX
         RTS

;**********
;*  OUT1BSP(x), OUT2BSP(x) - Output 1 or 2 bytes
;* at x followed by a space.  Returns x pointing to
;* next byte.
;**********
OUT2BSP  JSR  OUT1BYT           ; do first byte
OUT1BSP  JSR  OUT1BYT           ; do next byte
OUTSPAC  LDAA #$20              ; output a space
         JSR  OUTPUT
         RTS

;**********
;*  OUTCRLF() - Output a Carriage return and
;* a line feed.    Returns a = cr.
;**********
OUTCRLF  LDAA #$0D              ; cr
         JSR  OUTPUT            ; output a
         LDAA #$00
         JSR  OUTPUT            ; output padding
         LDAA #$0D
         CLR  CHRCNT            ; zero the column counter
         RTS

;**********
;*  OUTSTRG(x) - Output string of ASCII bytes
;* starting at x until end of text ($04).  Can
;* be paused by control w (any char restarts).
;**********
OUTSTRG  JSR  OUTCRLF
OUTSTRG0 PSHA
OUTSTRG1 LDAA 0,X               ; read char into a
         CMPA #EOT
         BEQ  OUTSTRG3          ; jump if eot
         JSR  OUTPUT            ; output character
         INX
         JSR  INPUT
         BEQ  OUTSTRG1          ; jump if no input
         CMPA #CTLW
         BNE  OUTSTRG1          ; jump if not cntlw
OUTSTRG2 JSR  INPUT
         BEQ  OUTSTRG2          ; jump if any input
         BRA  OUTSTRG1
OUTSTRG3 PULA
         RTS


;*********
;*  TABTO() - move cursor over to column 20.
;*while(chrcnt < 16) outspac.
TABTO    EQU  *
        PSHA
TABTOLP JSR  OUTSPAC
        LDAA CHRCNT
        CMPA #20
        BLE  TABTOLP
        PULA
        RTS

;**********
;*  INCHAR() - Reads input until character sent.
;*    Echoes char and returns with a = char.
INCHAR    JSR  INPUT
         TSTA
         BEQ  INCHAR            ; jump if no input
         JSR  OUTPUT            ; echo
         RTS

;*********************
;*** COMMAND TABLE ***
COMTABL  EQU  *
         FCB  5
         FCC  "ASSEM"
         FDB  ASSEM
         FCB  5
         FCC  "BREAK"
         FDB  BREAK
         FCB  4
         FCC  "BULK"
         FDB  BULK
         FCB  7
         FCC  "BULKALL"
         FDB  BULKALL
         FCB  4
         FCC  "CALL"
         FDB  CALL
         FCB  4
         FCC  "DUMP"
         FDB  DUMP
         FCB  4
         FCC  "FILL"
         FDB  FILL
         FCB  2
         FCC  "GO"
         FDB  GO
         FCB  4
         FCC  "HELP"
         FDB  HELP
         FCB  4
         FCC  "HOST"
         FDB  HOST
         FCB  4
         FCC  "LOAD"
         FDB  LOAD
         FCB  6                 ; LENGTH OF COMMAND
         FCC  "MEMORY"          ; ASCII COMMAND
         FDB  MEMORY            ; COMMAND ADDRESS
         FCB  4
         FCC  "MOVE"
         FDB  MOVE
         FCB  7
         FCC  "PROCEED"
         FDB  PROCEED
         FCB  8
         FCC  "REGISTER"
         FDB  REGISTER
         FCB  6
         FCC  "STOPAT"
         FDB  STOPAT
         FCB  5
         FCC  "TRACE"
         FDB  TRACE
         FCB  6
         FCC  "VERIFY"
         FDB  VERIFY
         FCB  1
         FCC  "?"               ; initial command
         FDB  HELP
         FCB  5
         FCC  "XBOOT"
         FDB  BOOT
;*
;*** Command names for evm compatability ***
;*
         FCB  3
         FCC  "ASM"
         FDB  ASSEM
         FCB  2
         FCC  "BF"
         FDB  FILL
         FCB  4
         FCC  "COPY"
         FDB  MOVE
         FCB  5
         FCC  "ERASE"
         FDB  BULK
         FCB  2
         FCC  "MD"
         FDB  DUMP
         FCB  2
         FCC  "MM"
         FDB  MEMORY
         FCB  2
         FCC  "RD"
         FDB  REGISTER
         FCB  2
         FCC  "RM"
         FDB  REGISTER
         FCB  4
         FCC  "READ"
         FDB  MOVE
         FCB  2
         FCC  "TM"
         FDB  HOST
         FCB  4
         FCC  "TEST"
         FDB  EVBTEST
         FCB  $FF

;*******************
;*** TEXT TABLES ***

MSG1    FCC   "BUFFALO 3.2 (int) - Bit User Fast Friendly Aid to Logical Operation"
        FCB   EOT
MSG2    FCC   "What?"
        FCB   EOT
MSG3    FCC   "Too Long"
        FCB   EOT
MSG4    FCC   "Full"
        FCB   EOT
MSG5    FCC   "Op- "
        FCB   EOT
MSG6    FCC   "rom-"
        FCB   EOT
MSG8    FCC   "Command?"
        FCB   EOT
MSG9    FCC   "Bad argument"
        FCB   EOT
MSG10   FCC   "No host port available"
        FCB   EOT
MSG11   FCC   "done"
        FCB   EOT
MSG12   FCC   "checksum error"
        FCB   EOT
MSG13   FCC   "error addr "
        FCB   EOT
MSG14   FCC   "receiver error"
        FCB   EOT

;**********
;*   break [-][<addr>] . . .
;* Modifies the breakpoint table.  More than
;* one argument can be entered on the command
;* line but the table will hold only 4 entries.
;* 4 types of arguments are implied above:
;* break    Prints table contents.
;* break <addr>      Inserts <addr>.
;* break -<addr>   Deletes <addr>.
;* break -           Clears all entries.
;**********
;* while 1
;*     a = wskip();
;*     switch(a)
;*           case(cr):
;*                 bprint(); return;

BREAK   JSR  WSKIP
        BNE  BRKDEL             ; jump if not cr
        JSR  BPRINT             ; print table
        RTS

;*           case("-"):
;*                 incbuff(); readbuff();
;*                 if(dchek(a))            /* look for wspac or cr */
;*                      bpclr();
;*                      breaksw;
;*                 a = buffarg();
;*                 if( !dchek(a) ) return(bad argument);
;*                 b = bpsrch();
;*                 if(b >= 0)
;*                      brktabl[b] = 0;
;*                 breaksw;

BRKDEL  CMPA #'-'
        BNE  BRKDEF             ; jump if not -
        JSR  INCBUFF
        JSR  READBUFF
        JSR  DCHEK
        BNE  BRKDEL1            ; jump if not delimeter
        JSR  BPCLR              ; clear table
        JMP  BREAK              ; do next argument
BRKDEL1 JSR  BUFFARG            ; get address to delete
        JSR  DCHEK
        BEQ  BRKDEL2            ; jump if delimeter
        LDX  #MSG9              ; "bad argument"
        JSR  OUTSTRG
        RTS
BRKDEL2 JSR  BPSRCH             ; look for addr in table
        TSTB
        BMI  BRKDEL3            ; jump if not found
        LDX  #BRKTABL
        ABX
        CLR  0,X                ; clear entry
        CLR  1,X
BRKDEL3 JMP  BREAK              ; do next argument

;*           default:
;*                 a = buffarg();
;*                 if( !dchek(a) ) return(bad argument);
;*                 b = bpsrch();
;*                 if(b < 0)              /* not already in table */
;*                      x = shftreg;
;*                      shftreg = 0;
;*                      a = x[0]; x[0] = $3F
;*                      b = x[0]; x[0] = a;
;*                      if(b != $3F) return(rom);
;*                      b = bpsrch();   /* look for hole */
;*                      if(b >= 0) return(table full);
;*                      brktabl[b] = x;
;*                 breaksw;

BRKDEF  JSR  BUFFARG            ; get argument
        JSR  DCHEK
        BEQ  BRKDEF1            ; jump if delimiter
        LDX  #MSG9              ; "bad argument"
        JSR  OUTSTRG
        RTS
BRKDEF1 JSR  BPSRCH             ; look for entry in table
        TSTB
        BGE  BREAK              ; jump if already in table

        LDX  SHFTREG            ; x = new entry addr
        LDAA 0,X                ; save original contents
        PSHA
        LDAA #SWI
        JSR  WRITE              ; write to entry addr
        LDAB 0,X                ; read back
        PULA
        JSR  WRITE              ; restore original
        CMPB #SWI
        BEQ  BRKDEF2            ; jump if writes ok
        STX  PTR1               ; save address
        LDX  #PTR1
        JSR  OUT2BSP            ; print address
        JSR  BPRINT
        RTS
BRKDEF2 CLR  SHFTREG
        CLR  SHFTREG+1
        PSHX
        JSR  BPSRCH             ; look for 0 entry
        PULX
        TSTB
        BPL  BRKDEF3            ; jump if table not full
        LDX  #MSG4              ; "full"
        JSR  OUTSTRG
        JSR  BPRINT
        RTS
BRKDEF3 LDY  #BRKTABL
        ABY
        STX  0,Y                ; put new entry in
        JMP  BREAK              ; do next argument

;**********
;*   bprint() - print the contents of the table.
;**********
BPRINT  JSR  OUTCRLF
        LDX  #BRKTABL
        LDAB #4
BPRINT1 JSR  OUT2BSP
        DECB
        BGT  BPRINT1           ; loop 4 times
        RTS

;**********
;*   bpsrch() - search table for address in
;* shftreg. Returns b = index to entry or
;* b = -1 if not found.
;**********
;*for(b=0; b=6; b=+2)
;*     x[] = brktabl + b;
;*     if(x[0] = shftreg)
;*           return(b);
;*return(-1);

BPSRCH   CLRB
BPSRCH1  LDX  #BRKTABL
         ABX
         LDX  0,X               ; get table entry
         CPX  SHFTREG
         BNE  BPSRCH2           ; jump if no match
         RTS
BPSRCH2  INCB
         INCB
         CMPB #$6
         BLE  BPSRCH1           ; loop 4 times
         LDAB #$FF
         RTS


;**********
;*  bulk  - Bulk erase the eeprom not config.
;* bulkall - Bulk erase eeprom and config.
;*********
BULK    EQU  *
        LDX  #$B600
        BRA  BULK1
BULKALL LDX  #CONFIG
BULK1   LDAA #$FF
        JSR  EEBULK
        RTS



;**********
;*  dump [<addr1> [<addr2>]]  - Dump memory
;* in 16 byte lines from <addr1> to <addr2>.
;*   Default starting address is "current
;* location" and default number of lines is 8.
;**********
;*ptr1 = ptrmem;        /* default start address */
;*ptr2 = ptr1 + $80;    /* default end address */
;*a = wskip();
;*if(a != cr)
;*     a = buffarg();
;*     if(countu1 = 0) return(bad argument);
;*     if( !dchek(a) ) return(bad argument);
;*     ptr1 = shftreg;
;*     ptr2 = ptr1 + $80;  /* default end address */
;*     a = wskip();
;*     if(a != cr)
;*           a = buffarg();
;*           if(countu1 = 0) return(bad argument);
;*           a = wskip();
;*           if(a != cr) return(bad argument);
;*           ptr2 = shftreg;

DUMP     LDX  PTRMEM            ; current location
         STX  PTR1              ; default start
         LDAB #$80
         ABX
         STX  PTR2              ; default end
         JSR  WSKIP
         BEQ  DUMP1             ; jump - no arguments
         JSR  BUFFARG           ; read argument
         TST  COUNT
         BEQ  DUMPERR           ; jump if no argument
         JSR  DCHEK
         BNE  DUMPERR           ; jump if delimiter
         LDX  SHFTREG
         STX  PTR1
         LDAB #$80
         ABX
         STX  PTR2              ; default end address
         JSR  WSKIP
         BEQ  DUMP1             ; jump - 1 argument
         JSR  BUFFARG           ; read argument
         TST  COUNT
         BEQ  DUMPERR           ; jump if no argument
         JSR  WSKIP
         BNE  DUMPERR           ; jump if not cr
         LDX  SHFTREG
         STX  PTR2
         BRA  DUMP1             ; jump - 2 arguments
DUMPERR  LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS

;*ptrmem = ptr1;
;*ptr1 = ptr1 & $fff0;

DUMP1    LDD  PTR1
         STD  PTRMEM            ; new current location
         ANDB #$F0
         STD  PTR1              ; start dump at 16 byte boundary

;*** dump loop starts here ***
;*do:
;*     output address of first byte;

DUMPLP   JSR  OUTCRLF
         LDX  #PTR1
         JSR  OUT2BSP           ; first address

;*     x = ptr1;
;*     for(b=0; b=16; b++)
;*           output contents;

         LDX  PTR1              ; base address
         CLRB                   ; loop counter
DUMPDAT  JSR  OUT1BSP           ; hex value loop
         INCB
         CMPB #$10
         BLT  DUMPDAT           ; loop 16 times

;*     x = ptr1;
;*     for(b=0; b=16; b++)
;*           a = x[b];
;*           if($7A < a < $20)  a = $20;
;*           output ascii contents;

         CLRB                   ; loop counter
DUMPASC  LDX  PTR1              ; base address
         ABX
         LDAA ,X                ; ascii value loop
         CMPA #$20
         BLO  DUMP3             ; jump if non printable
         CMPA #$7A
         BLS  DUMP4             ; jump if printable
DUMP3    LDAA #$20              ; space for non printables
DUMP4    JSR  OUTPUT            ; output ascii value
         INCB
         CMPB #$10
         BLT  DUMPASC           ; loop 16 times

;*     chkabrt();
;*     ptr1 = ptr1 + $10;
;*while(ptr1 <= ptr2);
;*return;

         JSR  CHKABRT           ; check abort or wait
         LDD  PTR1
         ADDD #$10              ; point to next 16 byte bound
         STD  PTR1              ; update ptr1
         CPD  PTR2
         BHI  DUMP5             ; quit if ptr1 > ptr2
         CPD  #$00              ; check wraparound at $ffff
         BNE  DUMPLP            ; jump - no wraparound
         LDD  PTR2
         CPD  #$FFF0
         BLO  DUMPLP            ; upper bound not at top
DUMP5    RTS                    ; quit



;**********
;*  fill <addr1> <addr2> [<data>]  - Block fill
;*memory from addr1 to addr2 with data.       Data
;*defaults to $FF.
;**********
;*get addr1 and addr2
FILL    EQU  *
        JSR  WSKIP
        JSR  BUFFARG
        TST  COUNT
        BEQ  FILLERR            ; jump if no argument
        JSR  WCHEK
        BNE  FILLERR            ; jump if bad argument
        LDX  SHFTREG
        STX  PTR1               ; address1
        JSR  WSKIP
        JSR  BUFFARG
        TST  COUNT
        BEQ  FILLERR            ; jump if no argument
        JSR  DCHEK
        BNE  FILLERR            ; jump if bad argument
        LDX  SHFTREG
        STX  PTR2               ; address2

;*Get data if it exists
        LDAA #$FF
        STAA TMP2               ; default data
        JSR  WSKIP
        BEQ  FILL1              ; jump if default data
        JSR  BUFFARG
        TST  COUNT
        BEQ  FILLERR            ; jump if no argument
        JSR  WSKIP
        BNE  FILLERR            ; jump if bad argument
        LDAA SHFTREG+1
        STAA TMP2

;*while(ptr1 <= ptr2)
;*   *ptr1 = data
;*   if(*ptr1 != data) abort

FILL1   EQU  *
        JSR  CHKABRT            ; check for abort
        LDX  PTR1               ; starting address
        LDAA TMP2               ; data
        JSR  WRITE              ; write the data to x
        CMPA 0,X
        BNE  FILLBAD            ; jump if no write
        CPX  PTR2
        BEQ  FILL2              ; quit yet?
        INX
        STX  PTR1
        BRA  FILL1              ; loop
FILL2   RTS

FILLERR LDX  #MSG9              ; "bad argument"
        JSR  OUTSTRG
        RTS

FILLBAD EQU  *
        LDX  #PTR1              ; output bad address
        JSR  OUT2BSP
        RTS



;**********
;*   call [<addr>] - Execute a jsr to <addr> or user
;*pc value.  Return to monitor via  rts or breakpoint.
;**********
;*a = wskip();
;*if(a != cr)
;*     a = buffarg();
;*     a = wskip();
;*     if(a != cr) return(bad argument)
;*     pc = shftreg;
CALL     JSR  WSKIP
         BEQ  CALL3             ; jump if no arg
         JSR  BUFFARG
         JSR  WSKIP
         BEQ  CALL2             ; jump if cr
         LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
CALL2    LDX  SHFTREG
         STX  REGS              ; pc = <addr>

;*put return address on user stack
;*setbps();
;*restack();       /* restack and go*/
CALL3    LDX  SP
         DEX                    ; user stack pointer
         LDD  #RETURN           ; return address
         STD  0,X
         DEX
         STX  SP                ; new user stack pointer
         JSR  SETBPS
         CLR  TMP2              ; 1=go, 0=call
         JMP  RESTACK           ; go to user code

;**********
;*   return() - Return here from rts after
;*call command.
;**********
RETURN   PSHA                   ; save a register
         TPA
         STAA REGS+8            ; cc register
         PULA
         STD  REGS+6            ; a and b registers
         STX  REGS+4            ; x register
         STY  REGS+2            ; y register
         STS  SP                ; user stack pointer
         LDS  PTR2              ; monitor stack pointer
         JSR  REMBPS            ; remove breakpoints
         JSR  OUTCRLF
         JSR  RPRINT            ; print user registers
         RTS


;**********
;*   proceed - Same as go except it ignores
;*a breakpoint at the first opcode.  Calls
;*runone for the first instruction only.
;**********
PROCEED  EQU  *
         JSR  RUNONE            ; run one instruction
         JSR  CHKABRT           ; check for abort
         CLR  TMP2              ; flag for breakpoints
         INC  TMP2              ; 1=go 0=call
         JSR  SETBPS
         JMP  RESTACK           ; go execute

;**********
;*   go [<addr>] - Execute starting at <addr> or
;*user's pc value.  Executes an rti to user code.
;*Returns to monitor via an swi through swiin.
;**********
;*a = wskip();
;*if(a != cr)
;*     a = buffarg();
;*     a = wskip();
;*     if(a != cr) return(bad argument)
;*     pc = shftreg;
;*setbps();
;*restack();       /* restack and go*/
GO       JSR  WSKIP
         BEQ  GO2               ; jump if no arg
         JSR  BUFFARG
         JSR  WSKIP
         BEQ  GO1               ; jump if cr
         LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
GO1      LDX  SHFTREG
         STX  REGS              ; pc = <addr>
GO2      CLR  TMP2
         INC  TMP2              ; 1=go, 0=call
         JSR  SETBPS
         JMP  RESTACK           ; go to user code

;*****
;** SWIIN - Breakpoints from go or call commands enter here.
;*Remove breakpoints, save user registers, return
SWIIN    EQU  *                 ; swi entry point
         TSX                    ; user sp -> x
         LDS  PTR2              ; restore monitor sp
         JSR  SAVSTACK          ; save user regs
         JSR  REMBPS            ; remove breakpoints from code
         LDX  REGS
         DEX
         STX  REGS              ; save user pc value

;*if(call command) remove call return addr from user stack;
         TST  TMP2              ; 1=go, 0=call
         BNE  GO3               ; jump if go command
         LDX  SP                ; remove return address
         INX                    ; user stack pointer
         INX
         STX  SP
GO3      JSR  OUTCRLF           ; print register values
         JSR  RPRINT
         RTS                    ; done

;**********
;*  setbps - Replace user code with swi's at
;*breakpoint addresses.
;**********
;*for(b=0; b=6; b =+ 2)
;*     x = brktabl[b];
;*     if(x != 0)
;*           optabl[b] = x[0];
;*           x[0] = $3F;
;*Put monitor SWI vector into jump table

SETBPS   CLRB
SETBPS1  LDX  #BRKTABL
         LDY  #PTR4
         ABX
         ABY
         LDX  0,X               ; breakpoint table entry
         BEQ  SETBPS2           ; jump if 0
         LDAA 0,X               ; save user opcode
         STAA 0,Y
         LDAA #SWI
         JSR  WRITE             ; insert swi into code
SETBPS2  ADDB #$2
         CMPB #$6
         BLE  SETBPS1           ; loop 4 times
         LDX  JSWI+1
         STX  PTR3              ; save user swi vector
         LDAA #$7E              ; jmp opcode
         STAA JSWI
         LDX  #SWIIN
         STX  JSWI+1            ; monitor swi vector
         RTS

;**********
;*   rembps - Remove breakpoints from user code.
;**********
;*for(b=0; b=6; b =+ 2)
;*     x = brktabl[b];
;*     if(x != 0)
;*           x[0] = optabl[b];
;*Replace user's SWI vector
REMBPS   CLRB
REMBPS1  LDX  #BRKTABL
         LDY  #PTR4
         ABX
         ABY
         LDX  0,X               ; breakpoint table entry
         BEQ  REMBPS2           ; jump if 0
         LDAA 0,Y
         JSR  WRITE             ; restore user opcode
REMBPS2  ADDB #$2
         CMPB #$6
         BLE  REMBPS1           ; loop 4 times
         LDX  PTR3              ; restore user swi vector
         STX  JSWI+1
         RTS


;**********
;*   trace <n> - Trace n instructions starting
;*at user's pc value. n is a hex number less than
;*$FF (defaults to 1).
;**********
;*a = wskip();
;*if(a != cr)
;*     a = buffarg(); a = wskip();
;*     if(a != cr) return(bad argument);
;*     countt1 = n
TRACE    CLR  TMP4
         INC  TMP4              ; default count=1
         CLR  CHRCNT            ; set up for display
         JSR  WSKIP
         BEQ  TRACE2            ; jump if cr
         JSR  BUFFARG
         JSR  WSKIP
         BEQ  TRACE1            ; jump if cr
         LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
TRACE1   LDAA SHFTREG+1         ; n
         STAA TMP4

;*Disassemble the line about to be traced
TRACE2   EQU  *
         LDAB TMP4
         PSHB
         LDX  REGS
         STX  PTR1              ; pc value for disass
         JSR  DISASSM
         PULB
         STAB TMP4

;*run one instruction
;*rprint();
;*while(count > 0) continue trace;
         JSR  RUNONE
         JSR  CHKABRT           ; check for abort
         JSR  TABTO             ; print registers for
         JSR  RPRINT            ; result of trace
         DEC  TMP4
         BEQ  TRACDON           ; quit if count=0
TRACE3   JSR  OUTCRLF
         BRA  TRACE2
TRACDON  RTS


;**********
;*   stopat <addr> - Trace instructions until <addr>
;*is reached.
;**********
;*if((a=wskip) != cr)
;*     a = buffarg(); a = wskip();
;*     if(a != cr) return(bad argument);
;*else return(bad argument);
STOPAT   EQU  *
         JSR  WSKIP
         BEQ  STOPGO            ; jump if cr - no argument
         JSR  BUFFARG
         JSR  WSKIP
         BEQ  STOPAT1           ; jump if cr
         LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
STOPAT1  TST  COUNT
         BEQ  STOPGO            ; jump if no argument
         LDX  SHFTREG
         STX  PTRMEM            ; update "current location"

;*while(!(ptrmem <= userpc < ptrmem+10)) runone();
;*rprint();
STOPGO   LDD  REGS              ; userpc
         CPD  PTRMEM
         BLO  STOPNEXT          ; if(userpc < ptrmem) runone
         LDD  PTRMEM
         ADDD #10
         CPD  REGS
         BHI  STOPDON           ; quit if ptrmem+10 > userpc
STOPNEXT JSR  RUNONE
         JSR  CHKABRT           ; check for abort
         BRA  STOPGO
STOPDON  JSR  OUTCRLF
         JSR  RPRINT            ; result of trace
         RTS                    ; done


;*************************
;* runone - This routine is used by the trace and
;* execute commands to run one only one user instruction.
;*   Control is passed to the user code via an RTI.  OC5
;* is then used to trigger an XIRQ as soon as the first user
;* opcode is fetched.  Control then returns to the monitor
;* through XIRQIN.
;*  Externally, the OC5 pin must be wired to the XIRQ pin.
;************************
;* Disable oc5 interrupts
;* Put monitor XIRQ vector into jump table
;* Unmask x bit in user ccr
;* Setup OC5 to go low when first user instruction executed
RUNONE  EQU  *
        LDAA #$7E               ; put "jmp xirqin" in jump table
        STAA JTOC5
        LDX  #XIRQIN
        STX  JXIRQ+1
        LDAA REGS+8             ; x bit will be cleared when
        ANDA #$BF               ; rti is executed below
        STAA REGS+8
        LDAB #87                ; cycles to end of rti
        LDX  TCNT
        ABX                     ;                     3~
        STX  TOC5               ; oc5 match register         5~
        LDAA TCTL1              ;                     4~
        ANDA #$FE               ; set up oc5 low on match 2~
        STAA TCTL1              ; enable oc5 interrupt       4~    / 86~

;** RESTACK - Restore user stack and RTI to user code.
;* This code is the pathway to execution of user code.
;*(Force extended addressing to maintain cycle count)
;*Restore user stack and rti to user code
RESTACK EQU  *                  ;                     68~
        STS  >PTR2              ; save monitor sp
        LDS  >SP                ; user stack pointer
        LDX  >REGS
        PSHX                    ; pc
        LDX  >REGS+2
        PSHX                    ; y
        LDX  >REGS+4
        PSHX                    ; x
        LDD  >REGS+6
        PSHA                    ; a
        PSHB                    ; b
        LDAA >REGS+8
        PSHA                    ; ccr
        RTI

;** Return here from run one line of user code.
XIRQIN  EQU  *
        TSX                     ; user sp -> x
        LDS  PTR2               ; restore monitor sp

;** SAVSTACK - Save user's registers.
;* On entry - x points to top of user stack.
SAVSTACK EQU *
        LDAA 0,X
        STAA REGS+8             ; user ccr
        LDD  1,X
        STAA REGS+7             ; b
        STAB REGS+6             ; a
        LDD  3,X
        STD  REGS+4             ; x
        LDD  5,X
        STD  REGS+2             ; y
        LDD  7,X
        STD  REGS               ; pc
        LDAB #8
        ABX
        STX  SP                 ; user stack pointer
        LDAA TCTL1              ; force oc5 pin high which
        ORAA #$03               ; is tied to xirq line
        STAA TCTL1
        LDAA #$08
        STAA CFORC
        RTS


;**********
;*   help  -  List buffalo commands to terminal.
;**********
HELP     EQU  *
         LDX  #HELPMSG1
         JSR  OUTSTRG           ; print help screen
         RTS

HELPMSG1 EQU  *
         FCC  "ASM [<addr>]  Line assembler/disassembler."
         FCB  $0D
         FCC  "    /        Do same address.           ^        Do previous address."
         FCB  $0D
         FCC  "    CTRL-J   Do next address.           RETURN   Do next opcode."
         FCB  $0D
         FCC  "    CTRL-A   Quit."
         FCB  $0D
         FCC  "BF <addr1> <addr2> [<data>]  Block fill."
         FCB  $0D
         FCC  "BR [-][<addr>]  Set up breakpoint table."
         FCB  $0D
         FCC  "BULK  Erase the EEPROM.                   BULKALL  Erase EEPROM and CONFIG."
         FCB  $0D
         FCC  "CALL [<addr>]  Call user subroutine.      G [<addr>]  Execute user code."
         FCB  $0D
         FCC  "LOAD, VERIFY [T] <host download command>  Load or verify S-records."
         FCB  $0D
         FCC  "MD [<addr1> [<addr2>]]  Memory dump."
         FCB  $0D
         FCC  "MM [<addr>]  Memory modify."
         FCB  $0D
         FCC  "    /        Open same address.         CTRL-H or ^   Open previous address."
         FCB  $0D
         FCC  "    CTRL-J   Open next address.         SPACE         Open next address."
         FCB  $0D
         FCC  "    RETURN   Quit.                      <addr>O       Compute offset to <addr>."
         FCB  $0D
         FCC  "MOVE <s1> <s2> [<d>]  Block move."
         FCB  $0D
         FCC  "P  Proceed/continue execution."
         FCB  $0D
         FCC  "RM [P, Y, X, A, B, C, or S]  Register modify."
         FCB  $0D
         FCC  "T [<n>]  Trace n instructions."
         FCB  $0D
         FCC  "TM  Transparent mode (CTRL-A = exit, CTRL-B = send break)."
         FCB  $0D
         FCC  "CTRL-H  Backspace.                      CTRL-W  Wait for any key."
         FCB  $0D
         FCC  "CTRL-X or DELETE  Abort/cancel command."
         FCB  $0D
         FCC  "RETURN  Repeat last command."
         FCB  4

;**********
;*   HOST() - Establishes transparent link between
;*        terminal and host.  Port used for host is
;*        determined in the reset initialization routine
;*        and stored in HOSTDEV.
;*           To exit type control A.
;*           To send break to host type control B.
;*if(no external device) return;
;*initialize host port;
;*While( !(control A))
;*     input(terminal); output(host);
;*     input(host); output(terminal);

HOST      LDAA EXTDEV
          BNE  HOST0            ; jump if host port avail.
          LDX  #MSG10           ; "no host port avail"
          JSR  OUTSTRG
          RTS
HOST0     CLR  AUTOLF           ; turn off autolf
          JSR  HOSTCO           ; connect sci (evb board)
          JSR  HOSTINIT         ; initialize host port
HOST1     JSR  INPUT            ; read terminal
          TSTA
          BEQ  HOST3            ; jump if no char
          CMPA #CTLA
          BEQ  HOSTEND          ; jump if control a
          CMPA #CTLB
          BNE  HOST2            ; jump if not control b
          JSR  TXBREAK          ; send break to host
          BRA  HOST3
HOST2     JSR  HOSTOUT          ; echo to host
HOST3     JSR  HOSTIN           ; read host
          TSTA
          BEQ  HOST1            ; jump if no char
          JSR  OUTPUT           ; echo to terminal
          BRA  HOST1
HOSTEND   INC  AUTOLF           ; turn on autolf
          JSR  TARGCO           ; disconnect sci (evb board)
          RTS                   ; return

;**********
;* txbreak() - transmit break to host port.
;* The duration of the transmitted break is
;* approximately 200,000 E-clock cycles, or
;* 100ms at 2.0 MHz.
;***********
TXBREAK   EQU  *
          LDAA HOSTDEV
          CMPA #$03
          BEQ  TXBDU            ; jump if duartb is host

TXBSCI    LDX  #SCCR2           ; sci is host
          BSET 0,X,#01          ; set send break bit
          BSR  TXBWAIT
          BCLR 0,X,#01          ; clear send break bit
          BRA TXB1

TXBDU     LDX  #PORTB           ; duart host port
          LDAA #$60             ; start break cmd
          STAA 2,X              ; port b command register
          BSR  TXBWAIT
          LDAA #$70             ; stop break cmd
          STAA 2,X              ; port b command register

TXB1      LDAA #$0D
          JSR  HOSTOUT          ; send carriage return
          LDAA #$0A
          JSR  HOSTOUT          ; send linefeed
          RTS

TXBWAIT   LDY  #$6F9B           ; loop count = 28571
TXBWAIT1  DEY                   ; 7 cycle loop
          BNE  TXBWAIT1
          RTS


;**********
;*   hostinit(), hostin(), hostout() - host i/o
;*routines.  Restores original terminal device.
;**********
HOSTINIT  LDAB IODEV            ; save terminal
          PSHB
          LDAB HOSTDEV
          STAB IODEV            ; point to host
          JSR  INIT             ; initialize host
          BRA  TERMRES          ; restore terminal
HOSTIN    LDAB IODEV            ; save terminal
          PSHB
          LDAB HOSTDEV
          STAB IODEV            ; point to host
          JSR  INPUT            ; read host
          BRA  TERMRES          ; restore terminal
HOSTOUT   LDAB IODEV            ; save terminal
          PSHB
          LDAB HOSTDEV
          STAB IODEV            ; point to host
          JSR  OUTPUT           ; write to host
TERMRES   PULB                  ; restore terminal device
          STAB IODEV
          RTS


;**********
;*   load(ptrbuff[]) - Load s1/s9 records from
;*host to memory.  Ptrbuff[] points to string in
;*input buffer which is a command to output s1/s9
;*records from the host ("cat filename" for unix).
;*    Returns error and address if it can't write
;*to a particular location.
;**********
;*   verify(ptrbuff[]) - Verify memory from load
;*command.  Ptrbuff[] is same as for load.
;* tmp3 is used as an error indication, 0=no errors,
;* 1=receiver, 2=rom error, 3=checksum error.
;**********
VERIFY    CLR  TMP2
          INC  TMP2             ; TMP2=1=verify
          BRA  LOAD1
LOAD      CLR  TMP2             ; 0=load

;*a=wskip();
;*if(a = cr) goto transparent mode;
;*if(t option) hostdev = iodev;
LOAD1      CLR  TMP3            ; clear error flag
          JSR  WSKIP
          BNE  LOAD2
          JMP  HOST             ; go to host if no args
LOAD2     JSR  UPCASE
          CMPA #'T'             ; look for t option
          BNE  LOAD3            ; jump not t option
          JSR  INCBUFF
          JSR  READBUFF         ; get next character
          JSR  DECBUFF
          CMPA #$0D
          BNE  LOAD3            ; jump if not t option
          CLR  AUTOLF
          LDAA IODEV
          STAA HOSTDEV          ; set host port = terminal
          BRA  LOAD10           ; go wait for s1 records

;*else while(not cr)
;*     read character from input buffer;
;*     send character to host;
LOAD3     CLR  AUTOLF
          JSR  HOSTCO           ; connect sci (evb board)
          JSR  HOSTINIT         ; initialize host port
LOAD4     JSR  READBUFF         ; get next char
          JSR  INCBUFF
          PSHA                  ; save char
          JSR  HOSTOUT          ; output to host
          JSR  OUTPUT           ; echo to terminal
          PULA
          CMPA #$0D
          BNE  LOAD4            ; jump if not cr

;*repeat:                      /* look for s records */
;*      if(hostdev != iodev) check abort;
;*      a = hostin();
;*      if(a = 'S')
;*           a = hostin;
;*           if(a = '1')
;*               checksum = 0;
;*               get byte count in b;
;*               get base address in x;
;*               while(byte count > 0)
;*                    byte();
;*                    x++; b--;
;*                    if(tmp3=0)              /* no error */
;*                        if(load) x[0] = shftreg+1;
;*                        if(x[0] != shftreg+1)
;*                             tmp3 = 2;      /* rom error */
;*                             ptr3 = x;      /* save address */
;*               if(tmp3 = 0) do checksum;
;*               if(checksum err) tmp3 = 3; /* checksum error */
LOAD10    EQU  *
          LDAA HOSTDEV
          CMPA IODEV
          BEQ  LOAD11           ; jump if hostdev=iodev
          JSR  CHKABRT          ; check for abort
LOAD11    JSR  HOSTIN           ; read host
          TSTA
          BEQ  LOAD10           ; jump if no input
          CMPA #'S'
          BNE  LOAD10           ; jump if not S
LOAD12    JSR  HOSTIN           ; read host
          TSTA
          BEQ  LOAD12           ; jump if no input
          CMPA #'9'
          BEQ  LOAD90           ; jump if S9 record
          CMPA #'1'
          BNE  LOAD10           ; jump if not S1
          CLR  TMP4             ; clear checksum
          JSR  BYTE
          LDAB SHFTREG+1
          SUBB #$2              ; b = byte count
          JSR  BYTE
          JSR  BYTE
          LDX  SHFTREG          ; x = base address
          DEX
LOAD20    JSR  BYTE             ; get next byte
          INX
          DECB                  ; check byte count
          BEQ  LOAD30           ; if b=0, go do checksum
          TST  TMP3
          BNE  LOAD10           ; jump if error flagged
          TST  TMP2
          BNE  LOAD21           ; jump if verify
          LDAA SHFTREG+1
          JSR  WRITE            ; load only
LOAD21    CMPA 0,X              ; verify ram location
          BEQ  LOAD20           ; jump if ram ok
          LDAA #$02
          STAA TMP3             ; indicate rom error
          STX  PTR3             ; save error address
          BRA  LOAD20           ; finish download

;* calculate checksum
LOAD30    TST  TMP3
          BNE  LOAD10           ; jump if error already
          LDAA TMP4
          INCA                  ; do checksum
          BEQ  LOAD10           ; jump if s1 record okay
          LDAA #$03
          STAA TMP3             ; indicate checksum error
          BRA  LOAD10

;*           if(a = '9')
;*               read rest of record;
;*               if(tmp3=2) return("[ptr3]");
;*               if(tmp3=1) return("rcv error");
;*               if(tmp3=3) return("checksum err");
;*               else return("done");
LOAD90    JSR  BYTE
          LDAB SHFTREG+1        ; b = byte count
LOAD91    JSR  BYTE
          DECB
          BNE  LOAD91           ; loop until end of record
          INC  AUTOLF           ; turn on autolf
          JSR  TARGCO           ; disconnect sci (evb)
          LDX  #MSG11           ; "done" default msg
          LDAA TMP3
          CMPA #$02
          BNE  LOAD92           ; jump not rom error
          LDX  #PTR3
          JSR  OUT2BSP          ; address of rom error
          BRA  LOAD95
LOAD92    CMPA #$01
          BNE  LOAD93           ; jump not rcv error
          LDX  #MSG14           ; "rcv error"
          BRA  LOAD94
LOAD93    CMPA #$03
          BNE  LOAD94           ; jump not checksum error
          LDX  #MSG12           ; "checksum error"
LOAD94    JSR  OUTSTRG
LOAD95    RTS


;**********
;*  byte() -  Read 2 ascii bytes from host and
;*convert to one hex byte.  Returns byte
;*shifted into shftreg and added to tmp4.
;**********
BYTE      PSHB
          PSHX
BYTE0     JSR  HOSTIN           ; read host (1st byte)
          TSTA
          BEQ  BYTE0            ; loop until input
          JSR  HEXBIN
BYTE1     JSR  HOSTIN           ; read host (2nd byte)
          TSTA
          BEQ  BYTE1            ; loop until input
          JSR  HEXBIN
          LDAA SHFTREG+1
          ADDA TMP4
          STAA TMP4             ; add to checksum
          PULX
          PULB
          RTS


;*******************************************
;*   MEMORY [<addr>]
;*   [<addr>]/
;* Opens memory and allows user to modify the
;*contents at <addr> or the last opened location.
;*    Subcommands:
;* [<data>]<cr> - Close current location and exit.
;* [<data>]<lf> - Close current and open next.
;* [<data>]<^> - Close current and open previous.
;* [<data>]<sp> - Close current and open next.
;* [<data>]/ - Reopen current location.
;*     The contents of the current location is only
;*  changed if valid data is entered before each
;*  subcommand.
;* [<addr>]O - Compute relative offset from current
;*     location to <addr>.  The current location must
;*     be the address of the offset byte.
;**********
;*a = wskip();
;*if(a != cr)
;*     a = buffarg();
;*     if(a != cr) return(bad argument);
;*     if(countu1 != 0) ptrmem[] = shftreg;

MEMORY   JSR  WSKIP
         BEQ  MEM1              ; jump if cr
         JSR  BUFFARG
         JSR  WSKIP
         BEQ  MSLASH            ; jump if cr
         LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
MSLASH   TST  COUNT
         BEQ  MEM1              ; jump if no argument
         LDX  SHFTREG
         STX  PTRMEM            ; update "current location"

;**********
;* Subcommands
;**********
;*outcrlf();
;*out2bsp(ptrmem[]);
;*out1bsp(ptrmem[0]);

MEM1     JSR  OUTCRLF
MEM2     LDX  #PTRMEM
         JSR  OUT2BSP           ; output address
MEM3     LDX  PTRMEM
         JSR  OUT1BSP           ; output contents
         CLR  SHFTREG
         CLR  SHFTREG+1
;*while 1
;*a = termarg();
;*     switch(a)
;*           case(space):
;*              chgbyt();
;*              ptrmem[]++;
;*           case(linefeed):
;*              chgbyt();
;*              ptrmem[]++;
;*           case(up arrow):
;*           case(backspace):
;*                 chgbyt();
;*                 ptrmem[]--;
;*           case("/"):
;*                 chgbyt();
;*                 outcrlf();
;*           case(O):
;*                 d = ptrmem[0] - (shftreg);
;*                 if($80 < d < $ff81)
;*                      print(out of range);
;*                 countt1 = d-1;
;*                 out1bsp(countt1);
;*           case(carriage return):
;*                 chgbyt();
;*                 return;
;*           default: return(command?)

MEM4     JSR  TERMARG
         JSR  UPCASE
         LDX  PTRMEM
         CMPA #$20
         BEQ  MEMSP             ; jump if space
         CMPA #$0A
         BEQ  MEMLF             ; jump if linefeed
         CMPA #$5E
         BEQ  MEMUA             ; jump if up arrow
         CMPA #$08
         BEQ  MEMBS             ; jump if backspace
         CMPA #'/'
         BEQ  MEMSL             ; jump if /
         CMPA #'O'
         BEQ  MEMOFF            ; jump if O
         CMPA #$0D
         BEQ  MEMCR             ; jump if carriage ret
         LDX  #MSG8             ; "command?"
         JSR  OUTSTRG
         JMP  MEM1
MEMSP    JSR  CHGBYT
         INX
         STX  PTRMEM
         JMP  MEM3              ; output contents
MEMLF    JSR  CHGBYT
         INX
         STX  PTRMEM
         JMP  MEM2              ; output addr, contents
MEMUA    EQU  *
MEMBS    JSR  CHGBYT
         DEX
         STX  PTRMEM
         JMP  MEM1              ; output cr, addr, contents
MEMSL    JSR  CHGBYT
         JMP  MEM1              ; output cr, addr, contents
MEMOFF   LDD  SHFTREG           ; destination addr
         SUBD PTRMEM
         CMPA #$0
         BNE  MEMOFF1           ; jump if not 0
         CMPB #$80
         BLS  MEMOFF3           ; jump if in range
         BRA  MEMOFF2           ; out of range
MEMOFF1  CMPA #$FF
         BNE  MEMOFF2           ; out of range
         CMPB #$81
         BHS  MEMOFF3           ; in range
MEMOFF2  LDX  #MSG3             ; "Too long"
         JSR  OUTSTRG
         JMP  MEM1              ; output cr, addr, contents
MEMOFF3  SUBD #$1               ; b now has offset
         STAB TMP4
         JSR  OUTSPAC
         LDX  #TMP4
         JSR  OUT1BSP           ; output offset
         JMP  MEM1              ; output cr, addr, contents
MEMCR    JSR  CHGBYT
         RTS                    ; exit task


;**********
;*   move <src1> <src2> [<dest>]  - move
;*block at <src1> to <src2> to <dest>.
;*  Moves block 1 byte up if no <dest>.
;**********
;*a = buffarg();
;*if(countu1 = 0) return(bad argument);
;*if( !wchek(a) ) return(bad argument);
;*ptr1 = shftreg;   /* src1 */

MOVE     EQU  *
         JSR  BUFFARG
         TST  COUNT
         BEQ  MOVERR            ; jump if no arg
         JSR  WCHEK
         BNE  MOVERR            ; jump if no delim
         LDX  SHFTREG           ; src1
         STX  PTR1

;*a = buffarg();
;*if(countu1 = 0) return(bad argument);
;*if( !dchek(a) ) return(bad argument);
;*ptr2 = shftreg;   /* src2 */

         JSR  BUFFARG
         TST  COUNT
         BEQ  MOVERR            ; jump if no arg
         JSR  DCHEK
         BNE  MOVERR            ; jump if no delim
         LDX  SHFTREG           ; src2
         STX  PTR2

;*a = buffarg();
;*a = wskip();
;*if(a != cr) return(bad argument);
;*if(countu1 != 0) tmp2 = shftreg;  /* dest */
;*else tmp2 = ptr1 + 1;

         JSR  BUFFARG
         JSR  WSKIP
         BNE  MOVERR            ; jump if not cr
         TST  COUNT
         BEQ  MOVE1             ; jump if no arg
         LDX  SHFTREG           ; dest
         BRA  MOVE2
MOVERR   LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS

MOVE1    LDX  PTR1
         INX                    ; default dest
MOVE2    STX  PTR3

;*if(src1 < dest <= src2)
;*     dest = dest+(src2-src1);
;*     for(x = src2; x = src1; x--)
;*           dest[0]-- = x[0]--;
         LDX  PTR3              ; dest
         CPX  PTR1              ; src1
         BLS  MOVE3             ; jump if dest =< src1
         CPX  PTR2              ; src2
         BHI  MOVE3             ; jump if dest > src2
         LDD  PTR2
         SUBD PTR1
         ADDD PTR3
         STD  PTR3              ; dest = dest+(src2-src1)
         LDX  PTR2
MOVELP1  JSR  CHKABRT           ; check for abort
         LDAA ,X                ; char at src2
         PSHX
         LDX  PTR3
         JSR  WRITE             ; write a to x
         CMPA 0,X
         BNE  MOVEBAD           ; jump if no write
         DEX
         STX  PTR3
         PULX
         CPX  PTR1
         BEQ  MOVRTS
         DEX
         BRA  MOVELP1           ; Loop SRC2 - SRC1 times
;*
;* else
;*     for(x=src1; x=src2; x++)
;*           dest[0]++ = x[0]++;


MOVE3    LDX  PTR1              ; srce1
MOVELP2  JSR  CHKABRT           ; check for abort
         LDAA ,X
         PSHX
         LDX  PTR3              ; dest
         JSR  WRITE             ; write a to x
         CMPA 0,X
         BNE  MOVEBAD           ; jump if no write
         INX
         STX  PTR3
         PULX
         CPX  PTR2
         BEQ  MOVRTS
         INX
         BRA  MOVELP2           ; Loop SRC2-SRC1 times
MOVRTS   RTS

MOVEBAD  LDX  #PTR3
         JSR  OUT2BSP           ; output bad address
         RTS

;**********
;*   register [<name>]  - prints the user regs
;*and opens them for modification.  <name> is
;*the first register opened (default = P).
;*   Subcommands:
;* [<nn>]<space>  Opens the next register.
;* [<nn>]<cr>       Return.
;*    The register value is only changed if
;*    <nn> is entered before the subcommand.
;**********
;*x[] = reglist
;*a = wskip(); a = upcase(a);
;*if(a != cr)
;*     while( a != x[0] )
;*           if( x[0] = "s") return(bad argument);
;*           x[]++;
;*     incbuff(); a = wskip();
;*     if(a != cr) return(bad argument);

REGISTER LDX  #REGLIST
         JSR  WSKIP             ; a = first char of arg
         JSR  UPCASE            ; convert to upper case
         CMPA #$D
         BEQ  REG4              ; jump if no argument
REG1     CMPA 0,X
         BEQ  REG3
         LDAB 0,X
         INX
         CMPB #'S'
         BNE  REG1              ; jump if not "s"
REG2     LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
REG3     PSHX
         JSR  INCBUFF
         JSR  WSKIP             ; next char after arg
         PULX
         BNE  REG2              ; jump if not cr

;*rprint();
;*     while(x[0] != "s")
;*           rprnt1(x);
;*           a = termarg();    /* read from terminal */
;*           if( ! dchek(a) ) return(bad argument);
;*           if(countu1 != 0)
;*                 if(x[14] = 1)
;*                      regs[x[7]++ = shftreg;
;*                 regs[x[7]] = shftreg+1;
;*           if(a = cr) break;
;*return;

REG4     JSR  RPRINT            ; print all registers
REG5     JSR  OUTCRLF
         JSR  RPRNT1            ; print reg name
         CLR  SHFTREG
         CLR  SHFTREG+1
         JSR  TERMARG           ; read subcommand
         JSR  DCHEK
         BEQ  REG6              ; jump if delimeter
         LDX  #MSG9             ; "bad argument"
         JSR  OUTSTRG
         RTS
REG6     PSHA
         PSHX
         TST  COUNT
         BEQ  REG8              ; jump if no input
         LDAB 7,X               ; get reg offset
         LDAA 14,X              ; byte size
         LDX  #REGS             ; user registers
         ABX
         TSTA
         BEQ  REG7              ; jump if 1 byte reg
         LDAA SHFTREG
         STAA 0,X               ; put in top byte
         INX
REG7     LDAA SHFTREG+1
         STAA 0,X               ; put in bottom byte
REG8     PULX
         PULA
         LDAB 0,X               ; CHECK FOR REGISTER S
         CMPB #'S'
         BEQ  REG9              ; jump if "s"
         INX                    ; point to next register
         CMPA #$D
         BNE  REG5              ; jump if not cr
REG9     RTS

PAGE1    EQU  $00               ; values for page opcodes
PAGE2    EQU  $18
PAGE3    EQU  $1A
PAGE4    EQU  $CD
IMMED    EQU  $0                ; addressing modes
INDX     EQU  $1
INDY     EQU  $2
LIMMED   EQU  $3                ; (long immediate)
OTHER    EQU  $4

;*** Rename variables for assem/disassem ***
AMODE    EQU  TMP2              ; addressing mode
YFLAG    EQU  TMP3
PNORM    EQU  TMP4              ; page for normal opcode
OLDPC    EQU  PTR8
PC       EQU  PTR1              ; program counter
PX       EQU  PTR2              ; page for x indexed
PY       EQU  PTR2+1            ; page for y indexed
BASEOP   EQU  PTR3              ; base opcode
CLASS    EQU  PTR3+1            ; class
DISPC    EQU  PTR4              ; pc for disassembler
BRADDR   EQU  PTR5              ; relative branch offset
MNEPTR   EQU  PTR6              ; pointer to table for dis
ASSCOMM  EQU  PTR7              ; subcommand for assembler

;*** Error messages for assembler ***
MSGDIR   FDB  MSGA1             ; message table index
         FDB  MSGA2
         FDB  MSGA3
         FDB  MSGA4
         FDB  MSGA5
         FDB  MSGA6
         FDB  MSGA7
         FDB  MSGA8
         FDB  MSGA9
MSGA1    FCC  "Immediate mode illegal"
         FCB  EOT
MSGA2    FCC  "Error in mnemonic table"
         FCB  EOT
MSGA3    FCC  "Illegal bit op"
         FCB  EOT
MSGA4    FCC  "Bad argument"
         FCB  EOT
MSGA5    FCC  "Mnemonic not found"
         FCB  EOT
MSGA6    FCC  "Unknown addressing mode"
         FCB  EOT
MSGA7    FCC  "Indexed addressing assumed"
         FCB  EOT
MSGA8    FCC  "Syntax error"
         FCB  EOT
MSGA9    FCC  "Branch out of range"
         FCB  EOT

;****************
;*  assem(addr) -68HC11 line assembler/disassembler.
;*        This routine will disassemble the opcode at
;*<addr> and then allow the user to enter a line for
;*assembly. Rules for assembly are as follows:
;* -A '#' sign indicates immediate addressing.
;* -A ',' (comma) indicates indexed addressing
;*        and the next character must be X or Y.
;* -All arguments are assumed to be hex and the
;*        '$' sign shouldn't be used.
;* -Arguments should be separated by 1 or more
;*        spaces or tabs.
;* -Any input after the required number of
;*        arguments is ignored.
;* -Upper or lower case makes no difference.
;*
;*        To signify end of input line, the following
;*commands are available and have the indicated action:
;*   <cr>  -Carriage return finds the next opcode for
;*           assembly.  If there was no assembly input,
;*           the next opcode disassembled is retrieved
;*           from the disassembler.
;*   <lf>  -Linefeed works the same as carriage return
;*           except if there was no assembly input, the
;*           <addr> is incremented and the next <addr> is
;*           disassembled.
;*    '^'  -Up arrow decrements <addr> and the previous
;*           address is then disassembled.
;*    '/'  -Slash redisassembles the current address.
;*
;*        To exit the assembler use CONTROL A.  Of course
;*control X and DEL will also allow you to abort.
;**********
;*oldpc = rambase;
;*a = wskip();
;*if (a != cr)
;*   buffarg()
;*   a = wskip();
;*   if ( a != cr ) return(error);
;*   oldpc = a;

ASSEM   EQU  *
        LDX  #RAMBS
        STX  OLDPC
        JSR  WSKIP
        BEQ  ASSLOOP            ; jump if no argument
        JSR  BUFFARG
        JSR  WSKIP
        BEQ  ASSEM1             ; jump if argument ok
        LDX  #MSGA4             ; "bad argument"
        JSR  OUTSTRG
        RTS
ASSEM1  LDX  SHFTREG
        STX  OLDPC

;*repeat
;*  pc = oldpc;
;*  out2bsp(pc);
;*  disassem();
;*  a=readln();
;*  asscomm = a;  /* save command */
;*  if(a == ('^' or '/')) outcrlf;
;*  if(a == 0) return(error);

ASSLOOP LDX  OLDPC
        STX  PC
        JSR  OUTCRLF
        LDX  #PC
        JSR  OUT2BSP            ; output the address
        JSR  DISASSM            ; disassemble opcode
        JSR  TABTO
        LDAA #PROMPT            ; prompt user
        JSR  OUTA               ; output prompt character
        JSR  READLN             ; read input for assembly
        STAA ASSCOMM
        CMPA #'^'
        BEQ  ASSLP0             ; jump if up arrow
        CMPA #'/'
        BEQ  ASSLP0             ; jump if slash
        CMPA #$00
        BNE  ASSLP1             ; jump if none of above
        RTS                     ; return if bad input
ASSLP0  JSR  OUTCRLF
ASSLP1  EQU  *
        JSR  OUTSPAC
        JSR  OUTSPAC
        JSR  OUTSPAC
        JSR  OUTSPAC
        JSR  OUTSPAC

;*  b = parse(input); /* get mnemonic */
;*  if(b > 5) print("not found"); asscomm='/';
;*  elseif(b >= 1)
;*     msrch();
;*     if(class==$FF)
;*         print("not found"); asscomm='/';
;*     else
;*         a = doop(opcode,class);
;*         if(a == 0) dispc=0;
;*         else process error; asscomm='/';

        JSR  PARSE
        CMPB #$5
        BLE  ASSLP2             ; jump if mnemonic <= 5 chars
        LDX  #MSGA5             ; "mnemonic not found"
        JSR  OUTSTRG
        BRA  ASSLP5
ASSLP2  EQU  *
        CMPB #$0
        BEQ  ASSLP10            ; jump if no input
        JSR  MSRCH
        LDAA CLASS
        CMPA #$FF
        BNE  ASSLP3
        LDX  #MSGA5             ; "mnemonic not found"
        JSR  OUTSTRG
        BRA  ASSLP5
ASSLP3  JSR  DOOP
        CMPA #$00
        BNE  ASSLP4             ; jump if doop error
        LDX  #$00
        STX  DISPC              ; indicate good assembly
        BRA  ASSLP10
ASSLP4  DECA                    ; a = error message index
        TAB
        LDX  #MSGDIR
        ABX
        ABX
        LDX  0,X
        JSR  OUTSTRG            ; output error message
ASSLP5  CLR  ASSCOMM            ; error command

;*  /* compute next address - asscomm holds subcommand
;*     and dispc indicates if valid assembly occured. */
;*  if(asscomm=='^') oldpc -= 1;
;*  if(asscomm==(lf or cr)
;*     if(dispc==0) oldpc=pc;
;*     else
;*         if(asscomm==lf) dispc=oldpc+1;
;*         oldpc=dispc;
;*until(eot)


ASSLP10 EQU  *
        LDAA ASSCOMM
        CMPA #'^'
        BNE  ASSLP11            ; jump if not up arrow
        LDX  OLDPC
        DEX
        STX  OLDPC              ; back up
        BRA  ASSLP15
ASSLP11 CMPA #$0A
        BEQ  ASSLP12            ; jump if linefeed
        CMPA #$0D
        BNE  ASSLP15            ; jump if not cr
ASSLP12 LDX  DISPC
        BNE  ASSLP13            ; jump if dispc != 0
        LDX  PC
        STX  OLDPC
        BRA  ASSLP15
ASSLP13 CMPA #$0A
        BNE  ASSLP14            ; jump if not linefeed
        LDX  OLDPC
        INX
        STX  DISPC
ASSLP14 LDX  DISPC
        STX  OLDPC
ASSLP15 JMP  ASSLOOP

;****************
;*  readln() --- Read input from terminal into buffer
;* until a command character is read (cr,lf,/,^).
;* If more chars are typed than the buffer will hold,
;* the extra characters are overwritten on the end.
;*  On exit: b=number of chars read, a=0 if quit,
;* else a=next command.
;****************
;*for(b==0;b<=bufflng;b++) inbuff[b] = cr;

READLN  CLRB
        LDAA #$0D               ; carriage ret
RLN0    LDX  #INBUFF
        ABX
        STAA 0,X                ; initialize input buffer
        INCB
        CMPB #BUFFLNG
        BLT  RLN0
;*b=0;
;*repeat
;*  if(a == (ctla, cntlc, cntld, cntlx, del))
;*     return(a=0);
;*  if(a == backspace)
;*     if(b > 0) b--;
;*     else b=0;
;*  else  inbuff[b] = upcase(a);
;*  if(b < bufflng) b++;
;*until (a == (cr,lf,^,/))
;*return(a);

        CLRB
RLN1    JSR  INCHAR
        CMPA #DEL               ; Delete
        BEQ  RLNQUIT
        CMPA #CTLX              ; Control X
        BEQ  RLNQUIT
        CMPA #CTLA              ; Control A
        BEQ  RLNQUIT
        CMPA #$03               ; Control C
        BEQ  RLNQUIT
        CMPA #$04               ; Control D
        BEQ  RLNQUIT
        CMPA #$08               ; backspace
        BNE  RLN2
        DECB
        BGT  RLN1
        BRA  READLN             ; start over
RLN2    LDX  #INBUFF
        ABX
        JSR  UPCASE
        STAA 0,X                ; put char in buffer
        CMPB #BUFFLNG           ; max buffer length
        BGE  RLN3               ; jump if buffer full
        INCB                    ; move buffer pointer
RLN3    JSR  ASSCHEK            ; check for subcommand
        BNE  RLN1
        RTS
RLNQUIT CLRA                    ; quit
        RTS                     ; return


;**********
;*  parse() -parse out the mnemonic from INBUFF
;* to COMBUFF. on exit: b=number of chars parsed.
;**********
;*combuff[3] = <space>;      initialize 4th character to space.
;*ptrbuff[] = inbuff[];
;*a=wskip();
;*for (b = 0; b = 5; b++)
;*   a=readbuff(); incbuff();
;*   if (a = (cr,lf,^,/,wspace)) return(b);
;*   combuff[b] = upcase(a);
;*return(b);

PARSE   LDAA #$20
        STAA COMBUFF+3
        LDX  #INBUFF            ; initialize buffer ptr
        STX  PTR0
        JSR  WSKIP              ; find first character
        CLRB
PARSLP  JSR  READBUFF           ; read character
        JSR  INCBUFF
        JSR  WCHEK
        BEQ  PARSRT             ; jump if whitespace
        JSR  ASSCHEK
        BEQ  PARSRT             ; jump if end of line
        JSR  UPCASE             ; convert to upper case
        LDX  #COMBUFF
        ABX
        STAA 0,X                ; store in combuff
        INCB
        CMPB #$5
        BLE  PARSLP             ; loop 6 times
PARSRT  RTS


;****************
;*  asschek() -perform compares for
;* cr, lf, ^, /
;****************
ASSCHEK CMPA #$0A               ; linefeed
        BEQ  ASSCHK1
        CMPA #$0D               ; carriage ret
        BEQ  ASSCHK1
        CMPA #'^'               ; up arrow
        BEQ  ASSCHK1
        CMPA #'/'               ; slash
ASSCHK1 RTS


;*********
;*  msrch() --- Search MNETABL for mnemonic in COMBUFF.
;*stores base opcode at baseop and class at class.
;*  Class = FF if not found.
;**********
;*while ( != EOF )
;*   if (COMBUFF[0-3] = MNETABL[0-3])
;*      return(MNETABL[4],MNETABL[5]);
;*   else *MNETABL =+ 6

MSRCH   LDX  #MNETABL           ; pointer to mnemonic table
        LDY  #COMBUFF           ; pointer to string
        BRA  MSRCH1
MSNEXT  EQU  *
        LDAB #6
        ABX                     ; point to next table entry
MSRCH1  LDAA 0,X                ; read table
        CMPA #EOT
        BNE  MSRCH2             ; jump if not end of table
        LDAA #$FF
        STAA CLASS              ; FF = not in table
        RTS
MSRCH2  CMPA 0,Y                ; op[0] = tabl[0] ?
        BNE  MSNEXT
        LDAA 1,X
        CMPA 1,Y                ; op[1] = tabl[1] ?
        BNE  MSNEXT
        LDAA 2,X
        CMPA 2,Y                ; op[2] = tabl[2] ?
        BNE  MSNEXT
        LDAA 3,X
        CMPA 3,Y                ; op[2] = tabl[2] ?
        BNE  MSNEXT
        LDD  4,X                ; opcode, class
        STAA BASEOP
        STAB CLASS
        RTS

;**********
;**   doop(baseop,class) --- process mnemonic.
;**   on exit: a=error code corresponding to error
;**                                         messages.
;**********
;*amode = OTHER; /* addressing mode */
;*yflag = 0;       /* ynoimm, nlimm, and cpd flag */
;*x[] = ptrbuff[]

DOOP    EQU  *
        LDAA #OTHER
        STAA AMODE              ; mode
        CLR  YFLAG
        LDX  PTR0

;*while (*x != end of buffer)
;*   if (x[0]++ == ',')
;*      if (x[0] == 'y') amode = INDY;
;*      else amod = INDX;
;*      break;
;*a = wskip()
;*if( a == '#' ) amode = IMMED;

DOPLP1  CPX  #ENDBUFF           ; (end of buffer)
        BEQ  DOOP1              ; jump if end of buffer
        LDD  0,X                ; read 2 chars from buffer
        INX                     ; move pointer
        CMPA #','
        BNE  DOPLP1
        CMPB #'Y'               ; look for ",y"
        BNE  DOPLP2
        LDAA #INDY
        STAA AMODE
        BRA  DOOP1
DOPLP2  CMPB #'X'               ; look for ",x"
        BNE  DOOP1              ; jump if not x
        LDAA #INDX
        STAA AMODE
        BRA  DOOP1
DOOP1   JSR  WSKIP
        CMPA #'#'               ; look for immediate mode
        BNE  DOOP2
        JSR  INCBUFF            ; point at argument
        LDAA #IMMED
        STAA AMODE
DOOP2   EQU  *

;*switch(class)
        LDAB CLASS
        CMPB #P2INH
        BNE  DOSW1
        JMP  DOP2I
DOSW1   CMPB #INH
        BNE  DOSW2
        JMP  DOINH
DOSW2   CMPB #REL
        BNE  DOSW3
        JMP  DOREL
DOSW3   CMPB #LIMM
        BNE  DOSW4
        JMP  DOLIM
DOSW4   CMPB #NIMM
        BNE  DOSW5
        JMP  DONOI
DOSW5   CMPB #GEN
        BNE  DOSW6
        JMP  DOGENE
DOSW6   CMPB #GRP2
        BNE  DOSW7
        JMP  DOGRP
DOSW7   CMPB #CPD
        BNE  DOSW8
        JMP  DOCPD
DOSW8   CMPB #XNIMM
        BNE  DOSW9
        JMP  DOXNOI
DOSW9   CMPB #XLIMM
        BNE  DOSW10
        JMP  DOXLI
DOSW10  CMPB #YNIMM
        BNE  DOSW11
        JMP  DOYNOI
DOSW11  CMPB #YLIMM
        BNE  DOSW12
        JMP  DOYLI
DOSW12  CMPB #BTB
        BNE  DOSW13
        JMP  DOBTB
DOSW13  CMPB #SETCLR
        BNE  DODEF
        JMP  DOSET

;*   default: return("error in mnemonic table");

DODEF   LDAA #$2
        RTS

;*  case P2INH: emit(PAGE2)

DOP2I   LDAA #PAGE2
        JSR  EMIT

;*  case INH: emit(baseop);
;*        return(0);

DOINH   LDAA BASEOP
        JSR  EMIT
        CLRA
        RTS

;*  case REL: a = assarg();
;*             if(a=4) return(a);
;*             d = address - pc + 2;
;*             if ($7f >= d >= $ff82)
;*                 return (out of range);
;*             emit(opcode);
;*             emit(offset);
;*             return(0);

DOREL   JSR  ASSARG
        CMPA #$04
        BNE  DOREL1             ; jump if arg ok
        RTS
DOREL1  LDD  SHFTREG            ; get branch address
        LDX  PC                 ; get program counter
        INX
        INX                     ; point to end of opcode
        STX  BRADDR
        SUBD BRADDR             ; calculate offset
        STD  BRADDR             ; save result
        CPD #$7F                ; in range ?
        BLS  DOREL2             ; jump if in range
        CPD #$FF80
        BHS  DOREL2             ; jump if in range
        LDAA #$09               ; 'Out of range'
        RTS
DOREL2  LDAA BASEOP
        JSR  EMIT               ; emit opcode
        LDAA BRADDR+1
        JSR  EMIT               ; emit offset
        CLRA                    ; normal return
        RTS

;*  case LIMM: if (amode == IMMED) amode = LIMMED;

DOLIM   LDAA AMODE
        CMPA #IMMED
        BNE  DONOI
        LDAA #LIMMED
        STAA AMODE

;*  case NIMM: if (amode == IMMED)
;*                  return("Immediate mode illegal");

DONOI   LDAA AMODE
        CMPA #IMMED
        BNE  DOGENE             ; jump if not immediate
        LDAA #$1                ; "immediate mode illegal"
        RTS

;*  case GEN: dogen(baseop,amode,PAGE1,PAGE1,PAGE2);
;*             return;

DOGENE  LDAA #PAGE1
        STAA PNORM
        STAA PX
        LDAA #PAGE2
        STAA PY
        JSR  DOGEN
        RTS

;*  case GRP2: if (amode == INDY)
;*                  emit(PAGE2);
;*                  amode = INDX;
;*              if( amode == INDX )
;*                  doindx(baseop);
;*              else a = assarg();
;*                  if(a=4) return(a);
;*                  emit(opcode+0x10);
;*                  emit(extended address);
;*              return;

DOGRP   LDAA AMODE
        CMPA #INDY
        BNE  DOGRP1
        LDAA #PAGE2
        JSR  EMIT
        LDAA #INDX
        STAA AMODE
DOGRP1  EQU  *
        LDAA AMODE
        CMPA #INDX
        BNE  DOGRP2
        JSR  DOINDEX
        RTS
DOGRP2  EQU  *
        LDAA BASEOP
        ADDA #$10
        JSR  EMIT
        JSR  ASSARG
        CMPA #$04
        BEQ  DOGRPRT            ; jump if bad arg
        LDD  SHFTREG            ; extended address
        JSR  EMIT
        TBA
        JSR  EMIT
        CLRA
DOGRPRT RTS

;*  case CPD: if (amode == IMMED)
;*                 amode = LIMMED; /* cpd */
;*             if( amode == INDY ) yflag = 1;
;*             dogen(baseop,amode,PAGE3,PAGE3,PAGE4);
;*             return;

DOCPD   LDAA AMODE
        CMPA #IMMED
        BNE  DOCPD1
        LDAA #LIMMED
        STAA AMODE
DOCPD1  LDAA AMODE
        CMPA #INDY
        BNE  DOCPD2
        INC  YFLAG
DOCPD2  LDAA #PAGE3
        STAA PNORM
        STAA PX
        LDAA #PAGE4
        STAA PY
        JSR  DOGEN
        RTS

;*  case XNIMM: if (amode == IMMED)  /* stx */
;*                   return("Immediate mode illegal");

DOXNOI  LDAA AMODE
        CMPA #IMMED
        BNE  DOXLI
        LDAA #$1               ; "immediate mode illegal"
        RTS

;*  case XLIMM: if (amode == IMMED)  /* cpx, ldx */
;*                   amode = LIMMED;
;*               dogen(baseop,amode,PAGE1,PAGE1,PAGE4);
;*               return;

DOXLI   LDAA AMODE
        CMPA #IMMED
        BNE  DOXLI1
        LDAA #LIMMED
        STAA AMODE
DOXLI1  LDAA #PAGE1
        STAA PNORM
        STAA PX
        LDAA #PAGE4
        STAA PY
        JSR  DOGEN
        RTS

;*  case YNIMM: if (amode == IMMED)  /* sty */
;*                   return("Immediate mode illegal");

DOYNOI  LDAA AMODE
        CMPA #IMMED
        BNE  DOYLI
        LDAA #$1                ; "immediate mode illegal"
        RTS

;*  case YLIMM: if (amode == INDY) yflag = 1;/* cpy, ldy */
;*               if(amode == IMMED) amode = LIMMED;
;*               dogen(opcode,amode,PAGE2,PAGE3,PAGE2);
;*               return;

DOYLI   LDAA AMODE
        CMPA #INDY
        BNE  DOYLI1
        INC  YFLAG
DOYLI1  CMPA #IMMED
        BNE  DOYLI2
        LDAA #LIMMED
        STAA AMODE
DOYLI2  LDAA #PAGE2
        STAA PNORM
        STAA PY
        LDAA #PAGE3
        STAA PX
        JSR  DOGEN
        RTS

;*  case BTB:          /* bset, bclr */
;*  case SETCLR: a = bitop(baseop,amode,class);
;*                 if(a=0) return(a = 3);
;*                 if( amode == INDY )
;*                    emit(PAGE2);
;*                    amode = INDX;

DOBTB   EQU  *
DOSET   JSR  BITOP
        CMPA #$00
        BNE  DOSET1
        LDAA #$3                ; "illegal bit op"
        RTS
DOSET1  LDAA AMODE
        CMPA #INDY
        BNE  DOSET2
        LDAA #PAGE2
        JSR  EMIT
        LDAA #INDX
        STAA AMODE
DOSET2  EQU  *

;*                 emit(baseop);
;*                 a = assarg();
;*                 if(a = 4) return(a);
;*                 emit(index offset);
;*                 if( amode == INDX )
;*                    Buffptr += 2;      /* skip ,x or ,y */

        LDAA BASEOP
        JSR  EMIT
        JSR  ASSARG
        CMPA #$04
        BNE  DOSET22            ; jump if arg ok
        RTS
DOSET22 LDAA SHFTREG+1          ; index offset
        JSR  EMIT
        LDAA AMODE
        CMPA #INDX
        BNE  DOSET3
        JSR  INCBUFF
        JSR  INCBUFF
DOSET3  EQU  *

;*                 a = assarg();
;*                 if(a = 4) return(a);
;*                 emit(mask);   /* mask */
;*                 if( class == SETCLR )
;*                    return;

        JSR  ASSARG
        CMPA #$04
        BNE  DOSET33            ; jump if arg ok
        RTS
DOSET33 LDAA SHFTREG+1          ; mask
        JSR  EMIT
        LDAA CLASS
        CMPA #SETCLR
        BNE  DOSET4
        CLRA
        RTS
DOSET4  EQU  *

;*                 a = assarg();
;*                 if(a = 4) return(a);
;*                 d = (pc+1) - shftreg;
;*                 if ($7f >= d >= $ff82)
;*                    return (out of range);
;*                 emit(branch offset);
;*                 return(0);

        JSR  ASSARG
        CMPA #$04
        BNE  DOSET5             ; jump if arg ok
        RTS
DOSET5  LDX  PC                 ; program counter
        INX                     ; point to next inst
        STX  BRADDR             ; save pc value
        LDD  SHFTREG            ; get branch address
        SUBD BRADDR             ; calculate offset
        CPD #$7F
        BLS  DOSET6             ; jump if in range
        CPD #$FF80
        BHS  DOSET6             ; jump if in range
        CLRA
        JSR  EMIT
        LDAA #$09               ; 'out of range'
        RTS
DOSET6  TBA                     ; offset
        JSR  EMIT
        CLRA
        RTS


;**********
;**   bitop(baseop,amode,class) --- adjust opcode on bit
;**        manipulation instructions.  Returns opcode in a
;**        or a = 0 if error
;**********
;*if( amode == INDX || amode == INDY ) return(op);
;*if( class == SETCLR ) return(op-8);
;*else if(class==BTB) return(op-12);
;*else fatal("bitop");

BITOP   EQU  *
        LDAA AMODE
        LDAB CLASS
        CMPA #INDX
        BNE  BITOP1
        RTS
BITOP1  CMPA #INDY
        BNE  BITOP2             ; jump not indexed
        RTS
BITOP2  CMPB #SETCLR
        BNE  BITOP3             ; jump not bset,bclr
        LDAA BASEOP             ; get opcode
        SUBA #8
        STAA BASEOP
        RTS
BITOP3  CMPB #BTB
        BNE  BITOP4             ; jump not bit branch
        LDAA BASEOP             ; get opcode
        SUBA #12
        STAA BASEOP
        RTS
BITOP4  CLRA                    ; 0 = fatal bitop
        RTS

;**********
;**   dogen(baseop,mode,pnorm,px,py) - process
;** general addressing modes. Returns a = error #.
;**********
;*pnorm = page for normal addressing modes: IMM,DIR,EXT
;*px = page for INDX addressing
;*py = page for INDY addressing
;*switch(amode)
DOGEN   LDAA AMODE
        CMPA #LIMMED
        BEQ  DOGLIM
        CMPA #IMMED
        BEQ  DOGIMM
        CMPA #INDY
        BEQ  DOGINDY
        CMPA #INDX
        BEQ  DOGINDX
        CMPA #OTHER
        BEQ  DOGOTH

;*default: error("Unknown Addressing Mode");

DOGDEF  LDAA #$06               ; unknown addre...
        RTS

;*case LIMMED: epage(pnorm);
;*              emit(baseop);
;*              a = assarg();
;*              if(a = 4) return(a);
;*              emit(2 bytes);
;*              return(0);

DOGLIM  LDAA PNORM
        JSR  EPAGE
DOGLIM1 LDAA BASEOP
        JSR  EMIT
        JSR  ASSARG             ; get next argument
        CMPA #$04
        BNE  DOGLIM2            ; jump if arg ok
        RTS
DOGLIM2 LDD  SHFTREG
        JSR  EMIT
        TBA
        JSR  EMIT
        CLRA
        RTS

;*case IMMED: epage(pnorm);
;*             emit(baseop);
;*             a = assarg();
;*             if(a = 4) return(a);
;*             emit(lobyte);
;*             return(0);

DOGIMM  LDAA PNORM
        JSR  EPAGE
        LDAA BASEOP
        JSR  EMIT
        JSR  ASSARG
        CMPA #$04
        BNE  DOGIMM1            ; jump if arg ok
        RTS
DOGIMM1 LDAA SHFTREG+1
        JSR  EMIT
        CLRA
        RTS

;*case INDY: epage(py);
;*            a=doindex(op+0x20);
;*            return(a);

DOGINDY LDAA PY
        JSR  EPAGE
        LDAA BASEOP
        ADDA #$20
        STAA BASEOP
        JSR  DOINDEX
        RTS

;*case INDX: epage(px);
;*            a=doindex(op+0x20);
;*            return(a);

DOGINDX LDAA PX
        JSR  EPAGE
        LDAA BASEOP
        ADDA #$20
        STAA BASEOP
        JSR  DOINDEX
        RTS

;*case OTHER: a = assarg();
;*             if(a = 4) return(a);
;*             epage(pnorm);
;*             if(countu1 <= 2 digits)   /* direct */
;*                 emit(op+0x10);
;*                 emit(lobyte(Result));
;*                 return(0);
;*             else    emit(op+0x30);    /* extended */
;*                 eword(Result);
;*                 return(0)

DOGOTH  JSR  ASSARG
        CMPA #$04
        BNE  DOGOTH0            ; jump if arg ok
        RTS
DOGOTH0 LDAA PNORM
        JSR  EPAGE
        LDAA COUNT
        CMPA #$2
        BGT  DOGOTH1
        LDAA BASEOP
        ADDA #$10               ; direct mode opcode
        JSR  EMIT
        LDAA SHFTREG+1
        JSR  EMIT
        CLRA
        RTS
DOGOTH1 LDAA BASEOP
        ADDA #$30               ; extended mode opcode
        JSR  EMIT
        LDD  SHFTREG
        JSR  EMIT
        TBA
        JSR  EMIT
        CLRA
        RTS

;**********
;**  doindex(op) --- handle all wierd stuff for
;**   indexed addressing. Returns a = error number.
;**********
;*emit(baseop);
;*a=assarg();
;*if(a = 4) return(a);
;*if( a != ',' ) return("Syntax");
;*buffptr++
;*a=readbuff()
;*if( a != 'x' &&  != 'y') warn("Ind Addr Assumed");
;*emit(lobyte);
;*return(0);

DOINDEX LDAA BASEOP
        JSR  EMIT
        JSR  ASSARG
        CMPA #$04
        BNE  DOINDX0            ; jump if arg ok
        RTS
DOINDX0 CMPA #','
        BEQ  DOINDX1
        LDAA #$08               ; "syntax error"
        RTS
DOINDX1 JSR  INCBUFF
        JSR  READBUFF
        CMPA #'Y'
        BEQ  DOINDX2
        CMPA #'X'
        BEQ  DOINDX2
        LDX  MSGA7              ; "index addr assumed"
        JSR  OUTSTRG
DOINDX2 LDAA SHFTREG+1
        JSR  EMIT
        CLRA
        RTS

;**********
;**   assarg(); - get argument.      Returns a = 4 if bad
;** argument, else a = first non hex char.
;**********
;*a = buffarg()
;*if(asschk(aa) && countu1 != 0) return(a);
;*return(bad argument);

ASSARG  JSR  BUFFARG
        JSR  ASSCHEK            ; check for command
        BEQ  ASSARG1            ; jump if ok
        JSR  WCHEK              ; check for whitespace
        BNE  ASSARG2            ; jump if not ok
ASSARG1 TST  COUNT
        BEQ  ASSARG2            ; jump if no argument
        RTS
ASSARG2 LDAA #$04               ; bad argument
        RTS

;**********
;**  epage(a) --- emit page prebyte
;**********
;*if( a != PAGE1 ) emit(a);

EPAGE   CMPA #PAGE1
        BEQ  EPAGRT             ; jump if page 1
        JSR  EMIT
EPAGRT  RTS

;**********
;*   emit(a) --- emit contents of a
;**********
EMIT    LDX  PC
        JSR  WRITE              ; write a to x
        JSR  OUT1BSP
        STX  PC
        RTS

;*Mnemonic table for hc11 line assembler
NULL     EQU  $0                ; nothing
INH      EQU  $1                ; inherent
P2INH    EQU  $2                ; page 2 inherent
GEN      EQU  $3                ; general addressing
GRP2     EQU  $4                ; group 2
REL      EQU  $5                ; relative
IMM      EQU  $6                ; immediate
NIMM     EQU  $7                ; general except for immediate
LIMM     EQU  $8                ; 2 byte immediate
XLIMM    EQU  $9                ; longimm for x
XNIMM    EQU  $10               ; no immediate for x
YLIMM    EQU  $11               ; longimm for y
YNIMM    EQU  $12               ; no immediate for y
BTB      EQU  $13               ; bit test and branch
SETCLR   EQU  $14               ; bit set or clear
CPD      EQU  $15               ; compare d
BTBD     EQU  $16               ; bit test and branch direct
SETCLRD  EQU  $17               ; bit set or clear direct

;**********
;*   mnetabl - includes all '11 mnemonics, base opcodes,
;* and type of instruction.  The assembler search routine
;*depends on 4 characters for each mnemonic so that 3 char
;*mnemonics are extended with a space and 5 char mnemonics
;*are truncated.
;**********

MNETABL EQU  *
        FCC  "ABA "             ; Mnemonic
        FCB  $1B                ; Base opcode
        FCB  INH                ; Class
        FCC  "ABX "
        FCB  $3A
        FCB  INH
        FCC  "ABY "
        FCB  $3A
        FCB  P2INH
        FCC  "ADCA"
        FCB  $89
        FCB  GEN
        FCC  "ADCB"
        FCB  $C9
        FCB  GEN
        FCC  "ADDA"
        FCB  $8B
        FCB  GEN
        FCC  "ADDB"
        FCB  $CB
        FCB  GEN
        FCC  "ADDD"
        FCB  $C3
        FCB  LIMM
        FCC  "ANDA"
        FCB  $84
        FCB  GEN
        FCC  "ANDB"
        FCB  $C4
        FCB  GEN
        FCC  "ASL "
        FCB  $68
        FCB  GRP2
        FCC  "ASLA"
        FCB  $48
        FCB  INH
        FCC  "ASLB"
        FCB  $58
        FCB  INH
        FCC  "ASLD"
        FCB  $05
        FCB  INH
        FCC  "ASR "
        FCB  $67
        FCB  GRP2
        FCC  "ASRA"
        FCB  $47
        FCB  INH
        FCC  "ASRB"
        FCB  $57
        FCB  INH
        FCC  "BCC "
        FCB  $24
        FCB  REL
        FCC  "BCLR"
        FCB  $1D
        FCB  SETCLR
        FCC  "BCS "
        FCB  $25
        FCB  REL
        FCC  "BEQ "
        FCB  $27
        FCB  REL
        FCC  "BGE "
        FCB  $2C
        FCB  REL
        FCC  "BGT "
        FCB  $2E
        FCB  REL
        FCC  "BHI "
        FCB  $22
        FCB  REL
        FCC  "BHS "
        FCB  $24
        FCB  REL
        FCC  "BITA"
        FCB  $85
        FCB  GEN
        FCC  "BITB"
        FCB  $C5
        FCB  GEN
        FCC  "BLE "
        FCB  $2F
        FCB  REL
        FCC  "BLO "
        FCB  $25
        FCB  REL
        FCC  "BLS "
        FCB  $23
        FCB  REL
        FCC  "BLT "
        FCB  $2D
        FCB  REL
        FCC  "BMI "
        FCB  $2B
        FCB  REL
        FCC  "BNE "
        FCB  $26
        FCB  REL
        FCC  "BPL "
        FCB  $2A
        FCB  REL
        FCC  "BRA "
        FCB  $20
        FCB  REL
        FCC  "BRCL"             ; (BRCLR)
        FCB  $1F
        FCB  BTB
        FCC  "BRN "
        FCB  $21
        FCB  REL
        FCC  "BRSE"             ; (BRSET)
        FCB  $1E
        FCB  BTB
        FCC  "BSET"
        FCB  $1C
        FCB  SETCLR
        FCC  "BSR "
        FCB  $8D
        FCB  REL
        FCC  "BVC "
        FCB  $28
        FCB  REL
        FCC  "BVS "
        FCB  $29
        FCB  REL
        FCC  "CBA "
        FCB  $11
        FCB  INH
        FCC  "CLC "
        FCB  $0C
        FCB  INH
        FCC  "CLI "
        FCB  $0E
        FCB  INH
        FCC  "CLR "
        FCB  $6F
        FCB  GRP2
        FCC  "CLRA"
        FCB  $4F
        FCB  INH
        FCC  "CLRB"
        FCB  $5F
        FCB  INH
        FCC  "CLV "
        FCB  $0A
        FCB  INH
        FCC  "CMPA"
        FCB  $81
        FCB  GEN
        FCC  "CMPB"
        FCB  $C1
        FCB  GEN
        FCC  "COM "
        FCB  $63
        FCB  GRP2
        FCC  "COMA"
        FCB  $43
        FCB  INH
        FCC  "COMB"
        FCB  $53
        FCB  INH
        FCC  "CPD "
        FCB  $83
        FCB  CPD
        FCC  "CPX "
        FCB  $8C
        FCB  XLIMM
        FCC  "CPY "
        FCB  $8C
        FCB  YLIMM
        FCC  "DAA "
        FCB  $19
        FCB  INH
        FCC  "DEC "
        FCB  $6A
        FCB  GRP2
        FCC  "DECA"
        FCB  $4A
        FCB  INH
        FCC  "DECB"
        FCB  $5A
        FCB  INH
        FCC  "DES "
        FCB  $34
        FCB  INH
        FCC  "DEX "
        FCB  $09
        FCB  INH
        FCC  "DEY "
        FCB  $09
        FCB  P2INH
        FCC  "EORA"
        FCB  $88
        FCB  GEN
        FCC  "EORB"
        FCB  $C8
        FCB  GEN
        FCC  "FDIV"
        FCB  $03
        FCB  INH
        FCC  "IDIV"
        FCB  $02
        FCB  INH
        FCC  "INC "
        FCB  $6C
        FCB  GRP2
        FCC  "INCA"
        FCB  $4C
        FCB  INH
        FCC  "INCB"
        FCB  $5C
        FCB  INH
        FCC  "INS "
        FCB  $31
        FCB  INH
        FCC  "INX "
        FCB  $08
        FCB  INH
        FCC  "INY "
        FCB  $08
        FCB  P2INH
        FCC  "JMP "
        FCB  $6E
        FCB  GRP2
        FCC  "JSR "
        FCB  $8D
        FCB  NIMM
        FCC  "LDAA"
        FCB  $86
        FCB  GEN
        FCC  "LDAB"
        FCB  $C6
        FCB  GEN
        FCC  "LDD "
        FCB  $CC
        FCB  LIMM
        FCC  "LDS "
        FCB  $8E
        FCB  LIMM
        FCC  "LDX "
        FCB  $CE
        FCB  XLIMM
        FCC  "LDY "
        FCB  $CE
        FCB  YLIMM
        FCC  "LSL "
        FCB  $68
        FCB  GRP2
        FCC  "LSLA"
        FCB  $48
        FCB  INH
        FCC  "LSLB"
        FCB  $58
        FCB  INH
        FCC  "LSLD"
        FCB  $05
        FCB  INH
        FCC  "LSR "
        FCB  $64
        FCB  GRP2
        FCC  "LSRA"
        FCB  $44
        FCB  INH
        FCC  "LSRB"
        FCB  $54
        FCB  INH
        FCC  "LSRD"
        FCB  $04
        FCB  INH
        FCC  "MUL "
        FCB  $3D
        FCB  INH
        FCC  "NEG "
        FCB  $60
        FCB  GRP2
        FCC  "NEGA"
        FCB  $40
        FCB  INH
        FCC  "NEGB"
        FCB  $50
        FCB  INH
        FCC  "NOP "
        FCB  $01
        FCB  INH
        FCC  "ORAA"
        FCB  $8A
        FCB  GEN
        FCC  "ORAB"
        FCB  $CA
        FCB  GEN
        FCC  "PSHA"
        FCB  $36
        FCB  INH
        FCC  "PSHB"
        FCB  $37
        FCB  INH
        FCC  "PSHX"
        FCB  $3C
        FCB  INH
        FCC  "PSHY"
        FCB  $3C
        FCB  P2INH
        FCC  "PULA"
        FCB  $32
        FCB  INH
        FCC  "PULB"
        FCB  $33
        FCB  INH
        FCC  "PULX"
        FCB  $38
        FCB  INH
        FCC  "PULY"
        FCB  $38
        FCB  P2INH
        FCC  "ROL "
        FCB  $69
        FCB  GRP2
        FCC  "ROLA"
        FCB  $49
        FCB  INH
        FCC  "ROLB"
        FCB  $59
        FCB  INH
        FCC  "ROR "
        FCB  $66
        FCB  GRP2
        FCC  "RORA"
        FCB  $46
        FCB  INH
        FCC  "RORB"
        FCB  $56
        FCB  INH
        FCC  "RTI "
        FCB  $3B
        FCB  INH
        FCC  "RTS "
        FCB  $39
        FCB  INH
        FCC  "SBA "
        FCB  $10
        FCB  INH
        FCC  "SBCA"
        FCB  $82
        FCB  GEN
        FCC  "SBCB"
        FCB  $C2
        FCB  GEN
        FCC  "SEC "
        FCB  $0D
        FCB  INH
        FCC  "SEI "
        FCB  $0F
        FCB  INH
        FCC  "SEV "
        FCB  $0B
        FCB  INH
        FCC  "STAA"
        FCB  $87
        FCB  NIMM
        FCC  "STAB"
        FCB  $C7
        FCB  NIMM
        FCC  "STD "
        FCB  $CD
        FCB  NIMM
        FCC  "STOP"
        FCB  $CF
        FCB  INH
        FCC  "STS "
        FCB  $8F
        FCB  NIMM
        FCC  "STX "
        FCB  $CF
        FCB  XNIMM
        FCC  "STY "
        FCB  $CF
        FCB  YNIMM
        FCC  "SUBA"
        FCB  $80
        FCB  GEN
        FCC  "SUBB"
        FCB  $C0
        FCB  GEN
        FCC  "SUBD"
        FCB  $83
        FCB  LIMM
        FCC  "SWI "
        FCB  $3F
        FCB  INH
        FCC  "TAB "
        FCB  $16
        FCB  INH
        FCC  "TAP "
        FCB  $06
        FCB  INH
        FCC  "TBA "
        FCB  $17
        FCB  INH
        FCC  "TPA "
        FCB  $07
        FCB  INH
        FCC  "TEST"
        FCB  $00
        FCB  INH
        FCC  "TST "
        FCB  $6D
        FCB  GRP2
        FCC  "TSTA"
        FCB  $4D
        FCB  INH
        FCC  "TSTB"
        FCB  $5D
        FCB  INH
        FCC  "TSX "
        FCB  $30
        FCB  INH
        FCC  "TSY "
        FCB  $30
        FCB  P2INH
        FCC  "TXS "
        FCB  $35
        FCB  INH
        FCC  "TYS "
        FCB  $35
        FCB  P2INH
        FCC  "WAI "
        FCB  $3E
        FCB  INH
        FCC  "XGDX"
        FCB  $8F
        FCB  INH
        FCC  "XGDY"
        FCB  $8F
        FCB  P2INH
        FCC  "BRSE"             ; bit direct modes for
        FCB  $12                ; disassembler.
        FCB  BTBD
        FCC  "BRCL"
        FCB  $13
        FCB  BTBD
        FCC  "BSET"
        FCB  $14
        FCB  SETCLRD
        FCC  "BCLR"
        FCB  $15
        FCB  SETCLRD
        FCB  EOT                ; End of table

;**********************************************
PG1      EQU      $0
PG2      EQU      $1
PG3      EQU      $2
PG4      EQU      $3

;******************
;*disassem() - disassemble the opcode.
;******************
;*(check for page prebyte)
;*baseop=pc[0];
;*pnorm=PG1;
;*if(baseop==$18) pnorm=PG2;
;*if(baseop==$1A) pnorm=PG3;
;*if(baseop==$CD) pnorm=PG4;
;*if(pnorm != PG1) dispc=pc+1;
;*else dispc=pc; (dispc points to next byte)

DISASSM EQU  *
        LDX  PC                 ; address
        LDAA 0,X                ; opcode
        LDAB #PG1
        CMPA #$18
        BEQ  DISP2              ; jump if page2
        CMPA #$1A
        BEQ  DISP3              ; jump if page3
        CMPA #$CD
        BNE  DISP1              ; jump if not page4
DISP4   INCB                    ; set up page value
DISP3   INCB
DISP2   INCB
        INX
DISP1   STX  DISPC              ; point to opcode
        STAB PNORM              ; save page

;*If(opcode == ($00-$5F or $8D or $8F or $CF))
;*  if(pnorm == (PG3 or PG4))
;*      disillop(); return();
;*  b=disrch(opcode,NULL);
;*  if(b==0) disillop(); return();

        LDAA 0,X                ; get current opcode
        STAA BASEOP
        INX
        STX  DISPC              ; point to next byte
        CMPA #$5F
        BLS  DIS1               ; jump if in range
        CMPA #$8D
        BEQ  DIS1               ; jump if bsr
        CMPA #$8F
        BEQ  DIS1               ; jump if xgdx
        CMPA #$CF
        BEQ  DIS1               ; jump if stop
        JMP  DISGRP             ; try next part of map
DIS1    LDAB PNORM
        CMPB #PG3
        BLO  DIS2               ; jump if page 1 or 2
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DIS2    LDAB BASEOP             ; opcode
        CLRB                    ; class=null
        JSR  DISRCH
        TSTB
        BNE  DISPEC             ; jump if opcode found
        JSR  DISILLOP           ; "illegal opcode"
        RTS

;*   if(opcode==$8D) dissrch(opcode,REL);
;*   if(opcode==($8F or $CF)) disrch(opcode,INH);

DISPEC  LDAA BASEOP
        CMPA #$8D
        BNE  DISPEC1
        LDAB #REL
        BRA  DISPEC3            ; look for BSR opcode
DISPEC1 CMPA #$8F
        BEQ  DISPEC2            ; jump if XGDX opcode
        CMPA #$CF
        BNE  DISINH             ; jump not STOP opcode
DISPEC2 LDAB #INH
DISPEC3 JSR  DISRCH             ; find other entry in table

;*   if(class==INH)              /* INH */
;*      if(pnorm==PG2)
;*          b=disrch(baseop,P2INH);
;*          if(b==0) disillop(); return();
;*      prntmne();
;*      return();

DISINH  EQU  *
        LDAB CLASS
        CMPB #INH
        BNE  DISREL             ; jump if not inherent
        LDAB PNORM
        CMPB #PG1
        BEQ  DISINH1            ; jump if page1
        LDAA BASEOP             ; get opcode
        LDAB #P2INH             ; class=p2inh
        JSR  DISRCH
        TSTB
        BNE  DISINH1            ; jump if found
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DISINH1 JSR  PRNTMNE
        RTS

;*   elseif(class=REL)          /* REL */
;*      if(pnorm != PG1)
;*          disillop(); return();
;*      prntmne();
;*      disrelad();
;*      return();

DISREL  EQU  *
        LDAB CLASS
        CMPB #REL
        BNE  DISBTD
        TST  PNORM
        BEQ  DISREL1            ; jump if page1
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DISREL1 JSR  PRNTMNE            ; output mnemonic
        JSR  DISRELAD           ; compute relative address
        RTS

;*   else    /* SETCLR,SETCLRD,BTB,BTBD */
;*      if(class == (SETCLRD or BTBD))
;*          if(pnorm != PG1)
;*             disillop(); return();   /* illop */
;*          prntmne();             /* direct */
;*          disdir();             /* output $byte */
;*      else (class == (SETCLR or BTB))
;*          prntmne();             /* indexed */
;*          disindx();
;*      outspac();
;*      disdir();
;*      outspac();
;*      if(class == (BTB or BTBD))
;*          disrelad();
;*   return();

DISBTD  EQU  *
        LDAB CLASS
        CMPB #SETCLRD
        BEQ  DISBTD1
        CMPB #BTBD
        BNE  DISBIT             ; jump not direct bitop
DISBTD1 TST  PNORM
        BEQ  DISBTD2            ; jump if page 1
        JSR  DISILLOP
        RTS
DISBTD2 JSR  PRNTMNE
        JSR  DISDIR             ; operand(direct)
        BRA  DISBIT1
DISBIT  EQU  *
        JSR  PRNTMNE
        JSR  DISINDX            ; operand(indexed)
DISBIT1 JSR  OUTSPAC
        JSR  DISDIR             ; mask
        LDAB CLASS
        CMPB #BTB
        BEQ  DISBIT2            ; jump if btb
        CMPB #BTBD
        BNE  DISBIT3            ; jump if not bit branch
DISBIT2 JSR  DISRELAD           ; relative address
DISBIT3 RTS


;*Elseif($60 <= opcode <= $7F)  /*  GRP2 */
;*   if(pnorm == (PG3 or PG4))
;*      disillop(); return();
;*   if((pnorm==PG2) and (opcode != $6x))
;*      disillop(); return();
;*   b=disrch(baseop & $6F,NULL);
;*   if(b==0) disillop(); return();
;*   prntmne();
;*   if(opcode == $6x)
;*      disindx();
;*   else
;*      disext();
;*   return();

DISGRP  EQU  *
        CMPA #$7F               ; a=opcode
        BHI  DISNEXT            ; try next part of map
        LDAB PNORM
        CMPB #PG3
        BLO  DISGRP2            ; jump if page 1 or 2
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DISGRP2 ANDA #$6F               ; mask bit 4
        CLRB                    ; class=null
        JSR  DISRCH
        TSTB
        BNE  DISGRP3            ; jump if found
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DISGRP3 JSR  PRNTMNE
        LDAA BASEOP             ; get opcode
        ANDA #$F0
        CMPA #$60
        BNE  DISGRP4            ; jump if not 6x
        JSR  DISINDX            ; operand(indexed)
        RTS
DISGRP4 JSR  DISEXT             ; operand(extended)
        RTS

;*Else  ($80 <= opcode <= $FF)
;*   if(opcode == ($87 or $C7))
;*      disillop(); return();
;*   b=disrch(opcode&$CF,NULL);
;*   if(b==0) disillop(); return();

DISNEXT EQU  *
        CMPA #$87               ; a=opcode
        BEQ  DISNEX1
        CMPA #$C7
        BNE  DISNEX2
DISNEX1 JSR  DISILLOP           ; "illegal opcode"
        RTS
DISNEX2 ANDA #$CF
        CLRB                    ; class=null
        JSR  DISRCH
        TSTB
        BNE  DISNEW             ; jump if mne found
        JSR  DISILLOP           ; "illegal opcode"
        RTS

;*   if(opcode&$CF==$8D) disrch(baseop,NIMM; (jsr)
;*   if(opcode&$CF==$8F) disrch(baseop,NIMM; (sts)
;*   if(opcode&$CF==$CF) disrch(baseop,XNIMM; (stx)
;*   if(opcode&$CF==$83) disrch(baseop,LIMM); (subd)

DISNEW  LDAA BASEOP
        ANDA #$CF
        CMPA #$8D
        BNE  DISNEW1            ; jump not jsr
        LDAB #NIMM
        BRA  DISNEW4
DISNEW1 CMPA #$8F
        BNE  DISNEW2            ; jump not sts
        LDAB #NIMM
        BRA  DISNEW4
DISNEW2 CMPA #$CF
        BNE  DISNEW3            ; jump not stx
        LDAB #XNIMM
        BRA  DISNEW4
DISNEW3 CMPA #$83
        BNE  DISGEN             ; jump not subd
        LDAB #LIMM
DISNEW4 JSR  DISRCH
        TSTB
        BNE  DISGEN             ; jump if found
        JSR  DISILLOP           ; "illegal opcode"
        RTS

;*   if(class == (GEN or NIMM or LIMM   ))   /* GEN,NIMM,LIMM,CPD */
;*      if(opcode&$CF==$83)
;*          if(pnorm==(PG3 or PG4)) disrch(opcode#$CF,CPD)
;*          class=LIMM;
;*      if((pnorm == (PG2 or PG4) and (opcode != ($Ax or $Ex)))
;*          disillop(); return();
;*      disgenrl();
;*      return();

DISGEN  LDAB CLASS              ; get class
        CMPB #GEN
        BEQ  DISGEN1
        CMPB #NIMM
        BEQ  DISGEN1
        CMPB #LIMM
        BNE  DISXLN             ; jump if other class
DISGEN1 LDAA BASEOP
        ANDA #$CF
        CMPA #$83
        BNE  DISGEN3            ; jump if not #$83
        LDAB PNORM
        CMPB #PG3
        BLO  DISGEN3            ; jump not pg3 or 4
        LDAB #CPD
        JSR  DISRCH             ; look for cpd mne
        LDAB #LIMM
        STAB CLASS              ; set class to limm
DISGEN3 LDAB PNORM
        CMPB #PG2
        BEQ  DISGEN4            ; jump if page 2
        CMPB #PG4
        BNE  DISGEN5            ; jump not page 2 or 4
DISGEN4 LDAA BASEOP
        ANDA #$B0               ; mask bits 6,3-0
        CMPA #$A0
        BEQ  DISGEN5            ; jump if $Ax or $Ex
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DISGEN5 JSR  DISGENRL           ; process general class
        RTS

;*   else       /* XLIMM,XNIMM,YLIMM,YNIMM */
;*      if(pnorm==(PG2 or PG3))
;*          if(class==XLIMM) disrch(opcode&$CF,YLIMM);
;*          else disrch(opcode&$CF,YNIMM);
;*      if((pnorm == (PG3 or PG4))
;*          if(opcode != ($Ax or $Ex))
;*             disillop(); return();
;*      class=LIMM;
;*      disgen();
;*   return();

DISXLN  LDAB PNORM
        CMPB #PG2
        BEQ  DISXLN1            ; jump if page2
        CMPB #PG3
        BNE  DISXLN4            ; jump not page3
DISXLN1 LDAA BASEOP
        ANDA #$CF
        LDAB CLASS
        CMPB #XLIMM
        BNE  DISXLN2
        LDAB #YLIMM
        BRA  DISXLN3            ; look for ylimm
DISXLN2 LDAB #YNIMM             ; look for ynimm
DISXLN3 JSR  DISRCH
DISXLN4 LDAB PNORM
        CMPB #PG3
        BLO  DISXLN5            ; jump if page 1 or 2
        LDAA BASEOP             ; get opcode
        ANDA #$B0               ; mask bits 6,3-0
        CMPA #$A0
        BEQ  DISXLN5            ; jump opcode = $Ax or $Ex
        JSR  DISILLOP           ; "illegal opcode"
        RTS
DISXLN5 LDAB #LIMM
        STAB CLASS
        JSR  DISGENRL           ; process general class
        RTS


;******************
;*disrch(a=opcode,b=class)
;*return b=0 if not found
;*  else mneptr=points to mnemonic
;*         class=class of opcode
;******************
;*x=#MNETABL
;*while(x[0] != eot)
;*   if((opcode==x[4]) && ((class=NULL) || (class=x[5])))
;*      mneptr=x;
;*      class=x[5];
;*      return(1);
;*   x += 6;
;*return(0);        /* not found */

DISRCH  EQU  *
        LDX  #MNETABL           ; point to top of table
DISRCH1 CMPA 4,X                ; test opcode
        BNE  DISRCH3            ; jump not this entry
        TSTB
        BEQ  DISRCH2            ; jump if class=null
        CMPB 5,X                ; test class
        BNE  DISRCH3            ; jump not this entry
DISRCH2 LDAB 5,X
        STAB CLASS
        STX  MNEPTR             ; return ptr to mnemonic
        INCB
        RTS                     ; return found
DISRCH3 PSHB                    ; save class
        LDAB #6
        ABX
        LDAB 0,X
        CMPB #EOT               ; test end of table
        PULB
        BNE  DISRCH1
        CLRB
        RTS                     ; return not found

;******************
;*prntmne() - output the mnemonic pointed
;*at by mneptr.
;******************
;*outa(mneptr[0-3]);
;*outspac;
;*return();

PRNTMNE EQU  *
        LDX  MNEPTR
        LDAA 0,X
        JSR  OUTA               ; output char1
        LDAA 1,X
        JSR  OUTA               ; output char2
        LDAA 2,X
        JSR  OUTA               ; output char3
        LDAA 3,X
        JSR  OUTA               ; output char4
        JSR  OUTSPAC
        RTS

;******************
;*disindx() - process indexed mode
;******************
;*disdir();
;*outa(',');
;*if(pnorm == (PG2 or PG4)) outa('Y');
;*else outa('X');
;*return();

DISINDX EQU  *
        JSR  DISDIR             ; output $byte
        LDAA #','
        JSR  OUTA               ; output ,
        LDAB PNORM
        CMPB #PG2
        BEQ  DISIND1            ; jump if page2
        CMPB #PG4
        BNE  DISIND2            ; jump if not page4
DISIND1 LDAA #'Y'
        BRA DISIND3
DISIND2 LDAA #'X'
DISIND3 JSR  OUTA               ; output x or y
        RTS

;******************
;*disrelad() - compute and output relative address.
;******************
;* braddr = dispc[0] + (dispc++);( 2's comp arith)
;*outa('$');
;*out2bsp(braddr);
;*return();

DISRELAD EQU *
        LDX  DISPC
        LDAB 0,X                ; get relative offset
        INX
        STX  DISPC
        TSTB
        BMI  DISRLD1            ; jump if negative
        ABX
        BRA  DISRLD2
DISRLD1 DEX
        INCB
        BNE  DISRLD1            ; subtract
DISRLD2 STX  BRADDR             ; save address
        JSR  OUTSPAC
        LDAA #'$'
        JSR  OUTA
        LDX  #BRADDR
        JSR  OUT2BSP            ; output address
        RTS


;******************
;*disgenrl() - output data for the general cases which
;*includes immediate, direct, indexed, and extended modes.
;******************
;*prntmne();
;*if(baseop == ($8x or $Cx))   /* immediate */
;*   outa('#');
;*   disdir();
;*   if(class == LIMM)
;*      out1byt(dispc++);
;*elseif(baseop == ($9x or $Dx))  /* direct */
;*   disdir();
;*elseif(baseop == ($Ax or $Ex)) /* indexed */
;*   disindx();
;*else  (baseop == ($Bx or $Fx)) /* extended */
;*   disext();
;*return();

DISGENRL EQU *
        JSR  PRNTMNE            ; print mnemonic
        LDAA BASEOP             ; get opcode
        ANDA #$B0               ;  mask bits 6,3-0
        CMPA #$80
        BNE  DISGRL2            ; jump if not immed
        LDAA #'#'               ; do immediate
        JSR  OUTA
        JSR  DISDIR
        LDAB CLASS
        CMPB #LIMM
        BEQ  DISGRL1            ; jump class = limm
        RTS
DISGRL1 LDX  DISPC
        JSR  OUT1BYT
        STX  DISPC
        RTS
DISGRL2 CMPA #$90
        BNE  DISGRL3            ; jump not direct
        JSR  DISDIR             ; do direct
        RTS
DISGRL3 CMPA #$A0
        BNE  DISGRL4            ; jump not indexed
        JSR  DISINDX            ; do extended
        RTS
DISGRL4 JSR  DISEXT             ; do extended
        RTS

;*****************
;*disdir() - output "$ next byte"
;*****************
DISDIR  EQU  *
        LDAA #'$'
        JSR  OUTA
        LDX  DISPC
        JSR  OUT1BYT
        STX  DISPC
        RTS

;*****************
;*disext() - output "$ next 2 bytes"
;*****************
DISEXT  EQU  *
        LDAA #'$'
        JSR  OUTA
        LDX  DISPC
        JSR  OUT2BSP
        STX  DISPC
        RTS


;*****************
;*disillop() - output "illegal opcode"
;*****************
DISMSG1 FCC  "ILLOP"
        FCB  EOT
DISILLOP EQU *
        PSHX
        LDX  #DISMSG1
        JSR  OUTSTRG0           ; no cr
        PULX
        RTS

;* Equates
JPORTD   EQU   $08
JDDRD    EQU   $09
JBAUD    EQU   $2B
JSCCR1   EQU   $2C
JSCCR2   EQU   $2D
JSCSR    EQU   $2E
JSCDAT   EQU   $2F
;*

;************
;*  xboot [<addr1> [<addr2>]] - Use SCI to talk to an 'hc11 in
;* boot mode.  Downloads bytes from addr1 thru addr2.
;* Default addr1 = $C000 and addr2 = $C0ff.
;*
;* IMPORTANT:
;* if talking to an 'A8 or 'A2: use either default addresses or ONLY
;*    addr1 - this sends 256 bytes
;* if talking to an 'E9: include BOTH addr1 and addr2 for variable
;*    length
;************

;*Get arguments
;*If no args, default $C000
BOOT    JSR   WSKIP
        BNE   BOT1              ; jump if arguments
        LDX   #$C0FF            ; addr2 default
        STX   PTR5
        LDY   #$C000            ; addr1 default
        BRA   BOT2              ; go - use default address

;*Else get arguments
BOT1    JSR   BUFFARG
        TST   COUNT
        BEQ   BOTERR            ; jump if no address
        LDY   SHFTREG           ; start address (addr1)
        JSR   WSKIP
        BNE   BOT1A             ; go get addr2
        STY   PTR5              ; default addr2...
        LDD   PTR5              ; ...by taking addr1...
        ADDD  #$FF              ; ...and adding 255 to it...
        STD   PTR5              ; ...for a total download of 256
        BRA   BOT2              ; continue
;*
BOT1A   JSR   BUFFARG
        TST   COUNT
        BEQ   BOTERR            ; jump if no address
        LDX   SHFTREG           ; end address (addr2)
        STX   PTR5
        JSR   WSKIP
        BNE   BOTERR            ; go use addr1 and addr2
        BRA   BOT2

;*
BOTERR  LDX   #MSG9             ; "bad argument"
        JSR   OUTSTRG
        RTS

;*Boot routine
BOT2    LDAB  #$FF              ; control character ($ff -> download)
        JSR   BTSUB             ; set up SCI and send control char
;*                                initializes X as register pointer
;*Download block
BLOP    LDAA  0,Y
        STAA  JSCDAT,X          ; write to transmitter
        BRCLR JSCSR,X,#80,*     ; wait for TDRE
        CPY   PTR5              ; if last...
        BEQ   BTDONE            ; ...quit
        INY                     ; else...
        BRA   BLOP              ; ...send next
BTDONE  RTS

;************************************************
;*Subroutine
;*  btsub   - sets up SCI and outputs control character
;* On entry, B = control character
;* On exit,  X = $1000
;*            A = $0C
;***************************

BTSUB   EQU   *
        LDX   #$1000            ; to use indexed addressing
        LDAA  #$02
        STAA  JPORTD,X          ; drive transmitter line
        STAA  JDDRD,X           ; high
        CLR   JSCCR2,X          ; turn off XMTR and RCVR
        LDAA  #$22              ; BAUD = /16
        STAA  JBAUD,X
        LDAA  #$0C              ; TURN ON XMTR & RCVR
        STAA  JSCCR2,X
        STAB  JSCDAT,X
        BRCLR JSCSR,X,#80,*     ; wait for TDRE
        RTS

;******************
;*
;*        EVBTEST - This routine makes it a little easier
;*        on us to test this board.
;*
;******************

EVBTEST  LDAA  #$FF
         STAA  $1000            ; Write ones to port A
         CLR  AUTOLF            ; Turn off auto lf
         JSR  HOSTCO            ; Connect host
         JSR  HOSTINIT          ; Initialize host
         LDAA #$7f
         JSR  HOSTOUT           ; Send Delete to Altos
         LDAA #$0d
         JSR  HOSTOUT           ; Send <CR>
         INC  AUTOLF            ; Turn on Auto LF
         LDX  #INBUFF+5         ; Point at Load message
         STX  PTR0              ; Set pointer for load command
         LDY  #MSGEVB           ; Point at cat line
LOOP     LDAA 0,Y               ; Loop to xfer command line
         CMPA #04               ; Into buffalo line buffer
         BEQ  DONE              ; Quit on $04
         STAA 0,X
         INX                    ; next character
         INY
         BRA  LOOP
DONE     CLR  TMP2              ; Set load vs. verify
         JSR  LOAD3             ; Jmp into middle of load
         LDS  #STACK            ; Reset Stack
         JMP  $C0B3             ; Jump to Downloaded code

MSGEVB   FCC  "cat evbtest.out"
         FCB  $0D
         FCB  $04



;*** Jump table ***
        ORG      ROMBS+$1F7C
.WARMST JMP       MAIN
.BPCLR  JMP       BPCLR
.RPRINT JMP       RPRINT
.HEXBIN JMP       HEXBIN
.BUFFAR JMP       BUFFARG
.TERMAR JMP       TERMARG
.CHGBYT JMP       CHGBYT
.READBU JMP       READBUFF
.INCBUF JMP       INCBUFF
.DECBUF JMP       DECBUFF
.WSKIP  JMP       WSKIP
.CHKABR JMP       CHKABRT

        ORG      ROMBS+$1FA0
.UPCASE JMP       UPCASE
.WCHEK  JMP       WCHEK
.DCHEK  JMP       DCHEK
.INIT   JMP       INIT
.INPUT  JMP       INPUT
.OUTPUT JMP       OUTPUT
.OUTLHL JMP       OUTLHLF
.OUTRHL JMP       OUTRHLF
.OUTA   JMP       OUTA
.OUT1BY JMP       OUT1BYT
.OUT1BS JMP       OUT1BSP
.OUT2BS JMP       OUT2BSP
.OUTCRL JMP       OUTCRLF
.OUTSTR JMP       OUTSTRG
.OUTST0 JMP       OUTSTRG0
.INCHAR JMP       INCHAR
.VECINT JMP       VECINIT

         ORG     ROMBS+$1FD6
;*** Vectors ***
VSCI      FDB     JSCI
VSPI      FDB     JSPI
VPAIE     FDB     JPAIE
VPAO      FDB     JPAO
VTOF      FDB     JTOF
VTOC5     FDB     JTOC5
VTOC4     FDB     JTOC4
VTOC3     FDB     JTOC3
VTOC2     FDB     JTOC2
VTOC1     FDB     JTOC1
VTIC3     FDB     JTIC3
VTIC2     FDB     JTIC2
VTIC1     FDB     JTIC1
VRTI      FDB     JRTI
VIRQ      FDB     JIRQ
VXIRQ     FDB     JXIRQ
VSWI      FDB     JSWI
VILLOP    FDB     JILLOP
VCOP      FDB     JCOP
VCLM      FDB     JCLM
VRST      FDB     BUFFALO
          END
