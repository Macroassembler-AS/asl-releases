		cpu	z80

		if	(mompass>1) && (symexist(init_platform))
		call	init_platform
		endif

		ifsymexist init_platform
		endif		

		jp	forw
forw		nop

		if	(mompass>1) && (symexist(exit_platform))
		call	exit_platform
		endif

		ifsymexist exit_platform
		endif		

;--------

init_platform 	nop
		ret

;--------

;exit_platform 	nop
;		ret
