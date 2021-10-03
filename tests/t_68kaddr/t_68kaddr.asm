	cpu	68000
	page	0

rega3	equ	a3
.rega3	equ	a3

	; basic 68000 modes

	move.l	d3,d7
	move.l	a3,d7
	move.l	(a3),d7
	move.l	(a3)+,d7
	move.l	-(a3),d7
	move.l	(rega3),d7		; register alias
	move.l	(rega3)+,d7
	move.l	-(rega3),d7
	move.l	(.rega3),d7		; register alias with period (since 1.42 Bld 212)
	move.l	(.rega3)+,d7
	move.l	-(.rega3),d7
	move.l	1000(a3),d7		; "68000 style"
	move.l	1000( a3),d7
	move.l	1000(a3 ),d7
	move.l	1000(rega3),d7		; register alias
	move.l	1000( rega3),d7
	move.l	1000(rega3 ),d7
	move.l	1000(.rega3),d7		; register alias with period (since 1.42 Bld 212)
	move.l	1000( .rega3),d7
	move.l	1000(.rega3 ),d7
	move.l	(1000,a3),d7		; "68020 style"
	move.l	( 1000, a3),d7
	move.l	(1000 ,a3 ),d7
	move.l	(1000,rega3),d7		; register alias
	move.l	( 1000, rega3),d7
	move.l	(1000 ,rega3 ),d7
	move.l	(1000,.rega3),d7	; register alias with period (since 1.42 Bld 212)
	move.l	( 1000, .rega3),d7
	move.l	(1000 ,.rega3 ),d7
	move.l	120(a3,a4.w),d7		; "68000 style"
	move.l	(120,a3,a4.w),d7	; "68020 style"
	move.l	120(a3,a4.l),d7		; "68000 style"
	move.l	(120,a3,a4.l),d7	; "68020 style"
	move.l	120(a3,d4.w),d7		; "68000 style"
	move.l	(120,a3,d4.w),d7	; "68020 style"
	move.l	120(a3,d4.l),d7		; "68000 style"
	move.l	(120,a3,d4.l),d7	; "68020 style"
	move.l	10000,d7		; "68000 style"
	move.l	10000.l,d7		; "68000 style"
	move.l	100000,d7		; "68000 style"
	move.l	(10000),d7		; "68020 style"
	move.l	(10000.l),d7		; "68020 style"
	move.l	(100000),d7		; "68020 style"
	move.l	*(pc,a4.w),d7		; "68000 style"
	move.l	(*,pc,a4.w),d7		; "68020 style"
	move.l	*(pc,a4.l),d7		; "68000 style"
	move.l	(*,pc,a4.l),d7		; "68020 style"
	move.l	*(pc,d4.w),d7		; "68000 style"
	move.l	(*,pc,d4.w),d7		; "68020 style"
	move.l	*(pc,d4.l),d7		; "68000 style"
	move.l	(*,pc,d4.l),d7		; "68020 style"
	move.l	#$aa554711,d7

	; extended 68020+ modes

	cpu	68020

	; base displacement
	move.l	(10000,a3,d4.l*4),d7	; all components
	move.l	(10000.l,a3,d4.l*4),d7
	move.l	(10000,a3,d4.l*1),d7	; ->scale field zero
	move.l	(10000.l,a3,d4.l*1),d7
	move.l	(10000,a3,d4.w*1),d7	; ->word instead of longword index
	move.l	(10000.l,a3,d4.w*1),d7
	move.l	(10000,a3),d7		; no index
	move.l	(10000.l,a3),d7
	move.l	(10000,d4.w*1),d7	; no basereg
	move.l	(10000.l,d4.w*1),d7

	; no index (post/pre-indexed setting in I/IS irrelevant?)
	move.l	([10000]),d7
	move.l	([10000.l]),d7
	move.l	([a3]),d7
	move.l	([a3,10000]),d7
	move.l	([a3,10000.l]),d7
	move.l	([10000],20000),d7
	move.l	([10000.l],20000.l),d7
	move.l	([a3],20000),d7
	move.l	([a3],20000.l),d7
	move.l	([10000,a3],20000),d7
	move.l	([10000.l,a3],20000.l),d7

	; postindexed
	move.l	([10000],d4.w*1),d7
	move.l	([10000.l],d4.l*4),d7
	move.l	([a3],d4.w*1),d7
	move.l	([a3],d4.l*4),d7
	move.l	([10000,a3],d4.w*1),d7
	move.l	([10000.l,a3],d4.l*4),d7
	move.l	([10000],d4.w*1,20000),d7
	move.l	([10000.l],d4.l*4,20000.l),d7
	move.l	([a3],d4.w*1,20000),d7
	move.l	([a3],d4.l*4,20000.l),d7
	move.l	([10000,a3],d4.w*1,20000),d7
	move.l	([10000.l,a3],d4.l*4,20000.l),d7

	; preindexed
        move.l  ([10000,d4.w*1]),d7
        move.l  ([10000.l,d4.l*4]),d7
	move.l  ([a3,d4.w*1]),d7
	move.l  ([a3,d4.l*4]),d7
	move.l	([10000,a3,d4.w*1]),d7
	move.l	([10000.l,a3,d4.l*4]),d7
	move.l	([10000,d4.w*1],20000),d7
	move.l	([10000.l,d4.l*4],20000.l),d7
	move.l	([a3,d4.w*1],20000),d7
	move.l	([a3,d4.l*4],20000.l),d7
	move.l	([10000,a3,d4.w*1],20000),d7
	move.l	([10000.l,a3,d4.l*4],20000.l),d7

	; PC with base displacement
	move.l	(*,pc,d4.l*4),d7	; all components
	move.l	(*.l,pc,d4.l*4),d7
	move.l	(*,pc,d4.l*1),d7	; ->scale field zero
	move.l	(*.l,pc,d4.l*1),d7
	move.l	(*,pc,d4.w*1),d7	; ->word instead of longword index
	move.l	(*.l,pc,d4.w*1),d7
	move.l	(*,pc),d7		; no index
	move.l	(*.l,pc),d7

	; PC postindexed
	move.l	([pc],d4.w*1),d7
	move.l	([pc],d4.l*4),d7
	move.l	([*,pc],d4.w*1),d7
	move.l	([*.l,pc],d4.l*4),d7
	move.l	([pc],d4.w*1,20000),d7
	move.l	([pc],d4.l*4,20000.l),d7
	move.l	([*,pc],d4.w*1,20000),d7
	move.l	([*.l,pc],d4.l*4,20000.l),d7

	; PC preindexed
	move.l  ([pc,d4.w*1]),d7
	move.l  ([pc,d4.l*4]),d7
	move.l	([*,pc,d4.w*1]),d7
	move.l	([*.l,pc,d4.l*4]),d7
	move.l	([pc,d4.w*1],20000),d7
	move.l	([pc,d4.l*4],20000.l),d7
	move.l	([*,pc,d4.w*1],20000),d7
	move.l	([*.l,pc,d4.l*4],20000.l),d7
