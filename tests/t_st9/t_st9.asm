        cpu     st9040
        page	0

        include regst9

        ccf
        di
        ei
        halt
        iret
        nop
        rcf
        ret
        scf
        sdm
        spm
        wfi

;----------------------------------------

        adc	r5,r13
	adc	r7,(r12)
        adc	R100,R130
        adc	r7,R22
        adc	R70,r11
        adc	(r7),R32
        adc	(r7),r11

        adc	R32,(r5)
        adc	R13,(rr4)
        adc	r13,(rr4)
        adc	R13,(rr4)+
        adc	r13,(rr4)+
        adc	R13,-(rr4)
        adc	r13,-(rr4)
        adc	r10,rr12(rr6)
        adc	R11,123(rr8)
        adc	r11,123(rr8)
	adc	r3,1234h
        adc	R11,1234(rr8)
        adc	r11,1234(rr8)

        adc	(rr4),R13
        adc	(rr4),r13
        adc	(rr4)+,R13
        adc	(rr4)+,r13
        adc	-(rr4),R13
        adc	-(rr4),r13
        adc	rr12(rr6),r10
        adc	123(rr8),R11
        adc	123(rr8),r11
        adc	1234h,r3
        adc	1234(rr8),R11
        adc	1234(rr8),r11

        adc	(RR4),(rr8)
        adc	(rr4),(rr8)

        adc	R14,#12h
        adc	r14,#12h
        adc	(rr6),#12h
        adc	1234h,#12h


        adcw    rr4,rr12
        adcw	RR100,RR130
        adcw	rr6,RR22
        adcw	RR70,rr10
        adcw	(r7),RR32
        adcw	(r7),rr10
        adcw	RR32,(r5)
        adcw	rr6,(r12)

        adcw	rr12,(rr4)
        adcw	RR12,(rr4)
        adcw	RR12,(rr4)+
        adcw	rr12,(rr4)+
        adcw	RR12,-(rr4)
        adcw	rr12,-(rr4)
        adcw    rr10,rr12(rr6)
        adcw	RR10,123(rr8)
        adcw	rr10,123(rr8)
	adcw	rr2,1234h
        adcw	RR10,1234(rr8)
        adcw	rr10,1234(rr8)

        adcw	(rr4),rr12
        adcw	(rr4),RR12
        adcw	(rr4)+,RR12
        adcw	(rr4)+,rr12
        adcw	-(rr4),RR12
        adcw	-(rr4),rr12
        adcw	rr12(rr6),rr10
        adcw	123(rr8),RR10
        adcw	123(rr8),rr10
        adcw	1234h,rr2
        adcw	1234(rr8),RR10
        adcw	1234(rr8),rr10

        adcw	(rr4),(rr8)

        adcw	RR14,#1234h
        adcw	rr14,#1234h
        adcw	(rr6),#1234h
        adcw	123(rr6),#1234h
        adcw	1234(rr6),#1234h
        adcw	1234h,#1234h

;---------------------------------------------

        add	r5,r13
	add	r7,(r12)
        add	R100,R130
        add	r7,R22
        add	R70,r11
        add	(r7),R32
        add	(r7),r11
        add	R32,(r5)

        add	R13,(rr4)
        add	r13,(rr4)
        add	R13,(rr4)+
        add	r13,(rr4)+
        add	R13,-(rr4)
        add	r13,-(rr4)
        add	r10,rr12(rr6)
        add	R11,123(rr8)
        add	r11,123(rr8)
	add	r3,1234h
        add	R11,1234(rr8)
        add	r11,1234(rr8)

        add	(rr4),R13
        add	(rr4),r13
        add	(rr4)+,R13
        add	(rr4)+,r13
        add	-(rr4),R13
        add	-(rr4),r13
        add	rr12(rr6),r10
        add	123(rr8),R11
        add	123(rr8),r11
        add	1234h,r3
        add	1234(rr8),R11
        add	1234(rr8),r11

        add	(RR4),(rr8)
        add	(rr4),(rr8)

        add	R14,#12h
        add	r14,#12h
        add	(rr6),#12h
        add	1234h,#12h


        addw    rr4,rr12
        addw	RR100,RR130
        addw	rr6,RR22
        addw	RR70,rr10
        addw	(r7),RR32
        addw	(r7),rr10
        addw	RR32,(r5)
        addw	rr6,(r12)

        addw	rr12,(rr4)
        addw	RR12,(rr4)
        addw	RR12,(rr4)+
        addw	rr12,(rr4)+
        addw	RR12,-(rr4)
        addw	rr12,-(rr4)
        addw    rr10,rr12(rr6)
        addw	RR10,123(rr8)
        addw	rr10,123(rr8)
	addw	rr2,1234h
        addw	RR10,1234(rr8)
        addw	rr10,1234(rr8)

        addw	(rr4),rr12
        addw	(rr4),RR12
        addw	(rr4)+,RR12
        addw	(rr4)+,rr12
        addw	-(rr4),RR12
        addw	-(rr4),rr12
        addw	rr12(rr6),rr10
        addw	123(rr8),RR10
        addw	123(rr8),rr10
        addw	1234h,rr2
        addw	1234(rr8),RR10
        addw	1234(rr8),rr10

        addw	(rr4),(rr8)

        addw	RR14,#1234h
        addw	rr14,#1234h
        addw	(rr6),#1234h
        addw	123(rr6),#1234h
        addw	1234(rr6),#1234h
        addw	1234h,#1234h

;---------------------------------------------

        and	r5,r13
	and	r7,(r12)
        and	R100,R130
        and	r7,R22
        and	R70,r11
        and	(r7),R32
        and	(r7),r11
        and	R32,(r5)

        and	R13,(rr4)
        and	r13,(rr4)
        and	R13,(rr4)+
        and	r13,(rr4)+
        and	R13,-(rr4)
        and	r13,-(rr4)
        and	r10,rr12(rr6)
        and	R11,123(rr8)
        and	r11,123(rr8)
	and	r3,1234h
        and	R11,1234(rr8)
        and	r11,1234(rr8)

        and	(rr4),R13
        and	(rr4),r13
        and	(rr4)+,R13
        and	(rr4)+,r13
        and	-(rr4),R13
        and	-(rr4),r13
        and	rr12(rr6),r10
        and	123(rr8),R11
        and	123(rr8),r11
        and	1234h,r3
        and	1234(rr8),R11
        and	1234(rr8),r11

        and	(RR4),(rr8)
        and	(rr4),(rr8)

        and	R14,#12h
        and	r14,#12h
        and	(rr6),#12h
        and	1234h,#12h


        andw    rr4,rr12
        andw	RR100,RR130
        andw	rr6,RR22
        andw	RR70,rr10
        andw	(r7),RR32
        andw	(r7),rr10
        andw	RR32,(r5)
        andw	rr6,(r12)

        andw	rr12,(rr4)
        andw	RR12,(rr4)
        andw	RR12,(rr4)+
        andw	rr12,(rr4)+
        andw	RR12,-(rr4)
        andw	rr12,-(rr4)
        andw    rr10,rr12(rr6)
        andw	RR10,123(rr8)
        andw	rr10,123(rr8)
	andw	rr2,1234h
        andw	RR10,1234(rr8)
        andw	rr10,1234(rr8)

        andw	(rr4),rr12
        andw	(rr4),RR12
        andw	(rr4)+,RR12
        andw	(rr4)+,rr12
        andw	-(rr4),RR12
        andw	-(rr4),rr12
        andw	rr12(rr6),rr10
        andw	123(rr8),RR10
        andw	123(rr8),rr10
        andw	1234h,rr2
        andw	1234(rr8),RR10
        andw	1234(rr8),rr10

        andw	(rr4),(rr8)

        andw	RR14,#1234h
        andw	rr14,#1234h
        andw	(rr6),#1234h
        andw	123(rr6),#1234h
        andw	1234(rr6),#1234h
        andw	1234h,#1234h

;---------------------------------------------

        cp	r5,r13
	cp	r7,(r12)
        cp	R100,R130
        cp	r7,R22
        cp	R70,r11
        cp	(r7),R32
        cp	(r7),r11
        cp	R32,(r5)

        cp	R13,(rr4)
        cp	r13,(rr4)
        cp	R13,(rr4)+
        cp	r13,(rr4)+
        cp	R13,-(rr4)
        cp	r13,-(rr4)
        cp	r10,rr12(rr6)
        cp	R11,123(rr8)
        cp	r11,123(rr8)
	cp	r3,1234h
        cp	R11,1234(rr8)
        cp	r11,1234(rr8)

        cp	(rr4),R13
        cp	(rr4),r13
        cp	(rr4)+,R13
        cp	(rr4)+,r13
        cp	-(rr4),R13
        cp	-(rr4),r13
        cp	rr12(rr6),r10
        cp	123(rr8),R11
        cp	123(rr8),r11
        cp	1234h,r3
        cp	1234(rr8),R11
        cp	1234(rr8),r11

        cp	(RR4),(rr8)
        cp	(rr4),(rr8)

        cp	R14,#12h
        cp	r14,#12h
        cp	(rr6),#12h
        cp	1234h,#12h


        cpw	rr4,rr12
        cpw	RR100,RR130
        cpw	rr6,RR22
        cpw	RR70,rr10
        cpw	(r7),RR32
        cpw	(r7),rr10
        cpw	RR32,(r5)
        cpw	rr6,(r12)

        cpw	rr12,(rr4)
        cpw	RR12,(rr4)
        cpw	RR12,(rr4)+
        cpw	rr12,(rr4)+
        cpw	RR12,-(rr4)
        cpw	rr12,-(rr4)
        cpw	rr10,rr12(rr6)
        cpw	RR10,123(rr8)
        cpw	rr10,123(rr8)
	cpw	rr2,1234h
        cpw	RR10,1234(rr8)
        cpw	rr10,1234(rr8)

        cpw	(rr4),rr12
        cpw	(rr4),RR12
        cpw	(rr4)+,RR12
        cpw	(rr4)+,rr12
        cpw	-(rr4),RR12
        cpw	-(rr4),rr12
        cpw	rr12(rr6),rr10
        cpw	123(rr8),RR10
        cpw	123(rr8),rr10
        cpw	1234h,rr2
        cpw	1234(rr8),RR10
        cpw	1234(rr8),rr10

        cpw	(rr4),(rr8)

        cpw	RR14,#1234h
        cpw	rr14,#1234h
        cpw	(rr6),#1234h
        cpw	123(rr6),#1234h
        cpw	1234(rr6),#1234h
        cpw	1234h,#1234h

;---------------------------------------------

        or	r5,r13
	or	r7,(r12)
        or	R100,R130
        or	r7,R22
        or	R70,r11
        or	(r7),R32
        or	(r7),r11
        or	R32,(r5)

        or	R13,(rr4)
        or	r13,(rr4)
        or	R13,(rr4)+
        or	r13,(rr4)+
        or	R13,-(rr4)
        or	r13,-(rr4)
        or	r10,rr12(rr6)
        or	R11,123(rr8)
        or	r11,123(rr8)
	or	r3,1234h
        or	R11,1234(rr8)
        or	r11,1234(rr8)

        or	(rr4),R13
        or	(rr4),r13
        or	(rr4)+,R13
        or	(rr4)+,r13
        or	-(rr4),R13
        or	-(rr4),r13
        or	rr12(rr6),r10
        or	123(rr8),R11
        or	123(rr8),r11
        or	1234h,r3
        or	1234(rr8),R11
        or	1234(rr8),r11

        or	(RR4),(rr8)
        or	(rr4),(rr8)

        or	R14,#12h
        or	r14,#12h
        or	(rr6),#12h
        or	1234h,#12h


        orw	rr4,rr12
        orw	RR100,RR130
        orw	rr6,RR22
        orw	RR70,rr10
        orw	(r7),RR32
        orw	(r7),rr10
        orw	RR32,(r5)
        orw	rr6,(r12)

        orw	rr12,(rr4)
        orw	RR12,(rr4)
        orw	RR12,(rr4)+
        orw	rr12,(rr4)+
        orw	RR12,-(rr4)
        orw	rr12,-(rr4)
        orw	rr10,rr12(rr6)
        orw	RR10,123(rr8)
        orw	rr10,123(rr8)
	orw	rr2,1234h
        orw	RR10,1234(rr8)
        orw	rr10,1234(rr8)

        orw	(rr4),rr12
        orw	(rr4),RR12
        orw	(rr4)+,RR12
        orw	(rr4)+,rr12
        orw	-(rr4),RR12
        orw	-(rr4),rr12
        orw	rr12(rr6),rr10
        orw	123(rr8),RR10
        orw	123(rr8),rr10
        orw	1234h,rr2
        orw	1234(rr8),RR10
        orw	1234(rr8),rr10

        orw	(rr4),(rr8)

        orw	RR14,#1234h
        orw	rr14,#1234h
        orw	(rr6),#1234h
        orw	123(rr6),#1234h
        orw	1234(rr6),#1234h
        orw	1234h,#1234h

;---------------------------------------------

        sbc	r5,r13
	sbc	r7,(r12)
        sbc	R100,R130
        sbc	r7,R22
        sbc	R70,r11
        sbc	(r7),R32
        sbc	(r7),r11
        sbc	R32,(r5)

        sbc	R13,(rr4)
        sbc	r13,(rr4)
        sbc	R13,(rr4)+
        sbc	r13,(rr4)+
        sbc	R13,-(rr4)
        sbc	r13,-(rr4)
        sbc	r10,rr12(rr6)
        sbc	R11,123(rr8)
        sbc	r11,123(rr8)
	sbc	r3,1234h
        sbc	R11,1234(rr8)
        sbc	r11,1234(rr8)

        sbc	(rr4),R13
        sbc	(rr4),r13
        sbc	(rr4)+,R13
        sbc	(rr4)+,r13
        sbc	-(rr4),R13
        sbc	-(rr4),r13
        sbc	rr12(rr6),r10
        sbc	123(rr8),R11
        sbc	123(rr8),r11
        sbc	1234h,r3
        sbc	1234(rr8),R11
        sbc	1234(rr8),r11

        sbc	(RR4),(rr8)
        sbc	(rr4),(rr8)

        sbc	R14,#12h
        sbc	r14,#12h
        sbc	(rr6),#12h
        sbc	1234h,#12h


        sbcw	rr4,rr12
        sbcw	RR100,RR130
        sbcw	rr6,RR22
        sbcw	RR70,rr10
        sbcw	(r7),RR32
        sbcw	(r7),rr10
        sbcw	RR32,(r5)
        sbcw	rr6,(r12)

        sbcw	rr12,(rr4)
        sbcw	RR12,(rr4)
        sbcw	RR12,(rr4)+
        sbcw	rr12,(rr4)+
        sbcw	RR12,-(rr4)
        sbcw	rr12,-(rr4)
        sbcw	rr10,rr12(rr6)
        sbcw	RR10,123(rr8)
        sbcw	rr10,123(rr8)
	sbcw	rr2,1234h
        sbcw	RR10,1234(rr8)
        sbcw	rr10,1234(rr8)

        sbcw	(rr4),rr12
        sbcw	(rr4),RR12
        sbcw	(rr4)+,RR12
        sbcw	(rr4)+,rr12
        sbcw	-(rr4),RR12
        sbcw	-(rr4),rr12
        sbcw	rr12(rr6),rr10
        sbcw	123(rr8),RR10
        sbcw	123(rr8),rr10
        sbcw	1234h,rr2
        sbcw	1234(rr8),RR10
        sbcw	1234(rr8),rr10

        sbcw	(rr4),(rr8)

        sbcw	RR14,#1234h
        sbcw	rr14,#1234h
        sbcw	(rr6),#1234h
        sbcw	123(rr6),#1234h
        sbcw	1234(rr6),#1234h
        sbcw	1234h,#1234h

;---------------------------------------------

        sub	r5,r13
	sub	r7,(r12)
        sub	R100,R130
        sub	r7,R22
        sub	R70,r11
        sub	(r7),R32
        sub	(r7),r11
        sub	R32,(r5)

        sub	R13,(rr4)
        sub	r13,(rr4)
        sub	R13,(rr4)+
        sub	r13,(rr4)+
        sub	R13,-(rr4)
        sub	r13,-(rr4)
        sub	r10,rr12(rr6)
        sub	R11,123(rr8)
        sub	r11,123(rr8)
	sub	r3,1234h
        sub	R11,1234(rr8)
        sub	r11,1234(rr8)

        sub	(rr4),R13
        sub	(rr4),r13
        sub	(rr4)+,R13
        sub	(rr4)+,r13
        sub	-(rr4),R13
        sub	-(rr4),r13
        sub	rr12(rr6),r10
        sub	123(rr8),R11
        sub	123(rr8),r11
        sub	1234h,r3
        sub	1234(rr8),R11
        sub	1234(rr8),r11

        sub	(RR4),(rr8)
        sub	(rr4),(rr8)

        sub	R14,#12h
        sub	r14,#12h
        sub	(rr6),#12h
        sub	1234h,#12h


        subw	rr4,rr12
        subw	RR100,RR130
        subw	rr6,RR22
        subw	RR70,rr10
        subw	(r7),RR32
        subw	(r7),rr10
        subw	RR32,(r5)
        subw	rr6,(r12)

        subw	rr12,(rr4)
        subw	RR12,(rr4)
        subw	RR12,(rr4)+
        subw	rr12,(rr4)+
        subw	RR12,-(rr4)
        subw	rr12,-(rr4)
        subw	rr10,rr12(rr6)
        subw	RR10,123(rr8)
        subw	rr10,123(rr8)
	subw	rr2,1234h
        subw	RR10,1234(rr8)
        subw	rr10,1234(rr8)

        subw	(rr4),rr12
        subw	(rr4),RR12
        subw	(rr4)+,RR12
        subw	(rr4)+,rr12
        subw	-(rr4),RR12
        subw	-(rr4),rr12
        subw	rr12(rr6),rr10
        subw	123(rr8),RR10
        subw	123(rr8),rr10
        subw	1234h,rr2
        subw	1234(rr8),RR10
        subw	1234(rr8),rr10

        subw	(rr4),(rr8)

        subw	RR14,#1234h
        subw	rr14,#1234h
        subw	(rr6),#1234h
        subw	123(rr6),#1234h
        subw	1234(rr6),#1234h
        subw	1234h,#1234h

;---------------------------------------------

        tcm	r5,r13
	tcm	r7,(r12)
        tcm	R100,R130
        tcm	r7,R22
        tcm	R70,r11
        tcm	(r7),R32
        tcm	(r7),r11
        tcm	R32,(r5)

        tcm	R13,(rr4)
        tcm	r13,(rr4)
        tcm	R13,(rr4)+
        tcm	r13,(rr4)+
        tcm	R13,-(rr4)
        tcm	r13,-(rr4)
        tcm	r10,rr12(rr6)
        tcm	R11,123(rr8)
        tcm	r11,123(rr8)
	tcm	r3,1234h
        tcm	R11,1234(rr8)
        tcm	r11,1234(rr8)

        tcm	(rr4),R13
        tcm	(rr4),r13
        tcm	(rr4)+,R13
        tcm	(rr4)+,r13
        tcm	-(rr4),R13
        tcm	-(rr4),r13
        tcm	rr12(rr6),r10
        tcm	123(rr8),R11
        tcm	123(rr8),r11
        tcm	1234h,r3
        tcm	1234(rr8),R11
        tcm	1234(rr8),r11

        tcm	(RR4),(rr8)
        tcm	(rr4),(rr8)

        tcm	R14,#12h
        tcm	r14,#12h
        tcm	(rr6),#12h
        tcm	1234h,#12h


        tcmw	rr4,rr12
        tcmw	RR100,RR130
        tcmw	rr6,RR22
        tcmw	RR70,rr10
        tcmw	(r7),RR32
        tcmw	(r7),rr10
        tcmw	RR32,(r5)
        tcmw	rr6,(r12)

        tcmw	rr12,(rr4)
        tcmw	RR12,(rr4)
        tcmw	RR12,(rr4)+
        tcmw	rr12,(rr4)+
        tcmw	RR12,-(rr4)
        tcmw	rr12,-(rr4)
        tcmw	rr10,rr12(rr6)
        tcmw	RR10,123(rr8)
        tcmw	rr10,123(rr8)
	tcmw	rr2,1234h
        tcmw	RR10,1234(rr8)
        tcmw	rr10,1234(rr8)

        tcmw	(rr4),rr12
        tcmw	(rr4),RR12
        tcmw	(rr4)+,RR12
        tcmw	(rr4)+,rr12
        tcmw	-(rr4),RR12
        tcmw	-(rr4),rr12
        tcmw	rr12(rr6),rr10
        tcmw	123(rr8),RR10
        tcmw	123(rr8),rr10
        tcmw	1234h,rr2
        tcmw	1234(rr8),RR10
        tcmw	1234(rr8),rr10

        tcmw	(rr4),(rr8)

        tcmw	RR14,#1234h
        tcmw	rr14,#1234h
        tcmw	(rr6),#1234h
        tcmw	123(rr6),#1234h
        tcmw	1234(rr6),#1234h
        tcmw	1234h,#1234h

;---------------------------------------------

        tm	r5,r13
	tm	r7,(r12)
        tm	R100,R130
        tm	r7,R22
        tm	R70,r11
        tm	(r7),R32
        tm	(r7),r11
        tm	R32,(r5)

        tm	R13,(rr4)
        tm	r13,(rr4)
        tm	R13,(rr4)+
        tm	r13,(rr4)+
        tm	R13,-(rr4)
        tm	r13,-(rr4)
        tm	r10,rr12(rr6)
        tm	R11,123(rr8)
        tm	r11,123(rr8)
	tm	r3,1234h
        tm	R11,1234(rr8)
        tm	r11,1234(rr8)

        tm	(rr4),R13
        tm	(rr4),r13
        tm	(rr4)+,R13
        tm	(rr4)+,r13
        tm	-(rr4),R13
        tm	-(rr4),r13
        tm	rr12(rr6),r10
        tm	123(rr8),R11
        tm	123(rr8),r11
        tm	1234h,r3
        tm	1234(rr8),R11
        tm	1234(rr8),r11

        tm	(RR4),(rr8)
        tm	(rr4),(rr8)

        tm	R14,#12h
        tm	r14,#12h
        tm	(rr6),#12h
        tm	1234h,#12h


        tmw	rr4,rr12
        tmw	RR100,RR130
        tmw	rr6,RR22
        tmw	RR70,rr10
        tmw	(r7),RR32
        tmw	(r7),rr10
        tmw	RR32,(r5)
        tmw	rr6,(r12)

        tmw	rr12,(rr4)
        tmw	RR12,(rr4)
        tmw	RR12,(rr4)+
        tmw	rr12,(rr4)+
        tmw	RR12,-(rr4)
        tmw	rr12,-(rr4)
        tmw	rr10,rr12(rr6)
        tmw	RR10,123(rr8)
        tmw	rr10,123(rr8)
	tmw	rr2,1234h
        tmw	RR10,1234(rr8)
        tmw	rr10,1234(rr8)

        tmw	(rr4),rr12
        tmw	(rr4),RR12
        tmw	(rr4)+,RR12
        tmw	(rr4)+,rr12
        tmw	-(rr4),RR12
        tmw	-(rr4),rr12
        tmw	rr12(rr6),rr10
        tmw	123(rr8),RR10
        tmw	123(rr8),rr10
        tmw	1234h,rr2
        tmw	1234(rr8),RR10
        tmw	1234(rr8),rr10

        tmw	(rr4),(rr8)

        tmw	RR14,#1234h
        tmw	rr14,#1234h
        tmw	(rr6),#1234h
        tmw	123(rr6),#1234h
        tmw	1234(rr6),#1234h
        tmw	1234h,#1234h

;---------------------------------------------

        xor	r5,r13
	xor	r7,(r12)
        xor	R100,R130
        xor	r7,R22
        xor	R70,r11
        xor	(r7),R32
        xor	(r7),r11
        xor	R32,(r5)

        xor	R13,(rr4)
        xor	r13,(rr4)
        xor	R13,(rr4)+
        xor	r13,(rr4)+
        xor	R13,-(rr4)
        xor	r13,-(rr4)
        xor	r10,rr12(rr6)
        xor	R11,123(rr8)
        xor	r11,123(rr8)
	xor	r3,1234h
        xor	R11,1234(rr8)
        xor	r11,1234(rr8)

        xor	(rr4),R13
        xor	(rr4),r13
        xor	(rr4)+,R13
        xor	(rr4)+,r13
        xor	-(rr4),R13
        xor	-(rr4),r13
        xor	rr12(rr6),r10
        xor	123(rr8),R11
        xor	123(rr8),r11
        xor	1234h,r3
        xor	1234(rr8),R11
        xor	1234(rr8),r11

        xor	(RR4),(rr8)
        xor	(rr4),(rr8)

        xor	R14,#12h
        xor	r14,#12h
        xor	(rr6),#12h
        xor	1234h,#12h


        xorw	rr4,rr12
        xorw	RR100,RR130
        xorw	rr6,RR22
        xorw	RR70,rr10
        xorw	(r7),RR32
        xorw	(r7),rr10
        xorw	RR32,(r5)
        xorw	rr6,(r12)

        xorw	rr12,(rr4)
        xorw	RR12,(rr4)
        xorw	RR12,(rr4)+
        xorw	rr12,(rr4)+
        xorw	RR12,-(rr4)
        xorw	rr12,-(rr4)
        xorw	rr10,rr12(rr6)
        xorw	RR10,123(rr8)
        xorw	rr10,123(rr8)
	xorw	rr2,1234h
        xorw	RR10,1234(rr8)
        xorw	rr10,1234(rr8)

        xorw	(rr4),rr12
        xorw	(rr4),RR12
        xorw	(rr4)+,RR12
        xorw	(rr4)+,rr12
        xorw	-(rr4),RR12
        xorw	-(rr4),rr12
        xorw	rr12(rr6),rr10
        xorw	123(rr8),RR10
        xorw	123(rr8),rr10
        xorw	1234h,rr2
        xorw	1234(rr8),RR10
        xorw	1234(rr8),rr10

        xorw	(rr4),(rr8)

        xorw	RR14,#1234h
        xorw	rr14,#1234h
        xorw	(rr6),#1234h
        xorw	123(rr6),#1234h
        xorw	1234(rr6),#1234h
        xorw	1234h,#1234h

;---------------------------------------------

        ld      r13,R230
        ld      r4,r12
        ld      R123,r5
        ld      (r6),r12
        ld      r12,(r6)
        ld      (r9),R56
        ld      (r10),r11
        ld      R100,(r7)
        ld      72(r5),r8
        ld      r8,72(r5)
        ld      R120,R130

        ld      r10,(rr12)
        ld      (r4)+,(rr6)+
        ld      R240,(rr2)+
        ld      r10,(rr2)+
        ld      R4,-(rr6)
        ld      r4,-(rr6)
        ld      R230,(rr8)
        ld      r2,rr6(rr4)
        ld      R14,123(rr6)
        ld      r14,123(rr6)
        ld      r12,1234h
        ld      R12,1234(rr8)
        ld      r12,1234(rr8)

        ld      (rr4)+,(r6)+
        ld      (rr12),(r10)
        ld      (rr2)+,R240
        ld      (rr2)+,r10
        ld      -(rr6),R4
        ld      -(rr6),r4
        ld      (rr8),R230
        ld      rr6(rr4),r2
        ld      123(rr6),R14
        ld      123(rr6),r14
        ld      1234h,r12
        ld      1234(rr8),R12
        ld      1234(rr8),r12

        ld      (RR4),(rr6)
        ld      (rr4),(rr6)

        ld      r8,#242
        ld      R8,#123
        ld      (rr6),#23h
        ld      1234h,#56h


        ldw     rr10,rr14
        ldw     (r7),RR128
        ldw     (r9),rr4
        ldw     RR40,(r5)
        ldw     rr6,(r5)
        ldw     123(r7),rr8
        ldw     rr10,123(r5)
        ldw     RR40,RR120
        ldw     rr12,RR136
        ldw     RR136,rr12

        ldw     rr8,(rr6)
        ldw     RR20,(rr10)+
        ldw     rr12,(rr10)+
        ldw     RR124,-(rr2)
        ldw     rr4,-(rr2)
        ldw     RR8,(rr6)
        ldw     rr10,rr2(rr6)
        ldw     RR20,123(rr8)
        ldw     rr8,123(rr8)
        ldw     rr4,1234h
        ldw     RR20,1234(rr8)
        ldw     rr8,1234(rr8)

        ldw     (rr6),rr8
        ldw     (rr10)+,RR20
        ldw     (rr10)+,rr12
        ldw     -(rr2),RR124
        ldw     -(rr2),rr4
        ldw     (rr6),RR8
        ldw     rr2(rr6),rr10
        ldw     123(rr8),RR20
        ldw     123(rr8),rr8
        ldw     1234h,rr4
        ldw     1234(rr8),RR20
        ldw     1234(rr8),rr8

        ldw     (rr6),(rr10)

        ldw     rr8,#2345h
        ldw     RR100,#4268
        ldw     (rr6),#1234h
        ldw     123(rr6),#1234h
        ldw     1234(rr6),#1234h
        ldw     1234h,#1234h

;-----------------------------------

        clr     R32
        clr     r2
        clr     (R32)
        clr     (r2)

        cpl     R32
        cpl     r2
        cpl     (R32)
        cpl     (r2)

        da      R32
        da      r2
        da      (R32)
        da      (r2)

        dec     R32
        dec     r2
        dec     (R32)
        dec     (r2)

        inc     R32
        inc     r2
        inc     (R32)
        inc     (r2)

        pop     R32
        pop     r2
        pop     (R32)
        pop     (r2)

        popu    R32
        popu    r2
        popu    (R32)
        popu    (r2)

        rlc     R32
        rlc     r2
        rlc     (R32)
        rlc     (r2)

        rol     R32
        rol     r2
        rol     (R32)
        rol     (r2)

        ror     R32
        ror     r2
        ror     (R32)
        ror     (r2)

        rrc     R32
        rrc     r2
        rrc     (R32)
        rrc     (r2)

        sra     R32
        sra     r2
        sra     (R32)
        sra     (r2)

        swap     R32
        swap     r2
        swap     (R32)
        swap     (r2)

        decw    RR32
        decw    rr2

        ext     RR10
        ext     rr10

        incw    RR32
        incw    rr2

        popuw   RR32
        popuw   rr2

        popw    RR32
        popw    rr2

        rlcw    RR32
        rlcw    rr2

        rrcw    RR32
        rrcw    rr2

        sraw    RR32
        sraw    rr2

;------------------------------

        band    r4.5,r8.2
        band    r4.5,r8.!2

        bld     r4.5,r8.!2
        bld     r4.5,r8.2

        bor     r4.5,r8.2
        bor     r4.5,r8.!2

        bxor    r4.5,r8.2
        bxor    r4.5,r8.!2

;------------------------------

        bcpl    r4.5

        bres    r4.5

        bset    r4.5

        btset   r4.5
        btset   (rr4).5

        btjf    r10.2,pc
        btjt    r10.2,pc

;------------------------------

        call    (RR30)
        call    (rr6)
        call    3521h

        jp      (RR30)
        jp      (rr6)
        jp      1024

        jpeq    1024

        jreq    pc

;------------------------------

        cpjfi   r2,(rr14),pc
        cpjti   r2,(rr14),pc

        djnz    r6,pc
        dwjnz   RR6,pc
        dwjnz   rr6,pc

;------------------------------

        div     rr8,r6
        mul     rr6,r8
        divws   rr6,rr8,RR10

;------------------------------

        ldpp    (rr8)+,(rr12)+
        lddp    (rr8)+,(rr12)+
        ldpd    (rr8)+,(rr12)+
        lddd    (rr8)+,(rr12)+

;------------------------------

        pea     16(RR32)
        pea     16(rr2)
        pea     1600(RR32)
        pea     1600(rr2)

        peau    16(RR32)
        peau    16(rr2)
        peau    1600(RR32)
        peau    1600(rr2)

;------------------------------

        push    R32
        push    r2
        push    (R32)
        push    (r2)
        push    #23h

        pushu   R32
        pushu   r2
        pushu   (R32)
        pushu   (r2)
        pushu   #23h

        pushuw  RR32
        pushuw  rr2
        pushuw  #1234h

        pushw   RR32
        pushw   rr2
        pushw   #1234h

;------------------------------

        sla     r6
        sla     R6
        sla     (rr6)

        slaw    rr4
        slaw    RR4
        slaw    (rr4)

;------------------------------

        spp     #5
        srp     #3
        srp0    #3
        srp1    #3

;------------------------------

        xch     r2,r4

dvar    equ     1234h,data
cvar    label   2345h

        assume  dp:0
;        ld      r0,dvar
        ld      r0,cvar
        assume  dp:1
        ld      r0,dvar
;        ld      r0,cvar

bit1    bit     r5.1
bit2    bit     r6.!7
bit3    bit     bit1
bit4    bit     bit1+1

        bld     r0.0,bit3
        bld     r0.1,!bit3

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
