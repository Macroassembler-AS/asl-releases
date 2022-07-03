	cpu	7725

	jmp	lab
	call	lab
	jnca	lab
	jncb	lab
        jcb	lab
        jnza    lab
	jza     lab
   	jnzb    lab
        jzb     lab
   	jnova0  lab
        jova0   lab
   	jnovb0  lab
        jovb0   lab
   	jnova1  lab
        jova1   lab
   	jnovb1  lab
        jovb1   lab
   	jnsa0   lab
        jsa0    lab
   	jnsb0   lab
        jsb0    lab
   	jnsa1   lab
        jsa1    lab
   	jnsb1   lab
        jsb1    lab
   	jdpl0   lab
        jdplf   lab
   	jnsiak  lab
        jsiak   lab
   	jnsoak  lab
        jsoak   lab
   	jnrqm   lab
        jrqm    lab
	jdpln0	lab
	jdplnf	lab
lab:

	ldi	@non,1234h
	ldi	@a,1234h
	ldi	@b,1234h
	ldi	@tr,1234h
	ldi	@dp,1234h
	ldi	@rp,1234h
	ldi	@dr,1234h
	ldi	@sr,1234h
	ldi	@sol,1234h
	ldi	@som,1234h
	ldi	@k,1234h
	ldi	@klr,1234h
	ldi	@klm,1234h
	ldi	@l,1234h
	ldi	@mem,1234h
	ldi	@trb,1234h

	op	mov	@a,non
	op	mov	@a,a
	op	mov	@a,b
	op	mov	@a,tr
	op	mov	@a,dp
	op	mov	@a,rp
	op	mov	@a,ro
	op	mov	@a,sgn
	op	mov	@a,dr
	op	mov	@a,drnf
	op	mov	@a,sr
	op	mov	@a,sim
	op	mov	@a,sil
	op	mov	@a,k
	op	mov	@a,l
	op	mov	@a,mem

	op	mov	@a,non
		or	acca,ram
	op	mov	@a,non
		or	accb,ram
	op      mov     @a,non
		or	acca,idb
	op      mov     @a,non
		or      acca,m
	op      mov     @a,non
		or      acca,n
	op	mov     @a,non
		and	acca,ram
	op      mov     @a,non
		xor	acca,ram
	op      mov     @a,non
		sub	acca,ram
	op      mov     @a,non
		add	acca,ram
	op      mov     @a,non
		sbb	acca,ram
	op      mov     @a,non
		adc	acca,ram
	op      mov     @a,non
		cmp	acca,ram
	op	mov     @a,non
		inc	accb
	op	mov	@a,non
		dec	acca
	op      mov     @a,non
		shr1	accb
	op	mov	@a,non
		shl1	acca
	op	mov	@a,non
		shl2	accb
	op	mov	@a,non
		shl4	acca
	op	mov	@a,non
		xchg	accb
	op	mov	@a,non
		nop

        op      mov     @a,non
		dpnop
        op      mov     @a,non
		dpinc
        op      mov     @a,non
		dpdec
        op      mov     @a,non
		dpclr

	op	mov	@a,non
		m0
	op	mov	@a,non
		m1
	op	mov	@a,non
		m2
	op	mov	@a,non
		m3
	op	mov	@a,non
		m4
	op	mov	@a,non
		m5
	op	mov	@a,non
		m6
	op	mov	@a,non
		m7
	op	mov	@a,non
		m8
	op	mov	@a,non
		m9
	op	mov	@a,non
		ma
	op	mov	@a,non
		mb
	op	mov	@a,non
		mc
	op	mov	@a,non
		md
	op	mov	@a,non
		me
	op	mov	@a,non
		mf

	op	mov	@a,non
		rpnop
	op	mov	@a,non
		rpdec

	; packed storage

	data	0
	data	1,2,3
	data	16777215
	data	-10000
	data	-8388608
	data	'ABC'
	data	"a"
	data	"ab"
	data	"abc"
	data	"abcd"
	data	"abcde"
	data	"abcdef"
	data	"abcdefg"
	data	"abcdefgh"

	; unpacked storage
	packing	off
	data	"a"
	data	"ab"
	data	"abc"
	data	"abcd"
	data	"abcde"
	data	"abcdef"
	data	"abcdefg"
	data	"abcdefgh"
