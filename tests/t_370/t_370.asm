		page	0

		cpu	370c010

cat		equ	r16
dog		equ	r011
mouse		equ	p055
rat		equ	p127

		clrc
		setc
		dint
		eint
		eintl
		einth
		idle
		ldsp
		stsp
		nop
		rti
		rts

Targ:		jmp	Targ
		jc	Targ
		jeq	Targ
		jg	Targ
		jge	Targ
		jhs	Targ
		jl	Targ
		jle	Targ
		jlo	Targ
		jn	Targ
		jnc	Targ
		jne	Targ
		jnv	Targ
		jnz	Targ
		jp	Targ
		jpz	Targ
		jv	Targ
		jz	Targ

		adc	b,a
		add	dog,a
		dac	cat,b
		dsb	dog,cat
		sbb	#55h,a
		sub	#0aah,b
		mpy	#' ',dog

		and	a,p050
		btjo	b,a,Targ
		btjz	b,p10,Targ
		or	cat,a
		xor	dog,b
		and	r020,r10
		btjo	#55,a,Targ
		btjz	#66,b,Targ
		or	#77,r10
		xor	#88,rat

		br	Targ
		br	@dog
		br	Targ(b)
		br	10(cat)
		call	Targ
		call	@dog
		call	Targ(b)
		call	10(cat)
		callr	Targ
		callr	@dog
		callr	Targ(b)
		callr	10(cat)
		jmpl	Targ
		jmpl	@dog
		jmpl	Targ(b)
		jmpl	10(cat)

		clr	a
		compl	b
		dec	cat
		inc	a
		inv	b
		pop	dog
		push	a
		pop	st
		push	st
		rl	b
		rlc	r020
		rr	a
		rrc	b
		swap	dog
		xchb	a
		djnz	b,$

		cmp	2000h,a
		cmp	@dog,a
		cmp	targ(b),a
		cmp	10(cat),a
		cmp	-5(sp),a
		cmp	b,a
		cmp	dog,a
		cmp	cat,b
		cmp	cat,dog
		cmp	#55h,a
		cmp	#66h,b
		cmp	#77h,r0ff

bit1		dbit	1,r12
bit2		dbit	4,p033
bit3		dbit	5,b

		cmpbit	bit1
		cmpbit	bit2
		jbit0	bit1,$
		jbit0	bit2,$
		jbit1	bit1,$
		jbit1	bit2,$
		sbit0	bit1
		sbit0	bit2
		sbit1	bit1
		sbit1	bit2

		div	r45,a
		incw	#56h,dog
		ldst	#12

		mov	a,b
		mov	a,cat
		mov	a,mouse
		mov	a,1234h
		mov	a,@r33
		mov	a,Targ(b)
		mov	a,15(r015)
		mov	a,-2(sp)
		mov	dog,a
		mov	cat,b
		mov	1234h,a
		mov	@dog,a
		mov	Targ(b),a
		mov	-33(cat),a
		mov	15(sp),a
		mov	b,a
		mov	b,dog
		mov	b,rat
		mov	cat,dog
		mov	dog,mouse
		mov	rat,a
		mov	p15,b
		mov	p15,r015
		mov	#11h,a
		mov	#-1,b
		mov	#0110110b,r10
		mov	#10h,rat

		movw	cat,dog
		movw	#12345,r010
		movw	#Targ(b),cat
		movw	#(cat),cat
		movw	#123(cat),cat		; F4 E8 7B 10 10

		trap	7

		tst	a
		tst	b

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
