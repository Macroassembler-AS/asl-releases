	cpu	70616
	page	0
	supmode	on
	include regv60.inc

regop0	equ	r4
regop1	reg	r12
regop2	equ	r14
regop3	reg	r16
sptr	equ	sp
fptr	reg	fp	
aptr	equ	ap
port1	port	22h

	; iterate through addressing modes (as source)

	; Register

	mov.w	regop1,regop2

	; Register Indirect

	mov.w	[regop0],r14

	; Register Indirect Indexed

	mov.w	[r4](regop1),regop2

	; Autoincrement

	mov.w	[regop0+],r14

	; Autodecrement

	mov.w	[-r4],regop2

	; Displacement

	mov.w	100[regop0],r14
	mov.w	100.8[regop0],r14
	mov.w	100.16[regop0],r14
	mov.w	100.32[regop0],r14
	mov.w	1000[regop0],r14
	expect	1320
	mov.w	1000.8[regop0],r14
	endexpect
	mov.w	1000.16[regop0],r14
	mov.w	1000.32[regop0],r14
	mov.w	100000[regop0],r14
	expect	1320
	mov.w	100000.8[regop0],r14
	endexpect
	expect	1320
	mov.w	100000.16[regop0],r14
	endexpect
	mov.w	100000.32[regop0],r14

	; PC Displacement

	mov.w	[pc],r14	; no PC-relative computation!
	mov.w	$-100[pc],r14
	mov.w	$-100.8[pc],r14
	mov.w	$-100.16[pc],r14
	mov.w	$-100.32[pc],r14
	mov.w	$-1000[pc],r14
	expect	1315
	mov.w	$-1000.8[pc],r14
	endexpect
	mov.w	$-1000.16[pc],r14
	mov.w	$-1000.32[pc],r14
	mov.w	$-100000[pc],r14
	expect	1315
	mov.w	$-100000.8[pc],r14
	endexpect
	expect	1315
	mov.w	$-100000.16[pc],r14
	endexpect
	mov.w	$-100000.32[pc],r14

	; Displacement Indexed

	mov.w	100[regop0](r12),r14
	mov.w	100.8[regop0](r12),r14
	mov.w	100.16[regop0](r12),r14
	mov.w	100.32[regop0](r12),r14
	mov.w	1000[regop0](r12),r14
	expect	1320
	mov.w	1000.8[regop0](r12),r14
	endexpect
	mov.w	1000.16[regop0](r12),r14
	mov.w	1000.32[regop0](r12),r14
	mov.w	100000[regop0](r12),r14
	expect	1320
	mov.w	100000.8[regop0](r12),r14
	endexpect
	expect	1320
	mov.w	100000.16[regop0](r12),r14
	endexpect
	mov.w	100000.32[regop0](r12),r14

	; PC Displacement Indexed

	mov.w	[pc](r12),r14	; no PC-relative computation!
	mov.w	$-100[pc](r12),r14
	mov.w	$-100.8[pc](r12),r14
	mov.w	$-100.16[pc](r12),r14
	mov.w	$-100.32[pc](r12),r14
	mov.w	$-1000[pc](r12),r14
	expect	1315
	mov.w	$-1000.8[pc](r12),r14
	endexpect
	mov.w	$-1000.16[pc](r12),r14
	mov.w	$-1000.32[pc](r12),r14
	mov.w	$-100000[pc](r12),r14
	expect	1315
	mov.w	$-100000.8[pc](r12),r14
	endexpect
	expect	1315
	mov.w	$-100000.16[pc](r12),r14
	endexpect
	mov.w	$-100000.32[pc](r12),r14

	; Displacement Indirect

	mov.w	[[r4]],r14
	mov.w	[100[r4]],r14
	mov.w	[100.8[r4]],r14
	mov.w	[100.16[r4]],r14
	mov.w	[100.32[r4]],r14
	mov.w	[1000[r4]],r14
	expect	1320
	mov.w	[1000.8[r4]],r14
	endexpect
	mov.w	[1000.16[r4]],r14
	mov.w	[1000.32[r4]],r14
	mov.w	[100000[r4]],r14
	expect	1320
	mov.w	[100000.8[r4]],r14
	endexpect
	expect	1320
	mov.w	[100000.16[r4]],r14
	endexpect
	mov.w	[100000.32[r4]],r14

	; PC Displacement Indirect

	mov.w	[[pc]],r14	; no PC-relative computation!
	mov.w	[$+100[pc]],r14
	mov.w	[$+100.8[pc]],r14
	mov.w	[$+100.16[pc]],r14
	mov.w	[$+100.32[pc]],r14
	mov.w	[$+1000[pc]],r14
	expect	1320
	mov.w	[$+1000.8[pc]],r14
	endexpect
	mov.w	[$+1000.16[pc]],r14
	mov.w	[$+1000.32[pc]],r14
	mov.w	[$+100000[pc]],r14
	expect	1320
	mov.w	[$+100000.8[pc]],r14
	endexpect
	expect	1320
	mov.w	[$+100000.16[pc]],r14
	endexpect
	mov.w	[$+100000.32[pc]],r14

	; Displacement Indirect Indexed

	mov.w	[[r4]](regop1),r14
	mov.w	[100[r4]](regop1),r14
	mov.w	[100.8[r4]](regop1),r14
	mov.w	[100.16[r4]](regop1),r14
	mov.w	[100.32[r4]](regop1),r14
	mov.w	[1000[r4]](regop1),r14
	expect	1320
	mov.w	[1000.8[r4]](regop1),r14
	endexpect
	mov.w	[1000.16[r4]](regop1),r14
	mov.w	[1000.32[r4]](regop1),r14
	mov.w	[100000[r4]](regop1),r14
	expect	1320
	mov.w	[100000.8[r4]](regop1),r14
	endexpect
	expect	1320
	mov.w	[100000.16[r4]](regop1),r14
	endexpect
	mov.w	[100000.32[r4]](regop1),r14

	; PC Displacement Indirect Indexed

	mov.w	[[pc]](regop1),r14	; no PC-relative computation!
	mov.w	[$+100[pc]](regop1),r14
	mov.w	[$+100.8[pc]](regop1),r14
	mov.w	[$+100.16[pc]](regop1),r14
	mov.w	[$+100.32[pc]](regop1),r14
	mov.w	[$+1000[pc]](regop1),r14
	expect	1320
	mov.w	[$+1000.8[pc]](regop1),r14
	endexpect
	mov.w	[$+1000.16[pc]](regop1),r14
	mov.w	[$+1000.32[pc]](regop1),r14
	mov.w	[$+100000[pc]](regop1),r14
	expect	1320
	mov.w	[$+100000.8[pc]](regop1),r14
	endexpect
	expect	1320
	mov.w	[$+100000.16[pc]](regop1),r14
	endexpect
	mov.w	[$+100000.32[pc]](regop1),r14

	; Double Displacement

	mov.w	100[100[regop0]],r14
	mov.w	100[1000[regop0]],r14
	mov.w	100[100000[regop0]],r14
	mov.w	1000[100[regop0]],r14
	mov.w	1000[1000[regop0]],r14
	mov.w	1000[100000[regop0]],r14
	mov.w	100000[100[regop0]],r14
	mov.w	100000[1000[regop0]],r14
	mov.w	100000[100000[regop0]],r14

	; PC Double Displacement

	mov.w	100[$+100[pc]],r14
	mov.w	100[$+1000[pc]],r14
	mov.w	100[$+100000[pc]],r14
	mov.w	1000[$+100[pc]],r14
	mov.w	1000[$+1000[pc]],r14
	mov.w	1000[$+100000[pc]],r14
	mov.w	100000[$+100[pc]],r14
	mov.w	100000[$+1000[pc]],r14
	mov.w	100000[$+100000[pc]],r14

	; Double Displacement Indexed not allowed!

	expect	1350
	mov.w	100[100[regop0]](r12),r14
	endexpect

	; PC Double Displacement Indexed not allowed!

	expect	1350
	mov.w	100[$+100[pc]](r12),r14
	endexpect

	; Direct Address

	mov.w	/12345678h,r14

	; Direct Address Indexed

	mov.w	/12345678h(r12),r14

	; Direct Address Deferred

	mov.w	[/12345678h],r14

	; Direct Address Deferred Indexed

	mov.w	[/12345678h](r12),r14

	; Immediate Quick

	mov.b	#2,r14
	mov.h	#8,r14
	mov.w	#12,r14

	; Immediate

	mov.b	#12h,r14
	mov.h	#1234h,r14
	mov.w	#12345678h,r14
	expect	1130		; no immediate mode if 64 bits
	mov.d	#12345678h,r14
	endexpect

	; we treat a 'plain symbol' as PC-relative addressing.  Though not
	; described by NEC, this is similar to e.g. the NS32K:

	mov.b	$+100,r14
	mov.b	$+100.8,r14
	mov.b	$+100.16,r14
	mov.b	$+100.32,r14
	mov.b	$+1000,r14
	expect	1320
	mov.b	$+1000.8,r14
	endexpect
	mov.b	$+1000.16,r14
	mov.b	$+1000.32,r14
	mov.b	$+100000,r14
	expect	1320,1320
	mov.b	$+100000.8,r14
	mov.b	$+100000.16,r14
	endexpect
	mov.b	$+100000.32,r14
	mov.h	$+100,r14
	mov.h	$+100.8,r14
	mov.h	$+100.16,r14
	mov.h	$+100.32,r14
	mov.h	$+1000,r14
	expect	1320
	mov.h	$+1000.8,r14
	endexpect
	mov.h	$+1000.16,r14
	mov.h	$+100000,r14
	expect	1320,1320
	mov.h	$+100000.8,r14
	mov.h	$+100000.16,r14
	endexpect
	mov.h	$+100000.32,r14
	mov.h	$+1000.32,r14
	mov.w	$+100,r14
	mov.w	$+100.8,r14
	mov.w	$+100.16,r14
	mov.w	$+100.32,r14
	mov.w	$+1000,r14
	expect	1320
	mov.w	$+1000.8,r14
	endexpect
	mov.w	$+1000.16,r14
	mov.w	$+1000.32,r14
	mov.w	$+100000,r14
	expect	1320,1320
	mov.w	$+100000.8,r14
	mov.w	$+100000.16,r14
	endexpect
	mov.w	$+100000.32,r14

	; and yes, this may also incude an optional index register:

	mov.w	$+100(r25),r14		; assure index prefix (0xc0 | 25 -> 0xd9) is included

	; walk through instructions:

	absf.s	r7,r9
	absf.s	[r7],[r9]
	absf.s	[r7+],[r9+]
	absf.s	[-r7],[-r9]
	absf.s	10[r7],10000[r9]
	absf.s	[10[r7]],[10000[r9]]
	absf.s	20[10[r7]],20000[10000[r9]]
	absf.s	/12345678h,/23456789h
	absf.s	[/12345678h],[/23456789h]
	absf.s	[r7](r8),[r9](r10)
	absf.s	10[r7](r8),10000[r9](r10)
	absf.s	[pc](r8),[pc](r10)
	absf.s	[10[r7]](r8),[10000[r9]](r10)
	absf.s	[[pc]](r8),[[pc]](r10)
	absf.s	/12345678h(r8),/23456789h(r10)
	absf.s	[/12345678h](r8),[/23456789h](r10)

	absf.l	r7,r9
	absf.l	[r7],[r9]
	absf.l	[r7+],[r9+]
	absf.l	[-r7],[-r9]
	absf.l	10[r7],10000[r9]
	absf.l	[10[r7]],[10000[r9]]
	absf.l	20[10[r7]],20000[10000[r9]]
	absf.l	/12345678h,/23456789h
	absf.l	[/12345678h],[/23456789h]
	absf.l	[r7](r8),[r9](r10)
	absf.l	10[r7](r8),10000[r9](r10)
	absf.l	[pc](r8),[pc](r10)
	absf.l	[10[r7]](r8),[10000[r9]](r10)
	absf.l	[[pc]](r8),[[pc]](r10)
	absf.l	/12345678h(r8),/23456789h(r10)
	absf.l	[/12345678h](r8),[/23456789h](r10)

	add.b	r7,r9
	add.b	[r7],[r9]
	add.b	[r7+],[r9+]
	add.b	[-r7],[-r9]
	add.b	10[r7],10000[r9]
	add.b	[10[r7]],[10000[r9]]
	add.b	20[10[r7]],20000[10000[r9]]
	add.b	/12345678h,/23456789h
	add.b	[/12345678h],[/23456789h]
	add.b	[r7](r8),[r9](r10)
	add.b	10[r7](r8),10000[r9](r10)
	add.b	[pc](r8),[pc](r10)
	add.b	[10[r7]](r8),[10000[r9]](r10)
	add.b	[[pc]](r8),[[pc]](r10)
	add.b	/12345678h(r8),/23456789h(r10)
	add.b	[/12345678h](r8),[/23456789h](r10)
	add.b	#2,r0
	add.b	#200,r0

	add.h	r7,r9
	add.h	[r7],[r9]
	add.h	[r7+],[r9+]
	add.h	[-r7],[-r9]
	add.h	10[r7],10000[r9]
	add.h	[10[r7]],[10000[r9]]
	add.h	20[10[r7]],20000[10000[r9]]
	add.h	/12345678h,/23456789h
	add.h	[/12345678h],[/23456789h]
	add.h	[r7](r8),[r9](r10)
	add.h	10[r7](r8),10000[r9](r10)
	add.h	[pc](r8),[pc](r10)
	add.h	[10[r7]](r8),[10000[r9]](r10)
	add.h	[[pc]](r8),[[pc]](r10)
	add.h	/12345678h(r8),/23456789h(r10)
	add.h	[/12345678h](r8),[/23456789h](r10)
	add.h	#2,r0
	add.h	#200,r0

	add.w	r7,r9
	add.w	[r7],[r9]
	add.w	[r7+],[r9+]
	add.w	[-r7],[-r9]
	add.w	10[r7],10000[r9]
	add.w	[10[r7]],[10000[r9]]
	add.w	20[10[r7]],20000[10000[r9]]
	add.w	/12345678h,/23456789h
	add.w	[/12345678h],[/23456789h]
	add.w	[r7](r8),[r9](r10)
	add.w	10[r7](r8),10000[r9](r10)
	add.w	[pc](r8),[pc](r10)
	add.w	[10[r7]](r8),[10000[r9]](r10)
	add.w	[[pc]](r8),[[pc]](r10)
	add.w	/12345678h(r8),/23456789h(r10)
	add.w	[/12345678h](r8),[/23456789h](r10)
	add.w	#2,r0
	add.w	#200,r0

	addc.b	r7,r9
	addc.b	[r7],[r9]
	addc.b	[r7+],[r9+]
	addc.b	[-r7],[-r9]
	addc.b	10[r7],10000[r9]
	addc.b	[10[r7]],[10000[r9]]
	addc.b	20[10[r7]],20000[10000[r9]]
	addc.b	/12345678h,/23456789h
	addc.b	[/12345678h],[/23456789h]
	addc.b	[r7](r8),[r9](r10)
	addc.b	10[r7](r8),10000[r9](r10)
	addc.b	[pc](r8),[pc](r10)
	addc.b	[10[r7]](r8),[10000[r9]](r10)
	addc.b	[[pc]](r8),[[pc]](r10)
	addc.b	/12345678h(r8),/23456789h(r10)
	addc.b	[/12345678h](r8),[/23456789h](r10)
	addc.b	#2,r0
	addc.b	#200,r0

	addc.h	r7,r9
	addc.h	[r7],[r9]
	addc.h	[r7+],[r9+]
	addc.h	[-r7],[-r9]
	addc.h	10[r7],10000[r9]
	addc.h	[10[r7]],[10000[r9]]
	addc.h	20[10[r7]],20000[10000[r9]]
	addc.h	/12345678h,/23456789h
	addc.h	[/12345678h],[/23456789h]
	addc.h	[r7](r8),[r9](r10)
	addc.h	10[r7](r8),10000[r9](r10)
	addc.h	[pc](r8),[pc](r10)
	addc.h	[10[r7]](r8),[10000[r9]](r10)
	addc.h	[[pc]](r8),[[pc]](r10)
	addc.h	/12345678h(r8),/23456789h(r10)
	addc.h	[/12345678h](r8),[/23456789h](r10)
	addc.h	#2,r0
	addc.h	#200,r0

	addc.w	r7,r9
	addc.w	[r7],[r9]
	addc.w	[r7+],[r9+]
	addc.w	[-r7],[-r9]
	addc.w	10[r7],10000[r9]
	addc.w	[10[r7]],[10000[r9]]
	addc.w	20[10[r7]],20000[10000[r9]]
	addc.w	/12345678h,/23456789h
	addc.w	[/12345678h],[/23456789h]
	addc.w	[r7](r8),[r9](r10)
	addc.w	10[r7](r8),10000[r9](r10)
	addc.w	[pc](r8),[pc](r10)
	addc.w	[10[r7]](r8),[10000[r9]](r10)
	addc.w	[[pc]](r8),[[pc]](r10)
	addc.w	/12345678h(r8),/23456789h(r10)
	addc.w	[/12345678h](r8),[/23456789h](r10)
	addc.w	#2,r0
	addc.w	#200,r0

	adddc.b	r7,r9,12h
	adddc	[r7],[r9],12h
	adddc.b	[r7+],[r9+],#12h
	adddc	[-r7],[-r9],#12h
	adddc.b	10[r7],10000[r9],12h
	adddc	[10[r7]],[10000[r9]],12h
	adddc.b	20[10[r7]],20000[10000[r9]],#12h
	adddc	/12345678h,/23456789h,#12h
	adddc.b	[/12345678h],[/23456789h],12h
	adddc	[r7](r8),[r9](r10),12h
	adddc.b	10[r7](r8),10000[r9](r10),#12h
	adddc	[pc](r8),[pc](r10),#12h
	adddc.b	[10[r7]](r8),[10000[r9]](r10),12h
	adddc	[[pc]](r8),[[pc]](r10),12h
	adddc.b	/12345678h(r8),/23456789h(r10),#12h
	adddc	[/12345678h](r8),[/23456789h](r10),#12h
	adddc.b	#2,r0,12h
	adddc	#200,r0,12h

	addf.s	r7,r9
	addf.s	[r7],[r9]
	addf.s	[r7+],[r9+]
	addf.s	[-r7],[-r9]
	addf.s	10[r7],10000[r9]
	addf.s	[10[r7]],[10000[r9]]
	addf.s	20[10[r7]],20000[10000[r9]]
	addf.s	/12345678h,/23456789h
	addf.s	[/12345678h],[/23456789h]
	addf.s	[r7](r8),[r9](r10)
	addf.s	10[r7](r8),10000[r9](r10)
	addf.s	[pc](r8),[pc](r10)
	addf.s	[10[r7]](r8),[10000[r9]](r10)
	addf.s	[[pc]](r8),[[pc]](r10)
	addf.s	/12345678h(r8),/23456789h(r10)
	addf.s	[/12345678h](r8),[/23456789h](r10)

	addf.l	r7,r9
	addf.l	[r7],[r9]
	addf.l	[r7+],[r9+]
	addf.l	[-r7],[-r9]
	addf.l	10[r7],10000[r9]
	addf.l	[10[r7]],[10000[r9]]
	addf.l	20[10[r7]],20000[10000[r9]]
	addf.l	/12345678h,/23456789h
	addf.l	[/12345678h],[/23456789h]
	addf.l	[r7](r8),[r9](r10)
	addf.l	10[r7](r8),10000[r9](r10)
	addf.l	[pc](r8),[pc](r10)
	addf.l	[10[r7]](r8),[10000[r9]](r10)
	addf.l	[[pc]](r8),[[pc]](r10)
	addf.l	/12345678h(r8),/23456789h(r10)
	addf.l	[/12345678h](r8),[/23456789h](r10)

	and.b	r7,r9
	and.b	[r7],[r9]
	and.b	[r7+],[r9+]
	and.b	[-r7],[-r9]
	and.b	10[r7],10000[r9]
	and.b	[10[r7]],[10000[r9]]
	and.b	20[10[r7]],20000[10000[r9]]
	and.b	/12345678h,/23456789h
	and.b	[/12345678h],[/23456789h]
	and.b	[r7](r8),[r9](r10)
	and.b	10[r7](r8),10000[r9](r10)
	and.b	[pc](r8),[pc](r10)
	and.b	[10[r7]](r8),[10000[r9]](r10)
	and.b	[[pc]](r8),[[pc]](r10)
	and.b	/12345678h(r8),/23456789h(r10)
	and.b	[/12345678h](r8),[/23456789h](r10)
	and.b	#2,r0
	and.b	#200,r0

	and.h	r7,r9
	and.h	[r7],[r9]
	and.h	[r7+],[r9+]
	and.h	[-r7],[-r9]
	and.h	10[r7],10000[r9]
	and.h	[10[r7]],[10000[r9]]
	and.h	20[10[r7]],20000[10000[r9]]
	and.h	/12345678h,/23456789h
	and.h	[/12345678h],[/23456789h]
	and.h	[r7](r8),[r9](r10)
	and.h	10[r7](r8),10000[r9](r10)
	and.h	[pc](r8),[pc](r10)
	and.h	[10[r7]](r8),[10000[r9]](r10)
	and.h	[[pc]](r8),[[pc]](r10)
	and.h	/12345678h(r8),/23456789h(r10)
	and.h	[/12345678h](r8),[/23456789h](r10)
	and.h	#2,r0
	and.h	#200,r0

	and.w	r7,r9
	and.w	[r7],[r9]
	and.w	[r7+],[r9+]
	and.w	[-r7],[-r9]
	and.w	10[r7],10000[r9]
	and.w	[10[r7]],[10000[r9]]
	and.w	20[10[r7]],20000[10000[r9]]
	and.w	/12345678h,/23456789h
	and.w	[/12345678h],[/23456789h]
	and.w	[r7](r8),[r9](r10)
	and.w	10[r7](r8),10000[r9](r10)
	and.w	[pc](r8),[pc](r10)
	and.w	[10[r7]](r8),[10000[r9]](r10)
	and.w	[[pc]](r8),[[pc]](r10)
	and.w	/12345678h(r8),/23456789h(r10)
	and.w	[/12345678h](r8),[/23456789h](r10)
	and.w	#2,r0
	and.w	#200,r0

	expect	1100
	andbsu.b @[r7],#3,@[r9]
	endexpect
	andbsu	@[r7],#3,@[r9]
	andbsu	@[r7+],#20,@[r9+]
	andbsu	@[-r7],r11,@[-r9]
	andbsu	10@[r7],#3,10000@[r9]
	andbsu	@[10[r7]],#20,@[10000[r9]]
	andbsu	20@[10[r7]],r11,20000@[10000[r9]]
	andbsu	@/12345678h,#3,@/23456789h
	andbsu	@[/12345678h],#20,@[/23456789h]
	andbsu	r8@[r7],r11,r10@[r9]
	andbsu	r8@10[r7],#3,r10@10000[r9]
	andbsu	r8@[pc],#20,r10@[pc]
	andbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	andbsu	r8@[[pc]],#3,r10@[[pc]]
	andbsu	r8@/12345678h,#20,r10@/23456789h
	andbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	andbsd.b @[r7],#3,@[r9]
	endexpect
	andbsd	@[r7],#3,@[r9]
	andbsd	@[r7+],#20,@[r9+]
	andbsd	@[-r7],r11,@[-r9]
	andbsd	10@[r7],#3,10000@[r9]
	andbsd	@[10[r7]],#20,@[10000[r9]]
	andbsd	20@[10[r7]],r11,20000@[10000[r9]]
	andbsd	@/12345678h,#3,@/23456789h
	andbsd	@[/12345678h],#20,@[/23456789h]
	andbsd	r8@[r7],r11,r10@[r9]
	andbsd	r8@10[r7],#3,r10@10000[r9]
	andbsd	r8@[pc],#20,r10@[pc]
	andbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	andbsd	r8@[[pc]],#3,r10@[[pc]]
	andbsd	r8@/12345678h,#20,r10@/23456789h
	andbsd	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	andnbsu.b @[r7],#3,@[r9]
	endexpect
	andnbsu	@[r7],#3,@[r9]
	andnbsu	@[r7+],#20,@[r9+]
	andnbsu	@[-r7],r11,@[-r9]
	andnbsu	10@[r7],#3,10000@[r9]
	andnbsu	@[10[r7]],#20,@[10000[r9]]
	andnbsu	20@[10[r7]],r11,20000@[10000[r9]]
	andnbsu	@/12345678h,#3,@/23456789h
	andnbsu	@[/12345678h],#20,@[/23456789h]
	andnbsu	r8@[r7],r11,r10@[r9]
	andnbsu	r8@10[r7],#3,r10@10000[r9]
	andnbsu	r8@[pc],#20,r10@[pc]
	andnbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	andnbsu	r8@[[pc]],#3,r10@[[pc]]
	andnbsu	r8@/12345678h,#20,r10@/23456789h
	andnbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	andnbsd.b @[r7],#3,@[r9]
	endexpect
	andnbsd	@[r7],#3,@[r9]
	andnbsd	@[r7+],#20,@[r9+]
	andnbsd	@[-r7],r11,@[-r9]
	andnbsd	10@[r7],#3,10000@[r9]
	andnbsd	@[10[r7]],#20,@[10000[r9]]
	andnbsd	20@[10[r7]],r11,20000@[10000[r9]]
	andnbsd	@/12345678h,#3,@/23456789h
	andnbsd	@[/12345678h],#20,@[/23456789h]
	andnbsd	r8@[r7],r11,r10@[r9]
	andnbsd	r8@10[r7],#3,r10@10000[r9]
	andnbsd	r8@[pc],#20,r10@[pc]
	andnbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	andnbsd	r8@[[pc]],#3,r10@[[pc]]
	andnbsd	r8@/12345678h,#20,r10@/23456789h
	andnbsd	r8@[/12345678h],r11,r10@[/23456789h]

	irp		op,bgt,bge,blt,ble,bh,bnl,bl,bnh,be,bne,bv,bnv,bn,bp,bc,bnc,bz,bnz,br
	op		$+10
	op		$-10
	op		$+1000
	op		$-1000
	op.s		$+10
	op.s		$-10
	expect		1370,1370
	op.s		$+1000
	op.s		$-1000
	endexpect
	op.l		$+10
	op.l		$-10
	op.l		$+1000
	op.l		$-1000
	op.b		$+10
	op.b		$-10
	expect		1370,1370
	op.b		$+1000
	op.b		$-1000
	endexpect
	op.h		$+10
	op.h		$-10
	op.h		$+1000
	op.h		$-1000
	expect		1130,1130,1130,1130
	op.w		$+10
	op.w		$-10
	op.w		$+1000
	op.w		$-1000
	endexpect
	endm

	brk

	brkv

	bsr		$+10
	bsr		$-10
	bsr		$+1000
	bsr		$-1000
	expect		1130,1130,1130,1130
	bsr.s		$+10
	bsr.s		$-10
	bsr.s		$+1000
	bsr.s		$-1000
	endexpect
	bsr.l		$+10
	bsr.l		$-10
	bsr.l		$+1000
	bsr.l		$-1000
	expect		1130,1130,1130,1130
	bsr.b		$+10
	bsr.b		$-10
	bsr.b		$+1000
	bsr.b		$-1000
	endexpect
	bsr.h		$+10
	bsr.h		$-10
	bsr.h		$+1000
	bsr.h		$-1000
	expect		1130,1130,1130,1130
	bsr.w		$+10
	bsr.w		$-10
	bsr.w		$+1000
	bsr.w		$-1000
	endexpect

	expect	1100,1350
	call.w	r7,r9
	call	r7,r9
	endexpect
	call	[r7],[r9]
	call	[r7+],[r9+]
	call	[-r7],[-r9]
	call	10[r7],10000[r9]
	call	[10[r7]],[10000[r9]]
	call	20[10[r7]],20000[10000[r9]]
	call	/12345678h,/23456789h
	call	[/12345678h],[/23456789h]
	call	[r7](r8),[r9](r10)
	call	10[r7](r8),10000[r9](r10)
	call	[pc](r8),[pc](r10)
	call	[10[r7]](r8),[10000[r9]](r10)
	call	[[pc]](r8),[[pc]](r10)
	call	/12345678h(r8),/23456789h(r10)
	call	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	call	#2,r0
	call	#200,r0
	endexpect

	caxi	r7,r9
	caxi.w	r7,[r9]
	caxi	r7,[r9+]
	caxi.w	r7,[-r9]
	caxi	r7,10000[r9]
	caxi.w	r7,[10000[r9]]
	caxi	r7,20000[10000[r9]]
	caxi.w	r7,/23456789h
	caxi	r7,[/23456789h]
	caxi.w	r7,[r9](r10)
	caxi	r7,10000[r9](r10)
	caxi.w	r7,[pc](r10)
	caxi	r7,[10000[r9]](r10)
	caxi.w	r7,[[pc]](r10)
	caxi	r7,/23456789h(r10)
	caxi.w	r7,[/23456789h](r10)
	expect	1350,1350,1350,1350
	caxi	r7,#2
	caxi.w	r7,#200
	caxi	[r7],r9
	caxi.w	[pc],r9
	endexpect

	expect	1100
	chkar.w	r7,r9
	endexpect
	chkar	[r7],[r9]
	chkar	[r7+],[r9+]
	chkar	[-r7],[-r9]
	chkar	10[r7],10000[r9]
	chkar	[10[r7]],[10000[r9]]
	chkar	20[10[r7]],20000[10000[r9]]
	chkar	/12345678h,/23456789h
	chkar	[/12345678h],[/23456789h]
	chkar	[r7](r8),[r9](r10)
	chkar	10[r7](r8),10000[r9](r10)
	chkar	[pc](r8),[pc](r10)
	chkar	[10[r7]](r8),[10000[r9]](r10)
	chkar	[[pc]](r8),[[pc]](r10)
	chkar	/12345678h(r8),/23456789h(r10)
	chkar	[/12345678h](r8),[/23456789h](r10)
	chkar	[r7],#2

	expect	1100
	chkaw.w	r7,r9
	endexpect
	chkaw	[r7],[r9]
	chkaw	[r7+],[r9+]
	chkaw	[-r7],[-r9]
	chkaw	10[r7],10000[r9]
	chkaw	[10[r7]],[10000[r9]]
	chkaw	20[10[r7]],20000[10000[r9]]
	chkaw	/12345678h,/23456789h
	chkaw	[/12345678h],[/23456789h]
	chkaw	[r7](r8),[r9](r10)
	chkaw	10[r7](r8),10000[r9](r10)
	chkaw	[pc](r8),[pc](r10)
	chkaw	[10[r7]](r8),[10000[r9]](r10)
	chkaw	[[pc]](r8),[[pc]](r10)
	chkaw	/12345678h(r8),/23456789h(r10)
	chkaw	[/12345678h](r8),[/23456789h](r10)
	chkaw	[r7],#2

	expect	1100
	chkae.w	r7,r9
	endexpect
	chkae	[r7],[r9]
	chkae	[r7+],[r9+]
	chkae	[-r7],[-r9]
	chkae	10[r7],10000[r9]
	chkae	[10[r7]],[10000[r9]]
	chkae	20[10[r7]],20000[10000[r9]]
	chkae	/12345678h,/23456789h
	chkae	[/12345678h],[/23456789h]
	chkae	[r7](r8),[r9](r10)
	chkae	10[r7](r8),10000[r9](r10)
	chkae	[pc](r8),[pc](r10)
	chkae	[10[r7]](r8),[10000[r9]](r10)
	chkae	[[pc]](r8),[[pc]](r10)
	chkae	/12345678h(r8),/23456789h(r10)
	chkae	[/12345678h](r8),[/23456789h](r10)
	chkae	[r7],#2

	expect	1100
	chlvl.b	#1,#55
	endexpect
	chlvl	r7,r9
	chlvl	r7,[r9]
	chlvl	[r7],r9
	chlvl	[r7],[r9]
	chlvl	[r7+],[r9+]
	chlvl	[-r7],[-r9]
	chlvl	10[r7],10000[r9]
	chlvl	[10[r7]],[10000[r9]]
	chlvl	20[10[r7]],20000[10000[r9]]
	chlvl	/12345678h,/23456789h
	chlvl	[/12345678h],[/23456789h]
	chlvl	[r7](r8),[r9](r10)
	chlvl	10[r7](r8),10000[r9](r10)
	chlvl	[pc](r8),[pc](r10)
	chlvl	[10[r7]](r8),[10000[r9]](r10)
	chlvl	[[pc]](r8),[[pc]](r10)
	chlvl	/12345678h(r8),/23456789h(r10)
	chlvl	[/12345678h](r8),[/23456789h](r10)
	chlvl	#3,#2
	chlvl	#3,#200

	expect	1100,1350
	clr1.b	r7,#55
	clr1	r7,#55
	endexpect
	clr1	r7,r9
	clr1	r7,[r9]
	clr1	[r7],r9
	clr1	[r7],[r9]
	clr1	[r7+],[r9+]
	clr1	[-r7],[-r9]
	clr1	10[r7],10000[r9]
	clr1	[10[r7]],[10000[r9]]
	clr1	20[10[r7]],20000[10000[r9]]
	clr1	/12345678h,/23456789h
	clr1	[/12345678h],[/23456789h]
	clr1	[r7](r8),[r9](r10)
	clr1	10[r7](r8),10000[r9](r10)
	clr1	[pc](r8),[pc](r10)
	clr1	[10[r7]](r8),[10000[r9]](r10)
	clr1	[[pc]](r8),[[pc]](r10)
	clr1	/12345678h(r8),/23456789h(r10)
	clr1	[/12345678h](r8),[/23456789h](r10)
	clr1	#3,r9
	clr1	#30,r9

	clrtlb.p	r9
	clrtlb		[r9]
	clrtlb.p	[r9+]
	clrtlb		[-r9]
	clrtlb.p	10000[r9]
	clrtlb		[10000[r9]]
	clrtlb.p	20000[10000[r9]]
	clrtlb		/23456789h
	clrtlb.p	[/23456789h]
	clrtlb		[r9](r10)
	clrtlb.p	10000[r9](r10)
	clrtlb		[pc](r10)
	clrtlb.p	[10000[r9]](r10)
	clrtlb		[[pc]](r10)
	clrtlb.p	/23456789h(r10)
	clrtlb		[/23456789h](r10)
	clrtlb.p	#2
	clrtlb		#200

	clrtlba

	cmp.b	r7,r9
	cmp.b	[r7],[r9]
	cmp.b	[r7+],[r9+]
	cmp.b	[-r7],[-r9]
	cmp.b	10[r7],10000[r9]
	cmp.b	[10[r7]],[10000[r9]]
	cmp.b	20[10[r7]],20000[10000[r9]]
	cmp.b	/12345678h,/23456789h
	cmp.b	[/12345678h],[/23456789h]
	cmp.b	[r7](r8),[r9](r10)
	cmp.b	10[r7](r8),10000[r9](r10)
	cmp.b	[pc](r8),[pc](r10)
	cmp.b	[10[r7]](r8),[10000[r9]](r10)
	cmp.b	[[pc]](r8),[[pc]](r10)
	cmp.b	/12345678h(r8),/23456789h(r10)
	cmp.b	[/12345678h](r8),[/23456789h](r10)
	cmp.b	#2,r0
	cmp.b	#200,r0
	cmp.b	#2,#4
	cmp.b	#200,#100

	cmp.h	r7,r9
	cmp.h	[r7],[r9]
	cmp.h	[r7+],[r9+]
	cmp.h	[-r7],[-r9]
	cmp.h	10[r7],10000[r9]
	cmp.h	[10[r7]],[10000[r9]]
	cmp.h	20[10[r7]],20000[10000[r9]]
	cmp.h	/12345678h,/23456789h
	cmp.h	[/12345678h],[/23456789h]
	cmp.h	[r7](r8),[r9](r10)
	cmp.h	10[r7](r8),10000[r9](r10)
	cmp.h	[pc](r8),[pc](r10)
	cmp.h	[10[r7]](r8),[10000[r9]](r10)
	cmp.h	[[pc]](r8),[[pc]](r10)
	cmp.h	/12345678h(r8),/23456789h(r10)
	cmp.h	[/12345678h](r8),[/23456789h](r10)
	cmp.h	#2,r0
	cmp.h	#200,r0
	cmp.h	#2,#4
	cmp.h	#200,#400

	cmp.w	r7,r9
	cmp.w	[r7],[r9]
	cmp.w	[r7+],[r9+]
	cmp.w	[-r7],[-r9]
	cmp.w	10[r7],10000[r9]
	cmp.w	[10[r7]],[10000[r9]]
	cmp.w	20[10[r7]],20000[10000[r9]]
	cmp.w	/12345678h,/23456789h
	cmp.w	[/12345678h],[/23456789h]
	cmp.w	[r7](r8),[r9](r10)
	cmp.w	10[r7](r8),10000[r9](r10)
	cmp.w	[pc](r8),[pc](r10)
	cmp.w	[10[r7]](r8),[10000[r9]](r10)
	cmp.w	[[pc]](r8),[[pc]](r10)
	cmp.w	/12345678h(r8),/23456789h(r10)
	cmp.w	[/12345678h](r8),[/23456789h](r10)
	cmp.w	#2,r0
	cmp.w	#200,r0
	cmp.w	#2,#4
	cmp.w	#200,#400

	expect	1100,1350,1350
	cmpbfs.w @r7,#3,r9
	cmpbfs	@r7,#11,r9
	cmpbfs	#5,r20,r9
	endexpect
	cmpbfs	@[r7],#3,[r9]
	cmpbfs	@[r7+],#11,[r9+]
	cmpbfs	@[-r7],r20,[-r9]
	cmpbfs	10@[r7],#3,10000[r9]
	cmpbfs	@[10[r7]],#11,[10000[r9]]
	cmpbfs	20@[10[r7]],r20,20000[10000[r9]]
	cmpbfs	@/12345678h,#3,/23456789h
	cmpbfs	@[/12345678h],#11,[/23456789h]
	cmpbfs	r8@[r7],r20,[r9](r10)
	cmpbfs	r8@10[r7],#3,10000[r9](r10)
	cmpbfs	r8@[pc],#11,[pc](r10)
	cmpbfs	r8@[10[r7]],r20,[10000[r9]](r10)
	cmpbfs	r8@[[pc]],#3,[[pc]](r10)
	cmpbfs	r8@/12345678h,#11,/23456789h(r10)
	cmpbfs	r8@[/12345678h],r20,[/23456789h](r10)
	cmpbfs	@[r7],#3,#2
	cmpbfs	@[r7],#11,#200

	expect	1100,1350,1350
	cmpbfz.w @r7,#3,r9
	cmpbfz	@r7,#11,r9
	cmpbfz	#5,r20,r9
	endexpect
	cmpbfz	@[r7],#3,[r9]
	cmpbfz	@[r7+],#11,[r9+]
	cmpbfz	@[-r7],r20,[-r9]
	cmpbfz	10@[r7],#3,10000[r9]
	cmpbfz	@[10[r7]],#11,[10000[r9]]
	cmpbfz	20@[10[r7]],r20,20000[10000[r9]]
	cmpbfz	@/12345678h,#3,/23456789h
	cmpbfz	@[/12345678h],#11,[/23456789h]
	cmpbfz	r8@[r7],r20,[r9](r10)
	cmpbfz	r8@10[r7],#3,10000[r9](r10)
	cmpbfz	r8@[pc],#11,[pc](r10)
	cmpbfz	r8@[10[r7]],r20,[10000[r9]](r10)
	cmpbfz	r8@[[pc]],#3,[[pc]](r10)
	cmpbfz	r8@/12345678h,#11,/23456789h(r10)
	cmpbfz	r8@[/12345678h],r20,[/23456789h](r10)
	cmpbfz	@[r7],#3,#2
	cmpbfz	@[r7],#11,#200

	expect	1100,1350,1350
	cmpbfl.w @r7,#3,r9
	cmpbfl	@r7,#11,r9
	cmpbfl	#5,r20,r9
	endexpect
	cmpbfl	@[r7],#3,[r9]
	cmpbfl	@[r7+],#11,[r9+]
	cmpbfl	@[-r7],r20,[-r9]
	cmpbfl	10@[r7],#3,10000[r9]
	cmpbfl	@[10[r7]],#11,[10000[r9]]
	cmpbfl	20@[10[r7]],r20,20000[10000[r9]]
	cmpbfl	@/12345678h,#3,/23456789h
	cmpbfl	@[/12345678h],#11,[/23456789h]
	cmpbfl	r8@[r7],r20,[r9](r10)
	cmpbfl	r8@10[r7],#3,10000[r9](r10)
	cmpbfl	r8@[pc],#11,[pc](r10)
	cmpbfl	r8@[10[r7]],r20,[10000[r9]](r10)
	cmpbfl	r8@[[pc]],#3,[[pc]](r10)
	cmpbfl	r8@/12345678h,#11,/23456789h(r10)
	cmpbfl	r8@[/12345678h],r20,[/23456789h](r10)
	cmpbfl	@[r7],#3,#2
	cmpbfl	@[r7],#11,#200

	expect	1350,1350
	cmpc.b	r7,#3,[r9],#12
	cmpc	/5,r20,#70,r24
	endexpect
	cmpc.b	[r7],#3,[r9],#12
	cmpc	[-r7],r20,[-r9],r24
	cmpc.b	10[r7],#3,10000[r9],#12
	cmpc	20[10[r7]],r20,20000[10000[r9]],r24
	cmpc.b	/12345678h,#3,/23456789h,#12
	cmpc	[r7](r8),r20,[r9](r10),r24
	cmpc.b	10[r7](r8),#3,10000[r9](r10),#12
	cmpc	[10[r7]](r8),r20,[10000[r9]](r10),r24
	cmpc.b	[[pc]](r8),#3,[[pc]](r10),#12
	cmpc	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	cmpc.h	r7,#3,[r9],#12
	cmpc.h	/5,r20,#70,r24
	endexpect
	cmpc.h	[r7],#3,[r9],#12
	cmpc.h	[-r7],r20,[-r9],r24
	cmpc.h	10[r7],#3,10000[r9],#12
	cmpc.h	20[10[r7]],r20,20000[10000[r9]],r24
	cmpc.h	/12345678h,#3,/23456789h,#12
	cmpc.h	[r7](r8),r20,[r9](r10),r24
	cmpc.h	10[r7](r8),#3,10000[r9](r10),#12
	cmpc.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	cmpc.h	[[pc]](r8),#3,[[pc]](r10),#12
	cmpc.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	cmpcf.b	r7,#3,[r9],#12
	cmpcf	/5,r20,#70,r24
	endexpect
	cmpcf.b	[r7],#3,[r9],#12
	cmpcf	[-r7],r20,[-r9],r24
	cmpcf.b	10[r7],#3,10000[r9],#12
	cmpcf	20[10[r7]],r20,20000[10000[r9]],r24
	cmpcf.b	/12345678h,#3,/23456789h,#12
	cmpcf	[r7](r8),r20,[r9](r10),r24
	cmpcf.b	10[r7](r8),#3,10000[r9](r10),#12
	cmpcf	[10[r7]](r8),r20,[10000[r9]](r10),r24
	cmpcf.b	[[pc]](r8),#3,[[pc]](r10),#12
	cmpcf	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	cmpcf.h	r7,#3,[r9],#12
	cmpcf.h	/5,r20,#70,r24
	endexpect
	cmpcf.h	[r7],#3,[r9],#12
	cmpcf.h	[-r7],r20,[-r9],r24
	cmpcf.h	10[r7],#3,10000[r9],#12
	cmpcf.h	20[10[r7]],r20,20000[10000[r9]],r24
	cmpcf.h	/12345678h,#3,/23456789h,#12
	cmpcf.h	[r7](r8),r20,[r9](r10),r24
	cmpcf.h	10[r7](r8),#3,10000[r9](r10),#12
	cmpcf.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	cmpcf.h	[[pc]](r8),#3,[[pc]](r10),#12
	cmpcf.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	cmpcs.b	r7,#3,[r9],#12
	cmpcs	/5,r20,#70,r24
	endexpect
	cmpcs.b	[r7],#3,[r9],#12
	cmpcs	[-r7],r20,[-r9],r24
	cmpcs.b	10[r7],#3,10000[r9],#12
	cmpcs	20[10[r7]],r20,20000[10000[r9]],r24
	cmpcs.b	/12345678h,#3,/23456789h,#12
	cmpcs	[r7](r8),r20,[r9](r10),r24
	cmpcs.b	10[r7](r8),#3,10000[r9](r10),#12
	cmpcs	[10[r7]](r8),r20,[10000[r9]](r10),r24
	cmpcs.b	[[pc]](r8),#3,[[pc]](r10),#12
	cmpcs	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	cmpcs.h	r7,#3,[r9],#12
	cmpcs.h	/5,r20,#70,r24
	endexpect
	cmpcs.h	[r7],#3,[r9],#12
	cmpcs.h	[-r7],r20,[-r9],r24
	cmpcs.h	10[r7],#3,10000[r9],#12
	cmpcs.h	20[10[r7]],r20,20000[10000[r9]],r24
	cmpcs.h	/12345678h,#3,/23456789h,#12
	cmpcs.h	[r7](r8),r20,[r9](r10),r24
	cmpcs.h	10[r7](r8),#3,10000[r9](r10),#12
	cmpcs.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	cmpcs.h	[[pc]](r8),#3,[[pc]](r10),#12
	cmpcs.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	cmpf.s	r7,r9
	cmpf.s	[r7],[r9]
	cmpf.s	[r7+],[r9+]
	cmpf.s	[-r7],[-r9]
	cmpf.s	10[r7],10000[r9]
	cmpf.s	[10[r7]],[10000[r9]]
	cmpf.s	20[10[r7]],20000[10000[r9]]
	cmpf.s	/12345678h,/23456789h
	cmpf.s	[/12345678h],[/23456789h]
	cmpf.s	[r7](r8),[r9](r10)
	cmpf.s	10[r7](r8),10000[r9](r10)
	cmpf.s	[pc](r8),[pc](r10)
	cmpf.s	[10[r7]](r8),[10000[r9]](r10)
	cmpf.s	[[pc]](r8),[[pc]](r10)
	cmpf.s	/12345678h(r8),/23456789h(r10)
	cmpf.s	[/12345678h](r8),[/23456789h](r10)

	cmpf.l	r7,r9
	cmpf.l	[r7],[r9]
	cmpf.l	[r7+],[r9+]
	cmpf.l	[-r7],[-r9]
	cmpf.l	10[r7],10000[r9]
	cmpf.l	[10[r7]],[10000[r9]]
	cmpf.l	20[10[r7]],20000[10000[r9]]
	cmpf.l	/12345678h,/23456789h
	cmpf.l	[/12345678h],[/23456789h]
	cmpf.l	[r7](r8),[r9](r10)
	cmpf.l	10[r7](r8),10000[r9](r10)
	cmpf.l	[pc](r8),[pc](r10)
	cmpf.l	[10[r7]](r8),[10000[r9]](r10)
	cmpf.l	[[pc]](r8),[[pc]](r10)
	cmpf.l	/12345678h(r8),/23456789h(r10)
	cmpf.l	[/12345678h](r8),[/23456789h](r10)

	expect	1107,1107
	cvt	r2,r3
	cvt.w	r2,r3
	endexpect

	cvt.sl	r7,r9
	cvt.sl	[r7],[r9]
	cvt.sl	[r7+],[r9+]
	cvt.sl	[-r7],[-r9]
	cvt.sl	10[r7],10000[r9]
	cvt.sl	[10[r7]],[10000[r9]]
	cvt.sl	20[10[r7]],20000[10000[r9]]
	cvt.sl	/12345678h,/23456789h
	cvt.sl	[/12345678h],[/23456789h]
	cvt.sl	[r7](r8),[r9](r10)
	cvt.sl	10[r7](r8),10000[r9](r10)
	cvt.sl	[pc](r8),[pc](r10)
	cvt.sl	[10[r7]](r8),[10000[r9]](r10)
	cvt.sl	[[pc]](r8),[[pc]](r10)
	cvt.sl	/12345678h(r8),/23456789h(r10)
	cvt.sl	[/12345678h](r8),[/23456789h](r10)
	expect	1350
	cvt.sl	#0,r9
	endexpect

	cvt.ls	r7,r9
	cvt.ls	[r7],[r9]
	cvt.ls	[r7+],[r9+]
	cvt.ls	[-r7],[-r9]
	cvt.ls	10[r7],10000[r9]
	cvt.ls	[10[r7]],[10000[r9]]
	cvt.ls	20[10[r7]],20000[10000[r9]]
	cvt.ls	/12345678h,/23456789h
	cvt.ls	[/12345678h],[/23456789h]
	cvt.ls	[r7](r8),[r9](r10)
	cvt.ls	10[r7](r8),10000[r9](r10)
	cvt.ls	[pc](r8),[pc](r10)
	cvt.ls	[10[r7]](r8),[10000[r9]](r10)
	cvt.ls	[[pc]](r8),[[pc]](r10)
	cvt.ls	/12345678h(r8),/23456789h(r10)
	cvt.ls	[/12345678h](r8),[/23456789h](r10)
	expect	1350
	cvt.ls	#0,r9
	endexpect

	cvt.ws	r7,r9
	cvt.ws	[r7],[r9]
	cvt.ws	[r7+],[r9+]
	cvt.ws	[-r7],[-r9]
	cvt.ws	10[r7],10000[r9]
	cvt.ws	[10[r7]],[10000[r9]]
	cvt.ws	20[10[r7]],20000[10000[r9]]
	cvt.ws	/12345678h,/23456789h
	cvt.ws	[/12345678h],[/23456789h]
	cvt.ws	[r7](r8),[r9](r10)
	cvt.ws	10[r7](r8),10000[r9](r10)
	cvt.ws	[pc](r8),[pc](r10)
	cvt.ws	[10[r7]](r8),[10000[r9]](r10)
	cvt.ws	[[pc]](r8),[[pc]](r10)
	cvt.ws	/12345678h(r8),/23456789h(r10)
	cvt.ws	[/12345678h](r8),[/23456789h](r10)
	expect	1350
	cvt.ws	#0,r9
	endexpect

	cvt.wl	r7,r9
	cvt.wl	[r7],[r9]
	cvt.wl	[r7+],[r9+]
	cvt.wl	[-r7],[-r9]
	cvt.wl	10[r7],10000[r9]
	cvt.wl	[10[r7]],[10000[r9]]
	cvt.wl	20[10[r7]],20000[10000[r9]]
	cvt.wl	/12345678h,/23456789h
	cvt.wl	[/12345678h],[/23456789h]
	cvt.wl	[r7](r8),[r9](r10)
	cvt.wl	10[r7](r8),10000[r9](r10)
	cvt.wl	[pc](r8),[pc](r10)
	cvt.wl	[10[r7]](r8),[10000[r9]](r10)
	cvt.wl	[[pc]](r8),[[pc]](r10)
	cvt.wl	/12345678h(r8),/23456789h(r10)
	cvt.wl	[/12345678h](r8),[/23456789h](r10)
	expect	1350
	cvt.wl	#0,r9
	endexpect

	cvt.sw	r7,r9
	cvt.sw	[r7],[r9]
	cvt.sw	[r7+],[r9+]
	cvt.sw	[-r7],[-r9]
	cvt.sw	10[r7],10000[r9]
	cvt.sw	[10[r7]],[10000[r9]]
	cvt.sw	20[10[r7]],20000[10000[r9]]
	cvt.sw	/12345678h,/23456789h
	cvt.sw	[/12345678h],[/23456789h]
	cvt.sw	[r7](r8),[r9](r10)
	cvt.sw	10[r7](r8),10000[r9](r10)
	cvt.sw	[pc](r8),[pc](r10)
	cvt.sw	[10[r7]](r8),[10000[r9]](r10)
	cvt.sw	[[pc]](r8),[[pc]](r10)
	cvt.sw	/12345678h(r8),/23456789h(r10)
	cvt.sw	[/12345678h](r8),[/23456789h](r10)
	expect	1350
	cvt.sw	#0,r9
	endexpect

	cvt.lw	r7,r9
	cvt.lw	[r7],[r9]
	cvt.lw	[r7+],[r9+]
	cvt.lw	[-r7],[-r9]
	cvt.lw	10[r7],10000[r9]
	cvt.lw	[10[r7]],[10000[r9]]
	cvt.lw	20[10[r7]],20000[10000[r9]]
	cvt.lw	/12345678h,/23456789h
	cvt.lw	[/12345678h],[/23456789h]
	cvt.lw	[r7](r8),[r9](r10)
	cvt.lw	10[r7](r8),10000[r9](r10)
	cvt.lw	[pc](r8),[pc](r10)
	cvt.lw	[10[r7]](r8),[10000[r9]](r10)
	cvt.lw	[[pc]](r8),[[pc]](r10)
	cvt.lw	/12345678h(r8),/23456789h(r10)
	cvt.lw	[/12345678h](r8),[/23456789h](r10)
	expect	1350
	cvt.lw	#0,r9
	endexpect

	expect	1107,1107,1110
	cvtd	r7,r9,#0ffh
	cvtd.bh	r7,r9,#0ffh
	cvtd.pz	r7,r9
	endexpect
	cvtd.pz	r7,r9,0ffh
	cvtd.pz	[r7],[r9],0ffh
	cvtd.pz	[r7+],[r9+],0ffh
	cvtd.pz	[-r7],[-r9],0ffh
	cvtd.pz	10[r7],10000[r9],0ffh
	cvtd.pz	[10[r7]],[10000[r9]],0ffh
	cvtd.pz	20[10[r7]],20000[10000[r9]],0ffh
	cvtd.pz	/12345678h,/23456789h,0ffh
	cvtd.pz	[/12345678h],[/23456789h],0ffh
	cvtd.pz	[r7](r8),[r9](r10),0ffh
	cvtd.pz	10[r7](r8),10000[r9](r10),0ffh
	cvtd.pz	[pc](r8),[pc](r10),0ffh
	cvtd.pz	[10[r7]](r8),[10000[r9]](r10),0ffh
	cvtd.pz	[[pc]](r8),[[pc]](r10),0ffh
	cvtd.pz	/12345678h(r8),/23456789h(r10),0ffh
	cvtd.pz	[/12345678h](r8),[/23456789h](r10),0ffh
	cvtd.pz	#2,r0,0ffh
	cvtd.pz	#200,r0,0ffh

	expect	1107,1107,1110
	cvtd	r7,r9,#0ffh
	cvtd.hb	r7,r9,#0ffh
	cvtd.zp	r7,r9
	endexpect
	cvtd.zp	r7,r9,0ffh
	cvtd.zp	[r7],[r9],0ffh
	cvtd.zp	[r7+],[r9+],0ffh
	cvtd.zp	[-r7],[-r9],0ffh
	cvtd.zp	10[r7],10000[r9],0ffh
	cvtd.zp	[10[r7]],[10000[r9]],0ffh
	cvtd.zp	20[10[r7]],20000[10000[r9]],0ffh
	cvtd.zp	/12345678h,/23456789h,0ffh
	cvtd.zp	[/12345678h],[/23456789h],0ffh
	cvtd.zp	[r7](r8),[r9](r10),0ffh
	cvtd.zp	10[r7](r8),10000[r9](r10),0ffh
	cvtd.zp	[pc](r8),[pc](r10),0ffh
	cvtd.zp	[10[r7]](r8),[10000[r9]](r10),0ffh
	cvtd.zp	[[pc]](r8),[[pc]](r10),0ffh
	cvtd.zp	/12345678h(r8),/23456789h(r10),0ffh
	cvtd.zp	[/12345678h](r8),[/23456789h](r10),0ffh
	cvtd.zp	#2,r0,0ffh
	cvtd.zp	#200,r0,0ffh

	irp		op,dbgt,dbge,dblt,dble,dbh,dbnl,dbl,dbnh,dbe,dbne,dbv,dbnv,dbn,dbp,dbc,dbnc,dbz,dbnz,dbr
	op		r3,$+10
	op		r3,$-10
	op		r12,$+1000
	op		r12,$-1000
	expect		1100,1100,1100,1100
	op.s		r3,$+10
	op.s		r3,$-10
	op.s		r12,$+1000
	op.s		r12,$-1000
	endexpect
	endm

	dec.b	r9
	dec.b	[r9]
	dec.b	[r9+]
	dec.b	[-r9]
	dec.b	10000[r9]
	dec.b	[10000[r9]]
	dec.b	20000[10000[r9]]
	dec.b	/23456789h
	dec.b	[/23456789h]
	dec.b	[r9](r10)
	dec.b	10000[r9](r10)
	dec.b	[pc](r10)
	dec.b	[10000[r9]](r10)
	dec.b	[[pc]](r10)
	dec.b	/23456789h(r10)
	dec.b	[/23456789h](r10)
	expect	1350,1350
	dec.b	#2
	dec.b	#200
	endexpect

	dec.h	r9
	dec.h	[r9]
	dec.h	[r9+]
	dec.h	[-r9]
	dec.h	10000[r9]
	dec.h	[10000[r9]]
	dec.h	20000[10000[r9]]
	dec.h	/23456789h
	dec.h	[/23456789h]
	dec.h	[r9](r10)
	dec.h	10000[r9](r10)
	dec.h	[pc](r10)
	dec.h	[10000[r9]](r10)
	dec.h	[[pc]](r10)
	dec.h	/23456789h(r10)
	dec.h	[/23456789h](r10)
	expect	1350,1350
	dec.h	#2
	dec.h	#200
	endexpect

	dec.w	r9
	dec	[r9]
	dec.w	[r9+]
	dec	[-r9]
	dec.w	10000[r9]
	dec	[10000[r9]]
	dec.w	20000[10000[r9]]
	dec	/23456789h
	dec.w	[/23456789h]
	dec	[r9](r10)
	dec.w	10000[r9](r10)
	dec	[pc](r10)
	dec.w	[10000[r9]](r10)
	dec	[[pc]](r10)
	dec.w	/23456789h(r10)
	dec	[/23456789h](r10)
	expect	1350,1350
	dec.w	#2
	dec	#200
	endexpect

	dispose

	div.b	r7,r9
	div.b	[r7],[r9]
	div.b	[r7+],[r9+]
	div.b	[-r7],[-r9]
	div.b	10[r7],10000[r9]
	div.b	[10[r7]],[10000[r9]]
	div.b	20[10[r7]],20000[10000[r9]]
	div.b	/12345678h,/23456789h
	div.b	[/12345678h],[/23456789h]
	div.b	[r7](r8),[r9](r10)
	div.b	10[r7](r8),10000[r9](r10)
	div.b	[pc](r8),[pc](r10)
	div.b	[10[r7]](r8),[10000[r9]](r10)
	div.b	[[pc]](r8),[[pc]](r10)
	div.b	/12345678h(r8),/23456789h(r10)
	div.b	[/12345678h](r8),[/23456789h](r10)
	div.b	#2,r0
	div.b	#200,r0

	div.h	r7,r9
	div.h	[r7],[r9]
	div.h	[r7+],[r9+]
	div.h	[-r7],[-r9]
	div.h	10[r7],10000[r9]
	div.h	[10[r7]],[10000[r9]]
	div.h	20[10[r7]],20000[10000[r9]]
	div.h	/12345678h,/23456789h
	div.h	[/12345678h],[/23456789h]
	div.h	[r7](r8),[r9](r10)
	div.h	10[r7](r8),10000[r9](r10)
	div.h	[pc](r8),[pc](r10)
	div.h	[10[r7]](r8),[10000[r9]](r10)
	div.h	[[pc]](r8),[[pc]](r10)
	div.h	/12345678h(r8),/23456789h(r10)
	div.h	[/12345678h](r8),[/23456789h](r10)
	div.h	#2,r0
	div.h	#200,r0

	div.w	r7,r9
	div.w	[r7],[r9]
	div.w	[r7+],[r9+]
	div.w	[-r7],[-r9]
	div.w	10[r7],10000[r9]
	div.w	[10[r7]],[10000[r9]]
	div.w	20[10[r7]],20000[10000[r9]]
	div.w	/12345678h,/23456789h
	div.w	[/12345678h],[/23456789h]
	div.w	[r7](r8),[r9](r10)
	div.w	10[r7](r8),10000[r9](r10)
	div.w	[pc](r8),[pc](r10)
	div.w	[10[r7]](r8),[10000[r9]](r10)
	div.w	[[pc]](r8),[[pc]](r10)
	div.w	/12345678h(r8),/23456789h(r10)
	div.w	[/12345678h](r8),[/23456789h](r10)
	div.w	#2,r0
	div.w	#200,r0

	divf.s	r7,r9
	divf.s	[r7],[r9]
	divf.s	[r7+],[r9+]
	divf.s	[-r7],[-r9]
	divf.s	10[r7],10000[r9]
	divf.s	[10[r7]],[10000[r9]]
	divf.s	20[10[r7]],20000[10000[r9]]
	divf.s	/12345678h,/23456789h
	divf.s	[/12345678h],[/23456789h]
	divf.s	[r7](r8),[r9](r10)
	divf.s	10[r7](r8),10000[r9](r10)
	divf.s	[pc](r8),[pc](r10)
	divf.s	[10[r7]](r8),[10000[r9]](r10)
	divf.s	[[pc]](r8),[[pc]](r10)
	divf.s	/12345678h(r8),/23456789h(r10)
	divf.s	[/12345678h](r8),[/23456789h](r10)

	divf.l	r7,r9
	divf.l	[r7],[r9]
	divf.l	[r7+],[r9+]
	divf.l	[-r7],[-r9]
	divf.l	10[r7],10000[r9]
	divf.l	[10[r7]],[10000[r9]]
	divf.l	20[10[r7]],20000[10000[r9]]
	divf.l	/12345678h,/23456789h
	divf.l	[/12345678h],[/23456789h]
	divf.l	[r7](r8),[r9](r10)
	divf.l	10[r7](r8),10000[r9](r10)
	divf.l	[pc](r8),[pc](r10)
	divf.l	[10[r7]](r8),[10000[r9]](r10)
	divf.l	[[pc]](r8),[[pc]](r10)
	divf.l	/12345678h(r8),/23456789h(r10)
	divf.l	[/12345678h](r8),[/23456789h](r10)

	divu.b	r7,r9
	divu.b	[r7],[r9]
	divu.b	[r7+],[r9+]
	divu.b	[-r7],[-r9]
	divu.b	10[r7],10000[r9]
	divu.b	[10[r7]],[10000[r9]]
	divu.b	20[10[r7]],20000[10000[r9]]
	divu.b	/12345678h,/23456789h
	divu.b	[/12345678h],[/23456789h]
	divu.b	[r7](r8),[r9](r10)
	divu.b	10[r7](r8),10000[r9](r10)
	divu.b	[pc](r8),[pc](r10)
	divu.b	[10[r7]](r8),[10000[r9]](r10)
	divu.b	[[pc]](r8),[[pc]](r10)
	divu.b	/12345678h(r8),/23456789h(r10)
	divu.b	[/12345678h](r8),[/23456789h](r10)
	divu.b	#2,r0
	divu.b	#200,r0

	divu.h	r7,r9
	divu.h	[r7],[r9]
	divu.h	[r7+],[r9+]
	divu.h	[-r7],[-r9]
	divu.h	10[r7],10000[r9]
	divu.h	[10[r7]],[10000[r9]]
	divu.h	20[10[r7]],20000[10000[r9]]
	divu.h	/12345678h,/23456789h
	divu.h	[/12345678h],[/23456789h]
	divu.h	[r7](r8),[r9](r10)
	divu.h	10[r7](r8),10000[r9](r10)
	divu.h	[pc](r8),[pc](r10)
	divu.h	[10[r7]](r8),[10000[r9]](r10)
	divu.h	[[pc]](r8),[[pc]](r10)
	divu.h	/12345678h(r8),/23456789h(r10)
	divu.h	[/12345678h](r8),[/23456789h](r10)
	divu.h	#2,r0
	divu.h	#200,r0

	divu.w	r7,r9
	divu.w	[r7],[r9]
	divu.w	[r7+],[r9+]
	divu.w	[-r7],[-r9]
	divu.w	10[r7],10000[r9]
	divu.w	[10[r7]],[10000[r9]]
	divu.w	20[10[r7]],20000[10000[r9]]
	divu.w	/12345678h,/23456789h
	divu.w	[/12345678h],[/23456789h]
	divu.w	[r7](r8),[r9](r10)
	divu.w	10[r7](r8),10000[r9](r10)
	divu.w	[pc](r8),[pc](r10)
	divu.w	[10[r7]](r8),[10000[r9]](r10)
	divu.w	[[pc]](r8),[[pc]](r10)
	divu.w	/12345678h(r8),/23456789h(r10)
	divu.w	[/12345678h](r8),[/23456789h](r10)
	divu.w	#2,r0
	divu.w	#200,r0

	divx	r7,r9
	divx.w	[r7],[r9]
	divx	[r7+],[r9+]
	divx.w	[-r7],[-r9]
	divx	10[r7],10000[r9]
	divx.w	[10[r7]],[10000[r9]]
	divx	20[10[r7]],20000[10000[r9]]
	divx.w	/12345678h,/23456789h
	divx	[/12345678h],[/23456789h]
	divx.w	[r7](r8),[r9](r10)
	divx	10[r7](r8),10000[r9](r10)
	divx.w	[pc](r8),[pc](r10)
	divx	[10[r7]](r8),[10000[r9]](r10)
	divx.w	[[pc]](r8),[[pc]](r10)
	divx	/12345678h(r8),/23456789h(r10)
	divx.w	[/12345678h](r8),[/23456789h](r10)
	divx	#2,r0
	divx.w	#200,r0

	divux	r7,r9
	divux.w	[r7],[r9]
	divux	[r7+],[r9+]
	divux.w	[-r7],[-r9]
	divux	10[r7],10000[r9]
	divux.w	[10[r7]],[10000[r9]]
	divux	20[10[r7]],20000[10000[r9]]
	divux.w	/12345678h,/23456789h
	divux	[/12345678h],[/23456789h]
	divux.w	[r7](r8),[r9](r10)
	divux	10[r7](r8),10000[r9](r10)
	divux.w	[pc](r8),[pc](r10)
	divux	[10[r7]](r8),[10000[r9]](r10)
	divux.w	[[pc]](r8),[[pc]](r10)
	divux	/12345678h(r8),/23456789h(r10)
	divux.w	[/12345678h](r8),[/23456789h](r10)
	divux	#2,r0
	divux.w	#200,r0

	expect	1100,1350,1350
	extbfs.w @r7,#3,r9
	extbfs	@r7,#11,r9
	extbfs	#5,r20,r9
	endexpect
	extbfs	@[r7],#3,[r9]
	extbfs	@[r7+],#11,[r9+]
	extbfs	@[-r7],r20,[-r9]
	extbfs	10@[r7],#3,10000[r9]
	extbfs	@[10[r7]],#11,[10000[r9]]
	extbfs	20@[10[r7]],r20,20000[10000[r9]]
	extbfs	@/12345678h,#3,/23456789h
	extbfs	@[/12345678h],#11,[/23456789h]
	extbfs	r8@[r7],r20,[r9](r10)
	extbfs	r8@10[r7],#3,10000[r9](r10)
	extbfs	r8@[pc],#11,[pc](r10)
	extbfs	r8@[10[r7]],r20,[10000[r9]](r10)
	extbfs	r8@[[pc]],#3,[[pc]](r10)
	extbfs	r8@/12345678h,#11,/23456789h(r10)
	extbfs	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	extbfs	@[r7],#3,#2
	extbfs	@[r7],#11,#200
	endexpect

	expect	1100,1350,1350
	extbfz.w @r7,#3,r9
	extbfz	@r7,#11,r9
	extbfz	#5,r20,r9
	endexpect
	extbfz	@[r7],#3,[r9]
	extbfz	@[r7+],#11,[r9+]
	extbfz	@[-r7],r20,[-r9]
	extbfz	10@[r7],#3,10000[r9]
	extbfz	@[10[r7]],#11,[10000[r9]]
	extbfz	20@[10[r7]],r20,20000[10000[r9]]
	extbfz	@/12345678h,#3,/23456789h
	extbfz	@[/12345678h],#11,[/23456789h]
	extbfz	r8@[r7],r20,[r9](r10)
	extbfz	r8@10[r7],#3,10000[r9](r10)
	extbfz	r8@[pc],#11,[pc](r10)
	extbfz	r8@[10[r7]],r20,[10000[r9]](r10)
	extbfz	r8@[[pc]],#3,[[pc]](r10)
	extbfz	r8@/12345678h,#11,/23456789h(r10)
	extbfz	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	extbfz	@[r7],#3,#2
	extbfz	@[r7],#11,#200
	endexpect

	expect	1100,1350,1350
	extbfl.w @r7,#3,r9
	extbfl	@r7,#11,r9
	extbfl	#5,r20,r9
	endexpect
	extbfl	@[r7],#3,[r9]
	extbfl	@[r7+],#11,[r9+]
	extbfl	@[-r7],r20,[-r9]
	extbfl	10@[r7],#3,10000[r9]
	extbfl	@[10[r7]],#11,[10000[r9]]
	extbfl	20@[10[r7]],r20,20000[10000[r9]]
	extbfl	@/12345678h,#3,/23456789h
	extbfl	@[/12345678h],#11,[/23456789h]
	extbfl	r8@[r7],r20,[r9](r10)
	extbfl	r8@10[r7],#3,10000[r9](r10)
	extbfl	r8@[pc],#11,[pc](r10)
	extbfl	r8@[10[r7]],r20,[10000[r9]](r10)
	extbfl	r8@[[pc]],#3,[[pc]](r10)
	extbfl	r8@/12345678h,#11,/23456789h(r10)
	extbfl	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	extbfl	@[r7],#3,#2
	extbfl	@[r7],#11,#200
	endexpect

	expect	1100,1350
	getate.b	r7,#55
	getate	r7,#55
	endexpect
	getate	r7,r9
	getate	r7,[r9]
	getate	[r7],r9
	getate	[r7],[r9]
	getate	[r7+],[r9+]
	getate	[-r7],[-r9]
	getate	10[r7],10000[r9]
	getate	[10[r7]],[10000[r9]]
	getate	20[10[r7]],20000[10000[r9]]
	getate	/12345678h,/23456789h
	getate	[/12345678h],[/23456789h]
	getate	[r7](r8),[r9](r10)
	getate	10[r7](r8),10000[r9](r10)
	getate	[pc](r8),[pc](r10)
	getate	[10[r7]](r8),[10000[r9]](r10)
	getate	[[pc]](r8),[[pc]](r10)
	getate	/12345678h(r8),/23456789h(r10)
	getate	[/12345678h](r8),[/23456789h](r10)
	getate	#3,r9
	getate	#3000,r9

	getpsw.w	r9
	getpsw		[r9]
	getpsw.w	[r9+]
	getpsw		[-r9]
	getpsw.w	10000[r9]
	getpsw		[10000[r9]]
	getpsw.w	20000[10000[r9]]
	getpsw		/23456789h
	getpsw.w	[/23456789h]
	getpsw		[r9](r10)
	getpsw.w	10000[r9](r10)
	getpsw		[pc](r10)
	getpsw.w	[10000[r9]](r10)
	getpsw		[[pc]](r10)
	getpsw.w	/23456789h(r10)
	getpsw		[/23456789h](r10)
	expect		1350,1350
	getpsw.w	#2
	getpsw		#200
	endexpect

	expect	1100,1350
	getpte.b	r7,#55
	getpte	r7,#55
	endexpect
	getpte	r7,r9
	getpte	r7,[r9]
	getpte	[r7],r9
	getpte	[r7],[r9]
	getpte	[r7+],[r9+]
	getpte	[-r7],[-r9]
	getpte	10[r7],10000[r9]
	getpte	[10[r7]],[10000[r9]]
	getpte	20[10[r7]],20000[10000[r9]]
	getpte	/12345678h,/23456789h
	getpte	[/12345678h],[/23456789h]
	getpte	[r7](r8),[r9](r10)
	getpte	10[r7](r8),10000[r9](r10)
	getpte	[pc](r8),[pc](r10)
	getpte	[10[r7]](r8),[10000[r9]](r10)
	getpte	[[pc]](r8),[[pc]](r10)
	getpte	/12345678h(r8),/23456789h(r10)
	getpte	[/12345678h](r8),[/23456789h](r10)
	getpte	#3,r9
	getpte	#3000,r9

	expect	1100,1350
	getra.b	r7,#55
	getra	r7,#55
	endexpect
	getra	r7,r9
	getra	r7,[r9]
	getra	[r7],r9
	getra	[r7],[r9]
	getra	[r7+],[r9+]
	getra	[-r7],[-r9]
	getra	10[r7],10000[r9]
	getra	[10[r7]],[10000[r9]]
	getra	20[10[r7]],20000[10000[r9]]
	getra	/12345678h,/23456789h
	getra	[/12345678h],[/23456789h]
	getra	[r7](r8),[r9](r10)
	getra	10[r7](r8),10000[r9](r10)
	getra	[pc](r8),[pc](r10)
	getra	[10[r7]](r8),[10000[r9]](r10)
	getra	[[pc]](r8),[[pc]](r10)
	getra	/12345678h(r8),/23456789h(r10)
	getra	[/12345678h](r8),[/23456789h](r10)
	getra	#3,r9
	getra	#3000,r9

	halt

	expect	1350
	in.b	r7,r9
	endexpect
	in.b	/port1,r9
	in.b	[r7],[r9]
	in.b	[r7+],[r9+]
	in.b	[-r7],[-r9]
	in.b	10[r7],10000[r9]
	in.b	[10[r7]],[10000[r9]]
	in.b	20[10[r7]],20000[10000[r9]]
	in.b	/12345678h,/23456789h
	in.b	[/12345678h],[/23456789h]
	in.b	[r7](r8),[r9](r10)
	in.b	10[r7](r8),10000[r9](r10)
	in.b	[pc](r8),[pc](r10)
	in.b	[10[r7]](r8),[10000[r9]](r10)
	in.b	[[pc]](r8),[[pc]](r10)
	in.b	/12345678h(r8),/23456789h(r10)
	in.b	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	in.b	#2,r0
	in.b	#200,r0
	endexpect

	expect	1350
	in.h	r7,r9
	endexpect
	in.h	/port1,r9
	in.h	[r7],[r9]
	in.h	[r7+],[r9+]
	in.h	[-r7],[-r9]
	in.h	10[r7],10000[r9]
	in.h	[10[r7]],[10000[r9]]
	in.h	20[10[r7]],20000[10000[r9]]
	in.h	/12345678h,/23456789h
	in.h	[/12345678h],[/23456789h]
	in.h	[r7](r8),[r9](r10)
	in.h	10[r7](r8),10000[r9](r10)
	in.h	[pc](r8),[pc](r10)
	in.h	[10[r7]](r8),[10000[r9]](r10)
	in.h	[[pc]](r8),[[pc]](r10)
	in.h	/12345678h(r8),/23456789h(r10)
	in.h	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	in.h	#2,r0
	in.h	#200,r0
	endexpect

	expect	1350
	in.w	r7,r9
	endexpect
	in.w	/port1,r9
	in.w	[r7],[r9]
	in.w	[r7+],[r9+]
	in.w	[-r7],[-r9]
	in.w	10[r7],10000[r9]
	in.w	[10[r7]],[10000[r9]]
	in.w	20[10[r7]],20000[10000[r9]]
	in.w	/12345678h,/23456789h
	in.w	[/12345678h],[/23456789h]
	in.w	[r7](r8),[r9](r10)
	in.w	10[r7](r8),10000[r9](r10)
	in.w	[pc](r8),[pc](r10)
	in.w	[10[r7]](r8),[10000[r9]](r10)
	in.w	[[pc]](r8),[[pc]](r10)
	in.w	/12345678h(r8),/23456789h(r10)
	in.w	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	in.w	#2,r0
	in.w	#200,r0
	endexpect

	inc.b	r9
	inc.b	[r9]
	inc.b	[r9+]
	inc.b	[-r9]
	inc.b	10000[r9]
	inc.b	[10000[r9]]
	inc.b	20000[10000[r9]]
	inc.b	/23456789h
	inc.b	[/23456789h]
	inc.b	[r9](r10)
	inc.b	10000[r9](r10)
	inc.b	[pc](r10)
	inc.b	[10000[r9]](r10)
	inc.b	[[pc]](r10)
	inc.b	/23456789h(r10)
	inc.b	[/23456789h](r10)
	expect	1350,1350
	inc.b	#2
	inc.b	#200
	endexpect

	inc.h	r9
	inc.h	[r9]
	inc.h	[r9+]
	inc.h	[-r9]
	inc.h	10000[r9]
	inc.h	[10000[r9]]
	inc.h	20000[10000[r9]]
	inc.h	/23456789h
	inc.h	[/23456789h]
	inc.h	[r9](r10)
	inc.h	10000[r9](r10)
	inc.h	[pc](r10)
	inc.h	[10000[r9]](r10)
	inc.h	[[pc]](r10)
	inc.h	/23456789h(r10)
	inc.h	[/23456789h](r10)
	expect	1350,1350
	inc.h	#2
	inc.h	#200
	endexpect

	inc.w	r9
	inc	[r9]
	inc.w	[r9+]
	inc	[-r9]
	inc.w	10000[r9]
	inc	[10000[r9]]
	inc.w	20000[10000[r9]]
	inc	/23456789h
	inc.w	[/23456789h]
	inc	[r9](r10)
	inc.w	10000[r9](r10)
	inc	[pc](r10)
	inc.w	[10000[r9]](r10)
	inc	[[pc]](r10)
	inc.w	/23456789h(r10)
	inc	[/23456789h](r10)
	expect	1350,1350
	inc.w	#2
	inc	#200
	endexpect

	expect	1100,1350,1350
	insbfr.w r7,@r9,#3
	insbfr	r7,@r9,#11
	insbfr	#5,@r9,r20
	endexpect
	insbfr	[r7],@[r9],#3
	insbfr	[r7+],@[r9+],#11
	insbfr	[-r7],@[-r9],r20
	insbfr	10000[r7],10@[r9],#3
	insbfr	[10[r7]],@[10000[r9]],#11
	insbfr	20[10[r7]],20000@[10000[r9]],r20
	insbfr	/12345678h,@/23456789h,#3
	insbfr	[/12345678h],@[/23456789h],#11
	insbfr	[r7](r8),r10@[r9],r20
	insbfr	10[r7](r8),r10@10000[r9],#3
	insbfr	[pc](r8),r10@[pc],#11
	insbfr	[10[r7]](r8),r10@[10000[r9]],r20
	insbfr	[[pc]](r8),r10@[[pc]],#3
	insbfr	/12345678h(r8),r10@/23456789h,#11
	insbfr	[/12345678h](r8),r10@[/23456789h],r20
	insbfr	#2,@[r7],#3
	insbfr	#200,@[r7],#11

	expect	1100,1350,1350
	insbfl.w r7,@r9,#3
	insbfl	r7,@r9,#11
	insbfl	#5,@r9,r20
	endexpect
	insbfl	[r7],@[r9],#3
	insbfl	[r7+],@[r9+],#11
	insbfl	[-r7],@[-r9],r20
	insbfl	10000[r7],10@[r9],#3
	insbfl	[10[r7]],@[10000[r9]],#11
	insbfl	20[10[r7]],20000@[10000[r9]],r20
	insbfl	/12345678h,@/23456789h,#3
	insbfl	[/12345678h],@[/23456789h],#11
	insbfl	[r7](r8),r10@[r9],r20
	insbfl	10[r7](r8),r10@10000[r9],#3
	insbfl	[pc](r8),r10@[pc],#11
	insbfl	[10[r7]](r8),r10@[10000[r9]],r20
	insbfl	[[pc]](r8),r10@[[pc]],#3
	insbfl	/12345678h(r8),r10@/23456789h,#11
	insbfl	[/12345678h](r8),r10@[/23456789h],r20
	insbfl	#2,@[r7],#3
	insbfl	#200,@[r7],#11

	expect	1100,1350
	jmp.b	r9
	jmp	r9
	endexpect
	jmp	[r9]
	jmp	[r9+]
	jmp	[-r9]
	jmp	10000[r9]
	jmp	[10000[r9]]
	jmp	20000[10000[r9]]
	jmp	/23456789h
	jmp	[/23456789h]
	jmp	[r9](r10)
	jmp	10000[r9](r10)
	jmp	[pc](r10)
	jmp	[10000[r9]](r10)
	jmp	[[pc]](r10)
	jmp	/23456789h(r10)
	jmp	[/23456789h](r10)
	expect	1100,1350
	jmp.b	#2
	jmp	#200
	endexpect

	expect	1100,1350
	jsr.b	r9
	jsr	r9
	endexpect
	jsr	[r9]
	jsr	[r9+]
	jsr	[-r9]
	jsr	10000[r9]
	jsr	[10000[r9]]
	jsr	20000[10000[r9]]
	jsr	/23456789h
	jsr	[/23456789h]
	jsr	[r9](r10)
	jsr	10000[r9](r10)
	jsr	[pc](r10)
	jsr	[10000[r9]](r10)
	jsr	[[pc]](r10)
	jsr	/23456789h(r10)
	jsr	[/23456789h](r10)
	expect	1100,1350
	jsr.b	#2
	jsr	#200
	endexpect

	ldpr.w	r7,r9
	ldpr	[r7],[r9]
	ldpr.w	[r7+],[r9+]
	ldpr	[-r7],[-r9]
	ldpr.w	10[r7],10000[r9]
	ldpr	[10[r7]],[10000[r9]]
	ldpr.w	20[10[r7]],20000[10000[r9]]
	ldpr	/12345678h,/23456789h
	ldpr.w	[/12345678h],[/23456789h]
	ldpr	[r7](r8),[r9](r10)
	ldpr.w	10[r7](r8),10000[r9](r10)
	ldpr	[pc](r8),[pc](r10)
	ldpr.w	[10[r7]](r8),[10000[r9]](r10)
	ldpr	[[pc]](r8),[[pc]](r10)
	ldpr.w	/12345678h(r8),/23456789h(r10)
	ldpr	[/12345678h](r8),[/23456789h](r10)
	ldpr.w	#2,#tr
	ldpr	#200,#trmod

	expect	1100
	ldtask.w	r7,r9
	endexpect
	ldtask	r7,r9
	ldtask	[r7],[r9]
	ldtask	[r7+],[r9+]
	ldtask	[-r7],[-r9]
	ldtask	10[r7],10000[r9]
	ldtask	[10[r7]],[10000[r9]]
	ldtask	20[10[r7]],20000[10000[r9]]
	ldtask	/12345678h,/23456789h
	ldtask	[/12345678h],[/23456789h]
	ldtask	[r7](r8),[r9](r10)
	ldtask	10[r7](r8),10000[r9](r10)
	ldtask	[pc](r8),[pc](r10)
	ldtask	[10[r7]](r8),[10000[r9]](r10)
	ldtask	[[pc]](r8),[[pc]](r10)
	ldtask	/12345678h(r8),/23456789h(r10)
	ldtask	[/12345678h](r8),[/23456789h](r10)
	ldtask	#2,#5
	ldtask	#200,#500

	mov.b	r12,r14
	mov.w	regop1,regop2
	mov.h	[r4],r14
	mov.h	[r4](regop1),regop2

	expect	1100
	movbsu.b @[r7],#3,@[r9]
	endexpect
	movbsu	@[r7],#3,@[r9]
	movbsu	@[r7+],#20,@[r9+]
	movbsu	@[-r7],r11,@[-r9]
	movbsu	10@[r7],#3,10000@[r9]
	movbsu	@[10[r7]],#20,@[10000[r9]]
	movbsu	20@[10[r7]],r11,20000@[10000[r9]]
	movbsu	@/12345678h,#3,@/23456789h
	movbsu	@[/12345678h],#20,@[/23456789h]
	movbsu	r8@[r7],r11,r10@[r9]
	movbsu	r8@10[r7],#3,r10@10000[r9]
	movbsu	r8@[pc],#20,r10@[pc]
	movbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	movbsu	r8@[[pc]],#3,r10@[[pc]]
	movbsu	r8@/12345678h,#20,r10@/23456789h
	movbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	movbsd.b @[r7],#3,@[r9]
	endexpect
	movbsd	@[r7],#3,@[r9]
	movbsd	@[r7+],#20,@[r9+]
	movbsd	@[-r7],r11,@[-r9]
	movbsd	10@[r7],#3,10000@[r9]
	movbsd	@[10[r7]],#20,@[10000[r9]]
	movbsd	20@[10[r7]],r11,20000@[10000[r9]]
	movbsd	@/12345678h,#3,@/23456789h
	movbsd	@[/12345678h],#20,@[/23456789h]
	movbsd	r8@[r7],r11,r10@[r9]
	movbsd	r8@10[r7],#3,r10@10000[r9]
	movbsd	r8@[pc],#20,r10@[pc]
	movbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	movbsd	r8@[[pc]],#3,r10@[[pc]]
	movbsd	r8@/12345678h,#20,r10@/23456789h
	movbsd	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1350,1350
	movcu.b	r7,#3,[r9],#12
	movcu	/5,r20,#70,r24
	endexpect
	movcu.b	[r7],#3,[r9],#12
	movcu	[-r7],r20,[-r9],r24
	movcu.b	10[r7],#3,10000[r9],#12
	movcu	20[10[r7]],r20,20000[10000[r9]],r24
	movcu.b	/12345678h,#3,/23456789h,#12
	movcu	[r7](r8),r20,[r9](r10),r24
	movcu.b	10[r7](r8),#3,10000[r9](r10),#12
	movcu	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcu.b	[[pc]](r8),#3,[[pc]](r10),#12
	movcu	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcu.h	r7,#3,[r9],#12
	movcu.h	/5,r20,#70,r24
	endexpect
	movcu.h	[r7],#3,[r9],#12
	movcu.h	[-r7],r20,[-r9],r24
	movcu.h	10[r7],#3,10000[r9],#12
	movcu.h	20[10[r7]],r20,20000[10000[r9]],r24
	movcu.h	/12345678h,#3,/23456789h,#12
	movcu.h	[r7](r8),r20,[r9](r10),r24
	movcu.h	10[r7](r8),#3,10000[r9](r10),#12
	movcu.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcu.h	[[pc]](r8),#3,[[pc]](r10),#12
	movcu.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcd.b	r7,#3,[r9],#12
	movcd	/5,r20,#70,r24
	endexpect
	movcd.b	[r7],#3,[r9],#12
	movcd	[-r7],r20,[-r9],r24
	movcd.b	10[r7],#3,10000[r9],#12
	movcd	20[10[r7]],r20,20000[10000[r9]],r24
	movcd.b	/12345678h,#3,/23456789h,#12
	movcd	[r7](r8),r20,[r9](r10),r24
	movcd.b	10[r7](r8),#3,10000[r9](r10),#12
	movcd	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcd.b	[[pc]](r8),#3,[[pc]](r10),#12
	movcd	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcd.h	r7,#3,[r9],#12
	movcd.h	/5,r20,#70,r24
	endexpect
	movcd.h	[r7],#3,[r9],#12
	movcd.h	[-r7],r20,[-r9],r24
	movcd.h	10[r7],#3,10000[r9],#12
	movcd.h	20[10[r7]],r20,20000[10000[r9]],r24
	movcd.h	/12345678h,#3,/23456789h,#12
	movcd.h	[r7](r8),r20,[r9](r10),r24
	movcd.h	10[r7](r8),#3,10000[r9](r10),#12
	movcd.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcd.h	[[pc]](r8),#3,[[pc]](r10),#12
	movcd.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect		1350,1350
	movcfu.b	r7,#3,[r9],#12
	movcfu		/5,r20,#70,r24
	endexpect
	movcfu.b	[r7],#3,[r9],#12
	movcfu		[-r7],r20,[-r9],r24
	movcfu.b	10[r7],#3,10000[r9],#12
	movcfu		20[10[r7]],r20,20000[10000[r9]],r24
	movcfu.b	/12345678h,#3,/23456789h,#12
	movcfu		[r7](r8),r20,[r9](r10),r24
	movcfu.b	10[r7](r8),#3,10000[r9](r10),#12
	movcfu		[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcfu.b	[[pc]](r8),#3,[[pc]](r10),#12
	movcfu		[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcfu.h	r7,#3,[r9],#12
	movcfu.h	/5,r20,#70,r24
	endexpect
	movcfu.h	[r7],#3,[r9],#12
	movcfu.h	[-r7],r20,[-r9],r24
	movcfu.h	10[r7],#3,10000[r9],#12
	movcfu.h	20[10[r7]],r20,20000[10000[r9]],r24
	movcfu.h	/12345678h,#3,/23456789h,#12
	movcfu.h	[r7](r8),r20,[r9](r10),r24
	movcfu.h	10[r7](r8),#3,10000[r9](r10),#12
	movcfu.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcfu.h	[[pc]](r8),#3,[[pc]](r10),#12
	movcfu.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcfd.b	r7,#3,[r9],#12
	movcfd		/5,r20,#70,r24
	endexpect
	movcfd.b	[r7],#3,[r9],#12
	movcfd		[-r7],r20,[-r9],r24
	movcfd.b	10[r7],#3,10000[r9],#12
	movcfd		20[10[r7]],r20,20000[10000[r9]],r24
	movcfd.b	/12345678h,#3,/23456789h,#12
	movcfd		[r7](r8),r20,[r9](r10),r24
	movcfd.b	10[r7](r8),#3,10000[r9](r10),#12
	movcfd		[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcfd.b	[[pc]](r8),#3,[[pc]](r10),#12
	movcfd		[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcfd.h	r7,#3,[r9],#12
	movcfd.h	/5,r20,#70,r24
	endexpect
	movcfd.h	[r7],#3,[r9],#12
	movcfd.h	[-r7],r20,[-r9],r24
	movcfd.h	10[r7],#3,10000[r9],#12
	movcfd.h	20[10[r7]],r20,20000[10000[r9]],r24
	movcfd.h	/12345678h,#3,/23456789h,#12
	movcfd.h	[r7](r8),r20,[r9](r10),r24
	movcfd.h	10[r7](r8),#3,10000[r9](r10),#12
	movcfd.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcfd.h	[[pc]](r8),#3,[[pc]](r10),#12
	movcfd.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcs.b	r7,#3,[r9],#12
	movcs	/5,r20,#70,r24
	endexpect
	movcs.b	[r7],#3,[r9],#12
	movcs	[-r7],r20,[-r9],r24
	movcs.b	10[r7],#3,10000[r9],#12
	movcs	20[10[r7]],r20,20000[10000[r9]],r24
	movcs.b	/12345678h,#3,/23456789h,#12
	movcs	[r7](r8),r20,[r9](r10),r24
	movcs.b	10[r7](r8),#3,10000[r9](r10),#12
	movcs	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcs.b	[[pc]](r8),#3,[[pc]](r10),#12
	movcs	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350,1350
	movcs.h	r7,#3,[r9],#12
	movcs.h	/5,r20,#70,r24
	endexpect
	movcs.h	[r7],#3,[r9],#12
	movcs.h	[-r7],r20,[-r9],r24
	movcs.h	10[r7],#3,10000[r9],#12
	movcs.h	20[10[r7]],r20,20000[10000[r9]],r24
	movcs.h	/12345678h,#3,/23456789h,#12
	movcs.h	[r7](r8),r20,[r9](r10),r24
	movcs.h	10[r7](r8),#3,10000[r9](r10),#12
	movcs.h	[10[r7]](r8),r20,[10000[r9]](r10),r24
	movcs.h	[[pc]](r8),#3,[[pc]](r10),#12
	movcs.h	[/12345678h](r8),r20,[/23456789h](r10),r24

	expect	1350
	movea.b	r7,r9
	endexpect
	movea.b	[r7],r9
	movea.b	[r7],[r9]
	movea.b	[r7+],[r9+]
	movea.b	[-r7],[-r9]
	movea.b	10[r7],10000[r9]
	movea.b	[10[r7]],[10000[r9]]
	movea.b	20[10[r7]],20000[10000[r9]]
	movea.b	/12345678h,/23456789h
	movea.b	[/12345678h],[/23456789h]
	movea.b	[r7](r8),[r9](r10)
	movea.b	10[r7](r8),10000[r9](r10)
	movea.b	[pc](r8),[pc](r10)
	movea.b	[10[r7]](r8),[10000[r9]](r10)
	movea.b	[[pc]](r8),[[pc]](r10)
	movea.b	/12345678h(r8),/23456789h(r10)
	movea.b	[/12345678h](r8),[/23456789h](r10)
        expect  1350,1350
	movea.b	#2,#5
	movea.b	#200,#500
        endexpect

	expect	1350
	movea.h	r7,r9
	endexpect
	movea.h	[r7],r9
	movea.h	[r7],[r9]
	movea.h	[r7+],[r9+]
	movea.h	[-r7],[-r9]
	movea.h	10[r7],10000[r9]
	movea.h	[10[r7]],[10000[r9]]
	movea.h	20[10[r7]],20000[10000[r9]]
	movea.h	/12345678h,/23456789h
	movea.h	[/12345678h],[/23456789h]
	movea.h	[r7](r8),[r9](r10)
	movea.h	10[r7](r8),10000[r9](r10)
	movea.h	[pc](r8),[pc](r10)
	movea.h	[10[r7]](r8),[10000[r9]](r10)
	movea.h	[[pc]](r8),[[pc]](r10)
	movea.h	/12345678h(r8),/23456789h(r10)
	movea.h	[/12345678h](r8),[/23456789h](r10)
        expect  1350,1350
	movea.h	#2,#5
	movea.h	#200,#500
        endexpect

	expect	1350
	movea.w	r7,r9
	endexpect
	movea	[r7],r9
	movea.w	[r7],[r9]
	movea	[r7+],[r9+]
	movea.w	[-r7],[-r9]
	movea	10[r7],10000[r9]
	movea.w	[10[r7]],[10000[r9]]
	movea	20[10[r7]],20000[10000[r9]]
	movea.w	/12345678h,/23456789h
	movea	[/12345678h],[/23456789h]
	movea.w	[r7](r8),[r9](r10)
	movea	10[r7](r8),10000[r9](r10)
	movea.w	[pc](r8),[pc](r10)
	movea	[10[r7]](r8),[10000[r9]](r10)
	movea.w	[[pc]](r8),[[pc]](r10)
	movea	/12345678h(r8),/23456789h(r10)
	movea.w	[/12345678h](r8),[/23456789h](r10)
        expect  1350,1350
	movea	#2,#5
	movea.w	#200,#500
        endexpect

	movf.s	r7,r9
	movf.s	[r7],[r9]
	movf.s	[r7+],[r9+]
	movf.s	[-r7],[-r9]
	movf.s	10[r7],10000[r9]
	movf.s	[10[r7]],[10000[r9]]
	movf.s	20[10[r7]],20000[10000[r9]]
	movf.s	/12345678h,/23456789h
	movf.s	[/12345678h],[/23456789h]
	movf.s	[r7](r8),[r9](r10)
	movf.s	10[r7](r8),10000[r9](r10)
	movf.s	[pc](r8),[pc](r10)
	movf.s	[10[r7]](r8),[10000[r9]](r10)
	movf.s	[[pc]](r8),[[pc]](r10)
	movf.s	/12345678h(r8),/23456789h(r10)
	movf.s	[/12345678h](r8),[/23456789h](r10)

	movf.l	r7,r9
	movf.l	[r7],[r9]
	movf.l	[r7+],[r9+]
	movf.l	[-r7],[-r9]
	movf.l	10[r7],10000[r9]
	movf.l	[10[r7]],[10000[r9]]
	movf.l	20[10[r7]],20000[10000[r9]]
	movf.l	/12345678h,/23456789h
	movf.l	[/12345678h],[/23456789h]
	movf.l	[r7](r8),[r9](r10)
	movf.l	10[r7](r8),10000[r9](r10)
	movf.l	[pc](r8),[pc](r10)
	movf.l	[10[r7]](r8),[10000[r9]](r10)
	movf.l	[[pc]](r8),[[pc]](r10)
	movf.l	/12345678h(r8),/23456789h(r10)
	movf.l	[/12345678h](r8),[/23456789h](r10)

	movs.bh	r7,r9
	movs.bh	[r7],[r9]
	movs.bh	[r7+],[r9+]
	movs.bh	[-r7],[-r9]
	movs.bh	10[r7],10000[r9]
	movs.bh	[10[r7]],[10000[r9]]
	movs.bh	20[10[r7]],20000[10000[r9]]
	movs.bh	/12345678h,/23456789h
	movs.bh	[/12345678h],[/23456789h]
	movs.bh	[r7](r8),[r9](r10)
	movs.bh	10[r7](r8),10000[r9](r10)
	movs.bh	[pc](r8),[pc](r10)
	movs.bh	[10[r7]](r8),[10000[r9]](r10)
	movs.bh	[[pc]](r8),[[pc]](r10)
	movs.bh	/12345678h(r8),/23456789h(r10)
	movs.bh	[/12345678h](r8),[/23456789h](r10)
	movs.bh	#2,r9
	movs.bh	#200,r9

	movs.bw	r7,r9
	movs.bw	[r7],[r9]
	movs.bw	[r7+],[r9+]
	movs.bw	[-r7],[-r9]
	movs.bw	10[r7],10000[r9]
	movs.bw	[10[r7]],[10000[r9]]
	movs.bw	20[10[r7]],20000[10000[r9]]
	movs.bw	/12345678h,/23456789h
	movs.bw	[/12345678h],[/23456789h]
	movs.bw	[r7](r8),[r9](r10)
	movs.bw	10[r7](r8),10000[r9](r10)
	movs.bw	[pc](r8),[pc](r10)
	movs.bw	[10[r7]](r8),[10000[r9]](r10)
	movs.bw	[[pc]](r8),[[pc]](r10)
	movs.bw	/12345678h(r8),/23456789h(r10)
	movs.bw	[/12345678h](r8),[/23456789h](r10)
	movs.bw	#2,r9
	movs.bw	#200,r9

	movs.hw	r7,r9
	movs.hw	[r7],[r9]
	movs.hw	[r7+],[r9+]
	movs.hw	[-r7],[-r9]
	movs.hw	10[r7],10000[r9]
	movs.hw	[10[r7]],[10000[r9]]
	movs.hw	20[10[r7]],20000[10000[r9]]
	movs.hw	/12345678h,/23456789h
	movs.hw	[/12345678h],[/23456789h]
	movs.hw	[r7](r8),[r9](r10)
	movs.hw	10[r7](r8),10000[r9](r10)
	movs.hw	[pc](r8),[pc](r10)
	movs.hw	[10[r7]](r8),[10000[r9]](r10)
	movs.hw	[[pc]](r8),[[pc]](r10)
	movs.hw	/12345678h(r8),/23456789h(r10)
	movs.hw	[/12345678h](r8),[/23456789h](r10)
	movs.hw	#2,r9
	movs.hw	#2000,r9

	movt.hb	r7,r9
	movt.hb	[r7],[r9]
	movt.hb	[r7+],[r9+]
	movt.hb	[-r7],[-r9]
	movt.hb	10[r7],10000[r9]
	movt.hb	[10[r7]],[10000[r9]]
	movt.hb	20[10[r7]],20000[10000[r9]]
	movt.hb	/12345678h,/23456789h
	movt.hb	[/12345678h],[/23456789h]
	movt.hb	[r7](r8),[r9](r10)
	movt.hb	10[r7](r8),10000[r9](r10)
	movt.hb	[pc](r8),[pc](r10)
	movt.hb	[10[r7]](r8),[10000[r9]](r10)
	movt.hb	[[pc]](r8),[[pc]](r10)
	movt.hb	/12345678h(r8),/23456789h(r10)
	movt.hb	[/12345678h](r8),[/23456789h](r10)
	movt.hb	#2,r9
	movt.hb	#20000,r9

	movt.wb	r7,r9
	movt.wb	[r7],[r9]
	movt.wb	[r7+],[r9+]
	movt.wb	[-r7],[-r9]
	movt.wb	10[r7],10000[r9]
	movt.wb	[10[r7]],[10000[r9]]
	movt.wb	20[10[r7]],20000[10000[r9]]
	movt.wb	/12345678h,/23456789h
	movt.wb	[/12345678h],[/23456789h]
	movt.wb	[r7](r8),[r9](r10)
	movt.wb	10[r7](r8),10000[r9](r10)
	movt.wb	[pc](r8),[pc](r10)
	movt.wb	[10[r7]](r8),[10000[r9]](r10)
	movt.wb	[[pc]](r8),[[pc]](r10)
	movt.wb	/12345678h(r8),/23456789h(r10)
	movt.wb	[/12345678h](r8),[/23456789h](r10)
	movt.wb	#2,r9
	movt.wb	#2000000,r9

	movt.wh	r7,r9
	movt.wh	[r7],[r9]
	movt.wh	[r7+],[r9+]
	movt.wh	[-r7],[-r9]
	movt.wh	10[r7],10000[r9]
	movt.wh	[10[r7]],[10000[r9]]
	movt.wh	20[10[r7]],20000[10000[r9]]
	movt.wh	/12345678h,/23456789h
	movt.wh	[/12345678h],[/23456789h]
	movt.wh	[r7](r8),[r9](r10)
	movt.wh	10[r7](r8),10000[r9](r10)
	movt.wh	[pc](r8),[pc](r10)
	movt.wh	[10[r7]](r8),[10000[r9]](r10)
	movt.wh	[[pc]](r8),[[pc]](r10)
	movt.wh	/12345678h(r8),/23456789h(r10)
	movt.wh	[/12345678h](r8),[/23456789h](r10)
	movt.wh	#2,r9
	movt.wh	#2000000,r9

	movz.bh	r7,r9
	movz.bh	[r7],[r9]
	movz.bh	[r7+],[r9+]
	movz.bh	[-r7],[-r9]
	movz.bh	10[r7],10000[r9]
	movz.bh	[10[r7]],[10000[r9]]
	movz.bh	20[10[r7]],20000[10000[r9]]
	movz.bh	/12345678h,/23456789h
	movz.bh	[/12345678h],[/23456789h]
	movz.bh	[r7](r8),[r9](r10)
	movz.bh	10[r7](r8),10000[r9](r10)
	movz.bh	[pc](r8),[pc](r10)
	movz.bh	[10[r7]](r8),[10000[r9]](r10)
	movz.bh	[[pc]](r8),[[pc]](r10)
	movz.bh	/12345678h(r8),/23456789h(r10)
	movz.bh	[/12345678h](r8),[/23456789h](r10)
	movz.bh	#2,r9
	movz.bh	#200,r9

	movz.bw	r7,r9
	movz.bw	[r7],[r9]
	movz.bw	[r7+],[r9+]
	movz.bw	[-r7],[-r9]
	movz.bw	10[r7],10000[r9]
	movz.bw	[10[r7]],[10000[r9]]
	movz.bw	20[10[r7]],20000[10000[r9]]
	movz.bw	/12345678h,/23456789h
	movz.bw	[/12345678h],[/23456789h]
	movz.bw	[r7](r8),[r9](r10)
	movz.bw	10[r7](r8),10000[r9](r10)
	movz.bw	[pc](r8),[pc](r10)
	movz.bw	[10[r7]](r8),[10000[r9]](r10)
	movz.bw	[[pc]](r8),[[pc]](r10)
	movz.bw	/12345678h(r8),/23456789h(r10)
	movz.bw	[/12345678h](r8),[/23456789h](r10)
	movz.bw	#2,r9
	movz.bw	#200,r9

	movz.hw	r7,r9
	movz.hw	[r7],[r9]
	movz.hw	[r7+],[r9+]
	movz.hw	[-r7],[-r9]
	movz.hw	10[r7],10000[r9]
	movz.hw	[10[r7]],[10000[r9]]
	movz.hw	20[10[r7]],20000[10000[r9]]
	movz.hw	/12345678h,/23456789h
	movz.hw	[/12345678h],[/23456789h]
	movz.hw	[r7](r8),[r9](r10)
	movz.hw	10[r7](r8),10000[r9](r10)
	movz.hw	[pc](r8),[pc](r10)
	movz.hw	[10[r7]](r8),[10000[r9]](r10)
	movz.hw	[[pc]](r8),[[pc]](r10)
	movz.hw	/12345678h(r8),/23456789h(r10)
	movz.hw	[/12345678h](r8),[/23456789h](r10)
	movz.hw	#2,r9
	movz.hw	#2000,r9

	mul.b	r7,r9
	mul.b	[r7],[r9]
	mul.b	[r7+],[r9+]
	mul.b	[-r7],[-r9]
	mul.b	10[r7],10000[r9]
	mul.b	[10[r7]],[10000[r9]]
	mul.b	20[10[r7]],20000[10000[r9]]
	mul.b	/12345678h,/23456789h
	mul.b	[/12345678h],[/23456789h]
	mul.b	[r7](r8),[r9](r10)
	mul.b	10[r7](r8),10000[r9](r10)
	mul.b	[pc](r8),[pc](r10)
	mul.b	[10[r7]](r8),[10000[r9]](r10)
	mul.b	[[pc]](r8),[[pc]](r10)
	mul.b	/12345678h(r8),/23456789h(r10)
	mul.b	[/12345678h](r8),[/23456789h](r10)
	mul.b	#2,r0
	mul.b	#200,r0

	mul.h	r7,r9
	mul.h	[r7],[r9]
	mul.h	[r7+],[r9+]
	mul.h	[-r7],[-r9]
	mul.h	10[r7],10000[r9]
	mul.h	[10[r7]],[10000[r9]]
	mul.h	20[10[r7]],20000[10000[r9]]
	mul.h	/12345678h,/23456789h
	mul.h	[/12345678h],[/23456789h]
	mul.h	[r7](r8),[r9](r10)
	mul.h	10[r7](r8),10000[r9](r10)
	mul.h	[pc](r8),[pc](r10)
	mul.h	[10[r7]](r8),[10000[r9]](r10)
	mul.h	[[pc]](r8),[[pc]](r10)
	mul.h	/12345678h(r8),/23456789h(r10)
	mul.h	[/12345678h](r8),[/23456789h](r10)
	mul.h	#2,r0
	mul.h	#200,r0

	mul.w	r7,r9
	mul.w	[r7],[r9]
	mul.w	[r7+],[r9+]
	mul.w	[-r7],[-r9]
	mul.w	10[r7],10000[r9]
	mul.w	[10[r7]],[10000[r9]]
	mul.w	20[10[r7]],20000[10000[r9]]
	mul.w	/12345678h,/23456789h
	mul.w	[/12345678h],[/23456789h]
	mul.w	[r7](r8),[r9](r10)
	mul.w	10[r7](r8),10000[r9](r10)
	mul.w	[pc](r8),[pc](r10)
	mul.w	[10[r7]](r8),[10000[r9]](r10)
	mul.w	[[pc]](r8),[[pc]](r10)
	mul.w	/12345678h(r8),/23456789h(r10)
	mul.w	[/12345678h](r8),[/23456789h](r10)
	mul.w	#2,r0
	mul.w	#200,r0

	mulf.s	r7,r9
	mulf.s	[r7],[r9]
	mulf.s	[r7+],[r9+]
	mulf.s	[-r7],[-r9]
	mulf.s	10[r7],10000[r9]
	mulf.s	[10[r7]],[10000[r9]]
	mulf.s	20[10[r7]],20000[10000[r9]]
	mulf.s	/12345678h,/23456789h
	mulf.s	[/12345678h],[/23456789h]
	mulf.s	[r7](r8),[r9](r10)
	mulf.s	10[r7](r8),10000[r9](r10)
	mulf.s	[pc](r8),[pc](r10)
	mulf.s	[10[r7]](r8),[10000[r9]](r10)
	mulf.s	[[pc]](r8),[[pc]](r10)
	mulf.s	/12345678h(r8),/23456789h(r10)
	mulf.s	[/12345678h](r8),[/23456789h](r10)

	mulf.l	r7,r9
	mulf.l	[r7],[r9]
	mulf.l	[r7+],[r9+]
	mulf.l	[-r7],[-r9]
	mulf.l	10[r7],10000[r9]
	mulf.l	[10[r7]],[10000[r9]]
	mulf.l	20[10[r7]],20000[10000[r9]]
	mulf.l	/12345678h,/23456789h
	mulf.l	[/12345678h],[/23456789h]
	mulf.l	[r7](r8),[r9](r10)
	mulf.l	10[r7](r8),10000[r9](r10)
	mulf.l	[pc](r8),[pc](r10)
	mulf.l	[10[r7]](r8),[10000[r9]](r10)
	mulf.l	[[pc]](r8),[[pc]](r10)
	mulf.l	/12345678h(r8),/23456789h(r10)
	mulf.l	[/12345678h](r8),[/23456789h](r10)

	mulu.b	r7,r9
	mulu.b	[r7],[r9]
	mulu.b	[r7+],[r9+]
	mulu.b	[-r7],[-r9]
	mulu.b	10[r7],10000[r9]
	mulu.b	[10[r7]],[10000[r9]]
	mulu.b	20[10[r7]],20000[10000[r9]]
	mulu.b	/12345678h,/23456789h
	mulu.b	[/12345678h],[/23456789h]
	mulu.b	[r7](r8),[r9](r10)
	mulu.b	10[r7](r8),10000[r9](r10)
	mulu.b	[pc](r8),[pc](r10)
	mulu.b	[10[r7]](r8),[10000[r9]](r10)
	mulu.b	[[pc]](r8),[[pc]](r10)
	mulu.b	/12345678h(r8),/23456789h(r10)
	mulu.b	[/12345678h](r8),[/23456789h](r10)
	mulu.b	#2,r0
	mulu.b	#200,r0

	mulu.h	r7,r9
	mulu.h	[r7],[r9]
	mulu.h	[r7+],[r9+]
	mulu.h	[-r7],[-r9]
	mulu.h	10[r7],10000[r9]
	mulu.h	[10[r7]],[10000[r9]]
	mulu.h	20[10[r7]],20000[10000[r9]]
	mulu.h	/12345678h,/23456789h
	mulu.h	[/12345678h],[/23456789h]
	mulu.h	[r7](r8),[r9](r10)
	mulu.h	10[r7](r8),10000[r9](r10)
	mulu.h	[pc](r8),[pc](r10)
	mulu.h	[10[r7]](r8),[10000[r9]](r10)
	mulu.h	[[pc]](r8),[[pc]](r10)
	mulu.h	/12345678h(r8),/23456789h(r10)
	mulu.h	[/12345678h](r8),[/23456789h](r10)
	mulu.h	#2,r0
	mulu.h	#200,r0

	mulu.w	r7,r9
	mulu.w	[r7],[r9]
	mulu.w	[r7+],[r9+]
	mulu.w	[-r7],[-r9]
	mulu.w	10[r7],10000[r9]
	mulu.w	[10[r7]],[10000[r9]]
	mulu.w	20[10[r7]],20000[10000[r9]]
	mulu.w	/12345678h,/23456789h
	mulu.w	[/12345678h],[/23456789h]
	mulu.w	[r7](r8),[r9](r10)
	mulu.w	10[r7](r8),10000[r9](r10)
	mulu.w	[pc](r8),[pc](r10)
	mulu.w	[10[r7]](r8),[10000[r9]](r10)
	mulu.w	[[pc]](r8),[[pc]](r10)
	mulu.w	/12345678h(r8),/23456789h(r10)
	mulu.w	[/12345678h](r8),[/23456789h](r10)
	mulu.w	#2,r0
	mulu.w	#200,r0

	mulx	r7,r9
	mulx.w	[r7],[r9]
	mulx	[r7+],[r9+]
	mulx.w	[-r7],[-r9]
	mulx	10[r7],10000[r9]
	mulx.w	[10[r7]],[10000[r9]]
	mulx	20[10[r7]],20000[10000[r9]]
	mulx.w	/12345678h,/23456789h
	mulx	[/12345678h],[/23456789h]
	mulx.w	[r7](r8),[r9](r10)
	mulx	10[r7](r8),10000[r9](r10)
	mulx.w	[pc](r8),[pc](r10)
	mulx	[10[r7]](r8),[10000[r9]](r10)
	mulx.w	[[pc]](r8),[[pc]](r10)
	mulx	/12345678h(r8),/23456789h(r10)
	mulx.w	[/12345678h](r8),[/23456789h](r10)
	mulx	#2,r0
	mulx.w	#200,r0

	mulux	r7,r9
	mulux.w	[r7],[r9]
	mulux	[r7+],[r9+]
	mulux.w	[-r7],[-r9]
	mulux	10[r7],10000[r9]
	mulux.w	[10[r7]],[10000[r9]]
	mulux	20[10[r7]],20000[10000[r9]]
	mulux.w	/12345678h,/23456789h
	mulux	[/12345678h],[/23456789h]
	mulux.w	[r7](r8),[r9](r10)
	mulux	10[r7](r8),10000[r9](r10)
	mulux.w	[pc](r8),[pc](r10)
	mulux	[10[r7]](r8),[10000[r9]](r10)
	mulux.w	[[pc]](r8),[[pc]](r10)
	mulux	/12345678h(r8),/23456789h(r10)
	mulux.w	[/12345678h](r8),[/23456789h](r10)
	mulux	#2,r0
	mulux.w	#200,r0

	neg.b	r7,r9
	neg.b	[r7],[r9]
	neg.b	[r7+],[r9+]
	neg.b	[-r7],[-r9]
	neg.b	10[r7],10000[r9]
	neg.b	[10[r7]],[10000[r9]]
	neg.b	20[10[r7]],20000[10000[r9]]
	neg.b	/12345678h,/23456789h
	neg.b	[/12345678h],[/23456789h]
	neg.b	[r7](r8),[r9](r10)
	neg.b	10[r7](r8),10000[r9](r10)
	neg.b	[pc](r8),[pc](r10)
	neg.b	[10[r7]](r8),[10000[r9]](r10)
	neg.b	[[pc]](r8),[[pc]](r10)
	neg.b	/12345678h(r8),/23456789h(r10)
	neg.b	[/12345678h](r8),[/23456789h](r10)
	neg.b	#2,r0
	neg.b	#200,r0

	neg.h	r7,r9
	neg.h	[r7],[r9]
	neg.h	[r7+],[r9+]
	neg.h	[-r7],[-r9]
	neg.h	10[r7],10000[r9]
	neg.h	[10[r7]],[10000[r9]]
	neg.h	20[10[r7]],20000[10000[r9]]
	neg.h	/12345678h,/23456789h
	neg.h	[/12345678h],[/23456789h]
	neg.h	[r7](r8),[r9](r10)
	neg.h	10[r7](r8),10000[r9](r10)
	neg.h	[pc](r8),[pc](r10)
	neg.h	[10[r7]](r8),[10000[r9]](r10)
	neg.h	[[pc]](r8),[[pc]](r10)
	neg.h	/12345678h(r8),/23456789h(r10)
	neg.h	[/12345678h](r8),[/23456789h](r10)
	neg.h	#2,r0
	neg.h	#200,r0

	neg.w	r7,r9
	neg.w	[r7],[r9]
	neg.w	[r7+],[r9+]
	neg.w	[-r7],[-r9]
	neg.w	10[r7],10000[r9]
	neg.w	[10[r7]],[10000[r9]]
	neg.w	20[10[r7]],20000[10000[r9]]
	neg.w	/12345678h,/23456789h
	neg.w	[/12345678h],[/23456789h]
	neg.w	[r7](r8),[r9](r10)
	neg.w	10[r7](r8),10000[r9](r10)
	neg.w	[pc](r8),[pc](r10)
	neg.w	[10[r7]](r8),[10000[r9]](r10)
	neg.w	[[pc]](r8),[[pc]](r10)
	neg.w	/12345678h(r8),/23456789h(r10)
	neg.w	[/12345678h](r8),[/23456789h](r10)
	neg.w	#2,r0
	neg.w	#200,r0

	negf.s	r7,r9
	negf.s	[r7],[r9]
	negf.s	[r7+],[r9+]
	negf.s	[-r7],[-r9]
	negf.s	10[r7],10000[r9]
	negf.s	[10[r7]],[10000[r9]]
	negf.s	20[10[r7]],20000[10000[r9]]
	negf.s	/12345678h,/23456789h
	negf.s	[/12345678h],[/23456789h]
	negf.s	[r7](r8),[r9](r10)
	negf.s	10[r7](r8),10000[r9](r10)
	negf.s	[pc](r8),[pc](r10)
	negf.s	[10[r7]](r8),[10000[r9]](r10)
	negf.s	[[pc]](r8),[[pc]](r10)
	negf.s	/12345678h(r8),/23456789h(r10)
	negf.s	[/12345678h](r8),[/23456789h](r10)

	negf.l	r7,r9
	negf.l	[r7],[r9]
	negf.l	[r7+],[r9+]
	negf.l	[-r7],[-r9]
	negf.l	10[r7],10000[r9]
	negf.l	[10[r7]],[10000[r9]]
	negf.l	20[10[r7]],20000[10000[r9]]
	negf.l	/12345678h,/23456789h
	negf.l	[/12345678h],[/23456789h]
	negf.l	[r7](r8),[r9](r10)
	negf.l	10[r7](r8),10000[r9](r10)
	negf.l	[pc](r8),[pc](r10)
	negf.l	[10[r7]](r8),[10000[r9]](r10)
	negf.l	[[pc]](r8),[[pc]](r10)
	negf.l	/12345678h(r8),/23456789h(r10)
	negf.l	[/12345678h](r8),[/23456789h](r10)

	nop

	not.b	r7,r9
	not.b	[r7],[r9]
	not.b	[r7+],[r9+]
	not.b	[-r7],[-r9]
	not.b	10[r7],10000[r9]
	not.b	[10[r7]],[10000[r9]]
	not.b	20[10[r7]],20000[10000[r9]]
	not.b	/12345678h,/23456789h
	not.b	[/12345678h],[/23456789h]
	not.b	[r7](r8),[r9](r10)
	not.b	10[r7](r8),10000[r9](r10)
	not.b	[pc](r8),[pc](r10)
	not.b	[10[r7]](r8),[10000[r9]](r10)
	not.b	[[pc]](r8),[[pc]](r10)
	not.b	/12345678h(r8),/23456789h(r10)
	not.b	[/12345678h](r8),[/23456789h](r10)
	not.b	#2,r0
	not.b	#200,r0

	not.h	r7,r9
	not.h	[r7],[r9]
	not.h	[r7+],[r9+]
	not.h	[-r7],[-r9]
	not.h	10[r7],10000[r9]
	not.h	[10[r7]],[10000[r9]]
	not.h	20[10[r7]],20000[10000[r9]]
	not.h	/12345678h,/23456789h
	not.h	[/12345678h],[/23456789h]
	not.h	[r7](r8),[r9](r10)
	not.h	10[r7](r8),10000[r9](r10)
	not.h	[pc](r8),[pc](r10)
	not.h	[10[r7]](r8),[10000[r9]](r10)
	not.h	[[pc]](r8),[[pc]](r10)
	not.h	/12345678h(r8),/23456789h(r10)
	not.h	[/12345678h](r8),[/23456789h](r10)
	not.h	#2,r0
	not.h	#200,r0

	not.w	r7,r9
	not.w	[r7],[r9]
	not.w	[r7+],[r9+]
	not.w	[-r7],[-r9]
	not.w	10[r7],10000[r9]
	not.w	[10[r7]],[10000[r9]]
	not.w	20[10[r7]],20000[10000[r9]]
	not.w	/12345678h,/23456789h
	not.w	[/12345678h],[/23456789h]
	not.w	[r7](r8),[r9](r10)
	not.w	10[r7](r8),10000[r9](r10)
	not.w	[pc](r8),[pc](r10)
	not.w	[10[r7]](r8),[10000[r9]](r10)
	not.w	[[pc]](r8),[[pc]](r10)
	not.w	/12345678h(r8),/23456789h(r10)
	not.w	[/12345678h](r8),[/23456789h](r10)
	not.w	#2,r0
	not.w	#200,r0

	expect	1100,1350
	not1.b	r7,#55
	not1	r7,#55
	endexpect
	not1	r7,r9
	not1	r7,[r9]
	not1	[r7],r9
	not1	[r7],[r9]
	not1	[r7+],[r9+]
	not1	[-r7],[-r9]
	not1	10[r7],10000[r9]
	not1	[10[r7]],[10000[r9]]
	not1	20[10[r7]],20000[10000[r9]]
	not1	/12345678h,/23456789h
	not1	[/12345678h],[/23456789h]
	not1	[r7](r8),[r9](r10)
	not1	10[r7](r8),10000[r9](r10)
	not1	[pc](r8),[pc](r10)
	not1	[10[r7]](r8),[10000[r9]](r10)
	not1	[[pc]](r8),[[pc]](r10)
	not1	/12345678h(r8),/23456789h(r10)
	not1	[/12345678h](r8),[/23456789h](r10)
	not1	#3,r9
	not1	#30,r9

	expect	1100
	notbsu.b @[r7],#3,@[r9]
	endexpect
	notbsu	@[r7],#3,@[r9]
	notbsu	@[r7+],#20,@[r9+]
	notbsu	@[-r7],r11,@[-r9]
	notbsu	10@[r7],#3,10000@[r9]
	notbsu	@[10[r7]],#20,@[10000[r9]]
	notbsu	20@[10[r7]],r11,20000@[10000[r9]]
	notbsu	@/12345678h,#3,@/23456789h
	notbsu	@[/12345678h],#20,@[/23456789h]
	notbsu	r8@[r7],r11,r10@[r9]
	notbsu	r8@10[r7],#3,r10@10000[r9]
	notbsu	r8@[pc],#20,r10@[pc]
	notbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	notbsu	r8@[[pc]],#3,r10@[[pc]]
	notbsu	r8@/12345678h,#20,r10@/23456789h
	notbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	notbsd.b @[r7],#3,@[r9]
	endexpect
	notbsd	@[r7],#3,@[r9]
	notbsd	@[r7+],#20,@[r9+]
	notbsd	@[-r7],r11,@[-r9]
	notbsd	10@[r7],#3,10000@[r9]
	notbsd	@[10[r7]],#20,@[10000[r9]]
	notbsd	20@[10[r7]],r11,20000@[10000[r9]]
	notbsd	@/12345678h,#3,@/23456789h
	notbsd	@[/12345678h],#20,@[/23456789h]
	notbsd	r8@[r7],r11,r10@[r9]
	notbsd	r8@10[r7],#3,r10@10000[r9]
	notbsd	r8@[pc],#20,r10@[pc]
	notbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	notbsd	r8@[[pc]],#3,r10@[[pc]]
	notbsd	r8@/12345678h,#20,r10@/23456789h
	notbsd	r8@[/12345678h],r11,r10@[/23456789h]

	or.b	r7,r9
	or.b	[r7],[r9]
	or.b	[r7+],[r9+]
	or.b	[-r7],[-r9]
	or.b	10[r7],10000[r9]
	or.b	[10[r7]],[10000[r9]]
	or.b	20[10[r7]],20000[10000[r9]]
	or.b	/12345678h,/23456789h
	or.b	[/12345678h],[/23456789h]
	or.b	[r7](r8),[r9](r10)
	or.b	10[r7](r8),10000[r9](r10)
	or.b	[pc](r8),[pc](r10)
	or.b	[10[r7]](r8),[10000[r9]](r10)
	or.b	[[pc]](r8),[[pc]](r10)
	or.b	/12345678h(r8),/23456789h(r10)
	or.b	[/12345678h](r8),[/23456789h](r10)
	or.b	#2,r0
	or.b	#200,r0

	or.h	r7,r9
	or.h	[r7],[r9]
	or.h	[r7+],[r9+]
	or.h	[-r7],[-r9]
	or.h	10[r7],10000[r9]
	or.h	[10[r7]],[10000[r9]]
	or.h	20[10[r7]],20000[10000[r9]]
	or.h	/12345678h,/23456789h
	or.h	[/12345678h],[/23456789h]
	or.h	[r7](r8),[r9](r10)
	or.h	10[r7](r8),10000[r9](r10)
	or.h	[pc](r8),[pc](r10)
	or.h	[10[r7]](r8),[10000[r9]](r10)
	or.h	[[pc]](r8),[[pc]](r10)
	or.h	/12345678h(r8),/23456789h(r10)
	or.h	[/12345678h](r8),[/23456789h](r10)
	or.h	#2,r0
	or.h	#200,r0

	or.w	r7,r9
	or.w	[r7],[r9]
	or.w	[r7+],[r9+]
	or.w	[-r7],[-r9]
	or.w	10[r7],10000[r9]
	or.w	[10[r7]],[10000[r9]]
	or.w	20[10[r7]],20000[10000[r9]]
	or.w	/12345678h,/23456789h
	or.w	[/12345678h],[/23456789h]
	or.w	[r7](r8),[r9](r10)
	or.w	10[r7](r8),10000[r9](r10)
	or.w	[pc](r8),[pc](r10)
	or.w	[10[r7]](r8),[10000[r9]](r10)
	or.w	[[pc]](r8),[[pc]](r10)
	or.w	/12345678h(r8),/23456789h(r10)
	or.w	[/12345678h](r8),[/23456789h](r10)
	or.w	#2,r0
	or.w	#200,r0

	expect	1100
	orbsu.b @[r7],#3,@[r9]
	endexpect
	orbsu	@[r7],#3,@[r9]
	orbsu	@[r7+],#20,@[r9+]
	orbsu	@[-r7],r11,@[-r9]
	orbsu	10@[r7],#3,10000@[r9]
	orbsu	@[10[r7]],#20,@[10000[r9]]
	orbsu	20@[10[r7]],r11,20000@[10000[r9]]
	orbsu	@/12345678h,#3,@/23456789h
	orbsu	@[/12345678h],#20,@[/23456789h]
	orbsu	r8@[r7],r11,r10@[r9]
	orbsu	r8@10[r7],#3,r10@10000[r9]
	orbsu	r8@[pc],#20,r10@[pc]
	orbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	orbsu	r8@[[pc]],#3,r10@[[pc]]
	orbsu	r8@/12345678h,#20,r10@/23456789h
	orbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	orbsd.b @[r7],#3,@[r9]
	endexpect
	orbsd	@[r7],#3,@[r9]
	orbsd	@[r7+],#20,@[r9+]
	orbsd	@[-r7],r11,@[-r9]
	orbsd	10@[r7],#3,10000@[r9]
	orbsd	@[10[r7]],#20,@[10000[r9]]
	orbsd	20@[10[r7]],r11,20000@[10000[r9]]
	orbsd	@/12345678h,#3,@/23456789h
	orbsd	@[/12345678h],#20,@[/23456789h]
	orbsd	r8@[r7],r11,r10@[r9]
	orbsd	r8@10[r7],#3,r10@10000[r9]
	orbsd	r8@[pc],#20,r10@[pc]
	orbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	orbsd	r8@[[pc]],#3,r10@[[pc]]
	orbsd	r8@/12345678h,#20,r10@/23456789h
	orbsd	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	ornbsu.b @[r7],#3,@[r9]
	endexpect
	ornbsu	@[r7],#3,@[r9]
	ornbsu	@[r7+],#20,@[r9+]
	ornbsu	@[-r7],r11,@[-r9]
	ornbsu	10@[r7],#3,10000@[r9]
	ornbsu	@[10[r7]],#20,@[10000[r9]]
	ornbsu	20@[10[r7]],r11,20000@[10000[r9]]
	ornbsu	@/12345678h,#3,@/23456789h
	ornbsu	@[/12345678h],#20,@[/23456789h]
	ornbsu	r8@[r7],r11,r10@[r9]
	ornbsu	r8@10[r7],#3,r10@10000[r9]
	ornbsu	r8@[pc],#20,r10@[pc]
	ornbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	ornbsu	r8@[[pc]],#3,r10@[[pc]]
	ornbsu	r8@/12345678h,#20,r10@/23456789h
	ornbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	ornbsd.b @[r7],#3,@[r9]
	endexpect
	ornbsd	@[r7],#3,@[r9]
	ornbsd	@[r7+],#20,@[r9+]
	ornbsd	@[-r7],r11,@[-r9]
	ornbsd	10@[r7],#3,10000@[r9]
	ornbsd	@[10[r7]],#20,@[10000[r9]]
	ornbsd	20@[10[r7]],r11,20000@[10000[r9]]
	ornbsd	@/12345678h,#3,@/23456789h
	ornbsd	@[/12345678h],#20,@[/23456789h]
	ornbsd	r8@[r7],r11,r10@[r9]
	ornbsd	r8@10[r7],#3,r10@10000[r9]
	ornbsd	r8@[pc],#20,r10@[pc]
	ornbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	ornbsd	r8@[[pc]],#3,r10@[[pc]]
	ornbsd	r8@/12345678h,#20,r10@/23456789h
	ornbsd	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1350
	out.b	r7,r9
	endexpect
	out.b	r9,/port1
	out.b	[r7],[r9]
	out.b	[r7+],[r9+]
	out.b	[-r7],[-r9]
	out.b	10[r7],10000[r9]
	out.b	[10[r7]],[10000[r9]]
	out.b	20[10[r7]],20000[10000[r9]]
	out.b	/12345678h,/23456789h
	out.b	[/12345678h],[/23456789h]
	out.b	[r7](r8),[r9](r10)
	out.b	10[r7](r8),10000[r9](r10)
	out.b	[pc](r8),[pc](r10)
	out.b	[10[r7]](r8),[10000[r9]](r10)
	out.b	[[pc]](r8),[[pc]](r10)
	out.b	/12345678h(r8),/23456789h(r10)
	out.b	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	out.b	#2,r0
	out.b	#200,r0
	endexpect
	out.b	#2,/port1
	out.b	#200,/port1

	expect	1350
	out.h	r7,r9
	endexpect
	out.h	r9,/port1
	out.h	[r7],[r9]
	out.h	[r7+],[r9+]
	out.h	[-r7],[-r9]
	out.h	10[r7],10000[r9]
	out.h	[10[r7]],[10000[r9]]
	out.h	20[10[r7]],20000[10000[r9]]
	out.h	/12345678h,/23456789h
	out.h	[/12345678h],[/23456789h]
	out.h	[r7](r8),[r9](r10)
	out.h	10[r7](r8),10000[r9](r10)
	out.h	[pc](r8),[pc](r10)
	out.h	[10[r7]](r8),[10000[r9]](r10)
	out.h	[[pc]](r8),[[pc]](r10)
	out.h	/12345678h(r8),/23456789h(r10)
	out.h	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	out.h	#2,r0
	out.h	#200,r0
	endexpect
	out.h	#2,/port1
	out.h	#200,/port1

	expect	1350
	out.w	r7,r9
	endexpect
	out.w	r9,/port1
	out.w	[r7],[r9]
	out.w	[r7+],[r9+]
	out.w	[-r7],[-r9]
	out.w	10[r7],10000[r9]
	out.w	[10[r7]],[10000[r9]]
	out.w	20[10[r7]],20000[10000[r9]]
	out.w	/12345678h,/23456789h
	out.w	[/12345678h],[/23456789h]
	out.w	[r7](r8),[r9](r10)
	out.w	10[r7](r8),10000[r9](r10)
	out.w	[pc](r8),[pc](r10)
	out.w	[10[r7]](r8),[10000[r9]](r10)
	out.w	[[pc]](r8),[[pc]](r10)
	out.w	/12345678h(r8),/23456789h(r10)
	out.w	[/12345678h](r8),[/23456789h](r10)
	expect	1350,1350
	out.w	#2,r0
	out.w	#200,r0
	endexpect
	out.w	#2,/port1
	out.w	#200,/port1

	pop	r9
	pop.w	[r9]
	pop	[r9+]
	pop.w	[-r9]
	pop	10000[r9]
	pop.w	[10000[r9]]
	pop	20000[10000[r9]]
	pop.w	/23456789h
	pop	[/23456789h]
	pop.w	[r9](r10)
	pop	10000[r9](r10)
	pop.w	[pc](r10)
	pop	[10000[r9]](r10)
	pop.w	[[pc]](r10)
	pop	/23456789h(r10)
	pop.w	[/23456789h](r10)
	expect	1350,1350
	pop.w	#2
	pop	#200
	endexpect

	popm	r9
	popm.w	[r9]
	popm	[r9+]
	popm.w	[-r9]
	popm	10000[r9]
	popm.w	[10000[r9]]
	popm	20000[10000[r9]]
	popm.w	/23456789h
	popm	[/23456789h]
	popm.w	[r9](r10)
	popm	10000[r9](r10)
	popm.w	[pc](r10)
	popm	[10000[r9]](r10)
	popm.w	[[pc]](r10)
	popm	/23456789h(r10)
	popm.w	[/23456789h](r10)
	popm.w	#2
	popm	#200

	prepare		r9
	prepare.w	[r9]
	prepare		[r9+]
	prepare.w	[-r9]
	prepare		10000[r9]
	prepare.w	[10000[r9]]
	prepare		20000[10000[r9]]
	prepare.w	/23456789h
	prepare		[/23456789h]
	prepare.w	[r9](r10)
	prepare		10000[r9](r10)
	prepare.w	[pc](r10)
	prepare		[10000[r9]](r10)
	prepare.w	[[pc]](r10)
	prepare		/23456789h(r10)
	prepare.w	[/23456789h](r10)
	prepare.w	#2
	prepare		#200

	push	r9
	push.w	[r9]
	push	[r9+]
	push.w	[-r9]
	push	10000[r9]
	push.w	[10000[r9]]
	push	20000[10000[r9]]
	push.w	/23456789h
	push	[/23456789h]
	push.w	[r9](r10)
	push	10000[r9](r10)
	push.w	[pc](r10)
	push	[10000[r9]](r10)
	push.w	[[pc]](r10)
	push	/23456789h(r10)
	push.w	[/23456789h](r10)
	push.w	#2
	push	#200

	pushm	r9
	pushm.w	[r9]
	pushm	[r9+]
	pushm.w	[-r9]
	pushm	10000[r9]
	pushm.w	[10000[r9]]
	pushm	20000[10000[r9]]
	pushm.w	/23456789h
	pushm	[/23456789h]
	pushm.w	[r9](r10)
	pushm	10000[r9](r10)
	pushm.w	[pc](r10)
	pushm	[10000[r9]](r10)
	pushm.w	[[pc]](r10)
	pushm	/23456789h(r10)
	pushm.w	[/23456789h](r10)
	pushm.w	#2
	pushm	#200

	rem.b	r7,r9
	rem.b	[r7],[r9]
	rem.b	[r7+],[r9+]
	rem.b	[-r7],[-r9]
	rem.b	10[r7],10000[r9]
	rem.b	[10[r7]],[10000[r9]]
	rem.b	20[10[r7]],20000[10000[r9]]
	rem.b	/12345678h,/23456789h
	rem.b	[/12345678h],[/23456789h]
	rem.b	[r7](r8),[r9](r10)
	rem.b	10[r7](r8),10000[r9](r10)
	rem.b	[pc](r8),[pc](r10)
	rem.b	[10[r7]](r8),[10000[r9]](r10)
	rem.b	[[pc]](r8),[[pc]](r10)
	rem.b	/12345678h(r8),/23456789h(r10)
	rem.b	[/12345678h](r8),[/23456789h](r10)
	rem.b	#2,r0
	rem.b	#200,r0

	rem.h	r7,r9
	rem.h	[r7],[r9]
	rem.h	[r7+],[r9+]
	rem.h	[-r7],[-r9]
	rem.h	10[r7],10000[r9]
	rem.h	[10[r7]],[10000[r9]]
	rem.h	20[10[r7]],20000[10000[r9]]
	rem.h	/12345678h,/23456789h
	rem.h	[/12345678h],[/23456789h]
	rem.h	[r7](r8),[r9](r10)
	rem.h	10[r7](r8),10000[r9](r10)
	rem.h	[pc](r8),[pc](r10)
	rem.h	[10[r7]](r8),[10000[r9]](r10)
	rem.h	[[pc]](r8),[[pc]](r10)
	rem.h	/12345678h(r8),/23456789h(r10)
	rem.h	[/12345678h](r8),[/23456789h](r10)
	rem.h	#2,r0
	rem.h	#200,r0

	rem.w	r7,r9
	rem.w	[r7],[r9]
	rem.w	[r7+],[r9+]
	rem.w	[-r7],[-r9]
	rem.w	10[r7],10000[r9]
	rem.w	[10[r7]],[10000[r9]]
	rem.w	20[10[r7]],20000[10000[r9]]
	rem.w	/12345678h,/23456789h
	rem.w	[/12345678h],[/23456789h]
	rem.w	[r7](r8),[r9](r10)
	rem.w	10[r7](r8),10000[r9](r10)
	rem.w	[pc](r8),[pc](r10)
	rem.w	[10[r7]](r8),[10000[r9]](r10)
	rem.w	[[pc]](r8),[[pc]](r10)
	rem.w	/12345678h(r8),/23456789h(r10)
	rem.w	[/12345678h](r8),[/23456789h](r10)
	rem.w	#2,r0
	rem.w	#200,r0

	remu.b	r7,r9
	remu.b	[r7],[r9]
	remu.b	[r7+],[r9+]
	remu.b	[-r7],[-r9]
	remu.b	10[r7],10000[r9]
	remu.b	[10[r7]],[10000[r9]]
	remu.b	20[10[r7]],20000[10000[r9]]
	remu.b	/12345678h,/23456789h
	remu.b	[/12345678h],[/23456789h]
	remu.b	[r7](r8),[r9](r10)
	remu.b	10[r7](r8),10000[r9](r10)
	remu.b	[pc](r8),[pc](r10)
	remu.b	[10[r7]](r8),[10000[r9]](r10)
	remu.b	[[pc]](r8),[[pc]](r10)
	remu.b	/12345678h(r8),/23456789h(r10)
	remu.b	[/12345678h](r8),[/23456789h](r10)
	remu.b	#2,r0
	remu.b	#200,r0

	remu.h	r7,r9
	remu.h	[r7],[r9]
	remu.h	[r7+],[r9+]
	remu.h	[-r7],[-r9]
	remu.h	10[r7],10000[r9]
	remu.h	[10[r7]],[10000[r9]]
	remu.h	20[10[r7]],20000[10000[r9]]
	remu.h	/12345678h,/23456789h
	remu.h	[/12345678h],[/23456789h]
	remu.h	[r7](r8),[r9](r10)
	remu.h	10[r7](r8),10000[r9](r10)
	remu.h	[pc](r8),[pc](r10)
	remu.h	[10[r7]](r8),[10000[r9]](r10)
	remu.h	[[pc]](r8),[[pc]](r10)
	remu.h	/12345678h(r8),/23456789h(r10)
	remu.h	[/12345678h](r8),[/23456789h](r10)
	remu.h	#2,r0
	remu.h	#200,r0

	remu.w	r7,r9
	remu.w	[r7],[r9]
	remu.w	[r7+],[r9+]
	remu.w	[-r7],[-r9]
	remu.w	10[r7],10000[r9]
	remu.w	[10[r7]],[10000[r9]]
	remu.w	20[10[r7]],20000[10000[r9]]
	remu.w	/12345678h,/23456789h
	remu.w	[/12345678h],[/23456789h]
	remu.w	[r7](r8),[r9](r10)
	remu.w	10[r7](r8),10000[r9](r10)
	remu.w	[pc](r8),[pc](r10)
	remu.w	[10[r7]](r8),[10000[r9]](r10)
	remu.w	[[pc]](r8),[[pc]](r10)
	remu.w	/12345678h(r8),/23456789h(r10)
	remu.w	[/12345678h](r8),[/23456789h](r10)
	remu.w	#2,r0
	remu.w	#200,r0

	ret	r9
	ret.h	[r9]
	ret	[r9+]
	ret.h	[-r9]
	ret	10000[r9]
	ret.h	[10000[r9]]
	ret	20000[10000[r9]]
	ret.h	/23456789h
	ret	[/23456789h]
	ret.h	[r9](r10)
	ret	10000[r9](r10)
	ret.h	[pc](r10)
	ret	[10000[r9]](r10)
	ret.h	[[pc]](r10)
	ret	/23456789h(r10)
	ret.h	[/23456789h](r10)
	ret	#2
	ret.h	#200

	retis	r9
	retis.h	[r9]
	retis	[r9+]
	retis.h	[-r9]
	retis	10000[r9]
	retis.h	[10000[r9]]
	retis	20000[10000[r9]]
	retis.h	/23456789h
	retis	[/23456789h]
	retis.h	[r9](r10)
	retis	10000[r9](r10)
	retis.h	[pc](r10)
	retis	[10000[r9]](r10)
	retis.h	[[pc]](r10)
	retis	/23456789h(r10)
	retis.h	[/23456789h](r10)
	retis	#2
	retis.h	#200

	retiu	r9
	retiu.h	[r9]
	retiu	[r9+]
	retiu.h	[-r9]
	retiu	10000[r9]
	retiu.h	[10000[r9]]
	retiu	20000[10000[r9]]
	retiu.h	/23456789h
	retiu	[/23456789h]
	retiu.h	[r9](r10)
	retiu	10000[r9](r10)
	retiu.h	[pc](r10)
	retiu	[10000[r9]](r10)
	retiu.h	[[pc]](r10)
	retiu	/23456789h(r10)
	retiu.h	[/23456789h](r10)
	retiu	#2
	retiu.h	#200

	rot.b	r7,r9		; Format I (d=0)
	rot.b	r7,[r9]		; Format I (d=0)
	rot.b	[r7],r9		; Format I (d=1)
	rot.b	[r7],[r9]
	rot.b	[r7+],[r9+]
	rot.b	[-r7],[-r9]
	rot.b	10[r7],10000[r9]
	rot.b	[10[r7]],[10000[r9]]
	rot.b	20[10[r7]],20000[10000[r9]]
	rot.b	/12345678h,/23456789h
	rot.b	[/12345678h],[/23456789h]
	rot.b	[r7](r8),[r9](r10)
	rot.b	10[r7](r8),10000[r9](r10)
	rot.b	[pc](r8),[pc](r10)
	rot.b	[10[r7]](r8),[10000[r9]](r10)
	rot.b	[[pc]](r8),[[pc]](r10)
	rot.b	/12345678h(r8),/23456789h(r10)
	rot.b	[/12345678h](r8),[/23456789h](r10)
	rot.b	#2,r9
	rot.b	#-2,r9
	rot.b	#20,r9
	rot.b	#-20,r9

	rot.h	r7,r9		; Format I (d=0)
	rot.h	r7,[r9]		; Format I (d=0)
	rot.h	[r7],r9		; Format I (d=1)
	rot.h	[r7],[r9]
	rot.h	[r7+],[r9+]
	rot.h	[-r7],[-r9]
	rot.h	10[r7],10000[r9]
	rot.h	[10[r7]],[10000[r9]]
	rot.h	20[10[r7]],20000[10000[r9]]
	rot.h	/12345678h,/23456789h
	rot.h	[/12345678h],[/23456789h]
	rot.h	[r7](r8),[r9](r10)
	rot.h	10[r7](r8),10000[r9](r10)
	rot.h	[pc](r8),[pc](r10)
	rot.h	[10[r7]](r8),[10000[r9]](r10)
	rot.h	[[pc]](r8),[[pc]](r10)
	rot.h	/12345678h(r8),/23456789h(r10)
	rot.h	[/12345678h](r8),[/23456789h](r10)
	rot.h	#2,r9
	rot.h	#-2,r9
	rot.h	#20,r9
	rot.h	#-20,r9

	rot.w	r7,r9		; Format I (d=0)
	rot.w	r7,[r9]		; Format I (d=0)
	rot.w	[r7],r9		; Format I (d=1)
	rot.w	[r7],[r9]
	rot.w	[r7+],[r9+]
	rot.w	[-r7],[-r9]
	rot.w	10[r7],10000[r9]
	rot.w	[10[r7]],[10000[r9]]
	rot.w	20[10[r7]],20000[10000[r9]]
	rot.w	/12345678h,/23456789h
	rot.w	[/12345678h],[/23456789h]
	rot.w	[r7](r8),[r9](r10)
	rot.w	10[r7](r8),10000[r9](r10)
	rot.w	[pc](r8),[pc](r10)
	rot.w	[10[r7]](r8),[10000[r9]](r10)
	rot.w	[[pc]](r8),[[pc]](r10)
	rot.w	/12345678h(r8),/23456789h(r10)
	rot.w	[/12345678h](r8),[/23456789h](r10)
	rot.w	#2,r9
	rot.w	#-2,r9
	rot.w	#20,r9
	rot.w	#-20,r9

	rotc.b	r7,r9		; Format I (d=0)
	rotc.b	r7,[r9]		; Format I (d=0)
	rotc.b	[r7],r9		; Format I (d=1)
	rotc.b	[r7],[r9]
	rotc.b	[r7+],[r9+]
	rotc.b	[-r7],[-r9]
	rotc.b	10[r7],10000[r9]
	rotc.b	[10[r7]],[10000[r9]]
	rotc.b	20[10[r7]],20000[10000[r9]]
	rotc.b	/12345678h,/23456789h
	rotc.b	[/12345678h],[/23456789h]
	rotc.b	[r7](r8),[r9](r10)
	rotc.b	10[r7](r8),10000[r9](r10)
	rotc.b	[pc](r8),[pc](r10)
	rotc.b	[10[r7]](r8),[10000[r9]](r10)
	rotc.b	[[pc]](r8),[[pc]](r10)
	rotc.b	/12345678h(r8),/23456789h(r10)
	rotc.b	[/12345678h](r8),[/23456789h](r10)
	rotc.b	#2,r9
	rotc.b	#-2,r9
	rotc.b	#20,r9
	rotc.b	#-20,r9

	rotc.h	r7,r9		; Format I (d=0)
	rotc.h	r7,[r9]		; Format I (d=0)
	rotc.h	[r7],r9		; Format I (d=1)
	rotc.h	[r7],[r9]
	rotc.h	[r7+],[r9+]
	rotc.h	[-r7],[-r9]
	rotc.h	10[r7],10000[r9]
	rotc.h	[10[r7]],[10000[r9]]
	rotc.h	20[10[r7]],20000[10000[r9]]
	rotc.h	/12345678h,/23456789h
	rotc.h	[/12345678h],[/23456789h]
	rotc.h	[r7](r8),[r9](r10)
	rotc.h	10[r7](r8),10000[r9](r10)
	rotc.h	[pc](r8),[pc](r10)
	rotc.h	[10[r7]](r8),[10000[r9]](r10)
	rotc.h	[[pc]](r8),[[pc]](r10)
	rotc.h	/12345678h(r8),/23456789h(r10)
	rotc.h	[/12345678h](r8),[/23456789h](r10)
	rotc.h	#2,r9
	rotc.h	#-2,r9
	rotc.h	#20,r9
	rotc.h	#-20,r9

	rotc.w	r7,r9		; Format I (d=0)
	rotc.w	r7,[r9]		; Format I (d=0)
	rotc.w	[r7],r9		; Format I (d=1)
	rotc.w	[r7],[r9]
	rotc.w	[r7+],[r9+]
	rotc.w	[-r7],[-r9]
	rotc.w	10[r7],10000[r9]
	rotc.w	[10[r7]],[10000[r9]]
	rotc.w	20[10[r7]],20000[10000[r9]]
	rotc.w	/12345678h,/23456789h
	rotc.w	[/12345678h],[/23456789h]
	rotc.w	[r7](r8),[r9](r10)
	rotc.w	10[r7](r8),10000[r9](r10)
	rotc.w	[pc](r8),[pc](r10)
	rotc.w	[10[r7]](r8),[10000[r9]](r10)
	rotc.w	[[pc]](r8),[[pc]](r10)
	rotc.w	/12345678h(r8),/23456789h(r10)
	rotc.w	[/12345678h](r8),[/23456789h](r10)
	rotc.w	#2,r9
	rotc.w	#-2,r9
	rotc.w	#20,r9
	rotc.w	#-20,r9

	rsr

	rvbit.b	r7,r9
	rvbit	[r7],[r9]
	rvbit.b	[r7+],[r9+]
	rvbit	[-r7],[-r9]
	rvbit.b	10[r7],10000[r9]
	rvbit	[10[r7]],[10000[r9]]
	rvbit.b	20[10[r7]],20000[10000[r9]]
	rvbit	/12345678h,/23456789h
	rvbit.b	[/12345678h],[/23456789h]
	rvbit	[r7](r8),[r9](r10)
	rvbit.b	10[r7](r8),10000[r9](r10)
	rvbit	[pc](r8),[pc](r10)
	rvbit.b	[10[r7]](r8),[10000[r9]](r10)
	rvbit	[[pc]](r8),[[pc]](r10)
	rvbit.b	/12345678h(r8),/23456789h(r10)
	rvbit	[/12345678h](r8),[/23456789h](r10)
	rvbit.b	#5,[r9]
	rvbit	#200,r9
	expect	1350
	rvbit.b	#200,#100
	endexpect

	rvbyte.w	r7,r9
	rvbyte		[r7],[r9]
	rvbyte.w	[r7+],[r9+]
	rvbyte		[-r7],[-r9]
	rvbyte.w	10[r7],10000[r9]
	rvbyte		[10[r7]],[10000[r9]]
	rvbyte.w	20[10[r7]],20000[10000[r9]]
	rvbyte		/12345678h,/23456789h
	rvbyte.w	[/12345678h],[/23456789h]
	rvbyte		[r7](r8),[r9](r10)
	rvbyte.w	10[r7](r8),10000[r9](r10)
	rvbyte		[pc](r8),[pc](r10)
	rvbyte.w	[10[r7]](r8),[10000[r9]](r10)
	rvbyte		[[pc]](r8),[[pc]](r10)
	rvbyte.w	/12345678h(r8),/23456789h(r10)
	rvbyte		[/12345678h](r8),[/23456789h](r10)
	rvbyte.w	#5,[r9]
	rvbyte		#200,r9
	expect		1350
	rvbyte.w	#200,#100
	endexpect

	expect	1100,1350,1350
	sch0bsu.w @r7,#3,r9
	sch0bsu	@r7,#11,r9
	sch0bsu	#5,r20,r9
	endexpect
	sch0bsu	@[r7],#3,[r9]
	sch0bsu	@[r7+],#11,[r9+]
	sch0bsu	@[-r7],r20,[-r9]
	sch0bsu	10@[r7],#3,10000[r9]
	sch0bsu	@[10[r7]],#11,[10000[r9]]
	sch0bsu	20@[10[r7]],r20,20000[10000[r9]]
	sch0bsu	@/12345678h,#3,/23456789h
	sch0bsu	@[/12345678h],#11,[/23456789h]
	sch0bsu	r8@[r7],r20,[r9](r10)
	sch0bsu	r8@10[r7],#3,10000[r9](r10)
	sch0bsu	r8@[pc],#11,[pc](r10)
	sch0bsu	r8@[10[r7]],r20,[10000[r9]](r10)
	sch0bsu	r8@[[pc]],#3,[[pc]](r10)
	sch0bsu	r8@/12345678h,#11,/23456789h(r10)
	sch0bsu	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	sch0bsu	@[r7],#3,#2
	sch0bsu	@[r7],#11,#200
	endexpect

	expect	1100,1350,1350
	sch0bsd.w @r7,#3,r9
	sch0bsd	@r7,#11,r9
	sch0bsd	#5,r20,r9
	endexpect
	sch0bsd	@[r7],#3,[r9]
	sch0bsd	@[r7+],#11,[r9+]
	sch0bsd	@[-r7],r20,[-r9]
	sch0bsd	10@[r7],#3,10000[r9]
	sch0bsd	@[10[r7]],#11,[10000[r9]]
	sch0bsd	20@[10[r7]],r20,20000[10000[r9]]
	sch0bsd	@/12345678h,#3,/23456789h
	sch0bsd	@[/12345678h],#11,[/23456789h]
	sch0bsd	r8@[r7],r20,[r9](r10)
	sch0bsd	r8@10[r7],#3,10000[r9](r10)
	sch0bsd	r8@[pc],#11,[pc](r10)
	sch0bsd	r8@[10[r7]],r20,[10000[r9]](r10)
	sch0bsd	r8@[[pc]],#3,[[pc]](r10)
	sch0bsd	r8@/12345678h,#11,/23456789h(r10)
	sch0bsd	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	sch0bsd	@[r7],#3,#2
	sch0bsd	@[r7],#11,#200
	endexpect

	expect	1100,1350,1350
	sch1bsu.w @r7,#3,r9
	sch1bsu	@r7,#11,r9
	sch1bsu	#5,r20,r9
	endexpect
	sch1bsu	@[r7],#3,[r9]
	sch1bsu	@[r7+],#11,[r9+]
	sch1bsu	@[-r7],r20,[-r9]
	sch1bsu	10@[r7],#3,10000[r9]
	sch1bsu	@[10[r7]],#11,[10000[r9]]
	sch1bsu	20@[10[r7]],r20,20000[10000[r9]]
	sch1bsu	@/12345678h,#3,/23456789h
	sch1bsu	@[/12345678h],#11,[/23456789h]
	sch1bsu	r8@[r7],r20,[r9](r10)
	sch1bsu	r8@10[r7],#3,10000[r9](r10)
	sch1bsu	r8@[pc],#11,[pc](r10)
	sch1bsu	r8@[10[r7]],r20,[10000[r9]](r10)
	sch1bsu	r8@[[pc]],#3,[[pc]](r10)
	sch1bsu	r8@/12345678h,#11,/23456789h(r10)
	sch1bsu	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	sch1bsu	@[r7],#3,#2
	sch1bsu	@[r7],#11,#200
	endexpect

	expect	1100,1350,1350
	sch1bsd.w @r7,#3,r9
	sch1bsd	@r7,#11,r9
	sch1bsd	#5,r20,r9
	endexpect
	sch1bsd	@[r7],#3,[r9]
	sch1bsd	@[r7+],#11,[r9+]
	sch1bsd	@[-r7],r20,[-r9]
	sch1bsd	10@[r7],#3,10000[r9]
	sch1bsd	@[10[r7]],#11,[10000[r9]]
	sch1bsd	20@[10[r7]],r20,20000[10000[r9]]
	sch1bsd	@/12345678h,#3,/23456789h
	sch1bsd	@[/12345678h],#11,[/23456789h]
	sch1bsd	r8@[r7],r20,[r9](r10)
	sch1bsd	r8@10[r7],#3,10000[r9](r10)
	sch1bsd	r8@[pc],#11,[pc](r10)
	sch1bsd	r8@[10[r7]],r20,[10000[r9]](r10)
	sch1bsd	r8@[[pc]],#3,[[pc]](r10)
	sch1bsd	r8@/12345678h,#11,/23456789h(r10)
	sch1bsd	r8@[/12345678h],r20,[/23456789h](r10)
	expect	1350,1350
	sch1bsd	@[r7],#3,#2
	sch1bsd	@[r7],#11,#200
	endexpect

	expect	1350,1350,1350
	schcu.b r7,#3,r9
	schcu	r7,#11,r9
	schcu.b	#5,r20,r9
	endexpect
	schcu	[r7],#3,[r9]
	schcu.b	[r7+],#11,[r9+]
	schcu	[-r7],r20,[-r9]
	schcu.b	10[r7],#3,10000[r9]
	schcu	[10[r7]],#11,[10000[r9]]
	schcu.b	20[10[r7]],r20,20000[10000[r9]]
	schcu	/12345678h,#3,/23456789h
	schcu.b	[/12345678h],#11,[/23456789h]
	schcu	[r7](r8),r20,[r9](r10)
	schcu.b	10[r7](r8),#3,10000[r9](r10)
	schcu	[pc](r8),#11,[pc](r10)
	schcu.b	[10[r7]](r8),r20,[10000[r9]](r10)
	schcu	[[pc]](r8),#3,[[pc]](r10)
	schcu.b	/12345678h(r8),#11,/23456789h(r10)
	schcu	[/12345678h](r8),r20,[/23456789h](r10)
	schcu.b	[r7],#3,#2
	schcu	[r7],#11,#200

	expect	1350,1350,1350
	schcu.h r7,#3,r9
	schcu.h	r7,#11,r9
	schcu.h	#5,r20,r9
	endexpect
	schcu.h	[r7],#3,[r9]
	schcu.h	[r7+],#11,[r9+]
	schcu.h	[-r7],r20,[-r9]
	schcu.h	10[r7],#3,10000[r9]
	schcu.h	[10[r7]],#11,[10000[r9]]
	schcu.h	20[10[r7]],r20,20000[10000[r9]]
	schcu.h	/12345678h,#3,/23456789h
	schcu.h	[/12345678h],#11,[/23456789h]
	schcu.h	[r7](r8),r20,[r9](r10)
	schcu.h	10[r7](r8),#3,10000[r9](r10)
	schcu.h	[pc](r8),#11,[pc](r10)
	schcu.h	[10[r7]](r8),r20,[10000[r9]](r10)
	schcu.h	[[pc]](r8),#3,[[pc]](r10)
	schcu.h	/12345678h(r8),#11,/23456789h(r10)
	schcu.h	[/12345678h](r8),r20,[/23456789h](r10)
	schcu.h	[r7],#3,#2
	schcu.h	[r7],#11,#200

	expect	1350,1350,1350
	schcd.b r7,#3,r9
	schcd	r7,#11,r9
	schcd.b	#5,r20,r9
	endexpect
	schcd	[r7],#3,[r9]
	schcd.b	[r7+],#11,[r9+]
	schcd	[-r7],r20,[-r9]
	schcd.b	10[r7],#3,10000[r9]
	schcd	[10[r7]],#11,[10000[r9]]
	schcd.b	20[10[r7]],r20,20000[10000[r9]]
	schcd	/12345678h,#3,/23456789h
	schcd.b	[/12345678h],#11,[/23456789h]
	schcd	[r7](r8),r20,[r9](r10)
	schcd.b	10[r7](r8),#3,10000[r9](r10)
	schcd	[pc](r8),#11,[pc](r10)
	schcd.b	[10[r7]](r8),r20,[10000[r9]](r10)
	schcd	[[pc]](r8),#3,[[pc]](r10)
	schcd.b	/12345678h(r8),#11,/23456789h(r10)
	schcd	[/12345678h](r8),r20,[/23456789h](r10)
	schcd.b	[r7],#3,#2
	schcd	[r7],#11,#200

	expect	1350,1350,1350
	schcd.h r7,#3,r9
	schcd.h	r7,#11,r9
	schcd.h	#5,r20,r9
	endexpect
	schcd.h	[r7],#3,[r9]
	schcd.h	[r7+],#11,[r9+]
	schcd.h	[-r7],r20,[-r9]
	schcd.h	10[r7],#3,10000[r9]
	schcd.h	[10[r7]],#11,[10000[r9]]
	schcd.h	20[10[r7]],r20,20000[10000[r9]]
	schcd.h	/12345678h,#3,/23456789h
	schcd.h	[/12345678h],#11,[/23456789h]
	schcd.h	[r7](r8),r20,[r9](r10)
	schcd.h	10[r7](r8),#3,10000[r9](r10)
	schcd.h	[pc](r8),#11,[pc](r10)
	schcd.h	[10[r7]](r8),r20,[10000[r9]](r10)
	schcd.h	[[pc]](r8),#3,[[pc]](r10)
	schcd.h	/12345678h(r8),#11,/23456789h(r10)
	schcd.h	[/12345678h](r8),r20,[/23456789h](r10)
	schcd.h	[r7],#3,#2
	schcd.h	[r7],#11,#200

	sclf.s	r7,r9
	sclf	[r7],[r9]
	sclf.s	[r7+],[r9+]
	sclf	[-r7],[-r9]
	sclf.s	10[r7],10000[r9]
	sclf	[10[r7]],[10000[r9]]
	sclf.s	20[10[r7]],20000[10000[r9]]
	sclf	/12345678h,/23456789h
	sclf.s	[/12345678h],[/23456789h]
	sclf	[r7](r8),[r9](r10)
	sclf.s	10[r7](r8),10000[r9](r10)
	sclf	[pc](r8),[pc](r10)
	sclf.s	[10[r7]](r8),[10000[r9]](r10)
	sclf	[[pc]](r8),[[pc]](r10)
	sclf.s	/12345678h(r8),/23456789h(r10)
	sclf	[/12345678h](r8),[/23456789h](r10)
	sclf.s	#4,r9
	sclf	#50,r9
	expect	1350
	sclf.s	#50,#100
	endexpect

	sclf.l	r7,r9
	sclf.l	[r7],[r9]
	sclf.l	[r7+],[r9+]
	sclf.l	[-r7],[-r9]
	sclf.l	10[r7],10000[r9]
	sclf.l	[10[r7]],[10000[r9]]
	sclf.l	20[10[r7]],20000[10000[r9]]
	sclf.l	/12345678h,/23456789h
	sclf.l	[/12345678h],[/23456789h]
	sclf.l	[r7](r8),[r9](r10)
	sclf.l	10[r7](r8),10000[r9](r10)
	sclf.l	[pc](r8),[pc](r10)
	sclf.l	[10[r7]](r8),[10000[r9]](r10)
	sclf.l	[[pc]](r8),[[pc]](r10)
	sclf.l	/12345678h(r8),/23456789h(r10)
	sclf.l	[/12345678h](r8),[/23456789h](r10)
	sclf.l	#4,r9
	sclf.l	#50,r9
	expect	1350
	sclf.l	#50,#100
	endexpect

	expect	1100,1350
	set1.b	r7,#55
	set1	r7,#55
	endexpect
	set1	r7,r9
	set1	r7,[r9]
	set1	[r7],r9
	set1	[r7],[r9]
	set1	[r7+],[r9+]
	set1	[-r7],[-r9]
	set1	10[r7],10000[r9]
	set1	[10[r7]],[10000[r9]]
	set1	20[10[r7]],20000[10000[r9]]
	set1	/12345678h,/23456789h
	set1	[/12345678h],[/23456789h]
	set1	[r7](r8),[r9](r10)
	set1	10[r7](r8),10000[r9](r10)
	set1	[pc](r8),[pc](r10)
	set1	[10[r7]](r8),[10000[r9]](r10)
	set1	[[pc]](r8),[[pc]](r10)
	set1	/12345678h(r8),/23456789h(r10)
	set1	[/12345678h](r8),[/23456789h](r10)
	set1	#3,r9
	set1	#30,r9

	setf.b	r7,r9
	setf	[r7],[r9]
	setf.b	[r7+],[r9+]
	setf	[-r7],[-r9]
	setf.b	10[r7],10000[r9]
	setf	[10[r7]],[10000[r9]]
	setf.b	20[10[r7]],20000[10000[r9]]
	setf	/12345678h,/23456789h
	setf.b	[/12345678h],[/23456789h]
	setf	[r7](r8),[r9](r10)
	setf.b	10[r7](r8),10000[r9](r10)
	setf	[pc](r8),[pc](r10)
	setf.b	[10[r7]](r8),[10000[r9]](r10)
	setf	[[pc]](r8),[[pc]](r10)
	setf.b	/12345678h(r8),/23456789h(r10)
	setf	[/12345678h](r8),[/23456789h](r10)
	setf.b	#ge,r9
	setf	#50,r9
	expect	1350
	setf.b	#50,#100
	endexpect

	sha.b	r7,r9		; Format I (d=0)
	sha.b	r7,[r9]		; Format I (d=0)
	sha.b	[r7],r9		; Format I (d=1)
	sha.b	[r7],[r9]
	sha.b	[r7+],[r9+]
	sha.b	[-r7],[-r9]
	sha.b	10[r7],10000[r9]
	sha.b	[10[r7]],[10000[r9]]
	sha.b	20[10[r7]],20000[10000[r9]]
	sha.b	/12345678h,/23456789h
	sha.b	[/12345678h],[/23456789h]
	sha.b	[r7](r8),[r9](r10)
	sha.b	10[r7](r8),10000[r9](r10)
	sha.b	[pc](r8),[pc](r10)
	sha.b	[10[r7]](r8),[10000[r9]](r10)
	sha.b	[[pc]](r8),[[pc]](r10)
	sha.b	/12345678h(r8),/23456789h(r10)
	sha.b	[/12345678h](r8),[/23456789h](r10)
	sha.b	#2,r9
	sha.b	#-2,r9
	sha.b	#20,r9
	sha.b	#-20,r9

	sha.h	r7,r9		; Format I (d=0)
	sha.h	r7,[r9]		; Format I (d=0)
	sha.h	[r7],r9		; Format I (d=1)
	sha.h	[r7],[r9]
	sha.h	[r7+],[r9+]
	sha.h	[-r7],[-r9]
	sha.h	10[r7],10000[r9]
	sha.h	[10[r7]],[10000[r9]]
	sha.h	20[10[r7]],20000[10000[r9]]
	sha.h	/12345678h,/23456789h
	sha.h	[/12345678h],[/23456789h]
	sha.h	[r7](r8),[r9](r10)
	sha.h	10[r7](r8),10000[r9](r10)
	sha.h	[pc](r8),[pc](r10)
	sha.h	[10[r7]](r8),[10000[r9]](r10)
	sha.h	[[pc]](r8),[[pc]](r10)
	sha.h	/12345678h(r8),/23456789h(r10)
	sha.h	[/12345678h](r8),[/23456789h](r10)
	sha.h	#2,r9
	sha.h	#-2,r9
	sha.h	#20,r9
	sha.h	#-20,r9

	sha.w	r7,r9		; Format I (d=0)
	sha.w	r7,[r9]		; Format I (d=0)
	sha.w	[r7],r9		; Format I (d=1)
	sha.w	[r7],[r9]
	sha.w	[r7+],[r9+]
	sha.w	[-r7],[-r9]
	sha.w	10[r7],10000[r9]
	sha.w	[10[r7]],[10000[r9]]
	sha.w	20[10[r7]],20000[10000[r9]]
	sha.w	/12345678h,/23456789h
	sha.w	[/12345678h],[/23456789h]
	sha.w	[r7](r8),[r9](r10)
	sha.w	10[r7](r8),10000[r9](r10)
	sha.w	[pc](r8),[pc](r10)
	sha.w	[10[r7]](r8),[10000[r9]](r10)
	sha.w	[[pc]](r8),[[pc]](r10)
	sha.w	/12345678h(r8),/23456789h(r10)
	sha.w	[/12345678h](r8),[/23456789h](r10)
	sha.w	#2,r9
	sha.w	#-2,r9
	sha.w	#20,r9
	sha.w	#-20,r9

	shl.b	r7,r9		; Format I (d=0)
	shl.b	r7,[r9]		; Format I (d=0)
	shl.b	[r7],r9		; Format I (d=1)
	shl.b	[r7],[r9]
	shl.b	[r7+],[r9+]
	shl.b	[-r7],[-r9]
	shl.b	10[r7],10000[r9]
	shl.b	[10[r7]],[10000[r9]]
	shl.b	20[10[r7]],20000[10000[r9]]
	shl.b	/12345678h,/23456789h
	shl.b	[/12345678h],[/23456789h]
	shl.b	[r7](r8),[r9](r10)
	shl.b	10[r7](r8),10000[r9](r10)
	shl.b	[pc](r8),[pc](r10)
	shl.b	[10[r7]](r8),[10000[r9]](r10)
	shl.b	[[pc]](r8),[[pc]](r10)
	shl.b	/12345678h(r8),/23456789h(r10)
	shl.b	[/12345678h](r8),[/23456789h](r10)
	shl.b	#2,r9
	shl.b	#-2,r9
	shl.b	#20,r9
	shl.b	#-20,r9

	shl.h	r7,r9		; Format I (d=0)
	shl.h	r7,[r9]		; Format I (d=0)
	shl.h	[r7],r9		; Format I (d=1)
	shl.h	[r7],[r9]
	shl.h	[r7+],[r9+]
	shl.h	[-r7],[-r9]
	shl.h	10[r7],10000[r9]
	shl.h	[10[r7]],[10000[r9]]
	shl.h	20[10[r7]],20000[10000[r9]]
	shl.h	/12345678h,/23456789h
	shl.h	[/12345678h],[/23456789h]
	shl.h	[r7](r8),[r9](r10)
	shl.h	10[r7](r8),10000[r9](r10)
	shl.h	[pc](r8),[pc](r10)
	shl.h	[10[r7]](r8),[10000[r9]](r10)
	shl.h	[[pc]](r8),[[pc]](r10)
	shl.h	/12345678h(r8),/23456789h(r10)
	shl.h	[/12345678h](r8),[/23456789h](r10)
	shl.h	#2,r9
	shl.h	#-2,r9
	shl.h	#20,r9
	shl.h	#-20,r9

	shl.w	r7,r9		; Format I (d=0)
	shl.w	r7,[r9]		; Format I (d=0)
	shl.w	[r7],r9		; Format I (d=1)
	shl.w	[r7],[r9]
	shl.w	[r7+],[r9+]
	shl.w	[-r7],[-r9]
	shl.w	10[r7],10000[r9]
	shl.w	[10[r7]],[10000[r9]]
	shl.w	20[10[r7]],20000[10000[r9]]
	shl.w	/12345678h,/23456789h
	shl.w	[/12345678h],[/23456789h]
	shl.w	[r7](r8),[r9](r10)
	shl.w	10[r7](r8),10000[r9](r10)
	shl.w	[pc](r8),[pc](r10)
	shl.w	[10[r7]](r8),[10000[r9]](r10)
	shl.w	[[pc]](r8),[[pc]](r10)
	shl.w	/12345678h(r8),/23456789h(r10)
	shl.w	[/12345678h](r8),[/23456789h](r10)
	shl.w	#2,r9
	shl.w	#-2,r9
	shl.w	#20,r9
	shl.w	#-20,r9

	expect	1350,1350,1350
	skpcu.b r7,#3,r9
	skpcu	r7,#11,r9
	skpcu.b	#5,r20,r9
	endexpect
	skpcu	[r7],#3,[r9]
	skpcu.b	[r7+],#11,[r9+]
	skpcu	[-r7],r20,[-r9]
	skpcu.b	10[r7],#3,10000[r9]
	skpcu	[10[r7]],#11,[10000[r9]]
	skpcu.b	20[10[r7]],r20,20000[10000[r9]]
	skpcu	/12345678h,#3,/23456789h
	skpcu.b	[/12345678h],#11,[/23456789h]
	skpcu	[r7](r8),r20,[r9](r10)
	skpcu.b	10[r7](r8),#3,10000[r9](r10)
	skpcu	[pc](r8),#11,[pc](r10)
	skpcu.b	[10[r7]](r8),r20,[10000[r9]](r10)
	skpcu	[[pc]](r8),#3,[[pc]](r10)
	skpcu.b	/12345678h(r8),#11,/23456789h(r10)
	skpcu	[/12345678h](r8),r20,[/23456789h](r10)
	skpcu.b	[r7],#3,#2
	skpcu	[r7],#11,#200

	expect	1350,1350,1350
	skpcu.h r7,#3,r9
	skpcu.h	r7,#11,r9
	skpcu.h	#5,r20,r9
	endexpect
	skpcu.h	[r7],#3,[r9]
	skpcu.h	[r7+],#11,[r9+]
	skpcu.h	[-r7],r20,[-r9]
	skpcu.h	10[r7],#3,10000[r9]
	skpcu.h	[10[r7]],#11,[10000[r9]]
	skpcu.h	20[10[r7]],r20,20000[10000[r9]]
	skpcu.h	/12345678h,#3,/23456789h
	skpcu.h	[/12345678h],#11,[/23456789h]
	skpcu.h	[r7](r8),r20,[r9](r10)
	skpcu.h	10[r7](r8),#3,10000[r9](r10)
	skpcu.h	[pc](r8),#11,[pc](r10)
	skpcu.h	[10[r7]](r8),r20,[10000[r9]](r10)
	skpcu.h	[[pc]](r8),#3,[[pc]](r10)
	skpcu.h	/12345678h(r8),#11,/23456789h(r10)
	skpcu.h	[/12345678h](r8),r20,[/23456789h](r10)
	skpcu.h	[r7],#3,#2
	skpcu.h	[r7],#11,#200

	expect	1350,1350,1350
	skpcd.b r7,#3,r9
	skpcd	r7,#11,r9
	skpcd.b	#5,r20,r9
	endexpect
	skpcd	[r7],#3,[r9]
	skpcd.b	[r7+],#11,[r9+]
	skpcd	[-r7],r20,[-r9]
	skpcd.b	10[r7],#3,10000[r9]
	skpcd	[10[r7]],#11,[10000[r9]]
	skpcd.b	20[10[r7]],r20,20000[10000[r9]]
	skpcd	/12345678h,#3,/23456789h
	skpcd.b	[/12345678h],#11,[/23456789h]
	skpcd	[r7](r8),r20,[r9](r10)
	skpcd.b	10[r7](r8),#3,10000[r9](r10)
	skpcd	[pc](r8),#11,[pc](r10)
	skpcd.b	[10[r7]](r8),r20,[10000[r9]](r10)
	skpcd	[[pc]](r8),#3,[[pc]](r10)
	skpcd.b	/12345678h(r8),#11,/23456789h(r10)
	skpcd	[/12345678h](r8),r20,[/23456789h](r10)
	skpcd.b	[r7],#3,#2
	skpcd	[r7],#11,#200

	expect	1350,1350,1350
	skpcd.h r7,#3,r9
	skpcd.h	r7,#11,r9
	skpcd.h	#5,r20,r9
	endexpect
	skpcd.h	[r7],#3,[r9]
	skpcd.h	[r7+],#11,[r9+]
	skpcd.h	[-r7],r20,[-r9]
	skpcd.h	10[r7],#3,10000[r9]
	skpcd.h	[10[r7]],#11,[10000[r9]]
	skpcd.h	20[10[r7]],r20,20000[10000[r9]]
	skpcd.h	/12345678h,#3,/23456789h
	skpcd.h	[/12345678h],#11,[/23456789h]
	skpcd.h	[r7](r8),r20,[r9](r10)
	skpcd.h	10[r7](r8),#3,10000[r9](r10)
	skpcd.h	[pc](r8),#11,[pc](r10)
	skpcd.h	[10[r7]](r8),r20,[10000[r9]](r10)
	skpcd.h	[[pc]](r8),#3,[[pc]](r10)
	skpcd.h	/12345678h(r8),#11,/23456789h(r10)
	skpcd.h	[/12345678h](r8),r20,[/23456789h](r10)
	skpcd.h	[r7],#3,#2
	skpcd.h	[r7],#11,#200

	stpr.w	r7,r9
	stpr	[r7],[r9]
	stpr.w	[r7+],[r9+]
	stpr	[-r7],[-r9]
	stpr.w	10[r7],10000[r9]
	stpr	[10[r7]],[10000[r9]]
	stpr.w	20[10[r7]],20000[10000[r9]]
	stpr	/12345678h,/23456789h
	stpr.w	[/12345678h],[/23456789h]
	stpr	[r7](r8),[r9](r10)
	stpr.w	10[r7](r8),10000[r9](r10)
	stpr	[pc](r8),[pc](r10)
	stpr.w	[10[r7]](r8),[10000[r9]](r10)
	stpr	[[pc]](r8),[[pc]](r10)
	stpr.w	/12345678h(r8),/23456789h(r10)
	stpr	[/12345678h](r8),[/23456789h](r10)
	stpr.w	#tr,r7
	stpr	#trmod,r9
	expect	1350,1350
	stpr.w	#tr,#2
	stpr	#trmod,#200
	endexpect

	sttask		r9
	sttask.w	[r9]
	sttask		[r9+]
	sttask.w	[-r9]
	sttask		10000[r9]
	sttask.w	[10000[r9]]
	sttask		20000[10000[r9]]
	sttask.w	/23456789h
	sttask		[/23456789h]
	sttask.w	[r9](r10)
	sttask		10000[r9](r10)
	sttask.w	[pc](r10)
	sttask		[10000[r9]](r10)
	sttask.w	[[pc]](r10)
	sttask		/23456789h(r10)
	sttask.w	[/23456789h](r10)
	sttask.w	#2
	sttask		#200

	sub.b	r7,r9
	sub.b	[r7],[r9]
	sub.b	[r7+],[r9+]
	sub.b	[-r7],[-r9]
	sub.b	10[r7],10000[r9]
	sub.b	[10[r7]],[10000[r9]]
	sub.b	20[10[r7]],20000[10000[r9]]
	sub.b	/12345678h,/23456789h
	sub.b	[/12345678h],[/23456789h]
	sub.b	[r7](r8),[r9](r10)
	sub.b	10[r7](r8),10000[r9](r10)
	sub.b	[pc](r8),[pc](r10)
	sub.b	[10[r7]](r8),[10000[r9]](r10)
	sub.b	[[pc]](r8),[[pc]](r10)
	sub.b	/12345678h(r8),/23456789h(r10)
	sub.b	[/12345678h](r8),[/23456789h](r10)
	sub.b	#2,r0
	sub.b	#200,r0

	sub.h	r7,r9
	sub.h	[r7],[r9]
	sub.h	[r7+],[r9+]
	sub.h	[-r7],[-r9]
	sub.h	10[r7],10000[r9]
	sub.h	[10[r7]],[10000[r9]]
	sub.h	20[10[r7]],20000[10000[r9]]
	sub.h	/12345678h,/23456789h
	sub.h	[/12345678h],[/23456789h]
	sub.h	[r7](r8),[r9](r10)
	sub.h	10[r7](r8),10000[r9](r10)
	sub.h	[pc](r8),[pc](r10)
	sub.h	[10[r7]](r8),[10000[r9]](r10)
	sub.h	[[pc]](r8),[[pc]](r10)
	sub.h	/12345678h(r8),/23456789h(r10)
	sub.h	[/12345678h](r8),[/23456789h](r10)
	sub.h	#2,r0
	sub.h	#200,r0

	sub.w	r7,r9
	sub.w	[r7],[r9]
	sub.w	[r7+],[r9+]
	sub.w	[-r7],[-r9]
	sub.w	10[r7],10000[r9]
	sub.w	[10[r7]],[10000[r9]]
	sub.w	20[10[r7]],20000[10000[r9]]
	sub.w	/12345678h,/23456789h
	sub.w	[/12345678h],[/23456789h]
	sub.w	[r7](r8),[r9](r10)
	sub.w	10[r7](r8),10000[r9](r10)
	sub.w	[pc](r8),[pc](r10)
	sub.w	[10[r7]](r8),[10000[r9]](r10)
	sub.w	[[pc]](r8),[[pc]](r10)
	sub.w	/12345678h(r8),/23456789h(r10)
	sub.w	[/12345678h](r8),[/23456789h](r10)
	sub.w	#2,r0
	sub.w	#200,r0

	subc.b	r7,r9
	subc.b	[r7],[r9]
	subc.b	[r7+],[r9+]
	subc.b	[-r7],[-r9]
	subc.b	10[r7],10000[r9]
	subc.b	[10[r7]],[10000[r9]]
	subc.b	20[10[r7]],20000[10000[r9]]
	subc.b	/12345678h,/23456789h
	subc.b	[/12345678h],[/23456789h]
	subc.b	[r7](r8),[r9](r10)
	subc.b	10[r7](r8),10000[r9](r10)
	subc.b	[pc](r8),[pc](r10)
	subc.b	[10[r7]](r8),[10000[r9]](r10)
	subc.b	[[pc]](r8),[[pc]](r10)
	subc.b	/12345678h(r8),/23456789h(r10)
	subc.b	[/12345678h](r8),[/23456789h](r10)
	subc.b	#2,r0
	subc.b	#200,r0

	subc.h	r7,r9
	subc.h	[r7],[r9]
	subc.h	[r7+],[r9+]
	subc.h	[-r7],[-r9]
	subc.h	10[r7],10000[r9]
	subc.h	[10[r7]],[10000[r9]]
	subc.h	20[10[r7]],20000[10000[r9]]
	subc.h	/12345678h,/23456789h
	subc.h	[/12345678h],[/23456789h]
	subc.h	[r7](r8),[r9](r10)
	subc.h	10[r7](r8),10000[r9](r10)
	subc.h	[pc](r8),[pc](r10)
	subc.h	[10[r7]](r8),[10000[r9]](r10)
	subc.h	[[pc]](r8),[[pc]](r10)
	subc.h	/12345678h(r8),/23456789h(r10)
	subc.h	[/12345678h](r8),[/23456789h](r10)
	subc.h	#2,r0
	subc.h	#200,r0

	subc.w	r7,r9
	subc.w	[r7],[r9]
	subc.w	[r7+],[r9+]
	subc.w	[-r7],[-r9]
	subc.w	10[r7],10000[r9]
	subc.w	[10[r7]],[10000[r9]]
	subc.w	20[10[r7]],20000[10000[r9]]
	subc.w	/12345678h,/23456789h
	subc.w	[/12345678h],[/23456789h]
	subc.w	[r7](r8),[r9](r10)
	subc.w	10[r7](r8),10000[r9](r10)
	subc.w	[pc](r8),[pc](r10)
	subc.w	[10[r7]](r8),[10000[r9]](r10)
	subc.w	[[pc]](r8),[[pc]](r10)
	subc.w	/12345678h(r8),/23456789h(r10)
	subc.w	[/12345678h](r8),[/23456789h](r10)
	subc.w	#2,r0
	subc.w	#200,r0

	subdc.b	r7,r9,12h
	subdc	[r7],[r9],12h
	subdc.b	[r7+],[r9+],#12h
	subdc	[-r7],[-r9],#12h
	subdc.b	10[r7],10000[r9],12h
	subdc	[10[r7]],[10000[r9]],12h
	subdc.b	20[10[r7]],20000[10000[r9]],#12h
	subdc	/12345678h,/23456789h,#12h
	subdc.b	[/12345678h],[/23456789h],12h
	subdc	[r7](r8),[r9](r10),12h
	subdc.b	10[r7](r8),10000[r9](r10),#12h
	subdc	[pc](r8),[pc](r10),#12h
	subdc.b	[10[r7]](r8),[10000[r9]](r10),12h
	subdc	[[pc]](r8),[[pc]](r10),12h
	subdc.b	/12345678h(r8),/23456789h(r10),#12h
	subdc	[/12345678h](r8),[/23456789h](r10),#12h
	subdc.b	#2,r0,12h
	subdc	#200,r0,12h

	subf.s	r7,r9
	subf.s	[r7],[r9]
	subf.s	[r7+],[r9+]
	subf.s	[-r7],[-r9]
	subf.s	10[r7],10000[r9]
	subf.s	[10[r7]],[10000[r9]]
	subf.s	20[10[r7]],20000[10000[r9]]
	subf.s	/12345678h,/23456789h
	subf.s	[/12345678h],[/23456789h]
	subf.s	[r7](r8),[r9](r10)
	subf.s	10[r7](r8),10000[r9](r10)
	subf.s	[pc](r8),[pc](r10)
	subf.s	[10[r7]](r8),[10000[r9]](r10)
	subf.s	[[pc]](r8),[[pc]](r10)
	subf.s	/12345678h(r8),/23456789h(r10)
	subf.s	[/12345678h](r8),[/23456789h](r10)

	subf.l	r7,r9
	subf.l	[r7],[r9]
	subf.l	[r7+],[r9+]
	subf.l	[-r7],[-r9]
	subf.l	10[r7],10000[r9]
	subf.l	[10[r7]],[10000[r9]]
	subf.l	20[10[r7]],20000[10000[r9]]
	subf.l	/12345678h,/23456789h
	subf.l	[/12345678h],[/23456789h]
	subf.l	[r7](r8),[r9](r10)
	subf.l	10[r7](r8),10000[r9](r10)
	subf.l	[pc](r8),[pc](r10)
	subf.l	[10[r7]](r8),[10000[r9]](r10)
	subf.l	[[pc]](r8),[[pc]](r10)
	subf.l	/12345678h(r8),/23456789h(r10)
	subf.l	[/12345678h](r8),[/23456789h](r10)

	subrdc.b	r7,r9,12h
	subrdc		[r7],[r9],12h
	subrdc.b	[r7+],[r9+],#12h
	subrdc		[-r7],[-r9],#12h
	subrdc.b	10[r7],10000[r9],12h
	subrdc		[10[r7]],[10000[r9]],12h
	subrdc.b	20[10[r7]],20000[10000[r9]],#12h
	subrdc		/12345678h,/23456789h,#12h
	subrdc.b	[/12345678h],[/23456789h],12h
	subrdc		[r7](r8),[r9](r10),12h
	subrdc.b	10[r7](r8),10000[r9](r10),#12h
	subrdc		[pc](r8),[pc](r10),#12h
	subrdc.b	[10[r7]](r8),[10000[r9]](r10),12h
	subrdc		[[pc]](r8),[[pc]](r10),12h
	subrdc.b	/12345678h(r8),/23456789h(r10),#12h
	subrdc		[/12345678h](r8),[/23456789h](r10),#12h
	subrdc.b	#2,r0,12h
	subrdc		#200,r0,12h

	tasi	r9
	tasi.b	[r9]
	tasi	[r9+]
	tasi.b	[-r9]
	tasi	10000[r9]
	tasi.b	[10000[r9]]
	tasi	20000[10000[r9]]
	tasi.b	/23456789h
	tasi	[/23456789h]
	tasi.b	[r9](r10)
	tasi	10000[r9](r10)
	tasi.b	[pc](r10)
	tasi	[10000[r9]](r10)
	tasi.b	[[pc]](r10)
	tasi	/23456789h(r10)
	tasi.b	[/23456789h](r10)
	expect	1350,1350
	tasi.b	#2
	tasi	#200
	endexpect

	tb		r3,$+10
	tb		r3,$-10
	tb		r12,$+1000
	tb		r12,$-1000
	expect		1100,1100,1100,1100
	tb.s		r3,$+10
	tb.s		r3,$-10
	tb.s		r12,$+1000
	tb.s		r12,$-1000
	endexpect

	test.b	r9
	test.b	[r9]
	test.b	[r9+]
	test.b	[-r9]
	test.b	10000[r9]
	test.b	[10000[r9]]
	test.b	20000[10000[r9]]
	test.b	/23456789h
	test.b	[/23456789h]
	test.b	[r9](r10)
	test.b	10000[r9](r10)
	test.b	[pc](r10)
	test.b	[10000[r9]](r10)
	test.b	[[pc]](r10)
	test.b	/23456789h(r10)
	test.b	[/23456789h](r10)
	test.b	#2
	test.b	#200

	test.h	r9
	test.h	[r9]
	test.h	[r9+]
	test.h	[-r9]
	test.h	10000[r9]
	test.h	[10000[r9]]
	test.h	20000[10000[r9]]
	test.h	/23456789h
	test.h	[/23456789h]
	test.h	[r9](r10)
	test.h	10000[r9](r10)
	test.h	[pc](r10)
	test.h	[10000[r9]](r10)
	test.h	[[pc]](r10)
	test.h	/23456789h(r10)
	test.h	[/23456789h](r10)
	test.h	#2
	test.h	#200

	test.w	r9
	test	[r9]
	test.w	[r9+]
	test	[-r9]
	test.w	10000[r9]
	test	[10000[r9]]
	test.w	20000[10000[r9]]
	test	/23456789h
	test.w	[/23456789h]
	test	[r9](r10)
	test.w	10000[r9](r10)
	test	[pc](r10)
	test.w	[10000[r9]](r10)
	test	[[pc]](r10)
	test.w	/23456789h(r10)
	test	[/23456789h](r10)
	test.w	#2
	test	#200

	expect	1100,1350
	test1.b	r7,#55
	test1	r7,#55
	endexpect
	test1	r7,r9
	test1	r7,[r9]
	test1	[r7],r9
	test1	[r7],[r9]
	test1	[r7+],[r9+]
	test1	[-r7],[-r9]
	test1	10[r7],10000[r9]
	test1	[10[r7]],[10000[r9]]
	test1	20[10[r7]],20000[10000[r9]]
	test1	/12345678h,/23456789h
	test1	[/12345678h],[/23456789h]
	test1	[r7](r8),[r9](r10)
	test1	10[r7](r8),10000[r9](r10)
	test1	[pc](r8),[pc](r10)
	test1	[10[r7]](r8),[10000[r9]](r10)
	test1	[[pc]](r8),[[pc]](r10)
	test1	/12345678h(r8),/23456789h(r10)
	test1	[/12345678h](r8),[/23456789h](r10)
	test1	#3,r9
	test1	#30,r9

	trap	r9
	trap.b	[r9]
	trap	[r9+]
	trap.b	[-r9]
	trap	10000[r9]
	trap.b	[10000[r9]]
	trap	20000[10000[r9]]
	trap.b	/23456789h
	trap	[/23456789h]
	trap.b	[r9](r10)
	trap	10000[r9](r10)
	trap.b	[pc](r10)
	trap	[10000[r9]](r10)
	trap.b	[[pc]](r10)
	trap	/23456789h(r10)
	trap.b	[/23456789h](r10)
	trap.b	#2
	trap	#200

	trapfl

	expect	1100,1350
	update.b	r7,#55
	update	r7,#55
	endexpect
	update	r7,r9
	update	r7,[r9]
	update	[r7],r9
	update	[r7],[r9]
	update	[r7+],[r9+]
	update	[-r7],[-r9]
	update	10[r7],10000[r9]
	update	[10[r7]],[10000[r9]]
	update	20[10[r7]],20000[10000[r9]]
	update	/12345678h,/23456789h
	update	[/12345678h],[/23456789h]
	update	[r7](r8),[r9](r10)
	update	10[r7](r8),10000[r9](r10)
	update	[pc](r8),[pc](r10)
	update	[10[r7]](r8),[10000[r9]](r10)
	update	[[pc]](r8),[[pc]](r10)
	update	/12345678h(r8),/23456789h(r10)
	update	[/12345678h](r8),[/23456789h](r10)
	update	#3,r9
	update	#3000,r9

	updpsw		r7,r9
	updpsw.h	r7,[r9]
	updpsw		[r7],r9
	updpsw.h	[r7],[r9]
	updpsw		[r7+],[r9+]
	updpsw.h	[-r7],[-r9]
	updpsw		10[r7],10000[r9]
	updpsw.h	[10[r7]],[10000[r9]]
	updpsw		20[10[r7]],20000[10000[r9]]
	updpsw.h	/12345678h,/23456789h
	updpsw		[/12345678h],[/23456789h]
	updpsw.h	[r7](r8),[r9](r10)
	updpsw		10[r7](r8),10000[r9](r10)
	updpsw.h	[pc](r8),[pc](r10)
	updpsw		[10[r7]](r8),[10000[r9]](r10)
	updpsw.h	[[pc]](r8),[[pc]](r10)
	updpsw		/12345678h(r8),/23456789h(r10)
	updpsw.h	[/12345678h](r8),[/23456789h](r10)
	updpsw		#3,#10
	updpsw.h	#3000,#5000

	updpsw.w	r7,r9
	updpsw.w	r7,[r9]
	updpsw.w	[r7],r9
	updpsw.w	[r7],[r9]
	updpsw.w	[r7+],[r9+]
	updpsw.w	[-r7],[-r9]
	updpsw.w	10[r7],10000[r9]
	updpsw.w	[10[r7]],[10000[r9]]
	updpsw.w	20[10[r7]],20000[10000[r9]]
	updpsw.w	/12345678h,/23456789h
	updpsw.w	[/12345678h],[/23456789h]
	updpsw.w	[r7](r8),[r9](r10)
	updpsw.w	10[r7](r8),10000[r9](r10)
	updpsw.w	[pc](r8),[pc](r10)
	updpsw.w	[10[r7]](r8),[10000[r9]](r10)
	updpsw.w	[[pc]](r8),[[pc]](r10)
	updpsw.w	/12345678h(r8),/23456789h(r10)
	updpsw.w	[/12345678h](r8),[/23456789h](r10)
	updpsw.w	#3,#10
	updpsw.w	#3000,#5000

	expect	1100
	updpte.b	r7,#55
	endexpect
	updpte	r7,#55
	updpte	r7,r9
	updpte	r7,[r9]
	updpte	[r7],r9
	updpte	[r7],[r9]
	updpte	[r7+],[r9+]
	updpte	[-r7],[-r9]
	updpte	10[r7],10000[r9]
	updpte	[10[r7]],[10000[r9]]
	updpte	20[10[r7]],20000[10000[r9]]
	updpte	/12345678h,/23456789h
	updpte	[/12345678h],[/23456789h]
	updpte	[r7](r8),[r9](r10)
	updpte	10[r7](r8),10000[r9](r10)
	updpte	[pc](r8),[pc](r10)
	updpte	[10[r7]](r8),[10000[r9]](r10)
	updpte	[[pc]](r8),[[pc]](r10)
	updpte	/12345678h(r8),/23456789h(r10)
	updpte	[/12345678h](r8),[/23456789h](r10)
	updpte	#3,r9
	updpte	#3000,r9

	xch.b	r7,r9
	xch.b	r7,[r9]
	xch.b	[r9],r7
	xch.b	r7,[r9+]
	xch.b	[r9+],r7
	xch.b	r7,[-r9]
	xch.b	[-r9],r7
	xch.b	r7,10000[r9]
	xch.b	10000[r9],r7
	xch.b	r7,[10000[r9]]
	xch.b	[10000[r9]],r7
	xch.b	r7,20000[10000[r9]]
	xch.b	20000[10000[r9]],r7
	xch.b	r7,/23456789h
	xch.b	/23456789h,r7
	xch.b	r7,[/23456789h]
	xch.b	[/23456789h],r7
	xch.b	r7,[r9](r10)
	xch.b	[r9](r10),r7
	xch.b	r7,10000[r9](r10)
	xch.b	10000[r9](r10),r7
	xch.b	r7,[pc](r10)
	xch.b	[pc](r10),r7
	xch.b	r7,[10000[r9]](r10)
	xch.b	[10000[r9]](r10),r7
	xch.b	r7,[[pc]](r10)
	xch.b	[[pc]](r10),r7
	xch.b	r7,/23456789h(r10)
	xch.b	/23456789h(r10),r7
	xch.b	r7,[/23456789h](r10)
	xch.b	[/23456789h](r10),r7
	expect	1350,1350,1350,1350
	xch.b	#10,r7
	xch.b	r7,#10
	xch.b	#100,r7
	xch.b	r7,#100
	endexpect
	expect	1890
	xch.b	[r7],[r9]
	endexpect

	xch.h	r7,r9
	xch.h	r7,[r9]
	xch.h	[r9],r7
	xch.h	r7,[r9+]
	xch.h	[r9+],r7
	xch.h	r7,[-r9]
	xch.h	[-r9],r7
	xch.h	r7,10000[r9]
	xch.h	10000[r9],r7
	xch.h	r7,[10000[r9]]
	xch.h	[10000[r9]],r7
	xch.h	r7,20000[10000[r9]]
	xch.h	20000[10000[r9]],r7
	xch.h	r7,/23456789h
	xch.h	/23456789h,r7
	xch.h	r7,[/23456789h]
	xch.h	[/23456789h],r7
	xch.h	r7,[r9](r10)
	xch.h	[r9](r10),r7
	xch.h	r7,10000[r9](r10)
	xch.h	10000[r9](r10),r7
	xch.h	r7,[pc](r10)
	xch.h	[pc](r10),r7
	xch.h	r7,[10000[r9]](r10)
	xch.h	[10000[r9]](r10),r7
	xch.h	r7,[[pc]](r10)
	xch.h	[[pc]](r10),r7
	xch.h	r7,/23456789h(r10)
	xch.h	/23456789h(r10),r7
	xch.h	r7,[/23456789h](r10)
	xch.h	[/23456789h](r10),r7
	expect	1350,1350,1350,1350
	xch.h	#10,r7
	xch.h	r7,#10
	xch.h	#100,r7
	xch.h	r7,#100
	endexpect
	expect	1890
	xch.h	[r7],[r9]
	endexpect

	xch.w	r7,r9
	xch	r7,[r9]
	xch.w	[r9],r7
	xch	r7,[r9+]
	xch.w	[r9+],r7
	xch	r7,[-r9]
	xch.w	[-r9],r7
	xch	r7,10000[r9]
	xch.w	10000[r9],r7
	xch	r7,[10000[r9]]
	xch.w	[10000[r9]],r7
	xch	r7,20000[10000[r9]]
	xch.w	20000[10000[r9]],r7
	xch	r7,/23456789h
	xch.w	/23456789h,r7
	xch	r7,[/23456789h]
	xch.w	[/23456789h],r7
	xch	r7,[r9](r10)
	xch.w	[r9](r10),r7
	xch	r7,10000[r9](r10)
	xch.w	10000[r9](r10),r7
	xch	r7,[pc](r10)
	xch.w	[pc](r10),r7
	xch	r7,[10000[r9]](r10)
	xch.w	[10000[r9]](r10),r7
	xch	r7,[[pc]](r10)
	xch.w	[[pc]](r10),r7
	xch	r7,/23456789h(r10)
	xch.w	/23456789h(r10),r7
	xch	r7,[/23456789h](r10)
	xch.w	[/23456789h](r10),r7
	expect	1350,1350,1350,1350
	xch	#10,r7
	xch.w	r7,#10
	xch	#100,r7
	xch.w	r7,#100
	endexpect
	expect	1890
	xch	[r7],[r9]
	endexpect

	xor.b	r7,r9
	xor.b	[r7],[r9]
	xor.b	[r7+],[r9+]
	xor.b	[-r7],[-r9]
	xor.b	10[r7],10000[r9]
	xor.b	[10[r7]],[10000[r9]]
	xor.b	20[10[r7]],20000[10000[r9]]
	xor.b	/12345678h,/23456789h
	xor.b	[/12345678h],[/23456789h]
	xor.b	[r7](r8),[r9](r10)
	xor.b	10[r7](r8),10000[r9](r10)
	xor.b	[pc](r8),[pc](r10)
	xor.b	[10[r7]](r8),[10000[r9]](r10)
	xor.b	[[pc]](r8),[[pc]](r10)
	xor.b	/12345678h(r8),/23456789h(r10)
	xor.b	[/12345678h](r8),[/23456789h](r10)
	xor.b	#2,r0
	xor.b	#200,r0

	xor.h	r7,r9
	xor.h	[r7],[r9]
	xor.h	[r7+],[r9+]
	xor.h	[-r7],[-r9]
	xor.h	10[r7],10000[r9]
	xor.h	[10[r7]],[10000[r9]]
	xor.h	20[10[r7]],20000[10000[r9]]
	xor.h	/12345678h,/23456789h
	xor.h	[/12345678h],[/23456789h]
	xor.h	[r7](r8),[r9](r10)
	xor.h	10[r7](r8),10000[r9](r10)
	xor.h	[pc](r8),[pc](r10)
	xor.h	[10[r7]](r8),[10000[r9]](r10)
	xor.h	[[pc]](r8),[[pc]](r10)
	xor.h	/12345678h(r8),/23456789h(r10)
	xor.h	[/12345678h](r8),[/23456789h](r10)
	xor.h	#2,r0
	xor.h	#200,r0

	xor.w	r7,r9
	xor.w	[r7],[r9]
	xor.w	[r7+],[r9+]
	xor.w	[-r7],[-r9]
	xor.w	10[r7],10000[r9]
	xor.w	[10[r7]],[10000[r9]]
	xor.w	20[10[r7]],20000[10000[r9]]
	xor.w	/12345678h,/23456789h
	xor.w	[/12345678h],[/23456789h]
	xor.w	[r7](r8),[r9](r10)
	xor.w	10[r7](r8),10000[r9](r10)
	xor.w	[pc](r8),[pc](r10)
	xor.w	[10[r7]](r8),[10000[r9]](r10)
	xor.w	[[pc]](r8),[[pc]](r10)
	xor.w	/12345678h(r8),/23456789h(r10)
	xor.w	[/12345678h](r8),[/23456789h](r10)
	xor.w	#2,r0
	xor.w	#200,r0

	expect	1100
	xorbsu.b @[r7],#3,@[r9]
	endexpect
	xorbsu	@[r7],#3,@[r9]
	xorbsu	@[r7+],#20,@[r9+]
	xorbsu	@[-r7],r11,@[-r9]
	xorbsu	10@[r7],#3,10000@[r9]
	xorbsu	@[10[r7]],#20,@[10000[r9]]
	xorbsu	20@[10[r7]],r11,20000@[10000[r9]]
	xorbsu	@/12345678h,#3,@/23456789h
	xorbsu	@[/12345678h],#20,@[/23456789h]
	xorbsu	r8@[r7],r11,r10@[r9]
	xorbsu	r8@10[r7],#3,r10@10000[r9]
	xorbsu	r8@[pc],#20,r10@[pc]
	xorbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	xorbsu	r8@[[pc]],#3,r10@[[pc]]
	xorbsu	r8@/12345678h,#20,r10@/23456789h
	xorbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	xorbsd.b @[r7],#3,@[r9]
	endexpect
	xorbsd	@[r7],#3,@[r9]
	xorbsd	@[r7+],#20,@[r9+]
	xorbsd	@[-r7],r11,@[-r9]
	xorbsd	10@[r7],#3,10000@[r9]
	xorbsd	@[10[r7]],#20,@[10000[r9]]
	xorbsd	20@[10[r7]],r11,20000@[10000[r9]]
	xorbsd	@/12345678h,#3,@/23456789h
	xorbsd	@[/12345678h],#20,@[/23456789h]
	xorbsd	r8@[r7],r11,r10@[r9]
	xorbsd	r8@10[r7],#3,r10@10000[r9]
	xorbsd	r8@[pc],#20,r10@[pc]
	xorbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	xorbsd	r8@[[pc]],#3,r10@[[pc]]
	xorbsd	r8@/12345678h,#20,r10@/23456789h
	xorbsd	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	xornbsu.b @[r7],#3,@[r9]
	endexpect
	xornbsu	@[r7],#3,@[r9]
	xornbsu	@[r7+],#20,@[r9+]
	xornbsu	@[-r7],r11,@[-r9]
	xornbsu	10@[r7],#3,10000@[r9]
	xornbsu	@[10[r7]],#20,@[10000[r9]]
	xornbsu	20@[10[r7]],r11,20000@[10000[r9]]
	xornbsu	@/12345678h,#3,@/23456789h
	xornbsu	@[/12345678h],#20,@[/23456789h]
	xornbsu	r8@[r7],r11,r10@[r9]
	xornbsu	r8@10[r7],#3,r10@10000[r9]
	xornbsu	r8@[pc],#20,r10@[pc]
	xornbsu	r8@[10[r7]],r11,r10@[10000[r9]]
	xornbsu	r8@[[pc]],#3,r10@[[pc]]
	xornbsu	r8@/12345678h,#20,r10@/23456789h
	xornbsu	r8@[/12345678h],r11,r10@[/23456789h]

	expect	1100
	xornbsd.b @[r7],#3,@[r9]
	endexpect
	xornbsd	@[r7],#3,@[r9]
	xornbsd	@[r7+],#20,@[r9+]
	xornbsd	@[-r7],r11,@[-r9]
	xornbsd	10@[r7],#3,10000@[r9]
	xornbsd	@[10[r7]],#20,@[10000[r9]]
	xornbsd	20@[10[r7]],r11,20000@[10000[r9]]
	xornbsd	@/12345678h,#3,@/23456789h
	xornbsd	@[/12345678h],#20,@[/23456789h]
	xornbsd	r8@[r7],r11,r10@[r9]
	xornbsd	r8@10[r7],#3,r10@10000[r9]
	xornbsd	r8@[pc],#20,r10@[pc]
	xornbsd	r8@[10[r7]],r11,r10@[10000[r9]]
	xornbsd	r8@[[pc]],#3,r10@[[pc]]
	xornbsd	r8@/12345678h,#20,r10@/23456789h
	xornbsd	r8@[/12345678h],r11,r10@[/23456789h]

	; addressing modes with two displacements have the restriction
        ; that both displacements must be of same size (1, 2, or 4 bytes).
        ; This means that both fields have to be 'inflated' to hold the maximum
        ; of disp1 and disp2, resp. offset and disp for bit addressing:

	mov.w	10[10[r7]],r8		; (8)/(8) -> 8 bits
	mov.w	10[1000[r7]],r8		; (8)/(16) -> 16 bits
	mov.w	10[100000[r7]],r8	; (8)/(32) -> 32 bits
	mov.w	1000[10[r7]],r8		; (16)/(8) -> 16 bits
	mov.w	1000[1000[r7]],r8	; (16)/(16) -> 16 bits
	mov.w	1000[100000[r7]],r8	; (16)/(32) -> 32 bits
	mov.w	100000[10[r7]],r8	; (32)/(8) -> 32 bits
	mov.w	100000[1000[r7]],r8	; (32)/(16) -> 32 bits
	mov.w	100000[100000[r7]],r8	; (32)/(32) -> 32 bits

	sch1bsu	10@[10[r7]],#3,r9	; (8)/(8) -> 8 bits
	sch1bsu	10@[1000[r7]],#3,r9	; (8)/(16) -> 16 bits
	sch1bsu	10@[100000[r7]],#3,r9	; (8)/(32) -> 32 bits
	sch1bsu	1000@[10[r7]],#3,r9	; (16)/(8) -> 16 bits
	sch1bsu	1000@[1000[r7]],#3,r9	; (16)/(16) -> 16 bits
	sch1bsu	1000@[100000[r7]],#3,r9	; (16)/(32) -> 32 bits
	sch1bsu	100000@[10[r7]],#3,r9	; (32)/(8) -> 32 bits
	sch1bsu	100000@[1000[r7]],#3,r9	; (32)/(16) -> 32 bits
	sch1bsu	100000@[100000[r7]],#3,r9 ; (32)/(32) -> 32 bits

        ; If two explicit lengths were given, complain if they do not match:

	mov.w	10.8[10.8[r7]],r8	; 8/8 -> 8 bits
	expect	1131
	mov.w	10.8[10.16[r7]],r8	; 8/16 -> error
	endexpect
	expect	1131
	mov.w	10.8[10.32[r7]],r8	; 8/32 -> error
	endexpect
	expect	1131
	mov.w	10.16[10.8[r7]],r8	; 16/8 -> error
	endexpect
	mov.w	10.16[10.16[r7]],r8	; 16/16 -> 16 bits
	expect	1131
	mov.w	10.16[10.32[r7]],r8	; 16/32 -> error
	endexpect
	expect	1131
	mov.w	10.32[10.8[r7]],r8	; 32/8 -> error
	endexpect
	expect	1131
	mov.w	10.32[10.16[r7]],r8	; 32/16 -> error
	endexpect
	mov.w	10.32[10.32[r7]],r8	; 32/32 -> 32 bits

	sch1bsu	10.8@[10.8[r7]],#3,r9	; 8/8 -> 8 bits
	expect	1131
	sch1bsu	10.8@[10.16[r7]],#3,r9	; 8/16 -> error
	endexpect
	expect	1131
	sch1bsu	10.8@[10.32[r7]],#3,r9	; 8/32 -> error
	endexpect
	expect	1131
	sch1bsu	10.16@[10.8[r7]],#3,r9	; 16/8 -> error
	endexpect
	sch1bsu	10.16@[10.16[r7]],#3,r9	; 16/16 -> 16 bits
	expect	1131
	sch1bsu	10.16@[10.32[r7]],#3,r9	; 16/32 -> error
	endexpect
	expect	1131
	sch1bsu	10.32@[10.8[r7]],#3,r9	; 32/8 -> error
	endexpect
	expect	1131
	sch1bsu	10.32@[10.16[r7]],#3,r9	; 32/16 -> error
	endexpect
	sch1bsu	10.32@[10.32[r7]],#3,r9	; 32/32 -> 32 bits

	; only one explicit length was given, which forces the other displacement to same
        ; size, which is larger than necessary to hold the other displacement:

	mov.w	10.16[10[r7]],r8	; 16/(8) -> 16 bits
	mov.w	10.32[10[r7]],r8	; 32/(8) -> 32 bits
	mov.w	1000.32[1000[r7]],r8	; 32/(16) -> 32 bits
	mov.w	10[10.16[r7]],r8	; (8)/16 -> 16 bits
	mov.w	10[10.32[r7]],r8	; (8)/32 -> 32 bits
	mov.w	1000[1000.32[r7]],r8	; (16)/32 -> 32 bits

	sch1bsu	10.16@[10[r7]],#3,r9		; 16/(8) -> 16 bits
	sch1bsu	10.32@[10[r7]],#3,r9		; 32/(8) -> 32 bits
	sch1bsu	1000.32@[1000[r7]],#3,r9	; 32/(16) -> 32 bits
	sch1bsu	10@[10.16[r7]],#3,r9		; (8)/16 -> 16 bits
	sch1bsu	10@[10.32[r7]],#3,r9		; (8)/32 -> 32 bits
	sch1bsu	1000@[1000.32[r7]],#3,r9	; (16)/32 -> 32 bits

	; If only one length was given, but the other displacement does not fit into
        ; it, complain as well:

	expect	1320
	mov.w	10.8[1000[r7]],r8	; 8/(16) -> error
	endexpect
	expect	1320
	mov.w	10.8[100000[r7]],r8	; 8/(32) -> error
	endexpect
	expect	1320
	mov.w	10.16[100000[r7]],r8	; 16((32) -> error
	endexpect
	expect	1320
	mov.w	1000[10.8[r7]],r8	; (16)/8 -> error
	endexpect
	expect	1320
	mov.w	100000[10.8[r7]],r8	; (32)/8 -> error
	endexpect
	expect	1320
	mov.w	100000[10.16[r7]],r8	; (32)/16 -> error
	endexpect

	expect	1320
	sch1bsu	10.8@[1000[r7]],#3,r9	; 8/(16) -> error
	endexpect
	expect	1320
	sch1bsu	10.8@[100000[r7]],#3,r9	; 8/(32) -> error
	endexpect
	expect	1320
	sch1bsu	10.16@[100000[r7]],#3,r9	; 16((32) -> error
	endexpect
	expect	1320
	sch1bsu	1000@[10.8[r7]],#3,r9	; (16)/8 -> error
	endexpect
	expect	1320
	sch1bsu	100000@[10.8[r7]],#3,r9	; (32)/8 -> error
	endexpect
	expect	1320
	sch1bsu	100000@[10.16[r7]],#3,r9	; (32)/16 -> error
	endexpect

	; several instructions have restricted value ranges and
        ; will result in exceptions if the range is violated.  Of
        ; course, we can only warn if the arguments are immediate:

	; bit field insertion/extraction: offset+length must not exceed 32

	insbfr	r12,r13@[r17],r14	; cannot check
	insbfr	r12,13@[r17],r14	; cannot check
	insbfr	r12,r13@[r17],#14	; cannot check
	insbfr	r12,13@[r17],#14	; OK (13+14=27)
	insbfr	r12,13@[r17],#19	; just OK (13+19=32)
	expect	410
	insbfr	r12,13@[r17],#20	; no longer OK
	endexpect


	extbfs	r13@[r17],r14,r12	; cannot check
	extbfs	13@[r17],r14,r12	; cannot check
	extbfs	r13@[r17],#14,r12	; cannot check
	extbfs	13@[r17],#14,r12	; OK (13+14=27)
	extbfs	13@[r17],#19,r12	; just OK (13+19=32)
	expect	410
	extbfs	13@[r17],#20,r12	; no longer OK
	endexpect

	; Instructions that implicitly use the stack pointer
	; have unpredictable result if SP with autoincrement/decrement is used:

	expect	140
	call	[-r31],[r4]
	endexpect
	expect	140
	call	[r4],[-r31]
	endexpect
	expect	140
	jmp	[sp+]
	endexpect
	expect	140
	jsr	[-sp]
	endexpect

	; privilege levels may only be in the range 0..3:

	chkar	[r19],#2
	expect	410
	chkar	[r19],#20
	endexpect

	chlvl	#1,#30
	expect	410
	chlvl	#5,#30
	endexpect

	; bit positions may only be in the range 0..1:

	clr1	#26,r6
	expect	410
	clr1	#33,r6
	endexpect

	; addressing mode combinations with unpredictable results:

	; if first operand uses auto-increment/decrement, and second operand
	; the same register as index, result is unpredictable:

	mov.w	[ r3+ ], [ r4 ]( r5 )	; OK (different register)
	expect	140
	mov.w	[ r3+ ], [ r4 ]( r3 )	; unpredictable
	endexpect

	; auto-increment/decrement on same register is OK as long as operand size is same:

	mov.w	[ r3+ ], [ r3+ ]	; OK
	mov.w	[ -r3 ], [ r3+ ]	; OK
	mov.w	[ r3+ ], [ -r3 ]	; OK
	mov.w	[ -r3 ], [ -r3 ]	; OK

	movs.bw	[ -r20 ], [ -r21 ]	; OK (different register)
	expect	140
	movs.bw	[ -r20 ], [ -r20 ]	; unpredictable
	endexpect
	cvt.lw	[ r19+ ], [ r20+ ]	; OK (different register)
	expect	140
	cvt.lw	[ r20+ ], [ r20+ ]	; unpredictable
	endexpect

	; Endianess of dc.x was BE in first versions of the assembler.  However, V60 is LE:

	dc.h	1,2,3
	dc.b	1,2,3
	dc.w	1,2,3
	dc.d	1,2,3
	dc.s	1.0,2.0,3.0
	dc.l	1.0,2.0,3.0
