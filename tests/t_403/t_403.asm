        cpu     ppc403
        page    0
        include stddef60

	org	0x1000


        mtdcr   10,r5
        mtbear  r5
        mfdcr   r5,10
        mfbesr  r5
        wrtee   r10
        wrteei  1

        bdnzl   0x10
        bdztla  7,0x10
        beq     0x10
        beq     2,0x10
        beq	cr2,0x10

        cmpw    r1,r2
        clrlwi  r1,r2,5

        mtspr	pit,r5

        mcrf	cr4,cr5
        mcrf	4,5

; register aliases

		cpu	mpc601

myreg1e		equ	r4
myreg2e		equ	r5
myreg3e		equ	r6
myreg1r		reg	r4
myreg2r		reg	r5
myreg3r		reg	r6
myreg1re	reg	myreg1e
myreg2re	reg	myreg2e
myreg3re	reg	myreg3e

myfreg1e	equ	fr5
myfreg2e	equ	fr6
myfreg3e	equ	fr7
myfreg1r	reg	fr5
myfreg2r	reg	fr6
myfreg3r	reg	fr7
myfreg1re	reg	myfreg1e
myfreg2re	reg	myfreg2e
myfreg3re	reg	myfreg3e

		add	r4,r5,r6
		add	myreg1e,myreg2e,myreg3e
		add	myreg1r,myreg2r,myreg3r
		add	myreg1re,myreg2re,myreg3re

		fadd	fr5,fr6,fr7
		fadd	myfreg1e,myfreg2e,myfreg3e
		fadd	myfreg1r,myfreg2r,myfreg3r
		fadd	myfreg1re,myfreg2re,myfreg3re

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
