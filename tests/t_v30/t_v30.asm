	cpu	v30
	page	0

	; register names used by NEC

aw	reg	ax
bw	reg	bx
cw	reg	cx
dw	reg	dx
ix	reg	si
iy	reg	di
ps	reg	cs
ds0	reg	ds
ds1	reg	es

	ins	bh,bl
	ins	bh,7
	ext	bh,bl
	ext	bh,7

	rol4	ah
	rol4	[ix]
	rol4	[1234h]
	rol4	[bw+12h]

	ror4	ah
	ror4	[ix]
	ror4	[1234h]
	ror4	[bw+12h]

	test1	ch,cl
	test1	byte ptr[iy],cl
	test1	cw,cl
	test1	word ptr[iy],cl
	test1	ch,6
	test1	byte ptr[iy],6
	test1	ch,10
	test1	byte ptr[iy],10
	test1	cw,10
	test1	word ptr[iy],10

	not1	ch,cl
	not1	byte ptr[iy],cl
	not1	cw,cl
	not1	word ptr[iy],cl
	not1	ch,6
	not1	byte ptr[iy],6
	not1	ch,10
	not1	byte ptr[iy],10
	not1	cw,10
	not1	word ptr[iy],10
	not1	cy		; = CMC (0f5h)

	clr1	ch,cl
	clr1	byte ptr[iy],cl
	clr1	cw,cl
	clr1	word ptr[iy],cl
	clr1	ch,6
	clr1	byte ptr[iy],6
	clr1	ch,10
	clr1	byte ptr[iy],10
	clr1	cw,10
	clr1	word ptr[iy],10
	clr1	cy		; = CLC (0f8h)

	set1	ch,cl
	set1	byte ptr[iy],cl
	set1	cw,cl
	set1	word ptr[iy],cl
	set1	ch,6
	set1	byte ptr[iy],6
	set1	ch,10
	set1	byte ptr[iy],10
	set1	cw,10
	set1	word ptr[iy],10
	set1	cy		; = SEC (0f5h)

	fpo2	12
	fpo2	12,ch
	fpo2	12,[bw]
	fpo2	12,[1234h]

	add4s
	sub4s
	cmp4s

	brkem	23h

	repc	cmpsb
	repnc	cmpsw
