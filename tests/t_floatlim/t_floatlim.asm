	page	0

l10_max	equ	log(floatmax)

	; only fulfilled with extended float support
	; compute a 'large float' that is an exact power of two,
	; to avoid rounding issues

	if	l10_max > 616
x	set	65536.0
	rept	7
x	set	x*x
	endm
	endif

	cpu	68020

	ifdef	inf
	dc.s	inf
	dc.s	-inf
	dc.d	inf
	dc.d	-inf
	dc.x	inf
	dc.x	-inf
	dc.p	inf
	dc.p	-inf
	elseif
	dc.w	$7f80,$0000
	dc.w	$ff80,$0000
	dc.w	$7ff0,$0000,$0000,$0000
	dc.w	$fff0,$0000,$0000,$0000
	dc.w	$7fff,$0000,$8000,$0000,$0000,$0000            
	dc.w	$ffff,$0000,$8000,$0000,$0000,$0000            
        dc.w	$7fff,$0000,$0000,$0000,$0000,$0000            
	dc.w	$ffff,$0000,$0000,$0000,$0000,$0000            
	endif

	ifdef	x
	dc.x	x
	elseif
	dc.w	$47ff,$0000,$8000,$0000,$0000,$0000
	endif

	cpu	8086

	ifdef	inf
	dw	inf
	dw	-inf
	dd	inf
	dd	-inf
	dq	inf
	dq	-inf
	dt	inf
	dt	-inf
	do	inf
	do	-inf
	elseif
	db	00h,7ch
	db	00h,0fch
	db	00h,00h,80h,7fh
	db	00h,00h,80h,0ffh
	db	00h,00h,00h,00h,00h,00h,0f0h,7fh
	db	00h,00h,00h,00h,00h,00h,0f0h,0ffh
	db	00h,00h,00h,00h,00h,00h,00h,80h,0ffh,7fh
	db	00h,00h,00h,00h,00h,00h,00h,80h,0ffh,0ffh
	db	00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,0ffh,7fh
	db	00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,0ffh,0ffh
	endif

	ifdef	x
	dt	x
	elseif
	db	00h,00h,00h,00h,00h,00h,00h,80h,0ffh,047h
	endif