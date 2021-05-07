	supmode	on
	pmmu	on

	; MMU Testprogram
	jump	@x'10000010	; 00000000  7FAAD0000010 Enable DRAM at address 0
	.align	16
        ; Build up
	lprd	cfg,x'AFF	; 00000010  6FA600000AFF
	lprd	sp,x'1FC	; 00000016  EFA4000001FC System Stackpointer
	movd	x'B0051D0,TOS	; 0000001C  D7A50B0051D0 PSR and Module
	movd	x'1807FF0,TOS	; 00000022  D7A501807FF0 Jump to USER program
	addr	procexit,tos	; 00000028  E7DD80A7
	movd	x'123456,@x'600	; 0000002C  57A5001234568600 Data ...
	lprd	intbase,x'200	; 00000034  6FA700000200
	movqd	7,@0		; 0000003A  DFAB00 Level 1 USER MOD
	movqd	7,@x'14		; 0000003D  DFAB14 Level 2 USER MOD
	movqd	7,@x'18		; 00000040  DFAB18 Level 1 USER Programm
	movqd	7,@x'1C		; 00000043  DFAB1C Level 2 USER Programm
	movqd	0,@x'20		; 00000046  5FA820 next Page not there !
	movd	x'1007,@x'EC	; 00000049  57A50000100780EC Level 1 USER Data
	movmd	zero,@x'F0,3	; 00000051  CE43DD808F80F008 Level 2 User Data set to 0
	movd	x'2187,@x'FC	; 00000059  57A50000218780FC Level 2 USER Data : Stack
	lmr	mcr,5		; 00000061  1E8BA400000005 DS and TU
	lmr	ptb1,x'A2000	; 00000068  1E8BA6000A2000 does not matter ... but better visible
	movd	x'12380,@x'1D0	; 0000006F  57A50001238081D0 SB USER Modul
	movd	x'000230,@x'208	; 00000077  57A5000002308208 ABORT Vector
	movd	x'200230,@x'200	; 0000007F  57A5002002308200 Interrupt Vector
	movd	x'537C0,@x'230	; 00000087  57A5000537C08230 SB Supervisor Modul
	movd	x'80,@x'238	; 0000008F  57A5000000808238 Program Base
	movqd	0,@x'E0		; 00000097  5FA880E0 Page-Table Pointer
        ; load programcode in RAM :
	addr	pages,r1	; 0000009B  67D88055
	movzwd	x'70,r2		; 0000009F  CE99A00070
	movzbd	10,r0		; 000000A4  CE18A00A
	movsd			; 000000A8  0E0300
	movmd	use,@x'FF0,4	; 000000AB  CE43DD80658FF00C
	movqd	-1,@x'E8	; 000000B3  DFAF80E8 for writing ...
	ret	0		; 000000B7  1200
	.align	16
	.byte	0		; 000000C0  00
	.align	8
	.byte	0,0,0,0,0,0,0	; 000000C8  00000000000000
procexit:
	rett	4		; 000000CF  4204 now start the test

	.align	16
zero:	.double	0,0,0,0		; 000000E0  0000000000000000
          			;           0000000000000000

	; ABORT Service Routine at x'80
pages:	.double x'2007,x'3007,x'4007,x'5007 ;000000F0  0720000007300000
				;           0740000007500000
abo:	save	[r0,r1,r2]	; 00000100  6207
	movd	12(sp),r1	; 00000102  57C80C
always:
	addqd	1,r0		; 00000105  8F00
	br	always		; 00000107  EA7E

	; User Program at x'FF0
	.align 16
use:	movd	x'12345678,r7	; 00000110  D7A112345678
	movw	100,r6		; 00000116  95A10064
	movb	'!',r0		; 0000011A  14A021
	addr	-800(sb),r1	; 0000011D  67D0BCE0

	.align	16
	; only for example
	.long	3.5e6		; 00000130  00000000F0B34A41
