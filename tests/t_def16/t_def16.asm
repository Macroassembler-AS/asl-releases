	cpu	6800
	page	0
	include	"bitfuncs.inc"

def16	macro	NEWINST,LOINST,HIINST
NEWINST	macro	ARG1,ARG2
	if	"ARG2" != ""		; indexed?
	 LOINST	 (ARG1)+1,ARG2
	 HIINST	 ARG1,ARG2
	elseif				; not indexed?
_SARG1 	 set	 "ARG1"			; convert to string
	 if	 substr(_SARG1,0,1)='#'	; immediate?
_SARG1	  set	  substr(_SARG1,1,strlen(_SARG1)-1)	; yes->strip off leading #...
	  LOINST  #lo(VAL(_SARG1))	; ...and subtract lo/hi bytes
	  HIINST  #hi(VAL(_SARG1))
	 elseif				; no immediate, i.e absolute/direct
	  LOINST  (ARG1)+1		; ...and subtract lo/hi bytes
	  HIINST  ARG1
	 endif
	endif
	endm
	endm

	def16	addd,adda,adcb
	def16	subd,suba,sbcb
	def16	andd,anda,andb
	def16	ord,ora,orb
	def16	eord,eora,eorb

	addd	$0007			; direct
	addd	$1234			; absolute
	addd	#$55aa			; immediate
	addd	$12,x			; indexed

	subd	$0007			; direct
	subd	$1234			; absolute
	subd	#$55aa			; immediate
	subd	$12,x			; indexed

	andd	$0007			; direct
	andd	$1234			; absolute
	andd	#$55aa			; immediate
	andd	$12,x			; indexed

	ord	$0007			; direct
	ord	$1234			; absolute
	ord	#$55aa			; immediate
	ord	$12,x			; indexed

	eord	$0007			; direct
	eord	$1234			; absolute
	eord	#$55aa			; immediate
	eord	$12,x			; indexed
