		cpu	34010
		page	0

;*********************************************************************
;* Draw a line from point (xs,ys) to point (xe,ye) using Bresenham's *
;* algorithm. When -draw_line is called, xs is in the 16 LSBs of B2, *
;* ys is in the 16 MSBs of B2, xe is in the 16 LSBs of B0, and ye is *
;* in the 16 MSBs of B0.                                             *
;*********************************************************************

		section	_draw_line
		public	_draw_line
_draw_line:
		SUBXY	B2, B0		; Calculate a and b
; Now set up B7 (a,b) and B11 = (dx_diag,dy_diag). Assume that
; a < 0 and b < 0; if a >= 0 or b >= 0, make corrections later.
; Register B11 (INC1) contains dy_diag::dx_diag
; Register B12 (INC2) contains dy_nondiag::dx-nondiag
		MOVI	-1, B11		; dx-diag = dy_diag - 1
		MOVK	1, B12		; Constant = 1
		CLR	B7
		SUBXY	B0, B7		; B7 = (-a,-b)
		JRNC	L1		; Jump if b < 0
; Handle case where b >= 0:
		MOVY	B0, B7		; Make a in B7 positive
		SRL	15, B11		; Change dy_diag to +1
L1:
		JRNV	L2		; Jump if a < 0
; Handle case where a >= 0:
		MOVX	B0, B7		; Take absolute value of a
		MOVX	B12, B11	; Change dx-diag to +1
L2:
		MOVX	B11, B12	; dx_nondiag=dx-diag, dy_nondiag=0
; Compare magnitudes of a and b:
		MOVE	B7, B0		; Copy a and b
		SRL	16, B0		; Move b into 16 LSBs
		CMPXY	B0, B7		; Compare a and b
		JRNV	L3		; Jump if a >= b
; Handle case where a < b; must swap a and b so that a >= b:
		MOVX	B7, B0		; Copy b into BO
		RL	16, B7		; Swap a and b halves of B7
		CLR	B12
		MOVY	B11, B12	; dx_nondiag=0, dy_nondiag=dy_diag
; Calculate initial values of decision variable (d) and
;* loop counter:
L3:		ADD	B0, B0		; B0 = 2 x b
		MOVX    B7, B10		; B10 = a
		SUB     B10, B0		; B0 = d (2 x b - a)
		ADDK    1, B10		; Loop count = a + 1 (in B10)
; Draw line and return to caller:
		LINE	0		; Inner loop of line algorithm
		RETS	0		; Return to caller
		endsection

;*-----------------------------------------------------------------------
;* Assume that the registers have been loaded as follows:
;* B0 = linear start address of source bitmap
;* B1 = SPTCH (no restrictions)
;* B2 = start y coord. ytop in 16 MSBs, start x coord.
;*      xleft in 16 LSBs
;* B3 = DPTCH (must be power of 2)
;* B4 = OFFSET
;* B5 = WSTART
;* B6 = WEND
;* B7 = DY::DX (array height in 16 MSBs, array width in 16 LSBs)
;* B8 = COLORO
;* B9 = COLOR1
;* FS1 >= 16 (assume multiplier for MPYS below is less than 16 bits)
;* CONVSP will not be used.
;* Implied operands in other I/O registers (incl. CONVDP) are valid.
;* Window option = 3
;*-----------------------------------------------------------------------

		section	_color_expand
		public	_color_expand
_color_expand:
		MOVY	B2,B10		; copy ytop
		SUBXY   B5,B10          ; window y overlap = ytop - ystart
		JRYNN   INWINDOW        ; jump if ytop below top of window
; Need to clip destination array to top edge of clipping window
		MOVY	B5,B2		; clip ytop to top of window
		SRA     16,B10          ; shift y overlap to 16 LSBs
		MOVE    B1,B11          ; copy SPTCH
		MPYS    B10,B11         ; (y overlap) * SPTCH
		SUB     B11,B0          ; clip SADDR to top of window
		SLL     16,B10          ; shift y overlap to 16 MSBs
		ADDXY   B10,B7          ; clip DY to top of window
		JRLS    DONE            ; done if DY<=O (completely
					; above window)
; PIXBLT instruction will do any additional clipping required
INWINDOW:
		PIXBLT	B,XY		; color expand bitmap to screen
; Restore registers and return
DONE:
		RETS			; done                       
		endsection
