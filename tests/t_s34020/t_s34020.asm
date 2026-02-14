		cpu	34020
		page	0

		reg	off		; do not use built-in B register alias names

STK		.set	A14		; C-parameter stack pointer
SADDR		.set	B0		; Source address register
SPTCH		.set	B1		; Source pitch register
DADDR		.set	B2		; Destination address register
DYDX		.set	B7		; Delta X/delta Y register
MADDR		.set	B10		; Mask address register
INC1		.set	B11		; Minor axis (diagonal) increment
MPTCH		.set	B11		; Mask pitch register
PATTERN		.set	B13		; Pattern register
TEMP		.set	B14		; Temporary register

		section	_memcpy
		public	_memcpy
_memcpy:	mmtm	SP,SADDR,DADDR,DYDX,TEMP	; save the required registers
		move	STK,TEMP			; copy C-stack to B-file register
		move	*-TEMP,DADDR,1			; pop s1 (FS 1 assumed to be 32)
		move	*-TEMP,SADDR,1			; pop s2
		move	*-TEMP,DYDX,1			; pop n (byte count)
		move	TEMP,STK			; Update C stack
		sll	3,DYDX				; convert to a bit count
		blmove	1,1				; perform the block move
		mmfm	SP,SADDR,DADDR,DYDX,TEMP	; restore the required registers
		rets 2					; return to caller (with C-calling convention)
		endsection

_vfill_ok:	.int	0
		section	_fill_rect
		public	_fill_rect
_fill_rect:
		mmtm	SP,B2,B7,B10,B11,B12		; save required registers
		move	A14,B10				; move c-stack pointer into B-file
		move	*-B10,DYDX,1			; get width
		move	*-B10,B12,1			; get height
		sll	16,B12
		movy	B12,DYDX			; concatenate width & height
		move	*-B10,DADDR,1			; get xleft
		move	*-B10,B12,1			; get ytop
		move	B10,A14				; restore c-stack pointer
		sll	16,B12
		movy	B12,DADDR			; concatenate xleft & ytop
		move	@_vfill_ok,A8,1			; get state of vfill flag
		jrz	no_vfill
		clip					; clip to the window
		jrz	exit				; if outside the window, exit
		cvdxyl	DADDR				; convert to linear dest address
		vlcol					; load VRAM color latches
		vfill	L				; perform linear fill
		jruc	exit
no_vfill:
		fill	XY				; fill the rectangle using standard fill
exit:
		mmfm	SP,B2,B7,B10,B11,B12		; restore required registers
		rets	2
		endsection

		section	_draw_line
		public	_draw_line
_draw_line:	mmtm	SP,A0,A1
		mmtm	SP,B2,B7,B10,B11,B12,B13,B14
		move	*-A14,A0,1	; get xs
		move	*-A14,A1,1	; get ys
		sll	16,A1
		movy	A1,A0		; concatenate ys::xs
		move	*-A14,A1,1	; get xe
		move	*-A14,A8,1	; get ye
		sll	16,A8
		movy	A8,A1		; concatenate ye::xe
		move	A0,DADDR
		move	A1,DYDX
		movi	-1,PATTERN
		linit
		jrc	exit		; line is entirely outside window
		jrp	cliptst		; line is neither vertical nor horizontal
; Confirmed that line is vertical or horizontal, so FILL can be used
; instead of LINE. Now determine whether line points in the +x or
; +y direction, or in the -x or -y direction.
vorh:		cmpxy	A0,A1		; a = xe - xs, b = ye - ys
		jrv	negdir		; jump if -x direction
		jrc	negdir		; jump if -y direction
; Horizontal or vertical line points in +x or +y direction.
		move	A0,DADDR	; DADDR = ys::xs
		subxy	A0,A1		; a = xe - xs, b = ye - ys
		move	A1,DYDX		; DYDX = length of line
		jruc	vorhline	;
; Horizontal or vertical line points in -x or -y direction.
negdir:		move	A1,DADDR	; DADDR = ye::xe
		subxy	A1,A0		; a = xs - xe, b = ys - ye
		move	A0,DYDX		; DYDX = length of line
; Draw the vertical or horizontal line.
vorhline:	addi	010001h,DYDX	; ++DX, ++DY
		fill	XY		; draw line
		jruc	exit
cliptst:	jrv	draw		; clipping required use LINE
		cvdxyl	DADDR		; convert to linear address
		move	INC1,INC1	; does line point up or down?
		jrlt	fdown
		fline	0		; draw Bresenham line
		jruc	exit
fdown:		fline	1		; draw Bresenham line
		jruc	exit
draw:		move	INC1,INC1	; does line point up or down?
		jrlt	down
		line	0		; draw Bresenham line
		jruc	exit
down:		line	1		; draw Bresenham line
exit:		mmfm	SP,B2,B7,B10,B11,B12,B13,B14
		mmfm	SP,A0,A1
		rets	2		; return to caller
		endsection

		section	_fill_patnrect
		public	_fill_patnrect
_fill_patnrect:
		mmtm	SP,A0,A1,A2,A3	; save required registers
		mmtm	SP,B0,B1,B2,B7,B10,B11,B13,B14
		move	STK,B14
		move	*-B14,DYDX,1	; pop w
		move	*-B14,B10,1	; pop h
		move	*-B14,DADDR,1	; pop xleft
		move	*-B14,B11,1	; pop ytop
		move	B14,STK
		move	*-STK,A3,1	; pop pointer to pattern
		sll	16,B10
		movy	B10,DYDX	; concatenate w, h
		sll	16,B11
		movy	B11,DADDR	; concatenate xleft, ytop
		clip			; clip the rectangle to the window
		jrz	exit		; jump if rectangle outside window
		move	DYDX,A1		; set up y count
		move	A1,A2
		srl	16,A1
		movi	00010000,A0
		movy	A0,A2
		move	A2,DYDX
		move	DADDR,A2
		getps	B0		; calculate pattern adjustment
		rmo	B0,B0
		neg	B0
		movk	32,B1
		srl	B0,B1		; number pixels per 32 bit word
		subk	1,B1		; so complement will count pix's wrd
		move	DADDR,B0
		andn	B1,B0		; address rounded to pix's/word bndry
		neg	B0		; shift count = -(LSBs of x)
loop:
		move	A3,B10		; pattern start address
		movk	15,B11		; load 4-bit mask
		sll	16,B11		; align mask with 4 LSBs of y
		and	DADDR,B11	; isolate 4 LSBs of y
		srl	12,B11		; convert y to index value
		add	B11,B10		; index into pattern
		move	*B10,B10,0	; get 16-bit row of pattern
		move	B10,B11
		sll	16,B11
		movy	B11,B10		; replicate row to 32 bits
		rl	B0,B10		; align pattern for x address
		move	B10,PATTERN	; load aligned pattern
		pfill	XY
		addxy	A0,A2
		move	A2,DADDR
		dsj	A1,loop
exit:
		mmfm	SP,B0,B1,B2,B7,B10,B11,B13,B14
		mmfm	SP,A0,A1,A2,A3	; restore required registers
		rets	2
		endsection

		section	_tfill
		public	_tfill
_tfill:
		mmtm	SP,B0,B1,B2,B7,B10,B11,B12,B13,B14
		move	STK,B14		; get C-parameter stack into B-file
		move	*-B14,SADDR,1	; pop xla
		sll	16,SADDR	; convert to fixed point
		move	*-B14,DYDX,1	; pop x2a
		sll	16,DYDX		; convert to fixed point
		move	*-B14,MPTCH,1	; pop ya
		move	*-B14,SPTCH,1	; pop xlb
		sll	16,SPTCH	; convert to fixed point
		move	*-B14,B13,1	; pop x2b
		sll	16,B13		; convert to fixed point
		move	*-B14,B12,1	; pop yb
		move	B14,STK		; update C-parameter stack
		sub	SADDR,SPTCH	; delta x1
		sub	DYDX,B13	; delta x2
		sub	MPTCH,B12	; delta y
		divs	B12,SPTCH	; dx1
		divs	B12,B13		; dx1 (cant use MADDR since divs
					; requires odd numbered register)
		move	B13 ,MADDR	; copy into MADDR
		sll	16,MPTCH	; convert y to fixed point
loop:
		tfill
		dsjs	B12,loop
		mmfm	SP,B0,B1,B2,B7,B10,B11,B12,B13,B14
		rets	2
		endsection

_vblt_ok:	.int	0
		section	_vblt
		public	_vblt
_vblt:
		mmtm	SP,B0,B1,B2,B7,B10,B11,B12 ; save required registers
		move	STK,B10		; move c-stack pointer into B-file
		move	*-B10,DYDX,1	; get width
		move	DYDX,SPTCH	; save the width as source pitch
		move	*-B10,B12,1	; get height
		sll	16,B12		
		movy	B12,DYDX	; concatenate width & height
		move	*-B10,DADDR,1	; get xleft
		move	*-B10,B12,1	; get ytop
		move	*-B10,SADDR,1	; get source address
		move	B10,STK         ; restore c-stack pointer
		sll	16,B12
		movy	B12,DADDR	; concatenate xleft & ytop
		move	@_vblt_ok,A8,1	; get state of vblt flag
		jrz	no_vblt		
vblt:
		clip			; clip to the window
		jrz	exit		; if outside the window, exit
		cvdxyl	DADDR		; convert to linear dest address
		vlcol			; load VRAM color latches
		vblt	B,L		; perform linear vblt
		jruc	exit		
no_vblt:
		pixblt	B,XY
exit:
		mmfm	SP,B0,B1,B2,B7,B10,B11,B12 ; restore required registers
		rets	2
		endsection
