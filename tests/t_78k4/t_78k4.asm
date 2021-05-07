	cpu	784026
	page	0

	assume	location:0

saddr1	equ	0feaah
saddr1_ equ	0feaeh
saddr2	equ	0ff10h
saddr2_	equ	0ff14h
sfr	equ	0ff80h
addr16	equ	4711h
addr20	equ	6789ah
addr24	equ	0deadbfh

	; r1,#byte
	mov	a,#12h
	mov	x,#23h
	mov	b,#34h
	mov	c,#45h
	mov	r4,#56h
	mov	r5,#67h
	mov	r6,#78h
	mov	r7,#89h
	; r2,#byte
	mov	r8,#9ah
	mov	r9,#0abh
	mov	r10,#0bch
	mov	r11,#0cdh
	mov	d,#0deh
	mov	e,#0efh
	mov	h,#0f0h
	mov	l,#01h
	; saddr2,#byte
	mov	saddr2,#55h
	; saddr1,#byte
	mov	saddr1,#55h
	; sfr,#byte
	mov	pswl,#55h
	; !addr16,#byte
	mov	!saddr1,#55h
	mov	addr16,#55h
	; !addr24,#byte
	mov	!!saddr1,#55h
	mov	!!addr16,#55h
	mov	addr24,#55h
	; r,r1
	mov	b,a
	mov	b,x
	mov	b,b
	mov	b,c
	mov	b,r4
	mov	b,r5
	mov	b,r6
	mov	b,r7
	; r,r2
	mov	b,vpl
	mov	b,vph
	mov	b,upl
	mov	b,uph
	mov	b,d
	mov	b,e
	mov	b,h
	mov	b,l
	; A,r1
	mov	a,a
	mov	a,x
	mov	a,b
	mov	a,c
	mov	a,r4
	mov	a,r5
	mov	a,r6
	mov	a,r7
	; A,r2
	mov	a,vpl
	mov	a,vph
	mov	a,upl
	mov	a,uph
	mov	a,d
	mov	a,e
	mov	a,h
	mov	a,l
	; A,saddr2
	mov	a,saddr2
	; r,saddr2
	mov	x,saddr2
	mov	b,saddr2
	mov	c,saddr2
	mov	d,saddr2
	mov	e,saddr2
	mov	h,saddr2
	mov	l,saddr2
	; r,saddr1
	mov	a,saddr1
	mov	x,saddr1
	mov	b,saddr1
	mov	c,saddr1
	mov	d,saddr1
	mov	e,saddr1
	mov	h,saddr1
	mov	l,saddr1
	; saddr2,A
	mov	saddr2,a
	; saddr2,r
	mov	saddr2,x
	mov	saddr2,b
	mov	saddr2,c
	mov	saddr2,d
	mov	saddr2,e
	mov	saddr2,h
	mov	saddr2,l
	; saddr1,r
	mov	saddr1,a
	mov	saddr1,x
	mov	saddr1,b
	mov	saddr1,c
	mov	saddr1,d
	mov	saddr1,e
	mov	saddr1,h
	mov	saddr1,l
	; A,sfr
	mov	a,pswl
	; r,sfr
	mov	x,pswl
	mov	b,pswl
	mov	c,pswl
	mov	d,pswl
	mov	e,pswl
	mov	h,pswl
	mov	l,pswl
	; sfr,A
	mov	pswl,a
	; sfr,r
	mov	pswl,x
	mov	pswl,b
	mov	pswl,c
	mov	pswl,d
	mov	pswl,e
	mov	pswl,h
	mov	pswl,l
	; saddr2,saddr2'
	mov	saddr2,saddr2_
	; saddr2,saddr1
	mov	saddr2,saddr1
	; saddr1,saddr2
	mov	saddr1,saddr2
	; saddr1,saddr1'
	mov	saddr1,saddr1_
	; r,!addr16
	mov	a,addr16
	mov	x,addr16
	mov	b,addr16
	mov	c,addr16
	mov	d,addr16
	mov	e,addr16
	mov	h,addr16
	mov	l,addr16
	; !addr16,r
	mov	addr16,a
	mov	addr16,x
	mov	addr16,b
	mov	addr16,c
	mov	addr16,d
	mov	addr16,e
	mov	addr16,h
	mov	addr16,l
	; r,!addr24
	mov	a,addr24
	mov	x,addr24
	mov	b,addr24
	mov	c,addr24
	mov	d,addr24
	mov	e,addr24
	mov	h,addr24
	mov	l,addr24
	; !addr24,r
	mov	addr24,a
	mov	addr24,x
	mov	addr24,b
	mov	addr24,c
	mov	addr24,d
	mov	addr24,e
	mov	addr24,h
	mov	addr24,l
	; A,[saddrp2]
	mov	a,[saddr2]
	; A,[saddrp1]
	mov	a,[saddr1]
	; A,[%saddrg2]
	mov	a,[%saddr2]
	; A,[%saddrg1]
	mov	a,[%saddr1]
	; A,mem
	mov	a,[tde+]
	mov	a,[whl+]
	mov	a,[tde-]
	mov	a,[whl-]
	mov	a,[tde]
	mov	a,[whl]
	mov	a,[vvp]
	mov	a,[uup]
	mov	a,[tde+55h]
	mov	a,[sp+55h]
	mov	a,[whl+55h]
	mov	a,[uup+55h]
	mov	a,[vvp+55h]
	mov	a,123456h[de]
	mov	a,123456h[a]
	mov	a,123456h[hl]
	mov	a,123456h[b]
	mov	a,[tde+a]
	mov	a,[whl+a]
	mov	a,[tde+b]
	mov	a,[whl+b]
	mov	a,[vvp+de]
	mov	a,[vvp+hl]
	mov	a,[tde+c]
	mov	a,[whl+c]
	; [saddrp2],A
	mov	[saddr2],a
	; [saddrp1],A
	mov	[saddr1],a
	; [%saddrg2],A
	mov	[%saddr2],a
	; [%saddrg1],A
	mov	[%saddr1],a
	; mem,a
	mov	[tde+],a
	mov	[whl+],a
	mov	[tde-],a
	mov	[whl-],a
	mov	[tde],a
	mov	[whl],a
	mov	[vvp],a
	mov	[uup],a
	mov	[tde+55h],a
	mov	[sp+55h],a
	mov	[whl+55h],a
	mov	[uup+55h],a
	mov	[vvp+55h],a
	mov	123456h[de],a
	mov	123456h[a],a
	mov	123456h[hl],a
	mov	123456h[b],a
	mov	[tde+a],a
	mov	[whl+a],a
	mov	[tde+b],a
	mov	[whl+b],a
	mov	[vvp+de],a
	mov	[vvp+hl],a
	mov	[tde+c],a
	mov	[whl+c],a
	; built-in PSWL = 0fffeh
	mov	pswl,#34h
	mov	pswh,#34h
	mov	pswl,a
	mov	pswh,a
	mov	a,pswl
	mov	a,pswh
	; rU16,#byte
	mov	v,#12h
	mov	u,#34h
	mov	t,#56h
	mov	w,#78h
	; A,rU16
	mov	a,v
	mov	a,u
	mov	a,t
	mov	a,w
	; rU16,A
	mov	v,a
	mov	u,a
	mov	t,a
	mov	w,a

	; rp,#word
	mov	ax,#1234h
	movw	ax,#1234h
	mov	bc,#1234h
	movw	bc,#1234h
	mov	de,#1234h
	movw	de,#1234h
	mov	hl,#1234h
	movw	hl,#1234h
	mov	vp,#1234h
	movw	vp,#1234h
	mov	up,#1234h
	movw	up,#1234h
	; saddrp2,#word
	movw	saddr2,#4711h
	; saddrp1,#word
	movw	saddr1,#4711h
	; sfrp,#word
	movw	psw,#4711h
	; !addr16,#word
	movw	addr16,#4711h
	; !!addr24,#word
	movw	addr24,#4711h
	; rp,rp'
	irp	rpd,ax,bc,rp2,rp3,vp,up,de,hl
	irp	rps,ax,bc,rp2,rp3,vp,up,de,hl
	mov	rpd,rps
	endm
	endm
	; rp,saddrp2
	mov	ax,saddr2
	movw	bc,saddr2
	mov	de,saddr2
	movw	hl,saddr2
	; rp,saddrp1
	mov	ax,saddr1
	movw	bc,saddr1
	mov	de,saddr1
	movw	hl,saddr1
	; saddrp2,rp
	mov	saddr2,ax
	movw	saddr2,bc
	mov	saddr2,de
	movw	saddr2,hl
	; saddrp1,rp
	mov	saddr1,ax
	movw	saddr1,bc
	mov	saddr1,de
	movw	saddr1,hl
	; rp,sfrp
	mov	ax,psw
	movw	bc,psw
	mov	de,psw
	movw	hl,psw
	; sfrp,rp
	mov	psw,ax
	movw	psw,bc
	mov	psw,de
	movw	psw,hl
	; saddrp2,saddrp2'
	movw	saddr2,saddr2_
	; saddrp2,saddrp1
	movw	saddr2,saddr1
	; saddrp1,saddrp2
	movw	saddr1,saddr2
	; saddrp1,saddrp1'
	movw	saddr1,saddr1_
	; rp,!abs16
	mov	ax,addr16
	movw	bc,addr16
	mov	de,addr16
	movw	hl,addr16
	; !abs16,rp
	mov	addr16,ax
	movw	addr16,bc
	mov	addr16,de
	movw	addr16,hl
	; rp,!!abs24
	mov	ax,addr24
	movw	bc,addr24
	mov	de,addr24
	movw	hl,addr24
	; !!abs24,rp
	mov	addr24,ax
	movw	addr24,bc
	mov	addr24,de
	movw	addr24,hl
	; AX,[saddrp2]
	mov	ax,[saddr2]
	; AX,[saddrp1]
	mov	ax,[saddr1]
	; AX,[%saddrg2]
	mov	ax,[%saddr2]
	; AX,[%saddrg1]
	mov	ax,[%saddr1]
	; AX,mem
	mov	ax,[tde+]
	mov	ax,[whl+]
	mov	ax,[tde-]
	mov	ax,[whl-]
	mov	ax,[tde]
	mov	ax,[whl]
	mov	ax,[vvp]
	mov	ax,[uup]
	mov	ax,[tde+55h]
	mov	ax,[sp+55h]
	mov	ax,[whl+55h]
	mov	ax,[uup+55h]
	mov	ax,[vvp+55h]
	mov	ax,123456h[de]
	mov	ax,123456h[a]
	mov	ax,123456h[hl]
	mov	ax,123456h[b]
	mov	ax,[tde+a]
	mov	ax,[whl+a]
	mov	ax,[tde+b]
	mov	ax,[whl+b]
	mov	ax,[vvp+de]
	mov	ax,[vvp+hl]
	mov	ax,[tde+c]
	mov	ax,[whl+c]
	; [saddrp2],AX
        mov     [saddr2],ax
        ; [saddrp1],AX
        mov     [saddr1],ax
	; [%saddrg2],AX
        mov     [%saddr2],ax
        ; [%saddrg1],AX
        mov     [%saddr1],ax
	; mem,AX
	mov	[tde+],ax
	mov	[whl+],ax
	mov	[tde-],ax
	mov	[whl-],ax
	mov	[tde],ax
	mov	[whl],ax
	mov	[vvp],ax
	mov	[uup],ax
	mov	[tde+55h],ax
	mov	[sp+55h],ax
	mov	[whl+55h],ax
	mov	[uup+55h],ax
	mov	[vvp+55h],ax
	mov	123456h[de],ax
	mov	123456h[a],ax
	mov	123456h[hl],ax
	mov	123456h[b],ax
	mov	[tde+a],ax
	mov	[whl+a],ax
	mov	[tde+b],ax
	mov	[whl+b],ax
	mov	[vvp+de],ax
	mov	[vvp+hl],ax
	mov	[tde+c],ax
	mov	[whl+c],ax

	; rg,#imm24
	mov	vvp,#123456h
	movg	rg4,#123456h
	mov	uup,#123456h
	movg	rg5,#123456h
	mov	tde,#123456h
	movg	rg6,#123456h
	mov	whl,#123456h
	movg	rg7,#123456h
	; rg,rg'
	irp	dest,vvp,uup,tde,whl
	irp	src,rg4,rg5,rg6,rg7
	mov	dest,src
	movg	dest,src
	endm
	endm
	; rg,!!addr24
	mov	vvp,addr16
	mov	uup,addr24
	mov	tde,addr16
	mov	whl,addr24
	; !!addr24,rg
	mov	addr16,vvp
	mov	addr24,uup
	mov	addr16,tde
	mov	addr24,whl
	; rg,saddrg2
	mov	vvp,saddr2
        mov     uup,saddr2
        mov     tde,saddr2
        mov     whl,saddr2
	; rg,saddrg1
	mov	vvp,saddr1
        mov     uup,saddr1
        mov     tde,saddr1
        mov     whl,saddr1
	; saddrg2,rg
	mov	saddr2,vvp
        mov     saddr2,uup
        mov     saddr2,tde
        mov     saddr2,whl
	; saddrg1,rg
	mov	saddr1,vvp
        mov     saddr1,uup
        mov     saddr1,tde
        mov     saddr1,whl
	; WHL,[saddrg2]
        movg	whl,[%saddr2]
	; WHL,[saddrg1]
        movg	whl,[%saddr1]
	; [saddrg2],WHL
        movg	[%saddr2],whl
	; [saddrg1],WHL
        movg	[%saddr1],whl
	; WHL,mem
	mov	whl,[tde+]
	;mov	whl,[whl+]	; forbidden
	mov	whl,[tde-]
	;mov	whl,[whl-]	; forbidden
	mov	whl,[tde]
	mov	whl,[whl]
	mov	whl,[vvp]
	mov	whl,[uup]
	mov	whl,[tde+55h]
	mov	whl,[sp+55h]
	mov	whl,[whl+55h]
	mov	whl,[uup+55h]
	mov	whl,[vvp+55h]
	mov	whl,123456h[de]
	mov	whl,123456h[a]
	mov	whl,123456h[hl]
	mov	whl,123456h[b]
	mov	whl,[tde+a]
	mov	whl,[whl+a]
	mov	whl,[tde+b]
	mov	whl,[whl+b]
	mov	whl,[vvp+de]
	mov	whl,[vvp+hl]
	mov	whl,[tde+c]
	mov	whl,[whl+c]
	; mem,WHL
	mov	[tde+],whl
	;mov	[whl+],whl	; forbidden
	mov	[tde-],whl
	;mov	[whl-],whl	; forbidden
	mov	[tde],whl
	mov	[whl],whl
	mov	[vvp],whl
	mov	[uup],whl
	mov	[tde+55h],whl
	mov	[sp+55h],whl
	mov	[whl+55h],whl
	mov	[uup+55h],whl
	mov	[vvp+55h],whl
	mov	123456h[de],whl
	mov	123456h[a],whl
	mov	123456h[hl],whl
	mov	123456h[b],whl
	mov	[tde+a],whl
	mov	[whl+a],whl
	mov	[tde+b],whl
	mov	[whl+b],whl
	mov	[vvp+de],whl
	mov	[vvp+hl],whl
	mov	[tde+c],whl
	mov	[whl+c],whl

	; r,r1/r2
        irp	dest,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
        irp	src,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	xch	dest,src
	endm
	endm
	; A,r1
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	xch	a,reg
	xch	reg,a
	endm
	; A,saddr2
	xch	a,saddr2
	xch	saddr2,a
	; r,saddr2
	irp	reg,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	xch	reg,saddr2
	xch	saddr2,reg
	endm
	; r,saddr1
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	xch	reg,saddr1
	xch	saddr1,reg
	endm
	; r,sfr
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	xch	reg,sfr
	xch	sfr,reg
	endm
	; saddr2,saddr2'
	xch	saddr2,saddr2_
	; saddr2,saddr1
	xch	saddr2,saddr1
	; saddr1,saddr2
	xch	saddr1,saddr2
	; saddr1,saddr1'
	xch	saddr1,saddr1_
	; r,!addr16
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
        xch     reg,addr16
        xch     addr16,reg
	endm
	; r,!!addr24
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
        xch     reg,addr24
        xch     addr24,reg
	endm
	; A,[saddrp2]
	xch	a,[saddr2]
	xch	[saddr2],a
	; A,[saddrp1]
	xch	a,[saddr1]
	xch	[saddr1],a
	; A,[%saddrg2]
	xch	a,[%saddr2]
	xch	[%saddr2],a
	; A,[%saddrg1]
	xch	a,[%saddr1]
	xch	[%saddr1],a
	; A,mem
	xch	a,[tde+]
	xch	[tde+],a
	xch	a,[whl+]
	xch	[whl+],a
	xch	a,[tde-]
	xch	[tde-],a
	xch	a,[whl-]
	xch	[whl-],a
	xch	a,[tde]
	xch	[tde],a
	xch	a,[whl]
	xch	[whl],a
	xch	a,[vvp]
	xch	[vvp],a
	xch	a,[uup]
	xch	[uup],a
	xch	a,[tde+55h]
	xch	[tde+55h],a
	xch	a,[sp+55h]
	xch	[sp+55h],a
	xch	a,[whl+55h]
	xch	[whl+55h],a
	xch	a,[uup+55h]
	xch	[uup+55h],a
	xch	a,[vvp+55h]
	xch	[vvp+55h],a
	xch	a,123456h[de]
	xch	123456h[de],a
	xch	a,123456h[a]
	xch	123456h[a],a
	xch	a,123456h[hl]
	xch	123456h[hl],a
	xch	a,123456h[b]
	xch	123456h[b],a
	xch	a,[tde+a]
	xch	[tde+a],a
	xch	a,[whl+a]
	xch	[whl+a],a
	xch	a,[tde+b]
	xch	[tde+b],a
	xch	a,[whl+b]
	xch	[whl+b],a
	xch	a,[vvp+de]
	xch	[vvp+de],a
	xch	a,[vvp+hl]
	xch	[vvp+hl],a
	xch	a,[tde+c]
	xch	[tde+c],a
	xch	a,[whl+c]
	xch	[whl+c],a

	; rp,rp'
	irp	dest,ax,bc,rp2,rp3,vp,up,de,hl
	irp	src,ax,bc,rp2,rp3,vp,up,de,hl
	xch	dest,src
	xchw	dest,src
	endm
	endm
	; AX,saddrp2
	xch	ax,saddr2
	xchw	saddr2,ax
	; rp,saddrp2
	irp	reg,bc,rp2,rp3,vp,up,de,hl
	xch	reg,saddr2
	xchw	saddr2,reg
	endm
	; rp,saddrp1
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	xch	reg,saddr1
	xchw	saddr1,reg
	endm
	; rp,sfr
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	xch	reg,sfr
	xchw	sfr,reg
	endm
	; AX,[saddrp2]
	xch	ax,[saddr2]
	xchw	[saddr2],ax
	; AX,[saddrp1]
	xch	ax,[saddr1]
	xchw	[saddr1],ax
	; AX,[%saddrg2]
	xch	ax,[%saddr2]
	xchw	[%saddr2],ax
	; AX,[%saddrg1]
	xch	ax,[%saddr1]
	xchw	[%saddr1],ax
	; AX,!addr16
	xch	ax,addr16
	xch	addr16,ax
	; AX,!!addr24
	xch	ax,addr24
	xch	addr24,ax
	; saddrp2,saddrp2'
	xchw	saddr2,saddr2_
	; saddrp2,saddrp1
	xchw	saddr2,saddr1
	; saddrp1,saddrp2
	xchw	saddr1,saddr2
	; saddrp1,saddrp1'
	xchw	saddr1,saddr1_
	; AX,mem
	xch	ax,[tde+]
	xch	[tde+],ax
	xch	ax,[whl+]
	xch	[whl+],ax
	xch	ax,[tde-]
	xch	[tde-],ax
	xch	ax,[whl-]
	xch	[whl-],ax
	xch	ax,[tde]
	xch	[tde],ax
	xch	ax,[whl]
	xch	[whl],ax
	xch	ax,[vvp]
	xch	[vvp],ax
	xch	ax,[uup]
	xch	[uup],ax
	xch	ax,[tde+55h]
	xch	[tde+55h],ax
	xch	ax,[sp+55h]
	xch	[sp+55h],ax
	xch	ax,[whl+55h]
	xch	[whl+55h],ax
	xch	ax,[uup+55h]
	xch	[uup+55h],ax
	xch	ax,[vvp+55h]
	xch	[vvp+55h],ax
	xch	ax,123456h[de]
	xch	123456h[de],ax
	xch	ax,123456h[a]
	xch	123456h[a],ax
	xch	ax,123456h[hl]
	xch	123456h[hl],ax
	xch	ax,123456h[b]
	xch	123456h[b],ax
	xch	ax,[tde+a]
	xch	[tde+a],ax
	xch	ax,[whl+a]
	xch	[whl+a],ax
	xch	ax,[tde+b]
	xch	[tde+b],ax
	xch	ax,[whl+b]
	xch	[whl+b],ax
	xch	ax,[vvp+de]
	xch	[vvp+de],ax
	xch	ax,[vvp+hl]
	xch	[vvp+hl],ax
	xch	ax,[tde+c]
	xch	[tde+c],ax
	xch	ax,[whl+c]
	xch	[whl+c],ax

	irp	instr,add,addc,sub,subc,cmp,and,or,xor
	; A,#byte
	instr	a,#45h
	irp	reg,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	reg,#45h
	endm
	; saddr2,#byte
	instr	saddr2,#45h
	; saddr1,#byte
	instr	saddr1,#45h
	; sfr,#byte
	instr	sfr,#45h
	; r,r1/r2
	irp	dest,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	irp	src,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	dest,src
	endm
	endm
	; A,saddr2
	instr	a,saddr2
	; r,saddr2
        irp	reg,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	reg,saddr2
	endm
	; A,saddr1
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	reg,saddr1
	endm
	; saddr2,r
        irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	saddr2,reg
	endm
	; saddr1,r
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	saddr1,reg
	endm
	; r,sfr
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	reg,sfr
	endm
	; sfr,r
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	sfr,reg
	endm
	; saddr2,saddr2'
	instr	saddr2,saddr2_
	; saddr2,saddr1
	instr	saddr2,saddr1
	; saddr1,saddr2
	instr	saddr1,saddr2
	; saddr1,saddr1'
	instr	saddr1,saddr1_
	; A,[saddrp2]
	instr	a,[saddr2]
	; A,[saddrp1]
	instr	a,[saddr1]
	; A,[%saddrg2]
	instr	a,[%saddr2]
	; A,[%saddrg1]
	instr	a,[%saddr1]
	; [saddrp2],A
	instr	[saddr2],a
	; [saddrp1],A
	instr	[saddr1],a
	; [%saddrg2],A
	instr	[%saddr2],a
	; [%saddrg1],A
	instr	[%saddr1],a
	; A,!abs16
	instr	a,addr16
	; A,!!abs24
	instr	a,addr24
	; !abs16,A
	instr	addr16,a
	; !!abs24,A
	instr	addr24,a
	; A,mem
	instr	a,[tde+]
	instr	a,[whl+]
	instr	a,[tde-]
	instr	a,[whl-]
	instr	a,[tde]
	instr	a,[whl]
	instr	a,[vvp]
	instr	a,[uup]
	instr	a,[tde+55h]
	instr	a,[sp+55h]
	instr	a,[whl+55h]
	instr	a,[uup+55h]
	instr	a,[vvp+55h]
	instr	a,123456h[de]
	instr	a,123456h[a]
	instr	a,123456h[hl]
	instr	a,123456h[b]
	instr	a,[tde+a]
	instr	a,[whl+a]
	instr	a,[tde+b]
	instr	a,[whl+b]
	instr	a,[vvp+de]
	instr	a,[vvp+hl]
	instr	a,[tde+c]
	instr	a,[whl+c]
	; mem,A
	instr	[tde+],a
	instr	[whl+],a
	instr	[tde-],a
	instr	[whl-],a
	instr	[tde],a
	instr	[whl],a
	instr	[vvp],a
	instr	[uup],a
	instr	[tde+55h],a
	instr	[sp+55h],a
	instr	[whl+55h],a
	instr	[uup+55h],a
	instr	[vvp+55h],a
	instr	123456h[de],a
	instr	123456h[a],a
	instr	123456h[hl],a
	instr	123456h[b],a
	instr	[tde+a],a
	instr	[whl+a],a
	instr	[tde+b],a
	instr	[whl+b],a
	instr	[vvp+de],a
	instr	[vvp+hl],a
	instr	[tde+c],a
	instr	[whl+c],a
	endm

	; note that when stringifying the instruction field, the string is also
	; expanded while a macro body is read.  This is necessary because while reading
	; it, the assembler has to check for instructions like 'endm'.  So assure the
	; string variable is set to something harmless before the outer IRP is read.

si	set	""
	irp	instr,add,sub,cmp
si	set	"INSTR"
	; AX/rp,#word
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg,#0aa55h
	{si}w	reg,#0aa55h
	endm
	; rp,rp'
	irp	dest,ax,bc,rp2,rp3,vp,up,de,hl
	irp	src,ax,bc,rp2,rp3,vp,up,de,hl
	instr	dest,src
	{si}w	dest,src
	endm
	endm
	; AX/rp,saddrp2
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg,saddr2
	{si}w	reg,saddr2
	endm
	; rp,saddrp1
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg,saddr1
	{si}w	reg,saddr1
	endm
	; saddrp2,rp
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	saddr2,reg
	{si}w	saddr2,reg
	endm
	; saddrp1,rp
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	saddr1,reg
	{si}w	saddr1,reg
	endm
	; rp,sfp
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg,sfr
	{si}w	reg,sfr
	endm
	; sfp,rp
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	sfr,reg
	{si}w	sfr,reg
	endm
	; saddrp2,#word
	{si}w	saddr2,#6789h
	; saddrp1,#word
	{si}w	saddr1,#6789h
	; sfrp,#word
	{si}w	sfr,#6789h
	; saddrp2,saddrp2'
	{si}w	saddr2,saddr2_
	; saddrp2,saddrp1
	{si}w	saddr2,saddr1
	; saddrp1,saddrp2
	{si}w	saddr1,saddr2
	; saddrp1,saddrp1'
	{si}w	saddr1,saddr1_
	endm

si	set	""
	irp	instr,add,sub
si	set	"INSTR"
	; rg,rg'
	irp	dest,vvp,uup,tde,whl
	irp	src,rg4,rg5,rg6,rg7
	instr	dest,src
	{si}g	dest,src
	endm
	endm
	; rg,#imm24
	irp	reg,vvp,uup,tde,whl
	instr	reg,#654321h
	{si}g	reg,#654321h
	endm
	; WHL,saddrg2
	instr	whl,saddr2
	instr	whl,saddr1
	endm

	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	mulu	reg
	divuw	reg
	endm
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	mulu	reg
	muluw	reg
	mulw	reg
	divux	reg
	endm

	macw	12
	macsw	100
	sacw	[tde+],[whl+]

si	set	""
	irp	instr,inc,dec
si	set	"INSTR"
	; r1/r2
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	reg
	endm
	; saddr2
	instr	saddr2
	; saddr1
	instr	saddr1
	; rp
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg
	{si}w	reg
	endm
	; saddrp2
	{si}w	saddr2
	; saddrp1
	{si}w	saddr1
	; rg
	irp	reg,vvp,uup,tde,whl
	instr	reg
	{si}g	reg
	endm
	endm

	adjba
	adjbs
	cvtbw

	irp	instr,ror,rol,rorc,rolc,shr,shl
	irp	reg,a,x,b,c,r5,r4,r7,r6,vph,vpl,uph,upl,d,e,h,l
	instr	reg,5
	endm
	endm
si	set	""
	irp	instr,shr,shl
si	set 	"INSTR"
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg,6
	{si}w	reg,3
	endm
	endm

	irp	instr,ror4,rol4
	irp	reg,rp0,rg4,rp1,rg5,rp2,rg6,rp3,rg7
	instr	[reg]
	endm
	endm

s2bit	bit	saddr2.1
s1bit	bit	saddr1.2
sfrbit	bit	sfr.3
pswlbit	bit	pswl.4
pswhbit	bit	pswh.5
abit	bit	a.6
xbit	bit	x.7
tdebit	bit	[tde].1
whlbit	bit	[whl].2
a16bit	bit	addr16.3
a24bit	bit	addr24.4

	mov	cy,s2bit
	mov1	cy,s2bit
	mov	cy,s1bit
	mov1	cy,s1bit
	mov	cy,sfrbit
	mov1	cy,sfrbit
	mov	cy,abit
	mov1	cy,abit
	mov	cy,xbit
	mov1	cy,xbit
	mov	cy,pswlbit
	mov1	cy,pswlbit
	mov	cy,pswhbit
	mov1	cy,pswhbit
	mov	cy,tdebit
	mov1	cy,tdebit
	mov	cy,whlbit
	mov1	cy,whlbit
	mov	cy,a16bit
	mov1	cy,a16bit
	mov	cy,a24bit
	mov1	cy,a24bit
	mov	s2bit,cy
	mov1	s2bit,cy
	mov	s1bit,cy
	mov1	s1bit,cy
	mov	sfrbit,cy
	mov1	sfrbit,cy
	mov	abit,cy
	mov1	abit,cy
	mov	xbit,cy
	mov1	xbit,cy
	mov	pswlbit,cy
	mov1	pswlbit,cy
	mov	pswhbit,cy
	mov1	pswhbit,cy
	mov	tdebit,cy
	mov1	tdebit,cy
	mov	whlbit,cy
	mov1	whlbit,cy
	mov	a16bit,cy
	mov1	a16bit,cy
	mov	a24bit,cy
	mov1	a24bit,cy

si	set	""
	irp	instr,and,or
si	set	"INSTR"
	instr	cy,s2bit
	{si}1	cy,s2bit
	instr	cy,s1bit
	{si}1	cy,s1bit
	instr	cy,/s2bit
	{si}1	cy,/s2bit
	instr	cy,/s1bit
	{si}1	cy,/s1bit
	instr	cy,sfrbit
	{si}1	cy,sfrbit
	instr	cy,/sfrbit
	{si}1	cy,/sfrbit
	instr	cy,xbit
	{si}1	cy,xbit
	instr	cy,/xbit
	{si}1	cy,/xbit
	instr	cy,abit
	{si}1	cy,abit
	instr	cy,/abit
	{si}1	cy,/abit
	instr	cy,pswlbit
	{si}1	cy,pswlbit
	instr	cy,/pswlbit
	{si}1	cy,/pswlbit
	instr	cy,pswhbit
	{si}1	cy,pswhbit
	instr	cy,/pswhbit
	{si}1	cy,/pswhbit
	instr	cy,tdebit
	{si}1	cy,tdebit
	instr	cy,/tdebit
	{si}1	cy,/tdebit
	instr	cy,whlbit
	{si}1	cy,whlbit
	instr	cy,/whlbit
	{si}1	cy,/whlbit
	instr	cy,a16bit
	{si}1	cy,a16bit
	instr	cy,/a16bit
	{si}1	cy,/a16bit
	instr	cy,a24bit
	{si}1	cy,a24bit
	instr	cy,/a24bit
	{si}1	cy,/a24bit
	endm

	xor	cy,s2bit
	xor1	cy,s2bit
	xor	cy,s1bit
	xor1	cy,s1bit
	xor	cy,sfrbit
	xor1	cy,sfrbit
	xor	cy,xbit
	xor1	cy,xbit
	xor	cy,abit
	xor1	cy,abit
	xor	cy,pswlbit
	xor1	cy,pswlbit
	xor	cy,pswhbit
	xor1	cy,pswhbit
	xor	cy,tdebit
	xor1	cy,tdebit
	xor	cy,whlbit
	xor1	cy,whlbit
	xor	cy,a16bit
	xor1	cy,a16bit
	xor	cy,a24bit
	xor1	cy,a24bit

	irp	instr,not1,set1,clr1
	instr	s2bit
	instr	s1bit
	instr	sfrbit
	instr	xbit
	instr	abit
	instr	pswlbit
	instr	pswhbit
	instr	tdebit
	instr	whlbit
	instr	a16bit
	instr	a24bit
	instr	cy
	endm

	push	psw
	pushw	psw
	pushw	sfr
	push	sfr
	push	bc,rp3,up,hl
	pushw	bc,rp3,up,hl
	push	vvp
	pushg	vvp
	push	uup
	pushg	uup
	push	tde
	pushg	tde
	push	whl
	pushg	whl
	pushu	psw
	pushu	bc,rp3,psw,hl	
	pushuw	bc,rp3,psw,hl
	pop	psw
	popw	psw
	popw	sfr
	pop	sfr
	pop	bc,rp3,up,hl
	popw	bc,rp3,up,hl
	pop	vvp
	popg	vvp
	pop	uup
	popg	uup
	pop	tde
	popg	tde
	pop	whl
	popg	whl
	popu	psw
	popu	bc,rp3,psw,hl	
	popuw	bc,rp3,psw,hl

	mov	sp,#123456h
	movg	sp,#123456h
	mov	sp,whl
	movg	sp,whl
	mov	whl,sp
	movg	whl,sp

	add	sp,#1234h
	addwg	sp,#1234h
	sub	sp,#1234h
	subwg	sp,#1234h

	inc	sp
	incg	sp
	dec	sp
	decg	sp

	irp	instr,call,br
	instr	addr16+8000h	; force usage of !addr16 by value
	instr	!addr16
	instr	addr20
	instr	!!addr20
	irp	reg,ax,bc,rp2,rp3,vp,up,de,hl
	instr	reg
	instr	[reg]
	endm
	irp	reg,vvp,uup,tde,whl
	instr	reg
	instr	[reg]
	endm
	instr	PC+10
	instr	$PC+10
	instr	$!PC+10
	instr	PC+200
	instr	$!PC+200
	instr	PC-10
	instr	$PC-10
	instr	$!PC-10
	instr	PC-200
	instr	$!PC-200
	endm

	callf	0c23h
	callf	!0c23h
	callt	[60h]
	callt	[!60h]
	callt	[40h]
	callt	[!40h]
	callt	[7eh]
	callt	[!7eh]
	brk
	brkcs	rb6
	ret
	reti
	retb
	retcs	addr16
	retcs	!addr16
	retcsb	addr16

	bnz	PC+1
	bne	PC+2
	bz	PC+3
	be	PC+4
	bnc	PC+5
	bnl	PC+6
	bc	PC+7
	bl	PC+8
	bnv	PC+9
	bpo	PC+10
	bv	PC+11
	bpe	PC+12
	bp	PC+13
	bn	PC+14
	blt	PC+15
	bge	PC+16
	ble	PC+17
	bgt	PC+18
	bnh	PC+19
	bh	PC+20

	irp	instr,bf,bt,btclr,bfset
	instr	s2bit,PC+1
	instr	s1bit,PC+2
	instr	sfrbit,PC+3
	instr	xbit,PC+4
	instr	abit,PC+5
	instr	pswlbit,PC+6
	instr	pswhbit,PC+7
	instr	tdebit,PC+8
	instr	whlbit,PC+9
	instr	a16bit,PC+10
	instr	a24bit,PC+11
	endm

	dbnz	b,PC+1
	dbnz	c,PC+2
	dbnz	saddr2,PC+3
	dbnz	saddr1,PC+4

	mov	stbc,#55h
	mov	wdm,#0aah
	location 0
	location 15
	sel	rb5
	sel	rb2,alt
	swrs
	nop
	ei
	di

	chkl	pswl
	chkla	pswh

	movtblw	20h,45h
	movm	[tde+],a
	movm	[tde-],a
	movbk	[tde+],[whl+]
	movbk	[tde-],[whl-]
	xchm	[tde+],a
	xchm	[tde-],a
	xchbk	[tde+],[whl+]
	xchbk	[tde-],[whl-]
	cmpme	[tde+],a
	cmpme	[tde-],a
	cmpbke	[tde+],[whl+]
	cmpbke	[tde-],[whl-]
	cmpmne	[tde+],a
	cmpmne	[tde-],a
	cmpbkne	[tde+],[whl+]
	cmpbkne	[tde-],[whl-]
	cmpmc	[tde+],a
	cmpmc	[tde-],a
	cmpbkc	[tde+],[whl+]
	cmpbkc	[tde-],[whl-]
	cmpmnc	[tde+],a
	cmpmnc	[tde-],a
	cmpbknc	[tde+],[whl+]
	cmpbknc	[tde-],[whl-]
