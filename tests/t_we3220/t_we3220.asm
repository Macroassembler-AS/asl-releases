	cpu	we32200
	page	0
	execmode kernel

hi_src	reg	%r16

	; Auto Pre-Decrement
	movb	-(%r0),r1		; 87 5B 00 41
	movb	-(%r14),r1		; 87 5B 0E 41
	movb	-(%r16),r1		; 87 5B 10 41
	movb	-(%r31),r1		; 87 5B 1F 41

	; Auto Post-Decrement
	movb	(%r0)-,r1		; 87 5B 40 41
	movb	(%r14)-,r1		; 87 5B 4E 41
	movb	(%r16)-,r1		; 87 5B 50 41
	movb	(%r31)-,r1		; 87 5B 5F 41

	; Auto Pre-Increment
	movb	+(%r0),r1		; 87 5B 80 41
	movb	+(%r14),r1		; 87 5B 8E 41
	movb	+(%r16),r1		; 87 5B 90 41
	movb	+(%r31),r1		; 87 5B 9F 41

	; Auto Post-Increment
	movb	(%r0)+,r1		; 87 5B C0 41
	movb	(%r14)+,r1		; 87 5B CE 41
	movb	(%r16)+,r1		; 87 5B D0 41
	movb	(%r31)+,r1		; 87 5B DF 41

	; Indexed with byte displacement
	movb	(%r2,%r19),r1		; 87 AB 23 00 41
	movb	(%r19,%r2),r1
	expect	1350
	movb	(%r2,%r7),r1
	endexpect
	expect	1350
	movb	(%r18,%r19),r1
	endexpect
	movb	127(%r2,%r19),r1	; 87 AB 23 7F 41
	movb	-128(%r19,%r2),r1	; 87 AB 23 80 41

	; Indexed with halfword displacement
	movb	128(%r2,%r19),r1	; 87 BB 23 80 00 41
	movb	-129(%r19,%r2),r1	; 87 BB 23 7F FF 41
	movb	32767(%r2,%r19),r1	; 87 BB 23 FF 7F 41
	movb	-32768(%r19,%r2),r1	; 87 BB 23 00 80 41

	; No Indexed with word displacement!
	expect	1320
	movb	32768(%r2,%r19),r1	; 87 BB 23 FF 7F 41
	endexpect
	expect	1315
	movb	-32769(%r19,%r2),r1	; 87 BB 23 00 80 41
	endexpect

	; Indexed register with scaling
	movb	%r19[%r2],r1		; 87 DB 23 41
	expect	1350
	movb	%r3[%r2],r1		; base register must be 16..31
	endexpect
	expect	1350
	movb	%r19[%r18],r1		; index register must be 0...15 ...
	endexpect
	expect	1350
	movb    %r19[pc],r1		; ... and neither PC ...
	endexpect
	expect	1350
	movb	%r19[psw],r1		; ... nor PSW.
	endexpect

	; Do not mix up indexed syntax with sections:

	section	sub_1
entry:	nop
	section	sub_2
entry:	nop
	section	sub_3
	movaw	entry[sub_1],%r5	; 84 CF FC 45
	movaw	entry[sub_2],%r5	; 84 CF F9 45
	endsection
	endsection
	endsection

	; Register
	movb	%r0,%r1			; 87 40 41
	movb	%r14,%r1		; 87 4E 41
	movb	%r16,%r1		; 87 CB 40 41
	movb	%r23,%r1		; 87 CB 47 41
	movb	%r24,%r1		; 87 CB 48 41
	movb	%r31,%r1		; 87 CB 4F 41
	expect	1020
	movb	%r32,%r1
	endexpect

	; Register deferred
	movb	(%r0),%r1		; 87 50 41
	movb	(%r14),%r1		; 87 5E 41
	movb	(%r16),%r1		; 87 CB 50 41
	movb	(hi_src),%r1
	movb	(%r23),%r1		; 87 CB 57 41
	movb	(%r24),%r1		; 87 CB 58 41
	movb	(%r31),%r1		; 87 CB 5F 41
	if	mompass>1
	expect	1445
	endif
	movb	(%r32),%r1
	if	mompass>1
	endexpect
	endif

	; Byte Displacement: 
	movb	6(%r0),%r0		; 87 C0 06 40
	movb	-128(r0),r0		; 87 C0 80 40
	movb	127(r0),r0		; 87 C0 7F 40
	movb	6(%r14),%r0		; 87 CE 06 40
	movb	-128(r14),r0		; 87 CE 80 40
	movb	127(r14),r0		; 87 CE 7F 40
	movb	6(%r16),%r0		; 87 CB C0 06 40
	movb	-128(r16),r0		; 87 CB C0 80 40
	movb	127(r16),r0		; 87 CB C0 7F 40
	movb	6(%r31),%r0		; 87 CB CF 06 40
	movb	-128(%r31),r0		; 87 CB CF 80 40
	movb	127(%r31),r0		; 87 CB CF 7F 40

	; Byte Displacement Deferred:
	movb	*0x30(%r0),%r3		; 87 D0 30 43
	movb	*-128(r0),r0		; 87 D0 80 40
	movb	*127(r0),r0		; 87 D0 7F 40
	movb	*0x30(%r14),%r3		; 87 DE 30 43
	movb	*-128(r14),r0		; 87 DE 80 40
	movb	*127(r14),r0		; 87 DE 7F 40
	movb	*0x30(%r16),%r3		; 87 CB D0 30 43
	movb	*-128(r16),r0		; 87 CB D0 80 40
	movb	*127(r16),r0		; 87 CB D0 7F 40
	movb	*0x30(%r31),%r3		; 87 CB DF 30 43
	movb	*-128(r31),r0		; 87 CB DF 80 40
	movb	*127(r31),r0		; 87 CB DF 7F 40

	; Halfword Displacement:
	movb	0x1101(%r0),%r8		; 87 A0 01 11 48
	movb	-129(r0),r0		; 87 A0 7F FF 40
	movb	128(r0),r0		; 87 A0 80 00 40
	movb	-32768(r0),r0		; 87 A0 00 80 40
	movb	32767(r0),r0		; 87 A0 FF 7F 40
	movb	0x1101(%r14),%r8	; 87 AE 01 11 48
	movb	-129(r14),r0		; 87 AE 7F FF 40
	movb	128(r14),r0		; 87 AE 80 00 40
	movb	-32768(r14),r0		; 87 AE 00 80 40
	movb	32767(r14),r0		; 87 AE FF 7F 40
	movb	0x1101(%r16),%r8	; 87 CB A0 01 11 48
	movb	-129(r16),r0		; 87 CB A0 7F FF 40
	movb	128(r16),r0		; 87 CB A0 80 00 40
	movb	-32768(r16),r0		; 87 CB A0 00 80 40
	movb	32767(r16),r0		; 87 CB A0 FF 7F 40
	movb	0x1101(%r31),%r8	; 87 CB AF 01 11 48
	movb	-129(r31),r0		; 87 CB AF 7F FF 40
	movb	128(r31),r0		; 87 CB AF 80 00 40
	movb	-32768(r31),r0		; 87 CB AF 00 80 40
	movb	32767(r31),r0		; 87 CB AF FF 7F 40

	; Halfword Displacement Deferred:
	movb	*0x200(%r0),%r6		; 87 B0 00 02 46
	movb	*-129(r0),r0		; 87 B0 7F FF 40
	movb	*128(r0),r0		; 87 B0 80 00 40
	movb	*-32768(r0),r0		; 87 B0 00 80 40
	movb	*32767(r0),r0		; 87 B0 FF 7F 40
	movb	*0x200(%r14),%r6	; 87 BE 00 02 46
	movb	*-129(r14),r0		; 87 BE 7F FF 40
	movb	*128(r14),r0		; 87 BE 80 00 40
	movb	*-32768(r14),r0		; 87 BE 00 80 40
	movb	*32767(r14),r0		; 87 BE FF 7F 40
	movb	*0x200(%r16),%r6	; 87 CB B0 00 02 46
	movb	*-129(r16),r0		; 87 CB B0 7F FF 40
	movb	*128(r16),r0		; 87 CB B0 80 00 40
	movb	*-32768(r16),r0		; 87 CB B0 00 80 40
	movb	*32767(r16),r0		; 87 CB B0 FF 7F 40
	movb	*0x200(%r31),%r6	; 87 CB BF 00 02 46
	movb	*-129(r31),r0		; 87 CB BF 7F FF 40
	movb	*128(r31),r0		; 87 CB BF 80 00 40
	movb	*-32768(r31),r0		; 87 CB BF 00 80 40
	movb	*32767(r31),r0		; 87 CB BF FF 7F 40

	; Word Displacement:
	movb	0x112234(%r0),%r4	; 87 80 34 22 11 00 44
	movb	-32769(r0),r0		; 87 80 FF 7F FF FF 40
	movb	32768(r0),r0		; 87 80 00 80 00 00 40
	movb	-2147483648(r0),r0	; 87 80 00 00 00 80 40
	movb	2147483647(r0),r0	; 87 80 FF FF FF 7F 40
	movb	0x112234(%r14),%r4	; 87 8E 34 22 11 00 44
	movb	-32769(r14),r0		; 87 8E FF 7F FF FF 40
	movb	32768(r14),r0		; 87 8E 00 80 00 00 40
	movb	-2147483648(r14),r0	; 87 8E 00 00 00 80 40
	movb	2147483647(r14),r0	; 87 8E FF FF FF 7F 40
	movb	0x112234(%r16),%r4	; 87 CB 80 34 22 11 00 44
	movb	-32769(r16),r0		; 87 CB 80 FF 7F FF FF 40
	movb	32768(r16),r0		; 87 CB 80 00 80 00 00 40
	movb	-2147483648(r16),r0	; 87 CB 80 00 00 00 80 40
	movb	2147483647(r16),r0	; 87 CB 80 FF FF FF 7F 40
	movb	0x112234(%r31),%r4	; 87 CB 8F 34 22 11 00 44
	movb	-32769(r31),r0		; 87 CB 8F FF 7F FF FF 40
	movb	32768(r31),r0		; 87 CB 8F 00 80 00 00 40
	movb	-2147483648(r31),r0	; 87 CB 8F 00 00 00 80 40
	movb	2147483647(r31),r0	; 87 CB 8F FF FF FF 7F 40

	; Word Displacement Deferred:
	movb	*0x20304050(%r0),%r0	; 87 90 50 40 30 20 40
	movb	*-32769(r0),r0		; 87 90 FF 7F FF FF 40
	movb	*32768(r0),r0		; 87 90 00 80 00 00 40
	movb	*-2147483648(r0),r0	; 87 90 00 00 00 80 40
	movb	*2147483647(r0),r0	; 87 90 FF FF FF 7F 40
	movb	*0x20304050(%r14),%r0	; 87 9E 50 40 30 20 40
	movb	*-32769(r14),r0		; 87 9E FF 7F FF FF 40
	movb	*32768(r14),r0		; 87 9E 00 80 00 00 40
	movb	*-2147483648(r14),r0	; 87 9E 00 00 00 80 40
	movb	*2147483647(r14),r0	; 87 9E FF FF FF 7F 40
	movb	*0x20304050(%r16),%r0	; 87 CB 90 50 40 30 20 40
	movb	*-32769(r16),r0		; 87 CB 90 FF 7F FF FF 40
	movb	*32768(r16),r0		; 87 CB 90 00 80 00 00 40
	movb	*-2147483648(r16),r0	; 87 CB 90 00 00 00 80 40
	movb	*2147483647(r16),r0	; 87 CB 90 FF FF FF 7F 40
	movb	*0x20304050(%r31),%r0	; 87 CB 9F 50 40 30 20 40
	movb	*-32769(r31),r0		; 87 CB 9F FF 7F FF FF 40
	movb	*32768(r31),r0		; 87 CB 9F 00 80 00 00 40
	movb	*-2147483648(r31),r0	; 87 CB 9F 00 00 00 80 40
	movb	*2147483647(r31),r0	; 87 CB 9F FF FF FF 7F 40

	;-------------------
	; New Instructions

	addpb2	$0x100,%r0		; A3 7F 00 01 00 00 40
	addpb	$0x100,%r0

	addpb3	%r0,%r3,%r5		; E3 40 43 45
	addpb	%r0,%r3,%r5

	;caswi				; 09

	clrx				; 0B

	;dtb				; 29

	;dth				; 19

	;packb				; 0E

	setx				; 0A

	subpb2	%r6,*0x30(%r2)		; 9B 46 D2 30
	subpb	%r6,*0x30(%r2)

	subpb3	%r3,*$0x1005,%r2	; DB 43 EF 05 10 00 00 42
	subpb	%r3,*$0x1005,%r2

	;tedtb				; 4D

	;tedth				; 0D

	;tgdtb				; 6D

	;tgdth				; 2D

	;tgedtb				; 5D

	;tgedth				; 1D

	;tnedtb				; 7D

	;tnedth				; 3D

	;unpackb			; 0F

	;-------------------
	; New Operating System Instructions

	execmode kernel

	retqint				; 98

	ucallps				; 30 C0 
