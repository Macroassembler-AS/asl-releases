	cpu	pdp-11/94
	page	0

	;--------------------
	; default layout: CPU addresses map 1:1 to physical ones

	;prwins
	word	phys2cpu(0x0000)
	word	phys2cpu(0x1000)
	word	phys2cpu(0x2000)
	word	phys2cpu(0x3000)
	word	phys2cpu(0x4000)
	word	phys2cpu(0x5000)
	word	phys2cpu(0x6000)
	word	phys2cpu(0x7000)
	word	phys2cpu(0x8000)
	word	phys2cpu(0x9000)
	word	phys2cpu(0xa000)
	word	phys2cpu(0xb000)
	word	phys2cpu(0xc000)
	word	phys2cpu(0xd000)
	word	phys2cpu(0xe000)
	word	phys2cpu(0xf000)
	expect	1320
	word	phys2cpu(0x10000)
	endexpect
	
	;-----------------------
	; remap APR1: 0x2000...0x27ff -> 0x20000...0x207ff (upward, 2K mapped)

	assume	par1:0x0800,pdr1:0x1f05
	;prwins
	word	phys2cpu(0x20100)
	expect	1320
	word	phys2cpu(0x20800)	; the first address not mapped
	endexpect

	;-----------------------
	; remap APR2: 0x5000...0x5fff -> 0x41000...0x41fff (downward, 4K mapped)

	assume	par2:0x1000,pdr2:0xc00d
	;prwins
	word	phys2cpu(0x41800)
	expect	1320
	word	phys2cpu(0x40800)
	endexpect
