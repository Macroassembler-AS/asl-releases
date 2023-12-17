	cpu	8086
	page	0
	fpu	on

	; instructions ordered by ascending machine code:

	fadd	dword ptr [si]
	fmul	dword ptr [si]
	fcom	dword ptr [si]
	fcomp	dword ptr [si]
	fsub	dword ptr [si]
	fsubr	dword ptr [si]
	fdiv	dword ptr [si]
	fdivr	dword ptr [si]
	fadd	st,st(1)
	fmul	st,st(1)
	fcom	st(1)
	fcomp	st(1)
	fsub	st,st(1)
	fsubr	st,st(1)
	fdiv	st,st(1)
	fdivr	st,st(1)
	fld	dword ptr [si]
	;
	fst	dword ptr [si]
	fstp	dword ptr [si]
	fldenv	[si]
	fldcw	[si]
	fstenv	[si]
	fstcw	[si]
	fld	st(1)
	fxch	st(1)
	fnop
	;
	fchs
	fabs
	;
	ftst
	fxam
	;
	fld1
	fldl2t
	fldl2e
	fldpi
	fldlg2
	fldln2
	fldz
	;
	f2xm1
	fyl2x
	fptan
	fpatan
	fxtract
	;fprem1			; 387++
	fdecstp
	fincstp
	fprem
	fyl2xp1
	fsqrt
	;fsincos		; 387++
	frndint
	fscale
	;fsin			; 387++
	;fcos			; 387++
	fiadd	dword ptr[si]
	fimul	dword ptr[si]
	ficom	dword ptr[si]
	ficomp	dword ptr[si]
	fisub	dword ptr[si]
	fisubr	dword ptr[si]
	fidiv	dword ptr[si]
	fidivr	dword ptr[si]
	;
	;fucompp		; 387++
	;
	fild	dword ptr[si]
	;
	fist	dword ptr[si]
	fistp	dword ptr[si]
	;
	fld	tbyte ptr[si]
	;
	fstp	tbyte ptr[si]
	;
	fclex
	finit
	;
	fadd	qword ptr[si]
	fmul	qword ptr[si]
	fcom	qword ptr[si]
	fcomp	qword ptr[si]
	fsub	qword ptr[si]
	fsubr	qword ptr[si]
	fdiv	qword ptr[si]
	fdivr	qword ptr[si]
	fadd	st(1),st
	fmul	st(1),st
	;
	fsubr	st(1),st
	fsub	st(1),st
	fdivr	st(1),st
	fdiv	st(1),st
	fld	qword ptr[si]
	;
	fst	qword ptr[si]
	fstp	qword ptr[si]
	frstor	[si]
	;
	fsave	[si]
	fstsw	[si]
	ffree	st(1)
	;
	fst	st(1)
	fstp	st(1)
	;fucom	st(1)		; 387++
	;fucomp	st(1)		; 387++
	;
	fiadd	word ptr[si]
	fimul	word ptr[si]
	ficom	word ptr[si]
	ficomp	word ptr[si]
	fisub	word ptr[si]
	fisubr	word ptr[si]
	fidiv	word ptr[si]
	fidivr	word ptr[si]
	faddp	st(1),st
	fmulp	st(1),st
	;
	fcompp
	;
	fsubrp	st(1),st
	fsubp	st(1),st
	fdivrp	st(1),st
	fdivp	st(1),st
	fild	word ptr[si]
	;
	fist	word ptr[si]
	fistp	word ptr[si]
	fbld	tbyte ptr[si]
	fild	qword ptr[si]
	fbstp	tbyte ptr[si]
	fistp	qword ptr[si]
	;
	;fstsw	ax		; 287++
	