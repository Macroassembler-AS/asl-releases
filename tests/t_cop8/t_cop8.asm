        cpu     cop87l84

        macexp  off

        include regcop8.inc

        nop
        ret
        reti
        retsk
        sc
        rc
        ifc
        ifnc
        vis
        jid
        intr
        laid
        rpnd

        clr     a
        inc     a
        dec     a
        dcor    a
        rrc     a
        rlc     a
        swap    a
        pop     a
        push    a

        adc     a,[b]
        adc     a,0x12
        adc     a,#0x12

        subc    a,[b]
        subc    a,0x12
        subc    a,#0x12

        add     a,[b]
        add     a,0x12
        add     a,#0x12

        ifgt    a,[b]
        ifgt    a,0x12
        ifgt    a,#0x12

        and     a,[b]
        and     a,0x12
        and     a,#0x12

        xor     a,[b]
        xor     a,0x12
        xor     a,#0x12

        or      a,[b]
        or      a,0x12
        or      a,#0x12

        ld      a,[b]
        ld      a,0x12
        ld      a,#0x12
        ld      a,[x]
        ld      r4,#0x12
        ld      b,#12
        ld      b,#0x12
        ld      a,[b+]
        ld      a,[x+]
        ld      a,[b-]
        ld      a,[x-]
        ld      [b],#0x12
        ld      [b+],#0x12
        ld      [b-],#0x12
        ld      0x12,#0x12

        x       a,[b]
        x       [x],a
        x       a,[b+]
        x       [x+],a
        x       a,[b-]
        x       [x-],a
        x       a,0x12

        andsz   a,#0x12

        ifeq    a,[b]
        ifeq    a,0x12
        ifeq    a,#0x12
        ifeq    0x12,#0x12

        ifne    a,[b]
        ifne    a,0x12
        ifne    a,#0x12

        ifbit   3,[b]
        ifbit   5,0x12
        sbit    3,[b]
        sbit    5,0x12
        rbit    3,[b]
        rbit    5,0x12

        ifbne   #7
        ifbne   #0xc

        jmp     0x123
        jsr     0x987

        jmpl    0x1234
        jsrl    0x3456

        jp      .+6
        jp      .-20
	expect	60
         jp      .+1
	endexpect

        drsz    r12
        drsz    r8

reg     sfr     10

        addr    1,2,3,4,5
        addrw   1,2,3,4,5
        byte    1,2,3,4,5
        word    1,2,3,4,5
        dsb     20
        dsw     20
        fb      10,20
        fw      10,20

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
