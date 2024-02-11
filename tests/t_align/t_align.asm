	cpu	68020

	padding	off

	org	$80000000
	ds	0		; does nothing, address is already aligned
	dc.l	*

	org	$8000000f
	ds	0		; will align NOP to $80000010
	dc.l	*
