	cpu	90c141

	irp	instr,mul,div
	instr	hl,c
	instr	hl,34h
	instr	hl,(de)
	instr	hl,(iy-5)
	instr	hl,(hl+a)
	instr	hl,(1234h)
	instr	hl,(0ff34h)
	endm

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
