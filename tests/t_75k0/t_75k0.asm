        cpu     75104
        page    0
        include stddef75

        ret
        reti
        rets
        halt
        stop
        nop

        br	pc+5
        br	pc-5
        br	pc+200h
        br	pc+2000h
	br	$pc+5
        brcb	pc+5
        br	!pc+5
        br	$pc-5
        brcb	pc-5
        br	!pc-5
        brcb	pc+200h
        br	!pc+200h
        br	!pc+2000h
        br	pcde
        br	pcxa

        call	!200h
        call	!2000h
        call	200h
        call	2000h
        callf	200h
        callf	!200h

        adds	a,#5
        adds	xa,#30
        adds	a,@hl
        adds	xa,hl
        adds	xa,de
        adds	xa,bc
        adds	xa,xa'
        adds	xa,hl'
        adds	xa,de`
        adds	xa,bc`
        adds	de,xa

        addc	a,@hl
        addc	xa,bc
        addc	de`,xa

        subs	a,@hl
        subs	xa,bc
        subs	de`,xa

        subc	a,@hl
        subc	xa,bc
        subc	de`,xa

        and	a,#13
        and	a,@hl
        and	xa,bc
        and	de`,xa

        or	a,#13
        or	a,@hl
        or	xa,bc
        or	de`,xa

        xor	a,#13
        xor	a,@hl
        xor	xa,bc
        xor	de`,xa

	incs	d
        incs	de
        incs	@hl
        incs	20h

        decs	d
        decs	de'

        ske	b,#5
        ske	@hl,#6
        ske	a,@hl
        ske	@hl,a
        ske	xa,@hl
        ske	@hl,xa
        ske	a,c
        ske	c,a
        ske	xa,hl'
        ske	hl`,xa

        mov	a,#5
        mov	b,#5
        mov	de,#55h
        mov	a,@hl
        mov	a,@hl+
        mov	a,@hl-
        mov	a,@de
        mov	a,@dl
        mov	xa,@hl
        mov	@hl,a
        mov	@hl,xa
        mov	a,12h
        mov	xa,34h
        mov	56h,a
        mov	78h,xa
        mov	a,c
	mov	xa,bc'
        mov	d,a
        mov	hl`,xa

        xch	a,@hl+
        xch	@hl+,a
        xch	xa,@hl
        xch	@hl,xa
        xch	a,12h
        xch	12h,a
        xch	xa,34h
        xch	34h,xa
        xch	a,d
        xch	d,a
        xch	xa,de
        xch	de,xa

        movt	xa,@pcde
        movt	xa,@pcxa

        mov1	cy,0fb2h.2
        mov1	0ff4h.1,cy
        mov1	cy,0fe4h.@l
        mov1	@h+13.3,cy

        set1	cy
	set1	40h.2
        set1	0ff2h.3

        clr1	cy
	clr1	40h.2
        clr1	0ff2h.3

        skt 	cy
	skt 	40h.2
        skt 	0ff2h.3

	skf 	40h.2
        skf 	0ff2h.3

	not1	cy
        sktclr	0ff2h.3

        and1	cy,0ff2h.3
        or1	cy,0ff2h.3
        xor1	cy,0ff2h.3

        rorc	a
        not	a

        push	bs
        pop	bs
        push	hl
        pop	bc

        in	a,port3
        in	xa,port12
        out	port10,a
        out	port7,xa

        ei
        di
        ei	ieks
        di	iew

        sel	rb2
        sel	mb10

        geti	15

bit1	bit	0ff0h.@l
bit2	bit	0fb0h.2
bit3	bit	0ff0h.1
bit4	bit	0430h.3
bit5    bit     @h+5+3.2

	set1	bit1
	expect	110
         clr1    bit4
	endexpect
        sel     mb4
        set1	mbe
        assume  mbs:4,mbe:1
        clr1    bit4

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
