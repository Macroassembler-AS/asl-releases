	CPU	96C141

	LDA	BC,(XIX+1)
	LDA	HL,(HL+A)

	LDA	BC,XIX+1
	LDA	BC,XIX+1:8
	LDA	BC,XIX+1:16
	expect	2330
	LDA	BC,XIX+1:24
	endexpect
	LDA	BC,XIX+1000
	LDA	BC,XIX+1000:16
	expect	2330
	LDA	BC,XIX+1000:24
	endexpect
	LDA	HL,HL+A

	LDA	HL,12H
	LDA	HL,12H:8
	LDA	HL,12H:16
	LDA	HL,12H:24
	LDA	HL,1234H
	expect	1925
	LDA	HL,1234H:8
	endexpect
	LDA	HL,1234H:16
	LDA	HL,1234H:24
	LDA	HL,123456H
	expect	1925
	LDA	HL,123456H:8
	endexpect
	expect	1925
	LDA	HL,123456H:16
	endexpect
	LDA	HL,123456H:24

	JP	z,BC
	JP	z,(BC)
	JP	z,XIX-5
	JP	z,(XIX-5)
	JP	z,HL+A
	JP	z,(HL+A)
	JP	z,1234H
	JP	z,(1234H)
	JP	z,123456H
	JP	z,(123456H)
	CALL	z,BC
	CALL	z,(BC)
	CALL	z,XIX-5
	CALL	z,(XIX-5)
	CALL	z,HL+A
	CALL	z,(HL+A)
	CALL	z,1234H
	CALL	z,(1234H)
	CALL	z,123456H
	CALL	z,(123456H)
-	CALL	z,(-)
	CALL	z,( -)
	CALL	z,(- )
	CALL	z,( - )

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
