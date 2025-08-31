	cpu	68000
	page	0

	jmp	dest		; -> matched in global name space

	section	one

	 jmp	dest		; -> matched in one's name space

	 section	one_one
	  jmp	dest		; -> matched in one's name space
	 endsection
	 section	one_two
	  jmp	dest		; -> matched in one's name space
	 endsection
	 section	one_three
	  jmp	dest		; -> matched in one's name space
	 endsection
	 section	one_four
	  jmp	dest		; -> matched in one's name space
	 endsection

dest:	 if	symused(dest)	; -> used from one, one_one, one_two, one_three, and one_four
	  nop
	 endif

	endsection

	section	two

	 jmp	dest		; -> matched in two's name space

	 section	two_one
	 endsection

	 section	two_two
	 endsection

	 section	two_three
	 endsection

	 section	two_four
	 endsection

dest:	 if	symused(dest)	; -> used from two
	 nop
	 endif

	endsection

	section	three

dest:	 if	symused(dest)	; -> not used
	  nop
	 endif

	endsection

	section	four

	 jmp	dest		; -> matched in four's name space

	 section	four_one
	 endsection

	 section	four_two
	 endsection

	 section	four_three
	 endsection

	 section	four_four
dest:	  if	symused(dest)	; -> not used
	   nop
	  endif
	 endsection

dest:	 if	symused(dest)	; -> used from four
	 nop
	 endif

	endsection

dest:	if	symused(dest)	; -> used from global name space
	nop
	endif

