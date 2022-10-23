	cpu	ibm5100
	page	0

dest	reg	r5
mask	equ	r12
src	reg	r9
ptr	equ	r13

dev1	port	x'a'
dev2	port	b'1110'

var1	equ	x'aa'
var2	equ	x'ac'

	; in a first iteration, use the instruction in the same
        ; order as in t_palm_i.asm, to ease comparison:

	; basic jump instructions

	sle	dest,mask		; 'jle dest,mask' in IBM syntax
	slt	dest,mask		; 'jlo dest,mask' in IBM syntax
	se	dest,mask		; 'jeq dest,mask' in IBM syntax
	sz	dest			; 'jno dest' in IBM syntax
	ss	dest			; 'jall dest,mask' in IBM syntax
	sbs	dest,mask		; 'jallm dest,mask' in IBM syntax
	sbc	dest,mask		; 'jnom dest,mask' in IBM syntax
	sbsh	dest,mask		; 'jham dest,mask' in IBM syntax
	sgt	dest,mask		; 'jhi dest,mask' in IBM syntax
	sge	dest,mask		; 'jhe dest,mask' in IBM syntax
	sne	dest,mask		; 'jhl dest,mask' in IBM syntax
	snz	dest			; 'jsb dest' in IBM syntax
	sns	dest			; 'jsn dest,mask' in IBM syntax
	snbs	dest,mask		; 'jsnm dest,mask' in IBM syntax
	snbc	dest,mask		; 'jsm dest,mask' in IBM syntax
	snbsh	dest,mask		; 'jhsnm dest,mask' in IBM syntax

	; basic arithmetic

	dec2	dest, src		; 'mvm2 dest, src' in IBM syntax
	dec2	dest			; 'mvm2 dest, dest' in IBM syntax
	dec	dest, src		; 'mvm1 dest, src' in IBM syntax
	dec	dest			; 'mvm1 dest, dest' in IBM syntax
	inc	dest, src		; 'mvp1 dest, src' in IBM syntax
	inc	dest			; 'mvp1 dest, dest' in IBM syntax
	inc2	dest, src		; 'mvp2 dest, src' in IBM syntax
	inc2	dest			; 'mvp2 dest, dest' in IBM syntax
	move	dest, src
	and	dest, src
	or	dest, src
	xor	dest, src
	add	dest, src
	sub	dest, src
	addh	dest, src		; 'adds1 dest, src' in IBM syntax
	addh2	dest, src		; 'adds2 dest, src' in IBM syntax
	mhl	dest, src		; 'htl dest, src' in IBM syntax
	mlh	dest, src		; 'lth dest, src' in IBM syntax
	getb	dest, dev1		; 'getr dev1, dest' in IBM syntax
	getadd	dest, dev2		; 'geta dev2, dest' in IBM syntax

	; basic shift & rotate

	shr	dest			; 'shftr dest' in IBM syntax
	ror	dest			; 'rotr dest' in IBM syntax
	ror3	dest			; 'srr3 dest' in IBM syntax
	swap	dest			; 'srr4 dest' in IBM syntax

	; direct load/store

	move	dest,var1		; 'ldhd dest,var1' in IBM syntax
	expect	1351
	move	dest,var1+1
	endexpect
	move	var2,src		; 'sthd src,var2' in IBM syntax
	expect	1351
	move	var2-1,src
	endexpect

	; indirect load/store

	move	dest,(ptr)		; 'ldhi dest,ptr[,0]' in IBM syntax
	move	dest,(ptr)'		; 'ldhi dest,ptr,1' in IBM syntax
	move	dest,(ptr)+		; 'ldhi dest,ptr,2' in IBM syntax
	move	dest,(ptr)+'		; 'ldhi dest,ptr,3' in IBM syntax
	move	dest,(ptr)++		; 'ldhi dest,ptr,4' in IBM syntax
	expect	1110
	move	dest,(ptr)+++		; too much...
	endexpect
	move	dest,(ptr)~		; 'ldhi dest,ptr,-1' in IBM syntax
	move	dest,(ptr)-		; 'ldhi dest,ptr,-2' in IBM syntax
	move	dest,(ptr)-~		; 'ldhi dest,ptr,-3' in IBM syntax
	move	dest,(ptr)--		; 'ldhi dest,ptr,-4' in IBM syntax
	expect	1110
	move	dest,(ptr)---		; too much...
	endexpect

	move	(ptr),dest		; 'sthi dest,ptr[,0]' in IBM syntax
	move	(ptr)',dest		; 'sthi dest,ptr,1' in IBM syntax
	move	(ptr)+,dest		; 'sthi dest,ptr,2' in IBM syntax
	move	(ptr)+',dest		; 'sthi dest,ptr,3' in IBM syntax
	move	(ptr)++,dest		; 'sthi dest,ptr,4' in IBM syntax
	expect	1110
	move	(ptr)+++,dest		; too much...
	endexpect
	move	(ptr)~,dest		; 'sthi dest,ptr,-1' in IBM syntax
	move	(ptr)-,dest		; 'sthi dest,ptr,-2' in IBM syntax
	move	(ptr)-~,dest		; 'sthi dest,ptr,-3' in IBM syntax
	move	(ptr)--,dest		; 'sthi dest,ptr,-4' in IBM syntax
	expect	1110
	move	(ptr)---,dest		; too much...
	endexpect

	movb	dest,(ptr)		; 'ldbi dest,ptr[,0]' in IBM syntax
	movb	dest,(ptr)+		; 'ldbi dest,ptr,1' in IBM syntax
	movb	dest,(ptr)++		; 'ldbi dest,ptr,2' in IBM syntax
	movb	dest,(ptr)+++		; 'ldbi dest,ptr,3' in IBM syntax
	movb	dest,(ptr)++++		; 'ldbi dest,ptr,4' in IBM syntax
	expect	1110
	movb	dest,(ptr)+++++		; too much...
	endexpect
	movb	dest,(ptr)-		; 'ldbi dest,ptr,-1' in IBM syntax
	movb	dest,(ptr)--		; 'ldbi dest,ptr,-2' in IBM syntax
	movb	dest,(ptr)---		; 'ldbi dest,ptr,-3' in IBM syntax
	movb	dest,(ptr)----		; 'ldbi dest,ptr,-4' in IBM syntax
	expect	1110
	movb	dest,(ptr)-----		; too much...
	endexpect

	movb	(ptr),dest		; 'stbi dest,ptr[,0]' in IBM syntax
	movb	(ptr)+,dest		; 'stbi dest,ptr,1' in IBM syntax
	movb	(ptr)++,dest		; 'stbi dest,ptr,2' in IBM syntax
	movb	(ptr)+++,dest		; 'stbi dest,ptr,3' in IBM syntax
	movb	(ptr)++++,dest		; 'stbi dest,ptr,4' in IBM syntax
	expect	1110
	movb	(ptr)+++++,dest		; too much...
	endexpect
	movb	(ptr)-,dest		; 'stbi dest,ptr,-1' in IBM syntax
	movb	(ptr)--,dest		; 'stbi dest,ptr,-2' in IBM syntax
	movb	(ptr)---,dest		; 'stbi dest,ptr,-3' in IBM syntax
	movb	(ptr)----,dest		; 'stbi dest,ptr,-4' in IBM syntax
	expect	1110
	movb	(ptr)-----,dest		; too much...
	endexpect

	; immediate argument

	lbi	dest,#x'aa'		; 'emit dest,#x'aa'' in IBM syntax
	lbi	dest,x'aa'		; 'emit dest,x'aa'' in IBM syntax
	clr	dest,#x'aa'		; 'clri dest,#x'aa'' in IBM syntax
	clr	dest,x'aa'		; 'clri dest,x'aa'' in IBM syntax
	set	dest,#x'aa'		; 'seti dest,#x'aa'' in IBM syntax
	set	dest,x'aa'		; 'seti dest,x'aa'' in IBM syntax

	add	dest,#x'aa'		; 'addi dest,#x'aa'' in IBM syntax
	add	dest,#1			; 'addi dest,#1' in IBM syntax
	add	dest,#256		; 'addi dest,#256' in IBM syntax
	expect	1315
	add	dest,#0
	endexpect
	expect	1320
	add	dest,#257
	endexpect

	sub	dest,#x'aa'		; 'subi dest,#x'aa'' in IBM syntax
	sub	dest,#1			; 'subi dest,#1' in IBM syntax
	sub	dest,#256		; 'subi dest,#256' in IBM syntax
	expect	1315
	sub	dest,#0
	endexpect
	expect	1320
	sub	dest,#257
	endexpect

	; I/O

	ctrl	dev2,#x'cf'		; 'ctl dev2,#x'cf'' in IBM syntax
	ctrl	dev2,b'11001111'	; 'ctl dev2,b'11001111'' in IBM syntax

	putb	dev2,(ptr)		; 'putb dev2,ptr[,0]' in IBM syntax
	putb	dev2,(ptr)+		; 'putb dev2,ptr,1' in IBM syntax
	putb	dev2,(ptr)++		; 'putb dev2,ptr,2' in IBM syntax
	putb	dev2,(ptr)+++		; 'putb dev2,ptr,3' in IBM syntax
	putb	dev2,(ptr)++++		; 'putb dev2,ptr,4' in IBM syntax
	expect	1110
	putb	dev2,(ptr)+++++		; too much :-)
	endexpect
	putb	dev2,(ptr)-		; 'putb dev2,ptr,-1' in IBM syntax
	putb	dev2,(ptr)--		; 'putb dev2,ptr,-2' in IBM syntax
	putb	dev2,(ptr)---		; 'putb dev2,ptr,-3' in IBM syntax
	putb	dev2,(ptr)----		; 'putb dev2,ptr,-4' in IBM syntax
	expect	1110
	putb	dev2,(ptr)-----		; too much :-)
	endexpect

	getb	(ptr),dev1		; 'getb dev1,ptr[,0]' in IBM syntax
	getb	(ptr)+,dev1		; 'getb dev1,ptr,1' in IBM syntax
	getb	(ptr)++,dev1		; 'getb dev1,ptr,2' in IBM syntax
	getb	(ptr)+++,dev1		; 'getb dev1,ptr,3' in IBM syntax
	getb	(ptr)++++,dev1		; 'getb dev1,ptr,4' in IBM syntax
	expect	1110
	getb	(ptr)+++++,dev1		; too much :-)
	endexpect
	getb	(ptr)-,dev1		; 'getb dev1,ptr,-1' in IBM syntax
	getb	(ptr)--,dev1		; 'getb dev1,ptr,-2' in IBM syntax
	getb	(ptr)---,dev1		; 'getb dev1,ptr,-3' in IBM syntax
	getb	(ptr)----,dev1		; 'getb dev1,ptr,-4' in IBM syntax
	expect	1110
	getb	(ptr)-----,dev1		; too much :-)
	endexpect

	stat	r14,13			; 'getrb 13,r14' in IBM syntax

	; now everything again, but in machine code order, like on Christian's page:

i	port	1
ii	equ	x'aa'
j	equ	x'12'
Rx	reg	r1
Ry	reg	r2

	dec2	Rx, Ry
	halt
	dec	Rx, Ry
	inc	Rx, Ry
	inc2	Rx, Ry
	move	Rx, Ry
	nop
	and	Rx, Ry
	or	Rx, Ry
	xor	Rx, Ry
	add	Rx, Ry
	sub	Rx, Ry
	addh	Rx, Ry
	addh2	Rx, Ry
	mhl	Rx, Ry
	mlh	Rx, Ry
	getb	Ry, i
	getadd	Ry, i
	ctrl	i, #j
	move	Rx, ii
	move	ii, Rx
	putb	i, (Rx)+
	putb	i, (Rx)++
	putb	i, (Rx)+++
	putb	i, (Rx)++++
	putb	i, (Rx)-
	putb	i, (Rx)--
	putb	i, (Rx)---
	putb	i, (Rx)----
	putb	i, (Rx)
	move	(Ry)', Rx
	move	(Ry)+, Rx
	move	(Ry)+', Rx
	move	(Ry)++, Rx
	move	(Ry)~, Rx
	move	(Ry)-, Rx
	move	(Ry)-~, Rx
	move	(Ry)--, Rx
	move	(Ry), Rx
	movb	Rx, (Ry)+
	movb	Rx, (Ry)++
	movb	Rx, (Ry)+++
	movb	Rx, (Ry)++++
	movb	Rx, (Ry)-
	movb	Rx, (Ry)--
	movb	Rx, (Ry)---
	movb	Rx, (Ry)----
	movb	Rx, (Ry)
	movb	(Ry)+, Rx
	movb	(Ry)++, Rx
	movb	(Ry)+++, Rx
	movb	(Ry)++++, Rx
	movb	(Ry)-, Rx
	movb	(Ry)--, Rx
	movb	(Ry)---, Rx
	movb	(Ry)----, Rx
	movb	(Ry), Rx
	lbi	Rx, #i
	clr	Rx, #i
	add	Rx, #(i+1)
	set	Rx, #i
	sle	Rx, Ry
	slt	Rx, Ry
	se	Rx, Ry
	sz	Rx
	ss	Rx
	sbs	Rx, Ry
	sbc	Rx, Ry
	sbsh	Rx, Ry
	sgt	Rx, Ry
	sge	Rx, Ry
	sne	Rx, Ry
	snz	Rx
	sns	Rx
	snbs	Rx, Ry
	snbc	Rx, Ry
	snbsh	Rx, Ry

	; NOTE: The assembler checks for attempts to skip a multiword
	; instruction, which is here the case (LWI uses two machine words).
	; Since this is only a synthetic test and not code that will ever
	; be executed, we suppress the warning.

	expect	420
	lwi	Rx, #i
	endexpect
	move	Rx, (Ry)'
	move	Rx, (Ry)+
	move	Rx, (Ry)+'
	move	Rx, (Ry)++
	move	Rx, (Ry)~
	move	Rx, (Ry)-
	move	Rx, (Ry)-~
	move	Rx, (Ry)--
	move	Rx, (Ry)
	shr	Rx
	ror	Rx
	ror3	Rx
	swap	Rx
	getb	(Rx)+, i
	getb	(Rx)++, i
	getb	(Rx)+++, i
	getb	(Rx)++++, i
	getb	(Rx)-, i
	getb	(Rx)--, i
	getb	(Rx)---, i
	getb	(Rx)----, i
	getb	(Rx), i
	stat	Rx, i
	sub	Rx, #(i+1)
