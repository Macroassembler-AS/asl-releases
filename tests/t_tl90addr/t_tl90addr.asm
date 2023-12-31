	CPU	90C141

	LDA	BC,(IX+1)	; F4 01 38
	LDA	HL,(HL+A)	; F7 3A

	LDA	BC,IX+1		; F4+ix:d:38+rr
	LDA	HL,HL+A		; F7:38+rr

	JP	z,BC
	JP	z,(BC)
	JP	z,IX-5
	JP	z,(IX-5)
	JP	z,HL+A
	JP	z,(HL+A)
	JP	z,1234H
	JP	z,(1234H)
	CALL	z,BC
	CALL	z,(BC)
	CALL	z,IX-5
	CALL	z,(IX-5)
	CALL	z,HL+A
	CALL	z,(HL+A)
	CALL	z,1234H
-	CALL	z,(1234H)
	CALL	z,( - )
	CALL	z,(- )
	CALL	z,( -)
	CALL	z,(-)
