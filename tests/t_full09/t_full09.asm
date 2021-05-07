                cpu     6309

list            macro
                listing on
                endm

nolist          macro
                listing off
                endm

db              macro   op
                byt     (op)
                endm

fcb             macro   op
                byt     (op)
                endm

fcc             macro   op
                byt     (op)
                endm

dw              macro   op
                adr     (op)
                endm

fdb             macro   op
                adr     (op)
                endm

fcw             macro   op
                adr     (op)
                endm

ds              macro   op
                dfs     (op)
                endm

rmb             macro   op
                rept    op
                db      0
                endm
                endm

dd              macro   op
                adr     (op)>>16,(op)&$ffff
                endm

fcd             macro   op
                adr     (op)>>16,(op)&$ffff
                endm

direct          macro   num
                if      num=-1
                 assume dpr:nothing
                elseif  num>255
                 assume dpr:num>>8
                elseif
                 assume dpr:num
                endif
                endm

page            macro
                newpage
                endm

opt             macro
                endm

noopt           macro
                endm

nop             macro   cnt
                if      "CNT"=""
__cnt            set    1
                elseif
__cnt            set    cnt
                endif
                rept    __cnt
                 !nop
                endm
                endm

momseg          set     0
segsave_code    set     0
segsave_data    set     0
segsave_bss     set     0

saveseg         macro
                switch  momseg
                case    0
segsave_code    set     *
                case    1
segsave_data    set     *
                case    2
segsave_bss     set     *
                endcase
                endm

data            macro
                saveseg
                org     segsave_data
momseg          set     1
                endm

code            macro
                saveseg
                org     segsave_code
momseg          set     0
                endm

bss             macro
                saveseg
                org     segsave_bss
momseg          set     2
                endm

;--------------------------------------------------------------------------

; <:t17,25,41,45:>
; +=====================================================================+
; |                                                                     |
; |   TESTCASE.A09                                                      |
; |                                                                     |
; |   Test case for 6809/6309 assembler.                                |
; |                                                                     |
; |   Copyright 1993, Frank A. Vorstenbosch                             |
; |                                                                     |
; +=====================================================================+
;
; File created 13-oct-93

                title   "Test case for 6809/6309 assembler"

                list

; +---------------------------------------------------------------------+
; |                                                                     |
; |   Options.                                                          |
; |                                                                     |
; +---------------------------------------------------------------------+

;   -dERRORS   check error handling
;   -n         disable optimizations


; +---------------------------------------------------------------------+
; |                                                                     |
; |   Assembler pseudo instructions.                                    |
; |                                                                     |
; +---------------------------------------------------------------------+

; ----- expressions -----------------------------------------------------

                data
                org     4
                bss
                org     1634

TEST            equ     2+*/2
                ifdef ERRORS
TEST            equ     TEST+1
                endif

Constant8       equ     -43
Constant16      equ     16383
Constant32      equ     96285725
Address         equ     $bb5a

ANOTHER         set     3|24&8
ANOTHER         set     (3|24)&8
ANOTHER         set     4*(3>5)
ANOTHER         set     4*~~(3<5)
ANOTHER         set     15<<4
ANOTHER         set     ANOTHER+1
ANOTHER         set     ANOTHER+1       ; shorthand for SET

CHAR            equ     "a"
DOUBLECHAR      equ     "xy"
QUADCHAR        equ     "quad"

                ifdef ERRORS
TRIPLE          equ     "abc"
TOOMUCH         equ     "abcde"
                endif

                data
AddressFour     dw      TEST
                dw      **5

                org     $800

                direct  $8
                direct  $0800

                ds      14
DirectByte      db      123
                align   32
DirectWord      dw      12345
                align   48
DirectLong      dd      123456789
                align   79
DirectCode      rts

                dw      1234#12
                dw      %1010100101
                dw      (1+2)#8
                dw      1010101#%1010101

                bss
Unin_1          db      0
Unin_2          dw      4256
Unin_3          dd      34568957

                code
                org     $200

                page

                ifdef ERRORS
1
                equ     123
                psscht
                !
                endif

; ----- range checking on immediate values ------------------------------

                lda     #10
                lda     #100
                ifdef ERRORS
                lda     #1000
                lda     #10000
                lda     #100000
                lda     #1000000
                lda     #10000000
                lda     #100000000
                lda     #1000000000
                endif

                ldx     #10
                ldx     #100
                ldx     #1000
                ldx     #10000
                ifdef ERRORS
                ldx     #100000
                ldx     #1000000
                ldx     #10000000
                ldx     #100000000
                ldx     #1000000000
                endif

                ifdef __6309__
                ldq     #10
                ldq     #100
                ldq     #1000
                ldq     #10000
                ldq     #100000
                ldq     #1000000
                ldq     #10000000
                ldq     #100000000
                ldq     #1000000000
                endif

                page    10              ; keep 10 lines togethre

; ----- align -----------------------------------------------------------

                align   16
                align   32


; ----- code, data, org -------------------------------------------------

                code
                org     $1300
                data
                org     $1180

                code
                lda     #1

                data
Table           db      1,2,3

                code
                ldx     #Table


; ----- db, fcb, fcc ----------------------------------------------------

Message1        db      7,"Error",13,10,0

Message2        fcb     7
                fcc     "Error"
                fcb     13,10,0


; ----- ds, rmb ---------------------------------------------------------

                ds      10
                rmb     10


; ----- dw, fcw, fdb ----------------------------------------------------

                dw      23457
                fcw     13462
                fdb     6235


; ----- if ... else ... endif -------------------------------------------

                if      5=6
                db      0
                if      0
                db      1
                else
                db      2
                endif
                db      3
                else
                db      4
                if      1
                db      5
                else
                db      6
                endif
                db      7
                endif


; ----- list, nolist ----------------------------------------------------

                nolist
                ; comment not listed
                db      10

                list
                ; comment is listed
                db      10


; ----- opt, noopt ------------------------------------------------------

                noopt

                opt


; ----- nop -------------------------------------------------------------

                nop
                nop     3


; ----- struct ----------------------------------------------------------

;                struct  ListNode
;                dw      LN_Next
;                dw      LN_Previous
;                db      LN_Type
;                end struct


; ----- number bases ----------------------------------------------------

                dd      1
                dd      10
                dd      100
                dd      1000

                dd      $1
                dd      $10
                dd      $100
                dd      $1000

                dd      %1
                dd      %10
                dd      %100
                dd      %1000

                dd      @1
                dd      @10
                dd      @100
                dd      @1000

                dd      2#1
                dd      2#10
                dd      2#100
                dd      2#1000

                dd      3#1
                dd      3#10
                dd      3#100
                dd      3#1000
                dd      3#12

                dd      4#1
                dd      4#10
                dd      4#100
                dd      4#1000
                dd      4#123

                dd      5#1
                dd      5#10
                dd      5#100
                dd      5#1000
                dd      5#1234

                dd      6#1
                dd      6#10
                dd      6#100
                dd      6#1000
                dd      6#2345

                dd      7#1
                dd      7#10
                dd      7#100
                dd      7#1000
                dd      7#3456

                dd      8#1
                dd      8#10
                dd      8#100
                dd      8#1000
                dd      8#4567

                dd      9#1
                dd      9#10
                dd      9#100
                dd      9#1000
                dd      9#5678

                dd      10#1
                dd      10#10
                dd      10#100
                dd      10#1000
                dd      10#6789

                dd      11#1
                dd      11#10
                dd      11#100
                dd      11#1000
;                dd      11#789a

                dd      12#1
                dd      12#10
                dd      12#100
                dd      12#1000
;                dd      12#89ab

                dd      13#1
                dd      13#10
                dd      13#100
                dd      13#1000
;                dd      13#9abc

                dd      14#1
                dd      14#10
                dd      14#100
                dd      14#1000
;                dd      14#abcd

                dd      15#1
                dd      15#10
                dd      15#100
                dd      15#1000
;                dd      15#bcde

                dd      16#1
                dd      16#10
                dd      16#100
                dd      16#1000
;                dd      16#cdef

                dd      17#1
                dd      17#10
                dd      17#100
                dd      17#1000
;                dd      17#defg

                dd      18#1
                dd      18#10
                dd      18#100
                dd      18#1000
;                dd      18#efgh

                dd      19#1
                dd      19#10
                dd      19#100
                dd      19#1000
;                dd      19#fghi

                dd      20#1
                dd      20#10
                dd      20#100
                dd      20#1000
;                dd      20#ghij

                dd      21#1
                dd      21#10
                dd      21#100
                dd      21#1000
;                dd      21#hijk

                dd      22#1
                dd      22#10
                dd      22#100
                dd      22#1000
;                dd      22#ijkl

                dd      23#1
                dd      23#10
                dd      23#100
                dd      23#1000
;                dd      23#jklm

                dd      24#1
                dd      24#10
                dd      24#100
                dd      24#1000
;                dd      24#klmn

                dd      25#1
                dd      25#10
                dd      25#100
                dd      25#1000
;                dd      25#lmno

                dd      26#1
                dd      26#10
                dd      26#100
                dd      26#1000
;                dd      26#mnop

                dd      27#1
                dd      27#10
                dd      27#100
                dd      27#1000
;                dd      27#nopq

                dd      28#1
                dd      28#10
                dd      28#100
                dd      28#1000
;                dd      28#opqr

                dd      29#1
                dd      29#10
                dd      29#100
                dd      29#1000
;                dd      29#pqrs

                dd      30#1
                dd      30#10
                dd      30#100
                dd      30#1000
;                dd      30#qrst

                dd      31#1
                dd      31#10
                dd      31#100
                dd      31#1000
;                dd      31#rstu

                dd      32#1
                dd      32#10
                dd      32#100
                dd      32#1000
;                dd      32#stuv

                dd      33#1
                dd      33#10
                dd      33#100
                dd      33#1000
;                dd      33#tuvw

                dd      34#1
                dd      34#10
                dd      34#100
                dd      34#1000
;                dd      34#uvwx

                dd      35#1
                dd      35#10
                dd      35#100
                dd      35#1000
;                dd      35#vwxy

                dd      36#1
                dd      36#10
                dd      36#100
                dd      36#1000
;                dd      36#wxyz

                ifdef ERRORS
                dd      37#1
                dd      37#10
                dd      37#100
                dd      37#1000

                dd      1#1
                dd      1#10
                dd      1#100
                dd      1#1000

                dd      0#1
                dd      0#10
                dd      0#100
                dd      0#1000
                endif


; ----- garbage in inactive if-clause -----------------------------------

                if 0
 !"#$%&'()*+,-./
0123456789:;<=>?
@ABCDEFGHIJKLMNO
PQRSTUVWXYZ[\]^_
`abcdefghijklmno
pqrstuvwxyz{|}~
€‚ƒ„…†‡ˆ‰Š‹Œ
‘’“”•–—˜™š›œŸ
 ¡¢£¤¥¦§¨©ª«¬­®¯
°±²|´µ¶·¸¹|+¼½¾¿
ÀÁÂÃ-ÅÆÇÈ+ÊËÌ=ÎÏ
ĞÑÒÓÔÕÖ×ØÙÚÛÜİŞß
àáâãäåæçèéêëìíîï
ğñòóôõö÷øùúûüış
                endif


; +=====================================================================+
; |                                                                     |
; |   Instructions.                                                     |
; |                                                                     |
; È=====================================================================¼

; Ú---------------------------------------------------------------------¿
; |                                                                     |
; |   Register to register operations.                                  |
; |                                                                     |
; À---------------------------------------------------------------------Ù

Start           tfr     a,a             ; NOP
                tfr     a,b
                tfr     a,cc
                tfr     a,ccr
                tfr     a,dp
                tfr     a,dpr

                tfr     b,a
                tfr     b,b             ; NOP
                tfr     b,cc
                tfr     b,ccr
                tfr     b,dp
                tfr     b,dpr

                tfr     d,d             ; NOP
                tfr     d,x
                tfr     d,y
                tfr     d,u
                tfr     d,s
                tfr     d,sp

                tfr     x,d
                tfr     x,x             ; NOP
                tfr     x,y
                tfr     x,u
                tfr     x,s
                tfr     x,sp

                tfr     y,d
                tfr     y,x
                tfr     y,y             ; NOP
                tfr     y,u
                tfr     y,s
                tfr     y,sp

                tfr     u,d
                tfr     u,x
                tfr     u,y
                tfr     u,u             ; NOP
                tfr     u,s
                tfr     u,sp

                tfr     s,d
                tfr     s,x
                tfr     s,y
                tfr     s,u
                tfr     s,s             ; NOP
                tfr     s,sp            ; NOP

                tfr     sp,d
                tfr     sp,x
                tfr     sp,y
                tfr     sp,u
                tfr     sp,s            ; NOP
                tfr     sp,sp           ; NOP

                tfr     pc,d
                tfr     pc,x
                tfr     pc,y
                tfr     pc,u
                tfr     pc,s
                tfr     pc,sp

                ifdef __6309__
                tfr     a,e
                tfr     a,f
                tfr     b,e
                tfr     b,f

                tfr     e,a
                tfr     e,b
                tfr     e,cc
                tfr     e,ccr
                tfr     e,dp
                tfr     e,dpr
                tfr     e,e             ; NOP
                tfr     e,f

                tfr     f,a
                tfr     f,b
                tfr     f,cc
                tfr     f,ccr
                tfr     f,dp
                tfr     f,dpr
                tfr     f,e
                tfr     f,f             ; NOP

                tfr     d,v
                tfr     d,w

                tfr     v,d
                tfr     v,v             ; NOP
                tfr     v,w
                tfr     v,x
                tfr     v,y
                tfr     v,u
                tfr     v,s
                tfr     v,sp

                tfr     w,d
                tfr     w,v
                tfr     w,w             ; NOP
                tfr     w,x
                tfr     w,y
                tfr     w,u
                tfr     w,s
                tfr     w,sp

                tfr     x,v
                tfr     x,w
                tfr     y,v
                tfr     y,w
                tfr     u,v
                tfr     u,w
                tfr     s,v
                tfr     s,w
                tfr     pc,v
                tfr     pc,w

                tfr     z,a
                tfr     z,b
                tfr     z,cc
                tfr     z,ccr
                tfr     z,dp
                tfr     z,dpr
                tfr     z,e
                tfr     z,f
                tfr     z,d
                tfr     z,v
                tfr     z,w
                tfr     z,x
                tfr     z,y
                tfr     z,u
                tfr     z,s
                tfr     z,sp

                tfr     a,z
                tfr     b,z
                tfr     cc,z
                tfr     ccr,z
                tfr     dp,z
                tfr     dpr,z
                tfr     e,z
                tfr     f,z
                tfr     d,z
                tfr     v,z
                tfr     w,z
                tfr     x,z
                tfr     y,z
                tfr     u,z
                tfr     s,z
                tfr     sp,z
                tfr     pc,z
                endif

                ifdef ERRORS
                tfm     a,b
                tfr     a,d
                tfr     a,v
                tfr     a,w
                tfr     a,x
                tfr     a,y
                tfr     a,u
                tfr     a,s
                tfr     a,sp

                tfr     b,d
                tfr     b,v
                tfr     b,w
                tfr     b,x
                tfr     b,y
                tfr     b,u
                tfr     b,s
                tfr     b,sp
                endif


; +---------------------------------------------------------------------+
; |                                                                     |
; |   Addressing modes.                                                 |
; |                                                                     |
; +---------------------------------------------------------------------+

                lda     #0
                lda     DirectByte
                lda     >DirectByte
                lda     AddressFour
                ifdef ERRORS
                lda     <AddressFour
                endif
                lda     12+5*17/3
                lda     ,x
                noopt
                lda     0,x
                opt
                lda     0,x
                lda     <0,x
                lda     <<0,x
                noopt
                lda     <<0,x
                opt
                lda     >0,x
                lda     1,x
                lda     <1,x
                lda     <<1,x
                lda     >1,x
                lda     15,x
                lda     -16,x
                lda     16,x
                lda     -17,x
                lda     127,x
                lda     -128,x
                lda     128,x
                lda     -129,x
                lda     FORWARD5,x
                lda     <FORWARD5,x
                lda     <<FORWARD5,x
                lda     FORWARD99,x
                lda     <FORWARD99,x
                ifdef ERRORS
                lda     <<FORWARD99,x
                endif
                lda     a,x
                lda     b,x
                lda     d,x
                lda     ,x+
                lda     ,x++
                lda     ,-x
                lda     ,--x
                lda     NearData,pc
                lda     <NearData,pc
                lda     AddressFour,pc
                lda     [,x]
                lda     [0,x]
                lda     [1,x]
                lda     [15,x]
                lda     [-16,x]
                lda     [17,x]
                lda     [-17,x]
                lda     [127,x]
                lda     [-128,x]
                lda     [128,x]
                lda     [-129,x]
                lda     [a,x]
                lda     [b,x]
NearData        lda     [d,x]
                lda     [,x++]
                lda     [,--x]
                lda     [NearData,pc]
                lda     [>NearData,pc]
                lda     [AddressFour,pc]
                ifdef ERRORS
                lda     [<AddressFour,pc]
                endif

FORWARD5        equ     5
FORWARD99       equ     99

                ifdef __6309__
                lda     e,x
                lda     f,x
                lda     w,x
                lda     ,w
                lda     0,w
                lda     1,w
                lda     ,w++
                lda     ,--w
                lda     [e,x]
                lda     [f,x]
                lda     [w,x]
                lda     [,w]
                lda     [0,w]
                lda     [1000,w]
                lda     [,w++]
                lda     [,--w]
                endif


; +---------------------------------------------------------------------+
; |                                                                     |
; |   Instructions in numerical order.                                  |
; |                                                                     |
; +---------------------------------------------------------------------+

                neg     DirectByte                  ; $00,2
                ifdef __6309__
                oim     #123,DirectByte             ; $01,3
                aim     #123,DirectByte             ; $02,3
                endif
                com     DirectByte                  ; $03,2
                lsr     DirectByte                  ; $04,2
                ifdef __6309__
                eim     #123,DirectByte             ; $05,3
                endif
                ror     DirectByte                  ; $06,2
                asr     DirectByte                  ; $07,2
                asl     DirectByte                  ; $08,2
                lsl     DirectByte                  ; alternate
                rol     DirectByte                  ; $09,2
                dec     DirectByte                  ; $0A,2
                ifdef __6309__
                tim     #1,DirectByte               ; $0B,3
                endif
                inc     DirectByte                  ; $0C,2
                tst     DirectByte                  ; $0D,2
                jmp     DirectByte                  ; $0E,2
                clr     DirectByte                  ; $0F,2

; -----------------------------------------------------------------------

                nop                                 ; $12,1
                nop     4                           ; repeat count specified
                sync                                ; $13,1
                ifdef __6309__
                sexw                                ; $14,1
                endif
                noopt
                lbra    AddressFour                 ; $16,3
                lbsr    AddressFour                 ; $17,3
                opt
                daa                                 ; $19,1
                orcc    #1                          ; $1A,2
                orcc    c                           ; alternate, specifying flags
                andcc   #~6                         ; $1C,2
                andcc   z,v                         ; alternate
                sex                                 ; $1D,1
                exg     a,b                         ; $1E,2
                tfr     a,b                         ; $1F,2
                ifdef __6309__
                clrs                                ; using TFR to clear registers
                clrv
                clrx
                clry
                endif

; -----------------------------------------------------------------------

BranchTarget    bra     BranchTarget                ; $20,2
                brn     BranchTarget                ; $21,2
                bhi     BranchTarget                ; $22,2
                bls     BranchTarget                ; $23,2
                bhs     BranchTarget                ; $24,2
                bcc     BranchTarget                ; alternate
                blo     BranchTarget                ; $25,2
                bcs     BranchTarget                ; alternate
                bne     BranchTarget                ; $26,2
                beq     BranchTarget                ; $27,2
                bvc     BranchTarget                ; $28,2
                bvs     BranchTarget                ; $29,2
                bpl     BranchTarget                ; $2A,2
                bmi     BranchTarget                ; $2B,2
                bge     BranchTarget                ; $2C,2
                blt     BranchTarget                ; $2D,2
                bgt     BranchTarget                ; $2E,2
                ble     BranchTarget                ; $2F,2

; -----------------------------------------------------------------------

                leax    a,x                         ; 30,2+
                leay    b,y                         ; 31,2+
                leas    d,s                         ; 32,2+
                leau    1,u                         ; 33,2+
                pshs    a,b                         ; $34,2
		pshs	d                           ; alternate
                pshs    all                         ; alternate
                pshs    #123                        ; alternate
                puls    x                           ; $35,2
                puls    all                         ; alternate
                puls    #$ff                        ; alternate
                pshu    ccr                         ; $36,2
		pulu	cc                          ; alternate
                pulu    dpr                         ; $37,2
		pulu	dp                          ; alternate
		pulu	y                           ; $37,2
		puls	s/u                         ; $35,2
		puls	u                           ; alternate
		pshs	pc                          ; $34,2
                rts                                 ; $39,1
                abx                                 ; $3A,1
                rti                                 ; $3B,1
                cwai    #127                        ; $3C,2
                cwai    e                           ; alternate
                mul                                 ; $3D,1
                swi                                 ; $3F,1

; -----------------------------------------------------------------------

                nega                                ; $40,1
                coma                                ; $43,1
                lsra                                ; $44,1
                rora                                ; $46,1
                asra                                ; $47,1
                asla                                ; $48,1
                lsla                                ; alternate
                rola                                ; $49,1
                deca                                ; $4A,1
                inca                                ; $4C,1
                tsta                                ; $4D,1
                clra                                ; $4F,1

; -----------------------------------------------------------------------

                negb                                ; $50,1
                comb                                ; $53,1
                lsrb                                ; $54,1
                rorb                                ; $56,1
                asrb                                ; $57,1
                aslb                                ; $58,1
                lslb                                ; alternate
                rolb                                ; $59,1
                decb                                ; $5A,1
                incb                                ; $5C,1
                tstb                                ; $5D,1
                clrb                                ; $5F,1

; -----------------------------------------------------------------------

                neg     ,x                          ; $60,2+
                ifdef __6309__
                oim     #4,,x                       ; $61,3+
                aim     #8,,x                       ; $62,3+
                endif
                com     ,x                          ; $63,2+
                lsr     ,x                          ; $64,2+
                ifdef __6309__
                eim     #9,,x                       ; $65,3+
                endif
                ror     ,x                          ; $66,2+
                asr     ,x                          ; $67,2+
                asl     ,x                          ; $68,2+
                lsl     ,x                          ; alternate
                rol     ,x                          ; $69,2+
                dec     ,x                          ; $6A,2+
                ifdef __6309__
                tim     #123,,x                     ; $6B,3+
                endif
                inc     ,x                          ; $6C,2+
                tst     ,x                          ; $6D,2+
                jmp     ,x                          ; $6E,2+
                clr     ,x                          ; $6F,2+

; -----------------------------------------------------------------------

                neg     AddressFour                 ; $70,3
                ifdef __6309__
                oim     #99,AddressFour             ; $71,4
                aim     #99,AddressFour             ; $72,4
                endif
                com     AddressFour                 ; $73,3
                lsr     AddressFour                 ; $74,3
                ifdef __6309__
                eim     #-1,AddressFour             ; $75,4
                endif
                ror     AddressFour                 ; $76,3
                asr     AddressFour                 ; $77,3
                asl     AddressFour                 ; $78,3
                lsl     AddressFour                 ; alternate
                rol     AddressFour                 ; $79,3
                dec     AddressFour                 ; $7A,3
                ifdef __6309__
                tim     #-128,AddressFour           ; $7B,4
                endif
                inc     AddressFour                 ; $7C,3
                tst     AddressFour                 ; $7D,3
                jmp     AddressFour                 ; $7E,3
                clr     AddressFour                 ; $7F,3

; -----------------------------------------------------------------------

CallAddress     suba    #123                        ; $80,2
                cmpa    #123                        ; $81,2
                sbca    #123                        ; $82,2
                subd    #12345                      ; $83,3
                anda    #123                        ; $84,2
                bita    #123                        ; $85,2
                lda     #123                        ; $86,2
                eora    #123                        ; $88,2
                adca    #123                        ; $89,2
                ora     #123                        ; $8A,2
                adda    #123                        ; $8B,2
                cmpx    #12345                      ; $8C,3
                bsr     CallAddress                 ; $8D,2
                ldx     #12345                      ; $8E,3

; -----------------------------------------------------------------------

                suba    DirectByte                  ; $90,2
                cmpa    DirectByte                  ; $91,2
                sbca    DirectByte                  ; $92,2
                subd    DirectWord                  ; $93,3
                anda    DirectByte                  ; $94,2
                bita    DirectByte                  ; $95,2
                lda     DirectByte                  ; $96,2
                sta     DirectByte                  ; $97,2
                eora    DirectByte                  ; $98,2
                adca    DirectByte                  ; $99,2
                ora     DirectByte                  ; $9A,2
                adda    DirectByte                  ; $9B,2
                cmpx    DirectWord                  ; $9C,2
                jsr     DirectCode                  ; $9D,2
                ldx     DirectWord                  ; $9E,2
                stx     DirectWord                  ; $9F,2

; -----------------------------------------------------------------------

                suba    [3,s]                       ; $A0,2+
                cmpa    [3,s]                       ; $A1,2+
                sbca    [3,s]                       ; $A2,2+
                subd    [3,s]                       ; $A3,2+
                anda    [3,s]                       ; $A4,2+
                bita    [3,s]                       ; $A5,2+
                lda     [3,s]                       ; $A6,2+
                sta     [3,s]                       ; $A7,2+
                eora    [3,s]                       ; $A8,2+
                adca    [3,s]                       ; $A9,2+
                ora     [3,s]                       ; $AA,2+
                adda    [3,s]                       ; $AB,2+
                cmpx    [3,s]                       ; $AC,2+
                jsr     [3,s]                       ; $AD,2+
                ldx     [3,s]                       ; $AE,2+
                stx     [3,s]                       ; $AF,2+

; -----------------------------------------------------------------------

                suba    $ff00                       ; $B0,3
                cmpa    $ff00                       ; $B1,3
                sbca    $ff00                       ; $B2,3
                subd    $ff00                       ; $B3,3
                anda    $ff00                       ; $B4,3
                bita    $ff00                       ; $B5,3
                lda     $ff00                       ; $B6,3
                sta     $ff00                       ; $B7,3
                eora    $ff00                       ; $B8,3
                adca    $ff00                       ; $B9,3
                ora     $ff00                       ; $BA,3
                adda    $ff00                       ; $BB,3
                cmpx    $ff00                       ; $BC,3
                jsr     $ff00                       ; $BD,3
                ldx     $ff00                       ; $BE,3
                stx     $ff00                       ; $BF,3

; -----------------------------------------------------------------------

                subb    #123                        ; $C0,2
                cmpb    #123                        ; $C1,2
                sbcb    #123                        ; $C2,2
                addd    #12345                      ; $C3,3
                andb    #123                        ; $C4,2
                bitb    #123                        ; $C5,2
                ldb     #123                        ; $C6,2
                eorb    #123                        ; $C8,2
                adcb    #123                        ; $C9,2
                orb     #123                        ; $CA,2
                addb    #123                        ; $CB,2
                ldd     #12345                      ; $CC,3
                ifdef __6309__
                ldq     #123456789                  ; $CD,5
                endif
                ldu     #12345                      ; $CE,3

; -----------------------------------------------------------------------

                subb    DirectByte                  ; $D0,2
                cmpb    DirectByte                  ; $D1,2
                sbcb    DirectByte                  ; $D2,2
                addd    DirectWord                  ; $D3,3
                andb    DirectByte                  ; $D4,2
                bitb    DirectByte                  ; $D5,2
                ldb     DirectByte                  ; $D6,2
                stb     DirectByte                  ; $D7,2
                eorb    DirectByte                  ; $D8,2
                adcb    DirectByte                  ; $D9,2
                orb     DirectByte                  ; $DA,2
                addb    DirectByte                  ; $DB,2
                ldd     DirectWord                  ; $DC,2
                std     DirectWord                  ; $DD,2
                ldu     DirectWord                  ; $DE,2
                stu     DirectWord                  ; $DF,2

; -----------------------------------------------------------------------

; note effect of quasi-forward reference in the next line
LocalData       subb    LocalData,pc                ; $E0,2+
AnotherLocal    cmpb    <AnotherLocal,pc            ; $E1,2+
                sbcb    LocalData,pc                ; $E2,2+
                addd    LocalData,pc                ; $E3,2+
                andb    LocalData,pc                ; $E4,2+
                bitb    LocalData,pc                ; $E5,2+
                ldb     LocalData,pc                ; $E6,2+
                stb     LocalData,pc                ; $E7,2+
                eorb    LocalData,pc                ; $E8,2+
                adcb    LocalData,pc                ; $E9,2+
                orb     LocalData,pc                ; $EA,2+
                addb    LocalData,pc                ; $EB,2+
                ldd     LocalData,pc                ; $EC,2+
                std     LocalData,pc                ; $ED,2+
                ldu     LocalData,pc                ; $EE,2+
                stu     LocalData,pc                ; $EF,2+

; -----------------------------------------------------------------------

                subb    LocalData                   ; $F0,3
                cmpb    LocalData                   ; $F1,3
                sbcb    LocalData                   ; $F2,3
                addd    LocalData                   ; $F3,3
                andb    LocalData                   ; $F4,3
                bitb    LocalData                   ; $F5,3
                ldb     LocalData                   ; $F6,3
                stb     LocalData                   ; $F7,3
                eorb    LocalData                   ; $F8,3
                adcb    LocalData                   ; $F9,3
                orb     LocalData                   ; $FA,3
                addb    LocalData                   ; $FB,3
                ldd     LocalData                   ; $FC,3
                std     LocalData                   ; $FD,3
                ldu     LocalData                   ; $FE,3
                stu     LocalData                   ; $FF,3

; +---------------------------------------------------------------------+
; |                                                                     |
; |   Instructions with prefix byte $10.                                |
; |                                                                     |
; +---------------------------------------------------------------------+

                lbrn    BranchTarget                ; $1021,4
                lbhi    BranchTarget                ; $1022,4
                lbls    BranchTarget                ; $1023,4
                lbhs    BranchTarget                ; $1024,4
                lbcc    BranchTarget                ; alternate
                lblo    BranchTarget                ; $1025,4
                lbcs    BranchTarget                ; alternate
                lbne    BranchTarget                ; $1026,4
                lbeq    BranchTarget                ; $1027,4
                lbvc    BranchTarget                ; $1028,4
                lbvs    BranchTarget                ; $1029,4
                lbpl    BranchTarget                ; $102A,4
                lbmi    BranchTarget                ; $102B,4
                lbge    BranchTarget                ; $102C,4
                lblt    BranchTarget                ; $102D,4
                lbgt    BranchTarget                ; $102E,4
                lble    BranchTarget                ; $102F,4

; -----------------------------------------------------------------------

                ifdef __6309__
                addr    a,b                         ; $1030,3
                add     a,b                         ; alternate
                adcr    w,d                         ; $1031,3
                adc     w,d                         ; alternate
                subr    d,x                         ; $1032,3
                sub     d,x                         ; alternate
                sbcr    b,a                         ; $1033,3
                sbc     b,a                         ; alternate
                andr    a,ccr                       ; $1034,3
                and     a,ccr                       ; alternate
                orr     b,dpr                       ; $1035,3
                or      b,dpr                       ; alternate
                eorr    w,d                         ; $1036,3
                eor     w,d                         ; alternate
                cmpr    d,u                         ; $1037,3
                cmp     d,u                         ; alternate
                pshsw                               ; $1038,2
                pshs    w                           ; alternate
                pulsw                               ; $1039,2
                puls    w                           ; alternate
                pshuw                               ; $103A,2
                pshu    w                           ; alternate
                puluw                               ; $103B,2
                pulu    w                           ; alternate
                endif
                swi2                                ; $103F,2
                swi     2                           ; alternate

; -----------------------------------------------------------------------

                ifdef __6309__
                negd                                ; $1040,2
                comd                                ; $1043,2
                lsrd                                ; $1044,2
                rord                                ; $1046,2
                asrd                                ; $1047,2
                asld                                ; $1048,2
                rold                                ; $1049,2
                decd                                ; $104A,2
                incd                                ; $104C,2
                tstd                                ; $104D,2
                clrd                                ; $104F,2

; -----------------------------------------------------------------------

                comw                                ; $1053,2
                lsrw                                ; $1054,2
                rorw                                ; $1056,2
                rolw                                ; $1059,2
                decw                                ; $105A,2
                incw                                ; $105C,2
                tstw                                ; $105D,2
                clrw                                ; $105F,2

; -----------------------------------------------------------------------

                subw    #12345                      ; $1080,4
                cmpw    #12345                      ; $1081,4
                sbcd    #12345                      ; $1082,4
                endif
                cmpd    #12345                      ; $1083,4
                ifdef __6309__
                andd    #12345                      ; $1084,4
                bitd    #12345                      ; $1085,4
                ldw     #12345                      ; $1086,4
                eord    #12345                      ; $1088,4
                adcd    #12345                      ; $1089,4
                ord     #12345                      ; $108A,4
                addw    #12345                      ; $108B,4
                endif
                cmpy    #12345                      ; $108C,4
                ldy     #12345                      ; $108E,4

; -----------------------------------------------------------------------

                ifdef __6309__
                subw    DirectWord                  ; $1090,3
                cmpw    DirectWord                  ; $1091,3
                sbcd    DirectWord                  ; $1092,3
                endif
                cmpd    DirectWord                  ; $1093,3
                ifdef __6309__
                andd    DirectWord                  ; $1094,3
                bitd    DirectWord                  ; $1095,3
                ldw     DirectWord                  ; $1096,3
                stw     DirectWord                  ; $1097,3
                eord    DirectWord                  ; $1098,3
                adcd    DirectWord                  ; $1099,3
                ord     DirectWord                  ; $109A,3
                addw    DirectWord                  ; $109B,3
                endif
                cmpy    DirectWord                  ; $109C,3
                ldy     DirectWord                  ; $109E,3
                sty     DirectWord                  ; $109F,3

; -----------------------------------------------------------------------

                ifdef __6309__
                subw    ,w++                        ; $10A0,3+
                cmpw    ,w++                        ; $10A1,3+
                sbcd    ,w++                        ; $10A2,3+
                endif
                cmpd    ,--x                        ; $10A3,3+
                ifdef __6309__
                andd    ,w++                        ; $10A4,3+
                bitd    ,w++                        ; $10A5,3+
                ldw     ,w++                        ; $10A6,3+
                stw     ,w++                        ; $10A7,3+
                eord    ,w++                        ; $10A8,3+
                adcd    ,w++                        ; $10A9,3+
                ord     ,w++                        ; $10AA,3+
                addw    ,w++                        ; $10AB,3+
                endif
                cmpy    ,--x                        ; $10AC,3+
                ldy     ,--x                        ; $10AE,3+
                sty     ,--x                        ; $10AF,3+

; -----------------------------------------------------------------------

                ifdef __6309__
                subw    $7000                       ; $10B0,4
                cmpw    $7000                       ; $10B1,4
                sbcd    $7000                       ; $10B2,4
                endif
                cmpd    $7000                       ; $10B3,4
                ifdef __6309__
                andd    $7000                       ; $10B4,4
                bitd    $7000                       ; $10B5,4
                ldw     $7000                       ; $10B6,4
                stw     $7000                       ; $10B7,4
                eord    $7000                       ; $10B8,4
                adcd    $7000                       ; $10B9,4
                ord     $7000                       ; $10BA,4
                addw    $7000                       ; $10BB,4
                endif
                cmpy    $7000                       ; $10BC,4
                ldy     $7000                       ; $10BE,4
                sty     $7000                       ; $10BF,4

; -----------------------------------------------------------------------

                lds     #12345                      ; $10CE,4

                ifdef __6309__
                ldq     DirectLong                  ; $10DC,3
                stq     DirectLong                  ; $10DD,3
                endif
                lds     DirectWord                  ; $10DE,3
                sts     DirectWord                  ; $10DF,3

                ifdef __6309__
                ldq     ,x                          ; $10EC,3+
                stq     ,x                          ; $10ED,3+
                endif
                lds     ,x                          ; $10EE,3+
                sts     ,x                          ; $10EF,3+

                ifdef __6309__
                ldq     AddressFour                 ; $10FC,4
                stq     AddressFour                 ; $10FD,4
                endif
                lds     AddressFour                 ; $10FE,4
                sts     AddressFour                 ; $10FF,4

; +---------------------------------------------------------------------+
; |                                                                     |
; |   Instructions with prefix byte $11.                                |
; |                                                                     |
; +---------------------------------------------------------------------+

                ifdef __6309__
                band    a.7,DirectByte.0            ; $1130,4
                biand   b.6,DirectByte.1            ; $1131,4
                bor     cc.5,DirectByte.2           ; $1132,4
                bior    a.4,DirectByte.3            ; $1133,4
                beor    b.3,DirectByte.4            ; $1134,4
                bieor   cc.2,DirectByte.5           ; $1135,4
                ldbt    ccr.1,DirectByte.6          ; $1136,4
                stbt    ccr.0,DirectByte.7          ; $1137,4

                tfr     x+,y+                       ; $1138,3
                tfm     x+,y+                       ; alternate
                tfr     u-,x-                       ; $1139,3
                tfm     u-,x-                       ; alternate
                tfr     s+,x                        ; $113A,3
                tfm     s+,x                        ; alternate
                tfr     x,y+                        ; $113B,3
                tfm     x,y+                        ; alternate
                bitmd    #128                       ; $113C,3
                ldmd     #1                         ; $113D,3
                endif
                swi3                                ; $113F,2
                swi     3                           ; alternate

; -----------------------------------------------------------------------

                ifdef __6309__
                come                                ; $1143,2
                dece                                ; $114A,2
                ince                                ; $114C,2
                tste                                ; $114D,2
                clre                                ; $114F,2

                comf                                ; $1153,2
                decf                                ; $115A,2
                incf                                ; $115C,2
                tstf                                ; $115D,2
                clrf                                ; $115F,2
                endif

; -----------------------------------------------------------------------

                ifdef __6309__
                sube    #123                        ; $1180,3
                cmpe    #123                        ; $1181,3
                endif
                cmpu    #12345                      ; $1183,4
                ifdef __6309__
                lde     #123                        ; $1186,3
                adde    #123                        ; $118B,3
                endif
                cmps    #12345                      ; $118C,4
                ifdef __6309__
                divd    #123                        ; $118D,3
                divq    #12345                      ; $118E,4
                muld    #12345                      ; $118F,4
                endif

; -----------------------------------------------------------------------

                ifdef __6309__
                sube    DirectByte                  ; $1190,3
                cmpe    DirectByte                  ; $1191,3
                endif
                cmpu    DirectWord                  ; $1193,3
                ifdef __6309__
                lde     DirectByte                  ; $1196,3
                ste     DirectByte                  ; $1197,3
                adde    DirectByte                  ; $119B,3
                endif
                cmps    DirectWord                  ; $119C,3
                ifdef __6309__
                divd    DirectWord                  ; $119D,3
                divq    DirectWord                  ; $119E,3
                muld    DirectWord                  ; $119F,3
                endif

; -----------------------------------------------------------------------

                ifdef __6309__
                sube    ,s+                         ; $11A0,3+
                cmpe    ,s+                         ; $11A1,3+
                endif
                cmpu    ,s++                        ; $11A3,3+
                ifdef __6309__
                lde     ,s+                         ; $11A6,3+
                ste     ,s+                         ; $11A7,3+
                adde    ,s+                         ; $11AB,3+
                endif
                cmps    ,s++                        ; $11AC,3+
                ifdef __6309__
                divd    ,s+                         ; $11AD,3+
                divq    ,s++                        ; $11AE,3+
                muld    ,s++                        ; $11AF,3+
                endif

; -----------------------------------------------------------------------

                ifdef __6309__
                sube    $9000                       ; $11B0,4
                cmpe    $9000                       ; $11B1,4
                endif
                cmpu    $9000                       ; $11B3,4
                ifdef __6309__
                lde     $9000                       ; $11B6,4
                ste     $9000                       ; $11B7,4
                adde    $9000                       ; $11BB,4
                endif
                cmps    $9000                       ; $11BC,4
                ifdef __6309__
                divd    $9000                       ; $11BD,4
                divq    $9000                       ; $11BE,4
                muld    $9000                       ; $11BF,4
                endif

; -----------------------------------------------------------------------

                ifdef __6309__
                subf    #123                        ; $11C0,3
                cmpf    #123                        ; $11C1,3
                ldf     #123                        ; $11C6,3
                addf    #123                        ; $11CB,3
                endif

; -----------------------------------------------------------------------

                ifdef __6309__
                subf    DirectByte                  ; $11D0,3
                cmpf    DirectByte                  ; $11D1,3
                ldf     DirectByte                  ; $11D6,3
                stf     DirectByte                  ; $11D7,3
                addf    DirectByte                  ; $11DB,3

                subf    ,s+                         ; $11E0,3+
                cmpf    ,s+                         ; $11E1,3+
                ldf     ,s+                         ; $11E6,3+
                stf     ,s+                         ; $11E7,3+
                addf    ,s+                         ; $11EB,3+

                subf    $9000                       ; $11F0,4
                cmpf    $9000                       ; $11F1,4
                ldf     $9000                       ; $11F6,4
                stf     $9000                       ; $11F7,4
                addf    $9000                       ; $11FB,4
                endif


                end     Start
; ----- EOF -------------------------------------------------------------

