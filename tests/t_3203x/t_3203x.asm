                cpu     320C30
		relaxed	on
		page	0

		org	$123456

targ:		br	targ
		brd	targ
		call	targ

		bu	r0
		bud	r0
		bu	targ
		bud	targ
		blo	ar1
		blsd	targ
		bhi	ir0
		bhs	targ
		beqd	ar4
		bne	targ
		blt	targ
		ble	r10
		bgtd	targ
		bge	r6
		bzd	targ
		bnz	ar5
		bpd	targ
		bn	targ
		bnnd	ar3
		bnv	targ
		bv	ar7
		bnufd	ir1
		buf	targ
		bnc	ar2
		bcd	iof
		bnlv	targ
		blv	targ
		bnluf	targ
		bluf	targ
		bzuf	targ
		b	targ

		callne	r4
		callnluf targ

		dbne	ar1,r5
		dbn	ar5,targ
		dbud	ar2,ar7
		dbzufd	ar6,targ
		db	ar7,targ

		retine
		retsnc
		retsu
		rets

		trapu	7
		trapuf	2

		swi
		sigi
		idle

		rol	r4
		rolc	ar2
		ror	sp
		rorc	r20

		rptb	targ

		nop	ar2
		nop	*+ar3(2)
		nop	*-ar4(200)
		nop	*++ar5(30)
		nop	*--ar6
		nop	*ar5++(20)
		nop	*ar1--(12h)
		nop	*ar7++($56)%
		nop	*ar0--(0x10)%
		nop	*+ar6(ir0)
		nop	*-ar4(ir0)
		nop	*++ar2(ir0)
		nop	*--ar2(ir0)
		nop	*ar2++(ir0)
		nop	*ar2--(ir0)
		nop	*ar2++(ir0)%
		nop	*ar2--(ir0)%
		nop	*+ar6(ir1)
		nop	*-ar4(ir1)
		nop	*++ar2(ir1)
		nop	*--ar2(ir1)
		nop	*ar2++(ir1)
		nop	*ar2--(ir1)
		nop	*ar2++(ir1)%
		nop	*ar2--(ir1)%
		nop	*ar4
		nop	*ar3(100-$64)
		nop	*ar1++(ir0)B
		iack	*ar5
		rpts	ar5

		absf	r4
		absf	ar2,r5
		absf	@$1300,r7
		absf    *ar4++,r3
		absf	200,r5

		absi	r4
		absi	ar2,r5
		absi	@$1300,r7
		absi    *ar4++,r3
		absi	200,r5
		;
		addc	ar2,r5
		addc	@$1300,r7
		addc    *ar4++,r3
		addc	200,r5

                addc3	*ar1++(1),*ar2,r5
                addc3	*-ar3,r5,r2
                addc3	r6,*ar4++,r3
                addc3	r1,r2,r3

                stf	r4,@2000h
                stf	r6,*ar5

                tstb3	r5,*ar3++

                absf	*ar4++,r6
||		stf	r6,*ar5++

		sti	r5,*ar3
||		absi	*ar4++%,r1

		addf3	*ar4++,r5,r7
||		stf	r3,*ar5++

		sti	r3,*ar5++
||		addi3	*ar4++,r5,r7

		mpyi3	*ar4,*ar5,r1
||		subi3	r6,r7,r3

		subi3	*ar4,r6,r3
||		mpyi3	*ar5,r7,r1

		mpyi3	r7,*ar5,r1
||		subi3	*ar4,r6,r3

		mpyi3	*ar5,r7,r1
||		subi3	r6,*ar4,r3

		mpyi3	r7,*ar5,r1
||		subi3	r6,*ar4,r3

		mpyi3	r6,r7,r1
||		subi3	*ar5,*ar4,r3

		absf	*++ar3(ir1) ,r4
||		stf	r4,*-ar7(1)

		absi	*-ar5(1),r5
||		sti	r1,*ar2--(ir1)

		addf3	*+ar3(ir1),r2,r5
||		stf	r4,*ar2

		addi3	*ar0--(ir0),r5,r0
||		sti	r3,*ar7

		and3	*+ar1(ir0),r4,r7
||		sti	r3,*ar2

		ash3	r1,*ar6++(ir1),r0
||		sti	r5,*ar2

		fix	*++ar4(1),r1
||		sti	r0,*ar2

		float	*+ar2(ir0),r6
||		stf	r7,*ar1

		ldf	*--ar1(ir0),r7
||		ldf	*ar7++(1),r3

		ldf	*ar2--(1),r1
||		stf	r3,*ar4++(ir1)

		ldi	*-ar1(1),r7
||		ldi	*ar7++(ir0),r1

		ldi	*-ar1(1),r2
||		sti	r7,*ar5++(ir0)

		lsh3	r7,*ar2--(1),r2
||		sti	r0,*+ar0(1)

		mpyf3	*ar5++(1),*--ar1(ir0),r0
||		addf3	r5,r7,r3

		mpyf3	*-ar2(1),r7,r0
||		stf	r3,*ar0--(ir0)

		mpyf3	r5,*++ar7(ir1),r0
||		subf3	r7,*ar3--(1),r2

		mpyi3	r7,r4,r0
||		addi3	*-ar3,*ar5--(1),r3

		mpyi3	*++ar0(1),r5,r7
||		sti	r2,*-ar3(1)

		mpyi3	r2,*++ar0(1),r0
||		subi3	*ar5--(ir1),r4,r2

		negf	*ar4--(1),r7
||		stf	r2,*++ar5(1)

		negi	*-ar3,r2
||		sti	r2,*ar1++

		not	*+ar2,r3
||		sti	r7,*--ar4(ir1)

		or3	*++ar2,r5,r2
||		sti	r6,*ar1--

		stf	r4,*ar3--
||		stf	r3,*++ar5

		sti	r0,*++ar2(ir0)
||		sti	r5,*ar0

		subf3	r1,*-ar4(ir1),r0
||		stf	r7,*+ar5(ir0)

		subi3	r7,*+ar2(ir0),r1
||		sti	r3,*++ar7

		xor3	*ar1++,r3,r3
||		sti	r6,*-ar2(ir0)

		xor3	*ar1++,r3,r3
		||sti	r6,*-ar2(ir0)

		ldfz	r3,r5
		ldfzuf	20h,r6
		ldiz	r4,r6
		ldp	@123456h,dp

		pop	r3
		popf	r4
		push	r6
		pushf	r2

		ldfz	1.27578125e+01,r4

		ldi	*ar5,r6
		expect	140,210
		 ||ldi	 *ar5++,r6
		endexpect

		expect	210
		 addf3	 *ar5++,*ar5++,r3
		endexpect

		; packed mode (4 chars per word)

		single	 1.79750e+02
		single	-6.281250e+01
		single	-9.90337307e+27
		single   9.90337307e+27
		single	-6.118750e+01, 1.79750e+02
		extended 9.90337307e+27
		bss	20h
		word	20,55,'ABCD'
		data	12345h,-1.2345e6,"Hello world"
		data	0
		data	4294967295
		data	-2147483648
		data	'a'
		data	'ab'
		data	'abc'
		data	'abcd'
		data	"a"
		data	"ab"
		data	"abc"
		data	"abcd"

		; unpacked mode (one char per word)

		packing	off
		data	12345h,-1.2345e6,"Hello world"
		data	"abcd"
