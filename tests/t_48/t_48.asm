        cpu     8048

Myreg	reg	r2

        add     a,Myreg
        add     a,@r1
        add     a,#21h

        addc    a,r3
        addc    a,@r1
        addc    a,#21h

        anl     a,r4
        anl     a,@r1
        anl     a,#21h
        anl     bus,#12h

        anld    p5,a

        call    345h

        clr     a
        clr     c
        clr     f0
        clr     f1

        cpl     a
        cpl     c
        cpl     f0
        cpl     f1

        da      a

        dec     a
        dec     r1

        dis     i
        dis     tcnti

        djnz    Myreg,$

        en      i
        en      tcnti

        ent0    clk

        in      a,p1
        in      a,p2

        inc     a
        inc     r7
        inc     @r1

        ins     a,bus

        jb3     $

        jc      $

        jf0     $
        jf1     $

        jmp     123h

        jnc     $

        jni     $

        jnt0    $
        jnt1    $

        jnz     $

        jt0     $
        jt1     $

        jtf     $

        jz      $

        jmpp    @a

        mov     a,Myreg
        mov     a,@r1
        mov     a,#21h
        mov     r3,a
        mov     @r1,a
        mov     r4,#21h
        mov     @r1,#21h
        mov     a,psw
        mov     psw,a
        mov     a,t
        mov     t,a

        movd    a,p5
        movd    p6,a

        movx    a,@r1
        movx    @r1,a

        movp    a,@a
        movp3   a,@a

        nop

        orl     a,r5
        orl     a,@r1
        orl     a,#21h
        orl     bus,#12h

        orld    p5,a

        outl    p1,a
        outl    p2,a
        outl    bus,a

        ret

        retr

        rl      a

        rlc     a

        rr      a

        rrc     a

        sel     mb0
        sel     mb1
        sel     rb0
        sel     rb1

        strt    cnt
        strt    t

        stop    tcnt

        swap    a

        xch     a,r5
        xch     a,@r1

        xchd    a,@r1

        xrl     a,r6
        xrl     a,@r1
        xrl     a,#21h


        cpu     8041

        en      dma
        en      flags

        in      a,dbb

        jnibf   $

        jobf    $

        mov     sts,a

        out     dbb,a


        cpu     80c39

        idl


        cpu     8022

        in      a,p0
        outl    p0,a

        sel     an0
        sel     an1
        rad

	cpu	8039

	assume	mb:nothing
	jmp	856h		; auto-inserts SEL MB1

	assume	mb:0
	expect	110
	jmp	856h		; explicitly wrong bank
	endexpect

	assume	mb:1
	jmp	856h		; no SEL instruction inserted

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
