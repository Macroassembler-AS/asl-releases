; MEMCPY --
; Copy a block of memory from one location to another.
;
; Entry parameters
;       SRC: 14-bit address of source data block
;       DST: 14-bit address of target data block
;       CNT: 14-bit count of bytes to copy
 
            ORG     1700Q       ;Data at 001700q
SRC         DFB     0           ;SRC, low byte
            DFB     0           ;     high byte
DST         DFB     0           ;DST, low byte
            DFB     0           ;     high byte
CNT         DFB     0           ;CNT, low byte
            DFB     0           ;     high byte
 
            ORG     2000Q       ;Code at 002000q
MEMCPY      LLI     CNT & 255   ;HL = addr(CNT)
            LHI     CNT >> 8    ;(AND and SHR not supported)
            LCM                 ;BC = CNT
            INL
            LBM
LOOP        LAC                 ;If BC = 0,
            ORB
            RTZ                 ;Return
DECCNT      LAC                 ;BC = BC - 1
            SUI     1
            LCA
            LAB
            SBI     0
            LBA
GETSRC      LLI     SRC & 255   ;HL = addr(SRC)
            LHI     SRC >> 8
            LAC                 ;HL = SRC + BC
            ADM                 ;E = C + (HL)
            LEA                 ;(lower sum)
            INL                 ;point to upper SRC
            LAB
            ACM                 ;H = B + (HL) + CY
            LHA                 ;(upper sum)
            LLE                 ;L = E
            LDM                 ;Load D from (HL)
GETDST      LLI     DST & 255   ;HL = addr(DST)
            LHI     DST >> 8
            LAC                 ;HL = DST + BC
            ADM                 ;ADD code same as above
            LEA
            INL 
            LAB
            ACM
            LHA
            LLE
            LMD                 ;Store D to (HL)
            JMP     LOOP        ;Repeat the loop

; The same with Z80-style syntax:

            Z80SYNTAX EXCLUSIVE

            ORG	    2100Q
MEMCPY2     LD      L,CNT & 255 ;HL = addr(CNT)
            LD      H,CNT >> 8  ;(AND and SHR not supported)
            LD      C,(HL)      ;BC = CNT
            INC     L
            LD      B,(HL)
LOOP2       LD      A,C         ;If BC = 0,
            OR      A,B
            RET     Z           ;Return
DECCNT2     LD      A,C         ;BC = BC - 1
            SUB     A,1
            LD      C,A
            LD      A,B
            SBC     A,0
            LD      B,A
GETSRC2     LD      L,SRC & 255 ;HL = addr(SRC)
            LD      H,SRC >> 8
            LD      A,C         ;HL = SRC + BC
            ADD     A,(HL)      ;E = C + (HL)
            LD      E,A         ;(lower sum)
            INC     L           ;point to upper SRC
            LD      A,B
            ADC     A,(HL)      ;H = B + (HL) + CY
            LD      H,A         ;(upper sum)
            LD      L,E         ;L = E
            LD      D,(HL)      ;Load D from (HL)
GETDST2     LD      L,DST & 255 ;HL = addr(DST)
            LD      H,DST >> 8
            LD      A,C         ;HL = DST + BC
            ADD     A,(HL)      ;ADD code same as above
            LD      E,A
            INC     L 
            LD      A,B
            ADC     A,(HL)
            LD      H,A
            LD      L,E
            LD      (HL),D      ;Store D to (HL)
            JP      LOOP2       ;Repeat the loop
            END
