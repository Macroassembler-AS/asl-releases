	cpu	z80
	page	0
	org	1000h

	j	$	; -> jr
	jr	$
	j	nz,$	; -> jr
	jr	nz,$
	j	z,$	; -> jr
	jr	z,$
	j	nc,$	; -> jr
	jr	nc,$
	j	c,$	; -> jr
	jr	c,$
	j	po,$	; -> jp (no jr po,<addr>)
	jp	po,$
	j	pe,$	; -> jp (no jr pe,<addr>)
	jp	pe,$
	j	p,$	; -> jp (no jr p,<addr>)
	jp	p,$
	j	m,$	; -> jp (no jr m,<addr>)
	jp	m,$
	j	$+100h	; -> jp (too far for jr)
	jp	$+100h
	j	nz,$+100h	; -> jp (too far for jr)
	jp	nz,$+100h
	j	z,$-100h	; -> jp (too far for jr)
	jp	z,$-100h
	j	nc,$+100h	; -> jp (too far for jr)
	jp	nc,$+100h
	j	c,$-100h	; -> jp (too far for jr)
	jp	c,$-100h
	j	(hl)	; -> jp
	jp	(hl)
	j	(ix)	; -> jp
	jp	(ix)
	j	(iy)	; -> jp
	jp	(iy)
