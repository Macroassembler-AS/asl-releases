	cpu	z180

	; default mapping:
        ; - no common area 0
        ; - bank area 0000h...0efffh
        ; - common area 1 0f000...0ffffh

	assume	cbar:0f0h,cbr:00h, bbr:00h
	;prwins

	dw	phys2cpu(00000h)	; 00000h (bank area) -> 0000h
	dw	phys2cpu(01000h)	; 01000h (bank area) -> 1000h
	dw	phys2cpu(02000h)	; 02000h (bank area) -> 2000h
	dw	phys2cpu(03000h)	; 03000h (bank area) -> 3000h
	dw	phys2cpu(04000h)	; 04000h (bank area) -> 4000h
	dw	phys2cpu(05000h)	; 05000h (bank area) -> 5000h
	dw	phys2cpu(06000h)	; 06000h (bank area) -> 6000h
	dw	phys2cpu(07000h)	; 07000h (bank area) -> 7000h
	dw	phys2cpu(08000h)	; 08000h (bank area) -> 8000h
	dw	phys2cpu(09000h)	; 09000h (bank area) -> 9000h
	dw	phys2cpu(0a000h)	; 0a000h (bank area) -> a000h
	dw	phys2cpu(0b000h)	; 0b000h (bank area) -> b000h
	dw	phys2cpu(0c000h)	; 0c000h (bank area) -> c000h
	dw	phys2cpu(0d000h)	; 0d000h (bank area) -> d000h
	dw	phys2cpu(0e000h)	; 0e000h (bank area) -> e000h
	dw	phys2cpu(0f000h)	; 0f000h (common area 1) -> f000h

	; example mapping:
        ; - common area 0 0000h...3fffh -> 0000h...3fffh
        ; - bank area     4000h...0cfffh -> 20000h...28fff
        ; - common area 1 0d000...0ffffh -> 10000h...11fff

	assume	cbar:0d4h,cbr:03h, bbr:1ch
	;prwins

	dw	phys2cpu(00000h)	; 00000h (common area 0) -> 0000h
	dw	phys2cpu(01000h)	; 01000h (common area 0) -> 1000h
	dw	phys2cpu(02000h)	; 02000h (common area 0) -> 2000h
	dw	phys2cpu(03000h)	; 03000h (common area 0) -> 3000h
	dw	phys2cpu(20000h)	; 04000h (bank area) -> 20000h
	dw	phys2cpu(21000h)	; 05000h (bank area) -> 21000h
	dw	phys2cpu(22000h)	; 06000h (bank area) -> 22000h
	dw	phys2cpu(23000h)	; 07000h (bank area) -> 23000h
	dw	phys2cpu(24000h)	; 08000h (bank area) -> 24000h
	dw	phys2cpu(25000h)	; 09000h (bank area) -> 25000h
	dw	phys2cpu(26000h)	; 0a000h (bank area) -> 26000h
	dw	phys2cpu(27000h)	; 0b000h (bank area) -> 27000h
	dw	phys2cpu(28000h)	; 0c000h (bank area) -> 28000h
	dw	phys2cpu(10000h)	; 0d000h (common area 1) -> 10000h
	dw	phys2cpu(11000h)	; 0e000h (common area 1) -> 11000h
	dw	phys2cpu(12000h)	; 0f000h (common area 1) -> 12000h

	; test symmetry of mappings:

	listing	off
addr	set	0000h
	while	addr<10000h
	if 	phys2cpu(cpu2phys(addr)) != addr
	error	"mapping error at $\{ADDR}"
	endif
addr	set	addr+100h
	endm
	listing	on
