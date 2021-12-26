	cpu	z80undoc
	page	0

acc	equ	a
cnt	equ	bc
cnt_h	equ	b
cnt_l	equ	c
src	equ	de
src_h	equ	d
src_l	equ	e
dst	equ	hl
dst_h	equ	h
dst_l	equ	l
sptr	equ	sp

isrc	equ	ix
isrc_h	equ	ixh
xh	reg	ixh
isrc_l	equ	ixl
xl	reg	ixl
idst	equ	iy
idst_h	equ	iyh
yh	reg	iyh
idst_l	equ	iyl
yl	reg	iyl

	ld	a,ixh
	ld	a,isrc_h
	ld	a,xh

	cpl	a
	cpl	acc
	neg	a
	neg	acc

	ld	i,a
	ld	i,acc
	ld	a,i
	ld	acc,i
	ld	r,a
	ld	r,acc
	ld	a,r
	ld	acc,r

	out	(c),0
	out	(cnt_l),0
	out	(c),a
	out	(cnt_l),a
	in	a,(c)
	in	a,(cnt_l)
	out	(20h),a
	out	20h,a
	in	a,20h
	in	a,(20h)

	ld	a,(bc)
	ld	acc,(cnt)
	ld	a,(de)
	ld	acc,(src)
	ld	a,(hl)
	ld	acc,(dst)
	ld	a,(ix+12h)
	ld	acc,(isrc+12h)

	ex	de,hl
	ex	src,dst
	ex	hl,de
	ex	dst,src

	irp	reg,hl,dst,ix,isrc,iy,idst
	ex	(sp),reg
	ex	reg,(sp)
	endm

	jp	(hl)
	jp	(dst)
	jp	(ix)
	jp	(isrc)
	jp	(iy)
	jp	(idst)
	jp	(isrc+0)

	cpu	z380

	exts	a
	exts	acc
	cplw	hl
	cplw	dst
	negw	hl
	negw	dst
	extsw	hl
	extsw	dst

	ld	i,hl
	ld	i,dst
	ld	hl,i
	ld	dst,i

	sub	hl,(1234h)
	sub	dst,(1234h)
	sub	sp,1234h
	sub	sptr,1234h

	ina	a,20h
	ina	acc,20h
	inaw	hl,20h
	inaw	dst,20h
	outa	20h,a
	outa	20h,acc
	outaw	20h,hl
	outaw	20h,dst

	inw	hl,(c)
	inw	hl,(cnt_l)
	outw	(c),de
	outw	(cnt_l),de
	outw	(c),1234h
	outw	(cnt_l),1234h

	multw	hl,de
	multw	dst,src

	ex	(hl),a
	ex	(dst),acc
	ex	a,(hl)
	ex	acc,(dst)

	ex	de,de'
	ex	src,src'
