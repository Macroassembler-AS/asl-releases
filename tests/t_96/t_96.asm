		cpu	80196

                include reg96

ax		equ	20h
al		equ	ax
ah		equ	ax+1
eax		equ	ax
bx		equ	24h
bl		equ	bx
bh		equ	bx+1
ebx		equ	bx
cx		equ	28h
cl		equ	cx
ch		equ	cx+1
ecx		equ	cx
dx		equ	2ch
dl		equ	dx
dh		equ	dx+1
edx		equ	dx

		CLRC
		CLRVT
		DI
		EI
		NOP
		POPF
		PUSHF
		RET
		RST
		SETC
		TRAP
                PUSHA
                POPA
                EPTS
                DPTS

targ:
		JC 	targ
		JE	targ
		JGE	targ
		JGT	targ
		JH	targ
		JLE	targ
		JLT	targ
		JNC	targ
		JNE	targ
		JNH	targ
		JNST	targ
		JNV	targ
		JNVT	targ
		JST	targ
		JV	targ
		JVT	targ

                bmov	eax,cx
                bmovi	eax,cx

		add	ax,bx
		add	ax,2000h
		add	ax,[bx]
		add	ax,[bx]+
		add	ax,2[bx]
		add	ax,-15[bx]
		add	ax,700[bx]
		add	ax,-300[bx]
		add	ax,#1234h

		add	ax,cx,bx
		add	ax,cx,2000h
		add	ax,cx,[bx]
		add	ax,cx,[bx]+
		add	ax,cx,2[bx]
		add	ax,cx,-15[bx]
		add	ax,cx,700[bx]
		add	ax,cx,-300[bx]
		add	ax,cx,#1234h

		addb	al,bl
		addb	al,2000h
		addb	al,[bx]
		addb	al,[bx]+
		addb	al,2[bx]
		addb	al,-15[bx]
		addb	al,700[bx]
		addb	al,-300[bx]
		addb	al,#12h

		addb	al,cl,bl
		addb	al,cl,2000h
		addb	al,cl,[bx]
		addb	al,cl,[bx]+
		addb	al,cl,2[bx]
		addb	al,cl,-15[bx]
		addb	al,cl,700[bx]
		addb	al,cl,-300[bx]
		addb	al,cl,#12h

		and	dx,300h
		mulu	eax,bx,cx
		mulb	ax,cl,ch
		subb	cl,#5

		addc	ax,bx
		addcb	al,[bx]
		cmp	ax,[bx]+
		cmpb	al,2[bx]
                cmpl	ecx,edx
		div	eax,-15[bx]
		divb	ax,200[bx]
		divu	eax,-300[bx]
		divub	ax,200
		ld	ax,#2345h
		ldb	al,#16
		st	ax,bx
		stb	al,[bx]
		subc	ax,[bx]+
		subcb	al,2[bx]
		xor	ax,-15[bx]
		xorb	al,200[bx]

		push	ax
		push	[bx]
		push	#1234h
		pop	2000h
		pop	10[cx]

                xch	ax,bx
		expect	380
                xch	ax,[bx]
		endexpect
                xch	ax,10[bx]
                xch	ax,-150[bx]
		expect	1350
                xch	ax,[bx]+
		endexpect
                xch	ax,2000h
                xchb	bl,al
		expect	380
                xchb	[bx],al
		endexpect
                xchb	10[bx],al
                xchb	-150[bx],al
		expect	1350
                xchb	[bx]+,al
		endexpect
                xchb	2000h,al

		clr	ax
		clrb	al
		dec	bx
		decb	bh
		ext	eax
		extb	ax
		inc	cx
		incb	cl
		neg	dx
		negb	dh
		not	ax
		notb	al

		scall	targ
		expect	20
		 lcall	 targ
		endexpect
		call	targ

		sjmp	targ
		expect	20
		 ljmp	 targ
		endexpect
		br	targ
		br	[dx]

		djnz	cl,$
                djnzw	cx,$

		jbc	dh,3,$
		jbs	al,1,$

                tijmp	bx,ax,#127

		ldbse	ax,#-1
		ldbze	cx,[bx]+

		norml	eax,cl

		shl	ax,#5
		shl	ax,cl
		shlb	al,#6
		shlb	al,cl
		shll	eax,#7
		shll	eax,cl
		shr	ax,#5
		shr	ax,cl
		shrb	al,#6
		shrb	al,cl
		shrl	eax,#7
		shrl	eax,cl
		shra	ax,#5
		shra	ax,cl
		shrab	al,#6
		shrab	al,cl
		shral	eax,#7
		shral	eax,cl

		skip	dl

                idlpd	#2


                ldb	al,100h		; lang
                ldb	al,0c0h		; kurz
                ldb	al,000h		; kurz
                ldb	al,140h		; lang
                ldb     al,[0c0h]
                ldb     al,[000h]

                assume	wsr:24h		; =100h..13fh auf 0c0h..0ffh

                ldb	al,100h		; jetzt kurz
                ldb	al,0c0h		; jetzt lang
                ldb	al,000h		; immmer noch kurz
                ldb	al,140h		; immer noch lang
                ldb     al,[100h]
                ldb     al,[000h]

                bne     2000h
                bc      2000h
