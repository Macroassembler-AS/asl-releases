		cpu	32060

                page	0

mv		macro	src,dest
		add.ATTRIBUTE 0,src,dest
                endm

neg		macro	src,dest
		sub.ATTRIBUTE 0,src,dest
                endm

not		macro	src,dest
		xor.ATTRIBUTE -1,src,dest
                endm

                not	a0,a1

zero		macro	dest
                xor.ATTRIBUTE dst,dst,dst
                endm

                mv	a1,a3
                mv.l2	a1,b3

        	nop
||      	nop
||      	nop
        	nop
                nop

	[b0]    nop
|| 	[!b0]	nop

		idle

                nop
                nop	7

	abs	a4,a7
        abs.l1	a4,a7
        abs	b4,a7
        abs.l1	b4,a7
	abs.l1x b4,a7
        abs	b3:b2,b11:b10
        abs	a7:a6,b11:b10

        add	a1,a3,a7:a6
        add	a1,b3,a7:a6
        add	b1,a3,a7:a6
	add	a7,a5:a4,a13:a12
        add     b7,a5:a4,a13:a12
	add	a5:a4,a7,a13:a12
        add     a5:a4,b7,a13:a12
        add	b9:b8,-3,b15:b14
        add	-3,b9:b8,b15:b14

	addab.d1  a4,a2,a4
        addah.d1  a4,a2,a4
        addaw.d2  b4,2,b4
	subab.d1  a4,a2,a4
        subah.d1  a4,a2,a4
        subaw.d2  b4,2,b4

        addk	15401,a1
        addk.s1 15401,a1

        add2	a1,b1,a2
        sub2	b1,a0,b2

        clr	a1,4,19,a2
        clr	b1,b3,b2

        cmpeq.l2 a1,b1,b2
        cmpeq.l1 -9,a1,a2
        cmpeq.l2 a1,b3:b2,b1
        cmpgt.l1 a1,b2,a2
        cmpgt.l1 a1,b1,a2
        cmpgt.l1 8,a1,a2
        cmpgt.l1 a1,b1,a2
        cmpgtu.l1 a1,a2,a3
        cmpgtu.l1 0ah,a1,a2
        cmpgtu.l1 0eh,a3:a2,a4
        cmplt.l1 a1,a2,a3
        cmplt.l1 9,a1,a2
        cmpltu.l1 a1,a2,a3
        cmpltu.l1 14,a1,a2
        cmpltu.l1 a1,a5:a4,a2

        ext.s1	a1,10,19,a2
        ext.s1	a1,a2,a3
        extu.s1	a1,10,19,a2
        extu.s1 a1,a2,a3

        ldw.d1	*a10,a1
        ldb.d1	*-a5[4],a7
        ldh.d1	*++a4[a1],a8
        ldw.d1	*a4++[1],a6
        ldw.d1	*++a4(4),a6
        ldb.d2	*+b14[36],b1

        lmbd	a1,a2,a3

        mpy.m1	a1,a2,a3
        mpyu.m1 a1,a2,a3
        mpyus.m1 a1,a2,a3
        mpy.m1 13,a1,a2
        mpysu.m1 13,a1,a2
        mpyh.m1 a1,a2,a3
        mpyhu.m1 a1,a2,a3
        mpyhsu.m1 a1,a2,a3
	mpyhl.m1 a1,a2,a3
        mpylh.m1 a1,a2,a3

	mvc.s2	a1,amr
        mvc	istp,b4

        mvk.s1	293,a1
        mvk.s2	125h,b1
        mvk.s1	0ff12h,a1
        mvkh.s1	0a329123h,a1
        mvklh	7a8h,a1

        norm	a1,a2
        norm	a1:a0,a2

        sadd.l1	a1,a2,a3
        sadd.l1 b2,a5:a4,a7:a6

        sat.l1	a1:a0,a2
        sat.l2	b1:b0,b5

        set.s1	a0,7,21,a1
        set.s2	b0,b1,b2

        shl.s1	a0,4,a1
        shl.s2	b0,b1,b2
        shl.s2	b1:b0,b2,b3:b2

        shr.s1	a0,8,a1
        shr.s2	b0,b1,b2
        shr.s2	b1:b0,b2,b3:b2

        shru.s1	a0,8,a1

        smpy.m1	a1,a2,a3
        smpyhl.m1 a1,a2,a3
        smpylh.m1 a1,a2,a3

        sshl.s1	a0,2,a1
        sshl.s1	a0,a1,a2

        ssub.l2	b1,b2,b3
        ssub.l1	a0,a1,a2

        stb.d1	a1,*a10
        sth.d1	a1,*+a10(4)
        stw.d1	a1,*++a10[1]
        sth.d1	a1,*a10--[a11]
        stb.d2	b1,*+b14[40]

        align   32
        mvk.s1	2c80h,a0
||      mvk.s2	0200h,b0
	mvkh.s1 01880000h,a0
||	mvkh.s2 00000000h,b0
	mvc.s2	a0,pdata_o
        stp.s2	b0
        nop	4
        mpy.m1	a1,a2,a3

        sub.l1	a1,a2,a3
        subu.l1	a1,a2,a5:a4
        subc.l1	a0,a1,a0

        align   32
        ifdef error
        add.s1  a0,a1,a2
||      shr.s1  a3,15,a4
        endif
        add.l1  a0,a1,a2
||      shr.s1  a3,15,a4

        align   32
        ifdef   error
        add.l1x a0,b1,a1
||      mpy.m1x a4,b4,a5
        endif
        add.l1x a0,b1,a1
||      mpy.m2x a4,b4,b2

        align   32
        ifdef   error
        ldw.d1  *a0,a1
||      ldw.d1  *a2,b2
        endif
        ldw.d1  *a0,a1
||      ldw.d2  *b0,b2

        align   32
        ifdef   error
        ldw.d1  *a4,a5
||      stw.d2  a6,*b4
        endif
        ldw.d1  *a4,b4
||      stw.d2  a6,*b4

        align   32
        ifdef   error
        add.l1  a5:a4,a1,a3:a2
||      shl.s1  a8,a9,a7:a6
        endif
        add.l1  a5:a4,a1,a3:a2
||      shl.s2  b8,b9,b7:b6

        align   32
        ifdef   error
        add.l1  a5:a4,a1,a3:a2
||      stw.d1  a8,*a9
        endif
        add.l1  a4,a1,a3:a2
||      stw.d1  a8,*a9

        align   32
        ifdef   error
        mpy.m1  a1,a1,a4
||      add.l1  a1,a1,a5
||      sub.d1  a1,a2,a3
        endif
        mpy.m1  a1,a1,a4
|| [a1] add.l1  a0,a1,a5
||      sub.d1  a1,a2,a3

        align   32
        ifdef   error
        add.l2  b5,b6,b7
||      sub.s2  b8,b9,b7
        endif
[!b0]   add.l2  b5,b6,b7
|| [b0] sub.s2  b8,b9,b7

	; packed mode (4 chars per word)

	data	0
	data	12345678h
	data	4294967295
	data	-2147483648
	data	'A'
	data	'AB'
	data	'ABC'
	data	'ABCD'
	data	"A"
	data	"AB"
	data	"ABC"
	data	"ABCD"
	data	"Hello, world!"

	; unpacked mode (one char per word)
	packing off
	data	0
	data	12345678h
	data	4294967295
	data	-2147483648
	data	'A'
	data	'AB'
	data	'ABC'
	data	'ABCD'
	data	"A"
	data	"AB"
	data	"ABC"
	data	"ABCD"
	data	"Hello, world!"
