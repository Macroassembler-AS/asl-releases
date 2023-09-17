	cpu	v30
	page	0

	ins	bh,bl
	ins	bh,7
	ext	bh,bl
	ext	bh,7

	rol4	ah
	rol4	[si]
	rol4	[1234h]
	rol4	[bx+12h]

	ror4	ah
	ror4	[si]
	ror4	[1234h]
	ror4	[bx+12h]

	test1	ch,cl
	test1	byte ptr[di],cl
	test1	cx,cl
	test1	word ptr[di],cl
	test1	ch,6
	test1	byte ptr[di],6
	test1	ch,10
	test1	byte ptr[di],10
	test1	cx,10
	test1	word ptr[di],10

	not1	ch,cl
	not1	byte ptr[di],cl
	not1	cx,cl
	not1	word ptr[di],cl
	not1	ch,6
	not1	byte ptr[di],6
	not1	ch,10
	not1	byte ptr[di],10
	not1	cx,10
	not1	word ptr[di],10
	not1	cy		; = CMC (0f5h)

	clr1	ch,cl
	clr1	byte ptr[di],cl
	clr1	cx,cl
	clr1	word ptr[di],cl
	clr1	ch,6
	clr1	byte ptr[di],6
	clr1	ch,10
	clr1	byte ptr[di],10
	clr1	cx,10
	clr1	word ptr[di],10
	clr1	cy		; = CLC (0f8h)

	set1	ch,cl
	set1	byte ptr[di],cl
	set1	cx,cl
	set1	word ptr[di],cl
	set1	ch,6
	set1	byte ptr[di],6
	set1	ch,10
	set1	byte ptr[di],10
	set1	cx,10
	set1	word ptr[di],10
	set1	cy		; = SEC (0f5h)

	fpo2	12
	fpo2	12,ch
	fpo2	12,[bx]
	fpo2	12,[1234h]

	add4s
	sub4s
	cmp4s

	brkem	23h

	repc	cmpsb
	repnc	cmpsw
