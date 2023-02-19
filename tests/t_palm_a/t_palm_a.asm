	cpu	ibm5100
	page	0

	org	x'b00'

	halt			; mvm2 pc,pc

	nop			; move pc,pc

	move	r12,#x'55aa'	; move r12,(pc)+ plus dw x'55aa'

	movb	r12,#x'5a'	; emit r12,x'5a'

	lwi	r12,#x'55aa'	; move r12,(pc)+ plus dw x'55aa'
	lwi	r12,x'55aa'	; move r12,(pc)+ plus dw x'55aa'

	and	r12,#x'55'	; clri r12,x'55'!x'ff'
	or	r12,#x'55'	; seti r12,x'55'

	bra	*		; like HALT
	expect	1371
	bra	*+2		; would result in zero
	endexpect
	bra	*+258		; just allowed
	expect	1370
	bra	*+260		; not any more allowed
	endexpect
	bra	*-254		; just allowed
	expect	1370
	bra	*-256		; not any more allowed
	endexpect

	expect	1351		; cannot jump to odd addresses
	bra	*+127
	endexpect

	jmp	x'1234'		; ldhi pc,pc,2 ; dw x'1234'
	lwi	r0,#x'1234'

	expect	1351		; cannot jump to odd addresses
	jmp	x'1235'
	endexpect

	jmp	*+10		; bra *+10

	jmp	>*+10		; ldhi pc,pc,2 ; dw *+10 (forced absolute)

	jmp	(x'ac')		; ldhd pc,x'ac'
	move	r0,x'ac'

	jmp	(r5)		; ldhi pc,r5
	move	r0,(r5)

	jmp	r5		; move pc,r5
	move	r0,r5

	jmp	(r15)-		; move pc,(r15)-
	move	r0,(r15)-

	call	x'1234',r2 	; mvp2 r2,pc ; ldhi pc,r2,2 ; dw x'1234'
        inc2	r2,r0
	move	r0,(r2)+
	dw	x'1234'

	expect	1351		; cannot jump to odd addresses
	call	x'1235',r2
	endexpect

	call	(r5),r2		; mvp2 r2,pc ; ldhi pc,r5
	inc2	r2,r0
	move	r0,(r5)

	call	r5,r2		; mvp2 r2,pc ; move pc,r5
	inc2	r2,r0
	move	r0,r5

	rcall	*+100,r2	; mvp2 r2,pc ; addi pc,(100-(*+4)) with * being the address of the first micro-instruction!
	inc2	r2,r0
	bra	*+98

	ret	r2		; move pc,r2
	move	r0,r2
