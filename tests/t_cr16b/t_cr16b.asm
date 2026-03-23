	cpu	cr16b:model=0
	page	0

	expect	1320
	br	0x20000		; 128K code address space in small model, like CR16A
	endexpect

	cpu	cr16b		; default is large model

	org	0x100000	; 2M code address space in large model

	expect	1350
	bal	r3,*		; would be small model syntax
	endexpect
	expect	1350
	bal	10(r4,r3),*
	endexpect
	bal	(r4,r3),*		; 7660 0000 (displacement 0x000000)
        bal	(r4,r3),*-4		; 767E FFFD (displacement 0x1ffffc)
	bal	(r4,r3),*+4		; 7660 0004 (displacement 0x000004)
	expect	1351
	bal	(r4,r3),*+255
	endexpect
	bal	(r4,r3),*+254		; 7660 00FE (displacement 0x0000fe)
	bal	(r4,r3),*+256		; 7660 0100 (displacement 0x000100)
	bal	(r4,r3),*+0x0fffe	; 7660 FFFE (displacement 0x00fffe)
	bal	(r4,r3),*+0x10000	; 7670 0000 (displacement 0x010000)
	bal	(r4,r3),*+0x1fffe	; 7670 FFFE (displacement 0x01fffe)
	bal	(r4,r3),*+0x20000	; 7662 0000 (displacement 0x020000)
	bal	(r4,r3),*+0x2fffe	; 7662 FFFE (displacement 0x02fffe)
	bal	(r4,r3),*+0x30000	; 7672 0000 (displacement 0x030000)
	expect	1351
	bal	(r4,r3),*-253
	endexpect
	bal	(r4,r3),*-256		; 767E FF01 (displacement -256     = 0xffff00)
	bal	(r4,r3),*-258		; 767E FEFF (displacement -258     = 0xfffefe)
	bal	(r4,r3),*-0xfffe	; 767E 0003 (displacement -0xfffe  = 0xff0002)
	bal	(r4,r3),*-0x10000	; 767E 0001 (displacement -0x10000 = 0xff0000)
	bal	(r4,r3),*-0x1fffe	; 766E 0003 (displacement -0x1fffe = 0xfe0002)
	bal	(r4,r3),*-0x20000	; 766E 0001 (displacement -0x20000 = 0xfe0000)
	bal	(r4,r3),*-0x2fffe	; 767C 0003 (displacement -0x2fffe = 0xfd0002)
	bal	(r4,r3),*-0x30000	; 767C 0001 (displacement -0x30000 = 0xfd0000)

	beq	*		; 4000
	beq	*-2		; 5E1E
	beq	*+2		; 4002
	expect	1351
	beq	*+255
	endexpect
	beq	*+254		; 4E1E
	beq	*+256		; 7400 0100 (displacement 0x000100)
	beq	*+0x0fffe	; 7400 FFFE (displacement 0x00fffe)
	beq	*+0x10000	; 7410 0000 (displacement 0x010000)
	beq	*+0x1fffe	; 7410 FFFE (displacement 0x01fffe)
	beq	*+0x20000	; 7402 0000 (displacement 0x020000)
	beq	*+0x2fffe	; 7402 FFFE (displacement 0x02fffe)
	beq	*+0x30000	; 7412 0000 (displacement 0x030000)
	expect	1351
	beq	*-253
	endexpect
	beq	*-256		; 5000
	beq	*-258		; 741E FEFF (displacement -258     = 0xfffefe)
	beq	*-0x0fffe	; 741E 0003 (displacement -0x0fffe = 0xff0002)
	beq	*-0x10000	; 741E 0001 (displacement -0x10000 = 0xff0000)
	beq	*-0x1fffe	; 740E 0003 (displacement -0x1fffe = 0xfe0002)
	beq	*-0x20000	; 740E 0001 (displacement -0x20000 = 0xfe0000)
	beq	*-0x2fffe	; 741C 0003 (displacement -0x2fffe = 0xfd0002)
	beq	*-0x30000	; 741C 0001 (displacement -0x30000 = 0xfd0000)

	expect	1445
	beq0b	r7,*+2
	endexpect
	expect	1370
	beq0b	r9,*-2
	endexpect
	beq0b	r9,*		; 1521
	beq0b	r9,*+2		; 1523
	beq0b	r9,*+30		; 153F
	expect	1370
	beq0b	r9,*+32
	endexpect

	expect	1445
	beq0w	r7,*+2
	endexpect
	expect	1370
	beq0w	r9,*-2
	endexpect
	beq0w	r9,*		; 3521
	beq0w	r9,*+2		; 3523
	beq0w	r9,*+30		; 353F
	expect	1370
	beq0w	r9,*+32
	endexpect

	expect	1445
	beq1b	r7,*+2
	endexpect
	expect	1370
	beq1b	r9,*-2
	endexpect
	beq1b	r9,*		; 1561
	beq1b	r9,*+2		; 1563
	beq1b	r9,*+30		; 157F
	expect	1370
	beq1b	r9,*+32
	endexpect

	expect	1445
	beq1w	r7,*+2
	endexpect
	expect	1370
	beq1w	r9,*-2
	endexpect
	beq1w	r9,*		; 3561
	beq1w	r9,*+2		; 3563
	beq1w	r9,*+30		; 357F
	expect	1370
	beq1w	r9,*+32
	endexpect

	bne	*		; 4020
	bne	*-2		; 5E3E
	bne	*+2		; 4022
	expect	1351
	bne	*+255
	endexpect
	bne	*+254		; 4E3E
	bne	*+256		; 7420 0100 (displacement 0x000100)
	bne	*+0x0fffe	; 7420 FFFE (displacement 0x00fffe)
	bne	*+0x10000	; 7430 0000 (displacement 0x010000)
	bne	*+0x1fffe	; 7430 FFFE (displacement 0x01fffe)
	bne	*+0x20000	; 7422 0000 (displacement 0x020000)
	bne	*+0x2fffe	; 7422 FFFE (displacement 0x02fffe)
	bne	*+0x30000	; 7432 0000 (displacement 0x030000)
	expect	1351
	bne	*-253
	endexpect
	bne	*-256		; 5020
	bne	*-258		; 743E FEFF (displacement -258     = 0xfffefe)
	bne	*-0xfffe	; 743E 0003 (displacement -0xfffe  = 0xff0002)
	bne	*-0x10000	; 743E 0001 (displacement -0x10000 = 0xff0000)
	bne	*-0x1fffe	; 742E 0003 (displacement -0x1fffe = 0xfe0002)
	bne	*-0x20000	; 742E 0001 (displacement -0x20000 = 0xfe0000)
	bne	*-0x2fffe	; 743C 0003 (displacement -0x2fffe = 0xfd0002)
	bne	*-0x30000	; 743C 0001 (displacement -0x30000 = 0xfd0000)

	expect	1445
	bne0b	r7,*+2
	endexpect
	expect	1370
	bne0b	r9,*-2
	endexpect
	bne0b	r9,*		; 15A1
	bne0b	r9,*+2		; 15A3
	bne0b	r9,*+30		; 15BF
	expect	1370
	bne0b	r9,*+32
	endexpect

	expect	1445
	bne0w	r7,*+2
	endexpect
	expect	1370
	bne0w	r9,*-2
	endexpect
	bne0w	r9,*		; 35A1
	bne0w	r9,*+2		; 35A3
	bne0w	r9,*+30		; 35BF
	expect	1370
	bne0w	r9,*+32
	endexpect

	expect	1445
	bne1b	r7,*+2
	endexpect
	expect	1370
	bne1b	r9,*-2
	endexpect
	bne1b	r9,*		; 15E1
	bne1b	r9,*+2		; 15E3
	bne1b	r9,*+30		; 15FF
	expect	1370
	bne1b	r9,*+32
	endexpect

	expect	1445
	bne1w	r7,*+2
	endexpect
	expect	1370
	bne1w	r9,*-2
	endexpect
	bne1w	r9,*		; 35E1
	bne1w	r9,*+2		; 35E3
	bne1w	r9,*+30		; 35FF
	expect	1370
	bne1w	r9,*+32
	endexpect

	bge	*		; 41A0
	bge	*-2		; 5FBE
	bge	*+2		; 41A2
	expect	1351
	bge	*+255
	endexpect
	bge	*+254		; 4FBE
	bge	*+256		; 75A0 0100 (displacement 0x000100)
	bge	*+0x0fffe	; 75A0 FFFE (displacement 0x00fffe)
	bge	*+0x10000	; 75B0 0000 (displacement 0x010000)
	bge	*+0x1fffe	; 75B0 FFFE (displacement 0x01fffe)
	bge	*+0x20000	; 75A2 0000 (displacement 0x020000)
	bge	*+0x2fffe	; 75A2 FFFE (displacement 0x02fffe)
	bge	*+0x30000	; 75B2 0000 (displacement 0x030000)
	expect	1351
	bge	*-253
	endexpect
	bge	*-256		; 51A0
	bge	*-258		; 75BE FEFF (displacement -258     = 0xfffefe)
	bge	*-0xfffe	; 75BE 0003 (displacement -0xfffe  = 0xff0002)
	bge	*-0x10000	; 75BE 0001 (displacement -0x10000 = 0xff0000)
	bge	*-0x1fffe	; 75AE 0003 (displacement -0x1fffe = 0xfe0002)
	bge	*-0x20000	; 75AE 0001 (displacement -0x20000 = 0xfe0000)
	bge	*-0x2fffe	; 75BC 0003 (displacement -0x2fffe = 0xfd0002)
	bge	*-0x30000	; 75BC 0001 (displacement -0x30000 = 0xfd0000)

	bcs	*		; 4040
	bcs	*-2		; 5E5E
	bcs	*+2		; 4042
	expect	1351
	bcs	*+255
	endexpect
	bcs	*+254		; 4E5E
	bcs	*+256		; 7440 0100 (displacement 0x000100)
	bcs	*+0x0fffe	; 7440 FFFE (displacement 0x00fffe)
	bcs	*+0x10000	; 7450 0000 (displacement 0x010000)
	bcs	*+0x1fffe	; 7450 FFFE (displacement 0x01fffe)
	bcs	*+0x20000	; 7442 0000 (displacement 0x020000)
	bcs	*+0x2fffe	; 7442 FFFE (displacement 0x02fffe)
	bcs	*+0x30000	; 7452 0000 (displacement 0x030000)
	expect	1351
	bcs	*-253
	endexpect
	bcs	*-256		; 5040
	bcs	*-258		; 745E FEFF (displacement -258     = 0xfffefe)
	bcs	*-0xfffe	; 745E 0003 (displacement -0xfffe  = 0xff0002)
	bcs	*-0x10000	; 745E 0001 (displacement -0x10000 = 0xff0000)
	bcs	*-0x1fffe	; 744E 0003 (displacement -0x1fffe = 0xfe0002)
	bcs	*-0x20000	; 744E 0001 (displacement -0x20000 = 0xfe0000)
	bcs	*-0x2fffe	; 745C 0003 (displacement -0x2fffe = 0xfd0002)
	bcs	*-0x30000	; 745C 0001 (displacement -0x30000 = 0xfd0000)

	bcc	*		; 4060
	bcc	*-2		; 5E7E
	bcc	*+2		; 4062
	expect	1351
	bcc	*+255
	endexpect
	bcc	*+254		; 4E7E
	bcc	*+256		; 7460 0100 (displacement 0x000100)
	bcc	*+0x0fffe	; 7460 FFFE (displacement 0x00fffe)
	bcc	*+0x10000	; 7470 0000 (displacement 0x010000)
	bcc	*+0x1fffe	; 7470 FFFE (displacement 0x01fffe)
	bcc	*+0x20000	; 7462 0000 (displacement 0x020000)
	bcc	*+0x2fffe	; 7462 FFFE (displacement 0x02fffe)
	bcc	*+0x30000	; 7472 0000 (displacement 0x030000)
	expect	1351
	bcc	*-253
	endexpect
	bcc	*-256		; 5060
	bcc	*-258		; 747E FEFF (displacement -258     = 0xfffefe)
	bcc	*-0xfffe	; 747E 0003 (displacement -0xfffe  = 0xff0002)
	bcc	*-0x10000	; 747E 0001 (displacement -0x10000 = 0xff0000)
	bcc	*-0x1fffe	; 746E 0003 (displacement -0x1fffe = 0xfe0002)
	bcc	*-0x20000	; 746E 0001 (displacement -0x20000 = 0xfe0000)
	bcc	*-0x2fffe	; 747C 0003 (displacement -0x2fffe = 0xfd0002)
	bcc	*-0x30000	; 747C 0001 (displacement -0x30000 = 0xfd0000)

	bhi	*		; 4080
	bhi	*-2		; 5E9E
	bhi	*+2		; 4082
	expect	1351
	bhi	*+255
	endexpect
	bhi	*+254		; 4E9E
	bhi	*+256		; 7480 0100 (displacement 0x000100)
	bhi	*+0x0fffe	; 7480 FFFE (displacement 0x00fffe)
	bhi	*+0x10000	; 7490 0000 (displacement 0x010000)
	bhi	*+0x1fffe	; 7490 FFFE (displacement 0x01fffe)
	bhi	*+0x20000	; 7482 0000 (displacement 0x020000)
	bhi	*+0x2fffe	; 7482 FFFE (displacement 0x02fffe)
	bhi	*+0x30000	; 7492 0000 (displacement 0x030000)
	expect	1351
	bhi	*-253
	endexpect
	bhi	*-256		; 5080
	bhi	*-258		; 749E FEFF (displacement -258     = 0xfffefe)
	bhi	*-0xfffe	; 749E 0003 (displacement -0xfffe  = 0xff0002)
	bhi	*-0x10000	; 749E 0001 (displacement -0x10000 = 0xff0000)
	bhi	*-0x1fffe	; 748E 0003 (displacement -0x1fffe = 0xfe0002)
	bhi	*-0x20000	; 748E 0001 (displacement -0x20000 = 0xfe0000)
	bhi	*-0x2fffe	; 749C 0003 (displacement -0x2fffe = 0xfd0002)
	bhi	*-0x30000	; 749C 0001 (displacement -0x30000 = 0xfd0000)

	bls	*		; 40A0
	bls	*-2		; 5EBE
	bls	*+2		; 40A2
	expect	1351
	bls	*+255
	endexpect
	bls	*+254		; 4EBE
	bls	*+256		; 74A0 0100 (displacement 0x000100)
	bls	*+0x0fffe	; 74A0 FFFE (displacement 0x00fffe)
	bls	*+0x10000	; 74B0 0000 (displacement 0x010000)
	bls	*+0x1fffe	; 74B0 FFFE (displacement 0x01fffe)
	bls	*+0x20000	; 74A2 0000 (displacement 0x020000)
	bls	*+0x2fffe	; 74A2 FFFE (displacement 0x02fffe)
	bls	*+0x30000	; 74B2 0000 (displacement 0x030000)
	expect	1351
	bls	*-253
	endexpect
	bls	*-256		; 50A0
	bls	*-258		; 74BE FEFF (displacement -258     = 0xfffefe)
	bls	*-0xfffe	; 74BE 0003 (displacement -0xfffe  = 0xff0002)
	bls	*-0x10000	; 74BE 0001 (displacement -0x10000 = 0xff0000)
	bls	*-0x1fffe	; 74AE 0003 (displacement -0x1fffe = 0xfe0002)
	bls	*-0x20000	; 74AE 0001 (displacement -0x20000 = 0xfe0000)
	bls	*-0x2fffe	; 74BC 0003 (displacement -0x2fffe = 0xfd0002)
	bls	*-0x30000	; 74BC 0001 (displacement -0x30000 = 0xfd0000)

	blo	*		; 4140
	blo	*-2		; 5F5E
	blo	*+2		; 4142
	expect	1351
	blo	*+255
	endexpect
	blo	*+254		; 4F5E
	blo	*+256		; 7540 0100 (displacement 0x000100)
	blo	*+0x0fffe	; 7540 FFFE (displacement 0x00fffe)
	blo	*+0x10000	; 7550 0000 (displacement 0x010000)
	blo	*+0x1fffe	; 7550 FFFE (displacement 0x01fffe)
	blo	*+0x20000	; 7542 0000 (displacement 0x020000)
	blo	*+0x2fffe	; 7542 FFFE (displacement 0x02fffe)
	blo	*+0x30000	; 7552 0000 (displacement 0x030000)
	expect	1351
	blo	*-253
	endexpect
	blo	*-256		; 5140
	blo	*-258		; 755E FEFF (displacement -258     = 0xfffefc)
	blo	*-0xfffe	; 755E 0003 (displacement -0xfffe  = 0xff0002)
	blo	*-0x10000	; 755E 0001 (displacement -0x10000 = 0xff0000)
	blo	*-0x1fffe	; 754E 0003 (displacement -0x1fffe = 0xfe0002)
	blo	*-0x20000	; 754E 0001 (displacement -0x20000 = 0xfe0000)
	blo	*-0x2fffe	; 755C 0003 (displacement -0x2fffe = 0xfd0002)
	blo	*-0x30000	; 755C 0001 (displacement -0x30000 = 0xfd0000)

	bhs	*		; 4160
	bhs	*-2		; 5F7E
	bhs	*+2		; 4162
	expect	1351
	bhs	*+255
	endexpect
	bhs	*+254		; 4F7E
	bhs	*+256		; 7560 0100 (displacement 0x000100)
	bhs	*+0x0fffe	; 7560 FFFE (displacement 0x00fffe)
	bhs	*+0x10000	; 7570 0000 (displacement 0x010000)
	bhs	*+0x1fffe	; 7570 FFFE (displacement 0x01fffe)
	bhs	*+0x20000	; 7562 0000 (displacement 0x020000)
	bhs	*+0x2fffe	; 7562 FFFE (displacement 0x02fffe)
	bhs	*+0x30000	; 7572 0000 (displacement 0x030000)
	expect	1351
	bhs	*-253
	endexpect
	bhs	*-256		; 5160
	bhs	*-258		; 757E FEFF (displacement -258     = 0xfffefe)
	bhs	*-0xfffe	; 757E 0003 (displacement -0xfffe  = 0xff0002)
	bhs	*-0x10000	; 757E 0001 (displacement -0x10000 = 0xff0000)
	bhs	*-0x1fffe	; 756E 0003 (displacement -0x1fffe = 0xfe0002)
	bhs	*-0x20000	; 756E 0001 (displacement -0x20000 = 0xfe0000)
	bhs	*-0x2fffe	; 757C 0003 (displacement -0x2fffe = 0xfd0002)
	bhs	*-0x30000	; 757C 0001 (displacement -0x30000 = 0xfd0000)

	bgt	*		; 40C0
	bgt	*-2		; 5EDE
	bgt	*+2		; 40C2
	expect	1351
	bgt	*+255
	endexpect
	bgt	*+254		; 4EDE
	bgt	*+256		; 74C0 0100 (displacement 0x000100)
	bgt	*+0x0fffe	; 74C0 FFFE (displacement 0x00fffe)
	bgt	*+0x10000	; 74D0 0000 (displacement 0x010000)
	bgt	*+0x1fffe	; 74D0 FFFE (displacement 0x01fffe)
	bgt	*+0x20000	; 74C2 0000 (displacement 0x020000)
	bgt	*+0x2fffe	; 74C2 FFFE (displacement 0x02fffe)
	bgt	*+0x30000	; 74D2 0000 (displacement 0x030000)
	expect	1351
	bgt	*-253
	endexpect
	bgt	*-256		; 50C0
	bgt	*-258		; 74DE FEFF (displacement -258     = 0xfffefe)
	bgt	*-0xfffe	; 74DE 0003 (displacement -0xfffe  = 0xff0002)
	bgt	*-0x10000	; 74DE 0001 (displacement -0x10000 = 0xff0000)
	bgt	*-0x1fffe	; 74CE 0003 (displacement -0x1fffe = 0xfe0002)
	bgt	*-0x20000	; 74CE 0001 (displacement -0x20000 = 0xfe0000)
	bgt	*-0x2fffe	; 74DC 0003 (displacement -0x2fffe = 0xfd0002)
	bgt	*-0x30000	; 74DC 0001 (displacement -0x30000 = 0xfd0000)

	ble	*		; 40E0
	ble	*-2		; 5EFE
	ble	*+2		; 40E2
	expect	1351
	ble	*+255
	endexpect
	ble	*+254		; 4EFE
	ble	*+256		; 74E0 0100 (displacement 0x000100)
	ble	*+0x0fffe	; 74E0 FFFE (displacement 0x00fffe)
	ble	*+0x10000	; 74F0 0000 (displacement 0x010000)
	ble	*+0x1fffe	; 74F0 FFFE (displacement 0x01fffe)
	ble	*+0x20000	; 74E2 0000 (displacement 0x020000)
	ble	*+0x2fffe	; 74E2 FFFE (displacement 0x02fffe)
	ble	*+0x30000	; 74F2 0000 (displacement 0x030000)
	expect	1351
	ble	*-253
	endexpect
	ble	*-256		; 50E0
	ble	*-258		; 74FE FEFF (displacement -258     = 0xfffefe)
	ble	*-0xfffe	; 74FE 0003 (displacement -0xfffe  = 0xff0002)
	ble	*-0x10000	; 74FE 0001 (displacement -0x10000 = 0xff0000)
	ble	*-0x1fffe	; 74EE 0003 (displacement -0x1fffe = 0xfe0002)
	ble	*-0x20000	; 74EE 0001 (displacement -0x20000 = 0xfe0000)
	ble	*-0x2fffe	; 74FC 0003 (displacement -0x2fffe = 0xfd0002)
	ble	*-0x30000	; 74FC 0001 (displacement -0x30000 = 0xfd0000)

	bfs	*		; 4100
	bfs	*-2		; 5F1E
	bfs	*+2		; 4102
	expect	1351
	bfs	*+255
	endexpect
	bfs	*+254		; 4F1E
	bfs	*+256		; 7500 0100 (displacement 0x000100)
	bfs	*+0x0fffe	; 7500 FFFE (displacement 0x00fffe)
	bfs	*+0x10000	; 7510 0000 (displacement 0x010000)
	bfs	*+0x1fffe	; 7510 FFFE (displacement 0x01fffe)
	bfs	*+0x20000	; 7502 0000 (displacement 0x020000)
	bfs	*+0x2fffe	; 7502 FFFE (displacement 0x02fffe)
	bfs	*+0x30000	; 7512 0000 (displacement 0x030000)
	expect	1351
	bfs	*-253
	endexpect
	bfs	*-256		; 5100
	bfs	*-258		; 751E FEFF (displacement -258     = 0xfffefe)
	bfs	*-0xfffe	; 751E 0003 (displacement -0xfffe  = 0xff0002)
	bfs	*-0x10000	; 751E 0001 (displacement -0x10000 = 0xff0000)
	bfs	*-0x1fffe	; 750E 0003 (displacement -0x1fffe = 0xfe0002)
	bfs	*-0x20000	; 750E 0001 (displacement -0x20000 = 0xfe0000)
	bfs	*-0x2fffe	; 751C 0003 (displacement -0x2fffe = 0xfd0002)
	bfs	*-0x30000	; 751C 0001 (displacement -0x30000 = 0xfd0000)

	bfc	*		; 4120
	bfc	*-2		; 5F3E
	bfc	*+2		; 4122
	expect	1351
	bfc	*+255
	endexpect
	bfc	*+254		; 4F3E
	bfc	*+256		; 7520 0100 (displacement 0x000100)
	bfc	*+0x0fffe	; 7520 FFFE (displacement 0x00fffe)
	bfc	*+0x10000	; 7530 0000 (displacement 0x010000)
	bfc	*+0x1fffe	; 7530 FFFE (displacement 0x01fffe)
	bfc	*+0x20000	; 7522 0000 (displacement 0x020000)
	bfc	*+0x2fffe	; 7522 FFFE (displacement 0x02fffe)
	bfc	*+0x30000	; 7532 0000 (displacement 0x030000)
	expect	1351
	bfc	*-253
	endexpect
	bfc	*-256		; 5120
	bfc	*-258		; 753E FEFF (displacement -258     = 0xfffefe)
	bfc	*-0xfffe	; 753E 0003 (displacement -0xfffe  = 0xff0002)
	bfc	*-0x10000	; 753E 0001 (displacement -0x10000 = 0xff0000)
	bfc	*-0x1fffe	; 752E 0003 (displacement -0x1fffe = 0xfe0002)
	bfc	*-0x20000	; 752E 0001 (displacement -0x20000 = 0xfe0000)
	bfc	*-0x2fffe	; 753C 0003 (displacement -0x2fffe = 0xfd0002)
	bfc	*-0x30000	; 753C 0001 (displacement -0x30000 = 0xfd0000)

	br	*		; 41C0
	br	*-2		; 5FDE
	br	*+2		; 41C2
	expect	1351
	br	*+255
	endexpect
	br	*+254		; 4FDE
	br	*+256		; 75C0 0100 (displacement 0x000100)
	br	*+0x0fffe	; 75C0 FFFE (displacement 0x00fffe)
	br	*+0x10000	; 75D0 0000 (displacement 0x010000)
	br	*+0x1fffe	; 75D0 FFFE (displacement 0x01fffe)
	br	*+0x20000	; 75C2 0000 (displacement 0x020000)
	br	*+0x2fffe	; 75C2 FFFE (displacement 0x02fffe)
	br	*+0x30000	; 75D2 0000 (displacement 0x030000)
	expect	1351
	br	*-253
	endexpect
	br	*-256		; 51C0
	br	*-258		; 75DE FEFF (displacement -258     = 0xfffefe)
	br	*-0xfffe	; 75DE 0003 (displacement -0xfffe  = 0xff0002)
	br	*-0x10000	; 75DE 0001 (displacement -0x10000 = 0xff0000)
	br	*-0x1fffe	; 75CE 0003 (displacement -0x1fffe = 0xfe0002)
	br	*-0x20000	; 75CE 0001 (displacement -0x20000 = 0xfe0000)
	br	*-0x2fffe	; 75DC 0003 (displacement -0x2fffe = 0xfd0002)
	br	*-0x30000	; 75DC 0001 (displacement -0x30000 = 0xfd0000)

	blt	*		; 4180
	blt	*-2		; 5F9E
	blt	*+2		; 4182
	expect	1351
	blt	*+255
	endexpect
	blt	*+254		; 4F9E
	blt	*+256		; 7580 0100 (displacement 0x000100)
	blt	*+0x0fffe	; 7580 FFFE (displacement 0x00fffe)
	blt	*+0x10000	; 7590 0000 (displacement 0x010000)
	blt	*+0x1fffe	; 7590 FFFE (displacement 0x01fffe)
	blt	*+0x20000	; 7582 0000 (displacement 0x020000)
	blt	*+0x2fffe	; 7582 FFFE (displacement 0x02fffe)
	blt	*+0x30000	; 7592 0000 (displacement 0x030000)
	expect	1351
	blt	*-253
	endexpect
	blt	*-256		; 5180
	blt	*-258		; 759E FEFF (displacement -258     = 0xfffefe)
	blt	*-0xfffe	; 759E 0003 (displacement -0xfffe  = 0xff0002)
	blt	*-0x10000	; 759E 0001 (displacement -0x10000 = 0xff0000)
	blt	*-0x1fffe	; 758E 0003 (displacement -0x1fffe = 0xfe0002)
	blt	*-0x20000	; 758E 0001 (displacement -0x20000 = 0xfe0000)
	blt	*-0x2fffe	; 759C 0003 (displacement -0x2fffe = 0xfd0002)
	blt	*-0x30000	; 759C 0001 (displacement -0x30000 = 0xfd0000)

	expect	1350
	cbitb	6,(r2)
	endexpect
	expect	1320
	cbitb	8,(r9)
	endexpect
	cbitb	6,(r9)		; 452D
	cbitb	6,0(r9)		; 452D
	cbitb	6,1(r9)		; 052D 0001
	cbitb	6,15(r9)	; 052D 000F
	cbitb	6,16(r9)	; 052D 0010
	cbitb	6,32767(r9)	; 052D 7FFF
	expect	1320
	cbitb	6,32768(r9)
	endexpect
	cbitb	6,-1(r9)	; 052D FFFF
	cbitb	6,-16(r9)	; 052D FFF0
	cbitb	6,-17(r9)	; 052D FFEF
	cbitb	6,-32768(r9)	; 052D 8000
	expect	1315
	cbitb	6,-32769(r9)
	endexpect
	cbitb	6,0x34567	; 052C 4567

	expect	1350
	cbitw	13,(r2)
	endexpect
	expect	1320
	cbitw	16,(r9)
	endexpect
	cbitw	13,(r9)		; 653B
	cbitw	13,0(r9)	; 653B
	cbitw	13,1(r9)	; 253B 0001
	cbitw	13,15(r9)	; 253B 000F
	cbitw	13,16(r9)	; 253B 0010
	cbitw	13,32767(r9)	; 253B 7FFF
	expect	1320
	cbitw	13,32768(r9)
	endexpect
	cbitw	13,-1(r9)	; 253B FFFF
	cbitw	13,-16(r9)	; 253B FFF0
	cbitw	13,-17(r9)	; 253B FFEF
	cbitw	13,-32768(r9)	; 253B 8000
	expect	1315
	cbitw	13,-32769(r9)
	endexpect
	cbitw	13,0x34567	; 253A 4567

	eiwait			; 7FE6

	expect	1350
	jal	r5,r3		; would be small model syntax
	endexpect
	expect	1350
	jal	(r6,r5),8(r4,r3)
	endexpect
	expect	1350
	jal	-4(r6,r5),(r4,r3)
	endexpect
	jal	(r6,r5),(r4,r3)	; 16A6

	expect	1350
	jump	r3		; would be small model syntax
	endexpect
	expect	1350
	jump	10(r4,r3)
	endexpect
	jump	(r4,r3)		; 17C7
	jump	(ra,r13)	; 17DB
	jump	(sp,ra)		; 17DD

	jeq	(r2,r1)		; 1603
	jne	(r3,r2)		; 1625
	jge	(r4,r3)		; 17A7
	jcs	(r5,r4)		; 1649
	jcc	(r6,r5)		; 166B
	jhi	(r7,r6)		; 168D
	jls	(r8,r7)		; 16AF
	jlo	(r9,r8)		; 1751
	jhs	(r10,r9)	; 1773
	jgt	(r11,r10)	; 16D5
	jle	(r12,r11)	; 16F7
	jfs	(r13,r12)	; 1719
	jfc	(ra,r13)	; 173B
	jlt	(sp,ra)		; 179D

	expect	1315
	loadm	0
	endexpect
	loadm	$1		; 7E04
	loadm	2		; 7E24
	loadm	$3		; 7E44
	loadm	4		; 7E64
	expect	1320
	loadm	5
	endexpect

	movd	$0x123456,(r12,r11); 6762 3456

	mulsb	r7,r12		; 618E

	mulsw	r7,(r13,r12)	; 638E

	expect	1445
	muluw	r7,(r13,r12)
	endexpect
	muluw	r9,(r13,r12)	; 7F92

	expect	1315
	push	$0,r4
	endexpect
	push	1,r4		; 6C08
	push	2,r4		; 6C28
	push	3,r4		; 6C48
	push	4,r4		; 6C68
	expect	1320
	push	5,r4
	endexpect

	expect	1315
	pop	$0,r4
	endexpect
	pop	1,r4		; 6C88
	pop	2,r4		; 6CA8
	pop	3,r4		; 6CC8
	pop	4,r4		; 6CE8
	expect	1320
	pop	5,r4
	endexpect

	expect	1315
	popret	$0,r4
	endexpect
	popret	1,r4		; 6D88
	popret	2,r4		; 6DA8
	popret	3,r4		; 6DC8
	popret	4,r4		; 6DE8
	expect	1320
	popret	5,r4
	endexpect

	expect	1350
	sbitb	6,(r2)
	endexpect
	expect	1320
	sbitb	8,(r9)
	endexpect
	sbitb	6,(r9)		; 456D
	sbitb	6,0(r9)		; 456D
	sbitb	6,1(r9)		; 056D 0001
	sbitb	6,15(r9)	; 056D 000F
	sbitb	6,16(r9)	; 056D 0010
	sbitb	6,32767(r9)	; 056D 7FFF
	expect	1320
	sbitb	6,32768(r9)
	endexpect
	sbitb	6,-1(r9)	; 056D FFFF
	sbitb	6,-16(r9)	; 056D FFF0
	sbitb	6,-17(r9)	; 056D FFEF
	sbitb	6,-32768(r9)	; 056D 8000
	expect	1315
	sbitb	6,-32769(r9)
	endexpect
	sbitb	6,0x34567	; 056C 4567

	expect	1350
	sbitw	13,(r2)
	endexpect
	expect	1320
	sbitw	16,(r9)
	endexpect
	sbitw	13,(r9)		; 657B
	sbitw	13,0(r9)	; 657B
	sbitw	13,1(r9)	; 257B 0001
	sbitw	13,15(r9)	; 257B 000F
	sbitw	13,16(r9)	; 257B 0010
	sbitw	13,32767(r9)	; 257B 7FFF
	expect	1320
	sbitw	13,32768(r9)
	endexpect
	sbitw	13,-1(r9)	; 257B FFFF
	sbitw	13,-16(r9)	; 257B FFF0
	sbitw	13,-17(r9)	; 257B FFEF
	sbitw	13,-32768(r9)	; 257B 8000
	expect	1315
	sbitw	13,-32769(r9)
	endexpect
	sbitw	$13,0x34567	; 257A 4567

	expect	1350
	storb	$12,(r3)
	endexpect
	expect	1320
	storb	$16,(r9)
	endexpect
	storb	$12,(r9)	; 45F9
	storb	$12,32767(r9)	; 05F9 7FFF
	expect	1320
	storb	$12,32768(r9)
	endexpect
	storb	$12,-32768(r9)	; 05F9 8000
	expect	1315
	storb	$12,-32769(r9)
	endexpect
	storb	$12,0x34567	; 05F8 4567

	expect	1350
	storw	$12,(r3)
	endexpect
	expect	1320
	storw	$16,(r9)
	endexpect
	storw	$12,(r9)	; 65F9
	storw	$12,32767(r9)	; 25F9 7FFF
	expect	1320
	storw	$12,32768(r9)
	endexpect
	storw	$12,-32768(r9)	; 25F9 8000
	expect	1315
	storw	$12,-32769(r9)
	endexpect
	storw	$12,0x34567	; 25F8 4567

	expect	1315
	storm	0
	endexpect
	storm	$1		; 7E84
	storm	2		; 7EA4
	storm	$3		; 7EC4
	storm	4		; 7EE4
	expect	1320
	storm	5
	endexpect

	tbit	r5,r7		; 76EB
	; TBIT: 5 bit immediate value (unsigned), meaningful values 0..15
	tbit	$0,r7		; 36E0
	tbit	$1,r7		; 36E1
	tbit	$15,r7		; 36EF
	expect	1320
	tbit	$16,r7
	endexpect
	expect	1320
	tbit	$0xffef,r7
	endexpect
	expect	1320
	tbit	$0xfff0,r7
	endexpect
	expect	1320
	tbit	$0xffff,r7
	endexpect
	expect	1315
	tbit	$-1,r7
	endexpect
	expect	1315
	tbit	$-16,r7
	endexpect
	expect	1315
	tbit	$-17,r7
	endexpect
	expect	1315
	tbit	$-32768,r7
	endexpect

	expect	1350
	tbitb	6,(r2)
	endexpect
	expect	1320
	tbitb	8,(r9)
	endexpect
	tbitb	6,(r9)		; 45AD
	tbitb	6,0(r9)		; 45AD
	tbitb	6,1(r9)		; 05AD 0001
	tbitb	6,15(r9)	; 05AD 000F
	tbitb	6,16(r9)	; 05AD 0010
	tbitb	6,32767(r9)	; 05AD 7FFF
	expect	1320
	tbitb	6,32768(r9)
	endexpect
	tbitb	6,-1(r9)	; 05AD FFFF
	tbitb	6,-16(r9)	; 05AD FFF0
	tbitb	6,-17(r9)	; 05AD FFEF
	tbitb	6,-32768(r9)	; 05AD 8000
	expect	1315
	tbitb	6,-32769(r9)
	endexpect
	tbitb	6,0x34567	; 05AC 4567

	expect	1350
	tbitw	13,(r2)
	endexpect
	expect	1320
	tbitw	16,(r9)
	endexpect
	tbitw	13,(r9)		; 65BB
	tbitw	13,0(r9)	; 65BB
	tbitw	13,1(r9)	; 25BB 0001
	tbitw	13,15(r9)	; 25BB 000F
	tbitw	13,16(r9)	; 25BB 0010
	tbitw	13,32767(r9)	; 25BB 7FFF
	expect	1320
	tbitw	13,32768(r9)
	endexpect
	tbitw	13,-1(r9)	; 25BB FFFF
	tbitw	13,-16(r9)	; 25BB FFF0
	tbitw	13,-17(r9)	; 25BB FFEF
	tbitw	13,-32768(r9)	; 25BB 8000
	expect	1315
	tbitw	13,-32769(r9)
	endexpect
	tbitw	13,0x34567	; 25BA 4567

;	tbitw	xx,(r9)
;	tbitw	1,xx(r9)
;	tbitw	1,xx

;	br	unknown
;	br	unknown+512

	cpu	cr16b:model=0

	; different encoding in small model
	expect	1315
	popret	$0,r4
	endexpect
	popret	1,r4		; 6D08
	popret	2,r4		; 6D28
	popret	3,r4		; 6D48
	popret	4,r4		; 6D68
	expect	1320
	popret	5,r4
	endexpect
