		page		0

		cpu		mcf5202		; ISA A
		supmode		on

		cpu		mcf5208		; ISA A+
		supmode		on

		bitrev		d4		; $00cn
		bitrev.l	d4
		byterev		d6		; $02cn
		byterev.l	d6
		ff1		d2		; $04cn
		ff1.l		d2

		move.l		usp,a3		; $4e6(8n)
		move.l		a5,usp		; $4e6(0n)

		stldsr		#$55aa		; $40e7 $46fc iiii
		stldsr.w	#$55aa

		cpu		mcf5407		; ISA B

		bra.x		*+100000	; $60ff dddd dddd
		bra		*+100000
		bsr.x		*+100000	; $61ff dddd dddd
		bsr		*+100000
		bne.x		*+100000	; $66ff dddd dddd
		bne		*+100000

		cmp.b		d3,d2		; %1011 010(d2) 000(b)|001(w) eeeaaa
		cmp.w		d3,d2
		cmp.b		a3,d2
		cmp.w		a3,d2
		cmp.b		(a5),d2
		cmp.w		(a5),d2
		cmp.b		(a4)+,d2
		cmp.w		(a4)+,d2
		cmp.b		-(a1),d2
		cmp.w		-(a1),d2
		cmp.b		(1000,sp),d2
		cmp.w		(1000,sp),d2
		cmp.b		(-100,d4,a6),d2
		cmp.w		(-100,d4,a6),d2
		cmp.b		($1234),d2
		cmp.b		($1234.w),d2
		cmp.w		($1234),d2
		cmp.w		($1234.w),d2
		cmp.b		($12345678),d2
		cmp.b		($12345678.l),d2
		cmp.w		($12345678),d2
		cmp.w		($12345678.l),d2
		cmp.b		#$56,d2
		cmp.w		#$5678,d2
		cmp.b		(*+1000,pc),d2
		cmp.w		(*+1000,pc),d2
		cmp.b		(*-100,d3,pc),d2
		cmp.w		(*-100,d3,pc),d2

		cmpa.w		d3,a2		; %1011 010(a2) 011(w) eeeaaaa
		cmp.w		d3,a2
		cmpa.w		a3,a2
		cmp.w		a3,a2
		cmpa.w		(a5),a2
		cmp.w		(a5),a2
		cmpa.w		(a4)+,a2
		cmp.w		(a4)+,a2
		cmpa.w		-(a1),a2
		cmp.w		-(a1),a2
		cmpa.w		(1000,sp),a2
		cmp.w		(1000,sp),a2
		cmpa.w		(-100,d4,a6),a2
		cmp.w		(-100,d4,a6),a2
		cmpa.w		($1234),a2
		cmpa.w		($1234.w),a2
		cmp.w		($1234),a2
		cmp.w		($1234.w),a2
		cmpa.w		($12345678),a2
		cmpa.w		($12345678.l),a2
		cmp.w		($12345678),a2
		cmp.w		($12345678.l),a2
		cmpa.w		#$56,a2
		cmp.w		#$5678,a2
		cmpa.w		(*+1000,pc),a2
		cmp.w		(*+1000,pc),a2
		cmpa.w		(*-100,d3,pc),a2
		cmp.w		(*-100,d3,pc),a2

		cmpi.b		#$55,d3
		cmpi.w		#$6666,d3

		intouch		(a4)		; $f42(1|reg)

		mov3q		#-1,d3		; %1010 iii1 01ee eaaa
		mov3q.l		#-1,d3
		mov3q		#1,d3
		mov3q.l		#1,d3
		mov3q		#7,d3
		mov3q.l		#7,d3
		mov3q		#1,a5
		mov3q.l		#1,a5
		mov3q		#1,(a4)
		mov3q.l		#1,(a4)
		mov3q		#1,(a1)+
		mov3q.l		#1,(a1)+
		mov3q		#1,-(a6)
		mov3q.l		#1,-(a6)
		mov3q		#1,(1000,a6)
		mov3q.l		#1,(a6,1000)
		mov3q		#1,(-100,a6,d5)
		mov3q.l		#1,(d5,a6,-100)
		mov3q		#1,($3456)
		mov3q.l		#1,$3456
		mov3q		#1,($123456)
		mov3q.l		#1,$123456

		irp		instr,mvs,mvz
		irp		src,d3,a3,(a3),(a3)+,-(a3),(1000,a3),(-100,a3,d3),($1234),($12345678),(*+1000,pc),(*-100,pc,d3),#42
		instr		src,d7
		instr.b		src,d7
		instr.w		src,d7
		endm
		endm

		sats		d4		; $4c8(Dn)
		sats.l		d4

		irp		dest,(a3),(a3)+,-(a3),(1000,a3),(-100,a3,d3),($1234),($12345678)
		tas		dest
		tas.b		dest
		endm

		fpu		on

		irp		instr,fabs,fsabs,fdabs
		instr.b		d2,fp4
		instr.w		d2,fp4
		instr.l		d2,fp4
		instr.s		d2,fp4
		irp		src,(a2),(a2)+,-(a2),(1000,a2),(*+1000,pc)
		instr.b		src,fp4
		instr.w		src,fp4
		instr.l		src,fp4
		instr.s		src,fp4
		instr.d		src,fp4
		instr		src,fp4
		endm
		instr		fp2,fp4
		instr.d		fp2,fp4
		instr		fp4
		instr.d		fp4
		endm

		; (E)MAC stuff

		cpu		mcf5470

		irp		instr,mac,msac
		 irp		noshift,x
		  instr.l	a4,d3
		  instr.l	a4,d3,(a5),a6,acc
		  instr.l	a4,d3,(1000,a5),a6,acc0
		  instr.l	a4,d3,(a5)&,a6
		  instr.l	a4,d3,(1000,a5)&,a6
		  instr.l	a4,d3,(a5)&mask,a6
		  instr.l	a4,d3,(1000,a5)&mask,a6
		  irp		half1,l,u
		  irp		half2,l,u
		   instr.w	a4.half1,d3.half2
		   ; we might iterate here through mask arg and other EMAC ACCs either...
		  endm
		  endm
		 endm
		 irp		shift,<<0,>>0,<<,>>,<<1,>>1
		  instr.l	a4,d3,shift
		  instr.l	a4,d3,shift,(a5),a6,acc
		  instr.l	a4,d3,shift,(1000,a5),a6,acc0
		  instr.l	a4,d3,shift,(a5)&,a6
		  instr.l	a4,d3,shift,(1000,a5)&,a6
		  instr.l	a4,d3,shift,(a5)&mask,a6
		  instr.l	a4,d3,shift,(1000,a5)&mask,a6
		  irp		half1,l,u
		  irp		half2,l,u
		   instr.w	a4.half1,d3.half2,shift
		   ; we might iterate here through mask arg and other EMAC ACCs either...
		  endm
		  endm
		 endm
		endm

		irp		reg,acc,acc0,acc1,acc2,acc3,macsr,mask,accext01,accext23
		 move		reg,d4
		 move.l		reg,d4
		 move		reg,a4
		 move.l		reg,a4
		 move		d4,reg
		 move.l		d4,reg
		 move		a4,reg
		 move.l		a4,reg
		 move		#$12345678,reg
		 move.l		#$12345678,reg
		endm

		move		MACSR,ccr
		move.l		MACSR,ccr

		move		acc2,acc3
		move.l		acc2,acc3

		irp		reg,acc,acc0,acc1,acc2,acc3
		 movclr		reg,d4
		 movclr.l	reg,d4
		 movclr		reg,a4
		 movclr.l	reg,a4
		endm

		irp		instr,maaac,masac,msaac,mssac
		 irp		noshift,x
		  instr.l	a4,d3,acc1,acc3
		  irp		half1,l,u
		  irp		half2,l,u
		   instr.w	 a4.half1,d3.half2,acc1,acc3
		  endm
		  endm
		 endm
		 irp		shift,<<0,>>0,<<1,>>1
		  instr.l	a4,d3,shift,acc1,acc3
		  irp		half1,l,u
		  irp		half2,l,u
		   instr.w	 a4.half1,d3.half2,shift,acc1,acc3
		  endm
		  endm
		 endm
		endm

		; test CAU

		cpu		mcf51qm

		include		"regcold.inc"

		; it took me quite a while to get hold of cf_prm_coproc.pdf...

		cp0ld.l		#CNOP		; NOP
		cp0ld		#CNOP		; 32 bit is default opsize (and only one allowed for CAU)
		cp0ld		d0,CNOP
		cp0ld		d0,d0,CNOP
		cp0ld		d0,0,CNOP
		cp0ld		d0,d0,0,CNOP
		cp0nop				; should be same...
		cp0nop		0
		cp0ld		d1,#LDR+CA0	; Load D1 -> CA0
		cp0st		d2,#STR+CA1	; Store CA1 -> D2
		cp0ld		d3,#ADR+CA2	; CA2 += D3
		cp0ld		#ADRA+CA3	; CAA += CA3
		cp0ld		d4,#XOR+CAA	; CAA ^= D4
		cp0ld		#MVRA+CA5	; CAA <- CA5
		cp0ld		#MVAR+CA6	; CA6 <- CAA
		cp0ld		#AESS+CA7	; CA7 <- AES-subsitution(CA7)
		cp0ld		#AESIS+CA8	; CA8 <- AES-inverse-subsitution(CA8)
		cp0ld		d5,#AESC+CA1	; CA1 <- AES-column-operation(D5)
		cp0ld		d6,#AESIC+CA2	; CA2 <- AES-inverse-column-operation(D6)
		cp0ld		#AESR		; AES row shift op on CA0...CA3
		cp0ld		#AESIR		; AES inverse row shift op on CA0...CA3
		cp0ld		#DESR		; DES round on CA0...CA3
		cp0ld		#DESR+IP	; " with initial permutation
		cp0ld		#DESR+FP	; " with final permutation
		cp0ld		#DESR+KSL2	; " with left 2 key update
		cp0ld		#DESK		; DES key transformation
		cp0ld		#DESK+CP	; " with key parity error detection
		cp0ld		#DESK+DC	; " for decryption
		cp0ld		#HASH+HFF	; MD5 F() operation
		cp0ld		#HASH+HFG	; MD5 G() operation
		cp0ld		#HASH+HFH	; MD5 H() operation
		cp0ld		#HASH+HFI	; MD5 I() operation
		cp0ld		#HASH+HFC	; SHA Ch() operation
		cp0ld		#HASH+HFM	; SHA Maj() operation
		cp0ld		#HASH+HF2C	; SHA-256 Ch() operation
		cp0ld		#HASH+HF2M	; SHA-256 Maj() operation
		cp0ld		#HASH+HF2S	; SHA-256 Sigma 0 operation
		cp0ld		#HASH+HF2T	; SHA-256 Sigma 1 operation
		cp0ld		#HASH+HF2U	; SHA-256 Sigma 0 operation
		cp0ld		#HASH+HF2V	; SHA-256 Sigma 1 operation
		cp0ld		#SHS		; secure hash shift on CAA...CA4
		cp0ld		#MDS		; message digest shift on CAA...CA3
		cp0ld		#SHS2		; secure hash shift 2 on CAA...CA8
		cp0ld		#ILL		; illegal command
		cp0bcbusy	*+4		; not needed for CAU?
		cp1bcbusy	*-4
