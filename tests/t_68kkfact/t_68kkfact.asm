	cpu		68020
	fpu		on

	fmove.p		fp4,(a3)
	fmove.p{#5}	fp4,(a3)
	fmove.p		fp4,(a3){#5}
	fmove.p{#-3}	fp4,(a3)
	fmove.p		fp4,(a3){#-3}
	fmove.p{#-3}	fp0,(a3)
	fmove.p		fp4,(a3){#-3}
	fmove.p{d3}	fp4,(a3)
	fmove.p		fp4,(a3){d3}

