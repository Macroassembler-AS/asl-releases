	cpu	68000
	page	0

	padding	on

	org	$1000

	; Text string has an odd number of characters, so add a pad byte
        ; right before the machine instruction:

	dc.b	"Hello world"
	move.l	$100000,d0

        ; Byte strings themselves do not require word alignment, so no
        ; pad bytes are inserted among the dc.b statements.  dc.w however
        ; gets a pad byte in front.  TODO: 68020++ allow misaligned word
        ; access.  Do not pad data statements on them?

	dc.b	"Hello world"
	dc.b	"Hello world"
	dc.b	"Hello world"
	dc.w	1,2,3,4

	; Similar handling for space reservation.  The only difference is
        ; that a byte of space is reserved, and no zero byte is written:

	dc.b	[5]?
	dc.w	[4]?

	; ds.x is equivalent to dc.x ?

	ds.b	5
	ds.w	4

	; Now the nasty details about labels: The label's value should of
        ; course ; point to the instruction and not the (invisible) pad byte!
        ; This ; is the 'simple' case when the label and instruction are on
        ; the ; same line:

	dc.b	1
label1:	nop
	jmp	label1

	; Some people like putting the label on a separate line.  Though it
        ; is debatable, label fixup is also done in this case, and the result
        ; is the same as in the previous case:

	dc.b	1
label2:
	nop
	jmp	label2

	; Putting more empty lines (i.e. lines with at most a comment)
        ; between label and instruction does not change anything:

	dc.b	1
label3:

        ; blahblahblah

	nop
	jmp	label3

	; The 'memory' about the 'most recent label' ends when a non-empty
        ; line (i.e. a line with a statement and/or another label) is
        ; encountered.  So, label4 points to the odd address of the padding
        ; byte, not to the even address of the NOP instruction:

	dc.b	1
label4:
pos3	equ	label4
	nop
	expect	180
	jmp	label4
	endexpect

	; Only the most recent label is memorized and possibly adapted.  So
        ; in this case, label5 holds an odd address (of the pad byte...),
        ; and label6 the padded address:

	dc.b	1
label5:
label6:
	nop
	expect	180
	jmp	label5
	endexpect
	jmp	label6

	; The same is (of course) true if the second label is in the same
        ; line as the padded instruction:

	dc.b	1
label7:
label8:	nop
	expect	180
	jmp	label7
	endexpect
	jmp	label8
