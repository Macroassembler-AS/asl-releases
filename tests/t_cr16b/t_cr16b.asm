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
	bal	(r4,r3),*		; 767E FFFD (displacement 0x1ffffc)
	bal	(r4,r3),*+4		; 7660 0000 (displacement 0x000000)
	expect	1351
	bal	(r4,r3),*+255
	endexpect
	bal	(r4,r3),*+256		; 7660 00FC (displacement 0x0000fc)
	bal	(r4,r3),*+258		; 7660 00FE (displacement 0x0000fe)
	bal	(r4,r3),*+0x10002	; 7660 FFFE (displacement 0x00fffe)
	bal	(r4,r3),*+0x10004	; 7670 0000 (displacement 0x010000)
	bal	(r4,r3),*+0x20002	; 7670 FFFE (displacement 0x01fffe)
	bal	(r4,r3),*+0x20004	; 7662 0000 (displacement 0x020000)
	bal	(r4,r3),*+0x30002	; 7662 FFFE (displacement 0x02fffe)
	bal	(r4,r3),*+0x30004	; 7672 0000 (displacement 0x030000)
	expect	1351
	bal	(r4,r3),*-253
	endexpect
	bal	(r4,r3),*-254		; 767E FEFF (displacement -258     = 0xfffefe)
	bal	(r4,r3),*-256		; 767E FEFD (displacement -260     = 0xfffefc)
	bal	(r4,r3),*-0xfffa	; 767E 0003 (displacement -0xfffe  = 0xff0002)
	bal	(r4,r3),*-0xfffc	; 767E 0001 (displacement -0x10000 = 0xff0000)
	bal	(r4,r3),*-0x1fffa	; 766E 0003 (displacement -0x1fffe = 0xfe0002)
	bal	(r4,r3),*-0x1fffc	; 766E 0001 (displacement -0x20000 = 0xfe0000)
	bal	(r4,r3),*-0x2fffa	; 767C 0003 (displacement -0x2fffe = 0xfd0002)
	bal	(r4,r3),*-0x2fffc	; 767C 0001 (displacement -0x30000 = 0xfd0000)

	beq	*		; 5E1E
	beq	*+2		; 4000
	expect	1351
	beq	*+255
	endexpect
	beq	*+256		; 4E1E
	beq	*+258		; 7400 00FE (displacement 0x0000fe)
	beq	*+0x10002	; 7400 FFFE (displacement 0x00fffe)
	beq	*+0x10004	; 7410 0000 (displacement 0x010000)
	beq	*+0x20002	; 7410 FFFE (displacement 0x01fffe)
	beq	*+0x20004	; 7402 0000 (displacement 0x020000)
	beq	*+0x30002	; 7402 FFFE (displacement 0x02fffe)
	beq	*+0x30004	; 7412 0000 (displacement 0x030000)
	expect	1351
	beq	*-253
	endexpect
	beq	*-254		; 5000
	beq	*-256		; 741E FEFE (displacement -260     = 0xfffefc)
	beq	*-0xfffa	; 741E 0003 (displacement -0xfffe  = 0xff0002)
	beq	*-0xfffc	; 741E 0001 (displacement -0x10000 = 0xff0000)
	beq	*-0x1fffa	; 740E 0003 (displacement -0x1fffe = 0xfe0002)
	beq	*-0x1fffc	; 740E 0001 (displacement -0x20000 = 0xfe0000)
	beq	*-0x2fffa	; 741C 0003 (displacement -0x2fffe = 0xfd0002)
	beq	*-0x2fffc	; 741C 0001 (displacement -0x30000 = 0xfd0000)

	expect	1445
	beq0b	r7,*+2
	endexpect
	expect	1370
	beq0b	r9,*
	endexpect
	beq0b	r9,*+2		; 1521
	beq0b	r9,*+32		; 153F
	expect	1370
	beq0b	r9,*+34
	endexpect

	expect	1445
	beq0w	r7,*+2
	endexpect
	expect	1370
	beq0w	r9,*
	endexpect
	beq0w	r9,*+2		; 3521
	beq0w	r9,*+32		; 353F
	expect	1370
	beq0w	r9,*+34
	endexpect

	expect	1445
	beq1b	r7,*+2
	endexpect
	expect	1370
	beq1b	r9,*
	endexpect
	beq1b	r9,*+2		; 1561
	beq1b	r9,*+32		; 157F
	expect	1370
	beq1b	r9,*+34
	endexpect

	expect	1445
	beq1w	r7,*+2
	endexpect
	expect	1370
	beq1w	r9,*
	endexpect
	beq1w	r9,*+2		; 3561
	beq1w	r9,*+32		; 357F
	expect	1370
	beq1w	r9,*+34
	endexpect

	bne	*		; 5E3E
	bne	*+2		; 4020
	expect	1351
	bne	*+255
	endexpect
	bne	*+256		; 4E3E
	bne	*+258		; 7420 00FE (displacement 0x0000fe)
	bne	*+0x10002	; 7420 FFFE (displacement 0x00fffe)
	bne	*+0x10004	; 7430 0000 (displacement 0x010000)
	bne	*+0x20002	; 7430 FFFE (displacement 0x01fffe)
	bne	*+0x20004	; 7422 0000 (displacement 0x020000)
	bne	*+0x30002	; 7422 FFFE (displacement 0x02fffe)
	bne	*+0x30004	; 7432 0000 (displacement 0x030000)
	expect	1351
	bne	*-253
	endexpect
	bne	*-254		; 5020
	bne	*-256		; 743E FEFE (displacement -260     = 0xfffefc)
	bne	*-0xfffa	; 743E 0003 (displacement -0xfffe  = 0xff0002)
	bne	*-0xfffc	; 743E 0001 (displacement -0x10000 = 0xff0000)
	bne	*-0x1fffa	; 742E 0003 (displacement -0x1fffe = 0xfe0002)
	bne	*-0x1fffc	; 742E 0001 (displacement -0x20000 = 0xfe0000)
	bne	*-0x2fffa	; 743C 0003 (displacement -0x2fffe = 0xfd0002)
	bne	*-0x2fffc	; 743C 0001 (displacement -0x30000 = 0xfd0000)

	expect	1445
	bne0b	r7,*+2
	endexpect
	expect	1370
	bne0b	r9,*
	endexpect
	bne0b	r9,*+2		; 15A1
	bne0b	r9,*+32		; 15BF
	expect	1370
	bne0b	r9,*+34
	endexpect

	expect	1445
	bne0w	r7,*+2
	endexpect
	expect	1370
	bne0w	r9,*
	endexpect
	bne0w	r9,*+2		; 35A1
	bne0w	r9,*+32		; 35BF
	expect	1370
	bne0w	r9,*+34
	endexpect

	expect	1445
	bne1b	r7,*+2
	endexpect
	expect	1370
	bne1b	r9,*
	endexpect
	bne1b	r9,*+2		; 15E1
	bne1b	r9,*+32		; 15FF
	expect	1370
	bne1b	r9,*+34
	endexpect

	expect	1445
	bne1w	r7,*+2
	endexpect
	expect	1370
	bne1w	r9,*
	endexpect
	bne1w	r9,*+2		; 35E1
	bne1w	r9,*+32		; 35FF
	expect	1370
	bne1w	r9,*+34
	endexpect

	bge	*		; 5FBE
	bge	*+2		; 41A0
	expect	1351
	bge	*+255
	endexpect
	bge	*+256		; 4FBE
	bge	*+258		; 75A0 00FE (displacement 0x0000fe)
	bge	*+0x10002	; 75A0 FFFE (displacement 0x00fffe)
	bge	*+0x10004	; 75B0 0000 (displacement 0x010000)
	bge	*+0x20002	; 75B0 FFFE (displacement 0x01fffe)
	bge	*+0x20004	; 75A2 0000 (displacement 0x020000)
	bge	*+0x30002	; 75A2 FFFE (displacement 0x02fffe)
	bge	*+0x30004	; 75B2 0000 (displacement 0x030000)
	expect	1351
	bge	*-253
	endexpect
	bge	*-254		; 51A0
	bge	*-256		; 75BE FEFE (displacement -260     = 0xfffefc)
	bge	*-0xfffa	; 75BE 0003 (displacement -0xfffe  = 0xff0002)
	bge	*-0xfffc	; 75BE 0001 (displacement -0x10000 = 0xff0000)
	bge	*-0x1fffa	; 75AE 0003 (displacement -0x1fffe = 0xfe0002)
	bge	*-0x1fffc	; 75AE 0001 (displacement -0x20000 = 0xfe0000)
	bge	*-0x2fffa	; 75BC 0003 (displacement -0x2fffe = 0xfd0002)
	bge	*-0x2fffc	; 75BC 0001 (displacement -0x30000 = 0xfd0000)

	bcs	*		; 5E5E
	bcs	*+2		; 4040
	expect	1351
	bcs	*+255
	endexpect
	bcs	*+256		; 4E5E
	bcs	*+258		; 7440 00FE (displacement 0x0000fe)
	bcs	*+0x10002	; 7440 FFFE (displacement 0x00fffe)
	bcs	*+0x10004	; 7450 0000 (displacement 0x010000)
	bcs	*+0x20002	; 7450 FFFE (displacement 0x01fffe)
	bcs	*+0x20004	; 7442 0000 (displacement 0x020000)
	bcs	*+0x30002	; 7442 FFFE (displacement 0x02fffe)
	bcs	*+0x30004	; 7452 0000 (displacement 0x030000)
	expect	1351
	bcs	*-253
	endexpect
	bcs	*-254		; 5040
	bcs	*-256		; 745E FEFE (displacement -260     = 0xfffefc)
	bcs	*-0xfffa	; 745E 0003 (displacement -0xfffe  = 0xff0002)
	bcs	*-0xfffc	; 745E 0001 (displacement -0x10000 = 0xff0000)
	bcs	*-0x1fffa	; 744E 0003 (displacement -0x1fffe = 0xfe0002)
	bcs	*-0x1fffc	; 744E 0001 (displacement -0x20000 = 0xfe0000)
	bcs	*-0x2fffa	; 745C 0003 (displacement -0x2fffe = 0xfd0002)
	bcs	*-0x2fffc	; 745C 0001 (displacement -0x30000 = 0xfd0000)

	bcc	*		; 5E7E
	bcc	*+2		; 4060
	expect	1351
	bcc	*+255
	endexpect
	bcc	*+256		; 4E6E
	bcc	*+258		; 7470 00FE (displacement 0x0000fe)
	bcc	*+0x10002	; 7470 FFFE (displacement 0x00fffe)
	bcc	*+0x10004	; 7470 0000 (displacement 0x010000)
	bcc	*+0x20002	; 7470 FFFE (displacement 0x01fffe)
	bcc	*+0x20004	; 7462 0000 (displacement 0x020000)
	bcc	*+0x30002	; 7462 FFFE (displacement 0x02fffe)
	bcc	*+0x30004	; 7472 0000 (displacement 0x030000)
	expect	1351
	bcc	*-253
	endexpect
	bcc	*-254		; 5060
	bcc	*-256		; 747E FEFE (displacement -260     = 0xfffefc)
	bcc	*-0xfffa	; 747E 0003 (displacement -0xfffe  = 0xff0002)
	bcc	*-0xfffc	; 747E 0001 (displacement -0x10000 = 0xff0000)
	bcc	*-0x1fffa	; 746E 0003 (displacement -0x1fffe = 0xfe0002)
	bcc	*-0x1fffc	; 746E 0001 (displacement -0x20000 = 0xfe0000)
	bcc	*-0x2fffa	; 747C 0003 (displacement -0x2fffe = 0xfd0002)
	bcc	*-0x2fffc	; 747C 0001 (displacement -0x30000 = 0xfd0000)

	bhi	*		; 5E9E
	bhi	*+2		; 4080
	expect	1351
	bhi	*+255
	endexpect
	bhi	*+256		; 4E8E
	bhi	*+258		; 7480 00FE (displacement 0x0000fe)
	bhi	*+0x10002	; 7480 FFFE (displacement 0x00fffe)
	bhi	*+0x10004	; 7490 0000 (displacement 0x010000)
	bhi	*+0x20002	; 7490 FFFE (displacement 0x01fffe)
	bhi	*+0x20004	; 7482 0000 (displacement 0x020000)
	bhi	*+0x30002	; 7482 FFFE (displacement 0x02fffe)
	bhi	*+0x30004	; 7492 0000 (displacement 0x030000)
	expect	1351
	bhi	*-253
	endexpect
	bhi	*-254		; 5080
	bhi	*-256		; 749E FEFE (displacement -260     = 0xfffefc)
	bhi	*-0xfffa	; 749E 0003 (displacement -0xfffe  = 0xff0002)
	bhi	*-0xfffc	; 749E 0001 (displacement -0x10000 = 0xff0000)
	bhi	*-0x1fffa	; 748E 0003 (displacement -0x1fffe = 0xfe0002)
	bhi	*-0x1fffc	; 748E 0001 (displacement -0x20000 = 0xfe0000)
	bhi	*-0x2fffa	; 749C 0003 (displacement -0x2fffe = 0xfd0002)
	bhi	*-0x2fffc	; 749C 0001 (displacement -0x30000 = 0xfd0000)

	bls	*		; 5EBE
	bls	*+2		; 40A0
	expect	1351
	bls	*+255
	endexpect
	bls	*+256		; 4EBE
	bls	*+258		; 74A0 00FE (displacement 0x0000fe)
	bls	*+0x10002	; 74A0 FFFE (displacement 0x00fffe)
	bls	*+0x10004	; 74B0 0000 (displacement 0x010000)
	bls	*+0x20002	; 74B0 FFFE (displacement 0x01fffe)
	bls	*+0x20004	; 74A2 0000 (displacement 0x020000)
	bls	*+0x30002	; 74A2 FFFE (displacement 0x02fffe)
	bls	*+0x30004	; 74B2 0000 (displacement 0x030000)
	expect	1351
	bls	*-253
	endexpect
	bls	*-254		; 50A0
	bls	*-256		; 74BE FEFE (displacement -260     = 0xfffefc)
	bls	*-0xfffa	; 74BE 0003 (displacement -0xfffe  = 0xff0002)
	bls	*-0xfffc	; 74BE 0001 (displacement -0x10000 = 0xff0000)
	bls	*-0x1fffa	; 74AE 0003 (displacement -0x1fffe = 0xfe0002)
	bls	*-0x1fffc	; 74AE 0001 (displacement -0x20000 = 0xfe0000)
	bls	*-0x2fffa	; 74BC 0003 (displacement -0x2fffe = 0xfd0002)
	bls	*-0x2fffc	; 74BC 0001 (displacement -0x30000 = 0xfd0000)

	blo	*		; 5F5E
	blo	*+2		; 4140
	expect	1351
	blo	*+255
	endexpect
	blo	*+256		; 4F5E
	blo	*+258		; 7540 00FE (displacement 0x0000fe)
	blo	*+0x10002	; 7540 FFFE (displacement 0x00fffe)
	blo	*+0x10004	; 7550 0000 (displacement 0x010000)
	blo	*+0x20002	; 7550 FFFE (displacement 0x01fffe)
	blo	*+0x20004	; 7542 0000 (displacement 0x020000)
	blo	*+0x30002	; 7542 FFFE (displacement 0x02fffe)
	blo	*+0x30004	; 7552 0000 (displacement 0x030000)
	expect	1351
	blo	*-253
	endexpect
	blo	*-254		; 5140
	blo	*-256		; 755E FEFE (displacement -260     = 0xfffefc)
	blo	*-0xfffa	; 755E 0003 (displacement -0xfffe  = 0xff0002)
	blo	*-0xfffc	; 755E 0001 (displacement -0x10000 = 0xff0000)
	blo	*-0x1fffa	; 754E 0003 (displacement -0x1fffe = 0xfe0002)
	blo	*-0x1fffc	; 754E 0001 (displacement -0x20000 = 0xfe0000)
	blo	*-0x2fffa	; 755C 0003 (displacement -0x2fffe = 0xfd0002)
	blo	*-0x2fffc	; 755C 0001 (displacement -0x30000 = 0xfd0000)

	bhs	*		; 5F7E
	bhs	*+2		; 4160
	expect	1351
	bhs	*+255
	endexpect
	bhs	*+256		; 4F7E
	bhs	*+258		; 7560 00FE (displacement 0x0000fe)
	bhs	*+0x10002	; 7560 FFFE (displacement 0x00fffe)
	bhs	*+0x10004	; 7570 0000 (displacement 0x010000)
	bhs	*+0x20002	; 7570 FFFE (displacement 0x01fffe)
	bhs	*+0x20004	; 7562 0000 (displacement 0x020000)
	bhs	*+0x30002	; 7562 FFFE (displacement 0x02fffe)
	bhs	*+0x30004	; 7572 0000 (displacement 0x030000)
	expect	1351
	bhs	*-253
	endexpect
	bhs	*-254		; 5160
	bhs	*-256		; 757E FEFE (displacement -260     = 0xfffefc)
	bhs	*-0xfffa	; 757E 0003 (displacement -0xfffe  = 0xff0002)
	bhs	*-0xfffc	; 757E 0001 (displacement -0x10000 = 0xff0000)
	bhs	*-0x1fffa	; 756E 0003 (displacement -0x1fffe = 0xfe0002)
	bhs	*-0x1fffc	; 756E 0001 (displacement -0x20000 = 0xfe0000)
	bhs	*-0x2fffa	; 757C 0003 (displacement -0x2fffe = 0xfd0002)
	bhs	*-0x2fffc	; 757C 0001 (displacement -0x30000 = 0xfd0000)

	bgt	*		; 5EDE
	bgt	*+2		; 40C0
	expect	1351
	bgt	*+255
	endexpect
	bgt	*+256		; 4EDE
	bgt	*+258		; 74C0 00FE (displacement 0x0000fe)
	bgt	*+0x10002	; 74C0 FFFE (displacement 0x00fffe)
	bgt	*+0x10004	; 74D0 0000 (displacement 0x010000)
	bgt	*+0x20002	; 74D0 FFFE (displacement 0x01fffe)
	bgt	*+0x20004	; 74C2 0000 (displacement 0x020000)
	bgt	*+0x30002	; 74C2 FFFE (displacement 0x02fffe)
	bgt	*+0x30004	; 74D2 0000 (displacement 0x030000)
	expect	1351
	bgt	*-253
	endexpect
	bgt	*-254		; 50C0
	bgt	*-256		; 74DE FEFE (displacement -260     = 0xfffefc)
	bgt	*-0xfffa	; 74DE 0003 (displacement -0xfffe  = 0xff0002)
	bgt	*-0xfffc	; 74DE 0001 (displacement -0x10000 = 0xff0000)
	bgt	*-0x1fffa	; 74CE 0003 (displacement -0x1fffe = 0xfe0002)
	bgt	*-0x1fffc	; 74CE 0001 (displacement -0x20000 = 0xfe0000)
	bgt	*-0x2fffa	; 74DC 0003 (displacement -0x2fffe = 0xfd0002)
	bgt	*-0x2fffc	; 74DC 0001 (displacement -0x30000 = 0xfd0000)

	ble	*		; 5EFE
	ble	*+2		; 40E0
	expect	1351
	ble	*+255
	endexpect
	ble	*+256		; 4EFE
	ble	*+258		; 74E0 00FE (displacement 0x0000fe)
	ble	*+0x10002	; 74E0 FFFE (displacement 0x00fffe)
	ble	*+0x10004	; 74F0 0000 (displacement 0x010000)
	ble	*+0x20002	; 74F0 FFFE (displacement 0x01fffe)
	ble	*+0x20004	; 74E2 0000 (displacement 0x020000)
	ble	*+0x30002	; 74E2 FFFE (displacement 0x02fffe)
	ble	*+0x30004	; 74F2 0000 (displacement 0x030000)
	expect	1351
	ble	*-253
	endexpect
	ble	*-254		; 50E0
	ble	*-256		; 74FE FEFE (displacement -260     = 0xfffefc)
	ble	*-0xfffa	; 74FE 0003 (displacement -0xfffe  = 0xff0002)
	ble	*-0xfffc	; 74FE 0001 (displacement -0x10000 = 0xff0000)
	ble	*-0x1fffa	; 74EE 0003 (displacement -0x1fffe = 0xfe0002)
	ble	*-0x1fffc	; 74EE 0001 (displacement -0x20000 = 0xfe0000)
	ble	*-0x2fffa	; 74FC 0003 (displacement -0x2fffe = 0xfd0002)
	ble	*-0x2fffc	; 74FC 0001 (displacement -0x30000 = 0xfd0000)

	bfs	*		; 5F1E
	bfs	*+2		; 4100
	expect	1351
	bfs	*+255
	endexpect
	bfs	*+256		; 4F1E
	bfs	*+258		; 7500 00FE (displacement 0x0000fe)
	bfs	*+0x10002	; 7500 FFFE (displacement 0x00fffe)
	bfs	*+0x10004	; 7510 0000 (displacement 0x010000)
	bfs	*+0x20002	; 7510 FFFE (displacement 0x01fffe)
	bfs	*+0x20004	; 7502 0000 (displacement 0x020000)
	bfs	*+0x30002	; 7502 FFFE (displacement 0x02fffe)
	bfs	*+0x30004	; 7512 0000 (displacement 0x030000)
	expect	1351
	bfs	*-253
	endexpect
	bfs	*-254		; 5100
	bfs	*-256		; 751E FEFE (displacement -260     = 0xfffefc)
	bfs	*-0xfffa	; 751E 0003 (displacement -0xfffe  = 0xff0002)
	bfs	*-0xfffc	; 751E 0001 (displacement -0x10000 = 0xff0000)
	bfs	*-0x1fffa	; 750E 0003 (displacement -0x1fffe = 0xfe0002)
	bfs	*-0x1fffc	; 750E 0001 (displacement -0x20000 = 0xfe0000)
	bfs	*-0x2fffa	; 751C 0003 (displacement -0x2fffe = 0xfd0002)
	bfs	*-0x2fffc	; 751C 0001 (displacement -0x30000 = 0xfd0000)

	bfc	*		; 5F3E
	bfc	*+2		; 4120
	expect	1351
	bfc	*+255
	endexpect
	bfc	*+256		; 4F3E
	bfc	*+258		; 7520 00FE (displacement 0x0000fe)
	bfc	*+0x10002	; 7520 FFFE (displacement 0x00fffe)
	bfc	*+0x10004	; 7530 0000 (displacement 0x010000)
	bfc	*+0x20002	; 7530 FFFE (displacement 0x01fffe)
	bfc	*+0x20004	; 7522 0000 (displacement 0x020000)
	bfc	*+0x30002	; 7522 FFFE (displacement 0x02fffe)
	bfc	*+0x30004	; 7532 0000 (displacement 0x030000)
	expect	1351
	bfc	*-253
	endexpect
	bfc	*-254		; 5120
	bfc	*-256		; 753E FEFE (displacement -260     = 0xfffefc)
	bfc	*-0xfffa	; 753E 0003 (displacement -0xfffe  = 0xff0002)
	bfc	*-0xfffc	; 753E 0001 (displacement -0x10000 = 0xff0000)
	bfc	*-0x1fffa	; 752E 0003 (displacement -0x1fffe = 0xfe0002)
	bfc	*-0x1fffc	; 752E 0001 (displacement -0x20000 = 0xfe0000)
	bfc	*-0x2fffa	; 753C 0003 (displacement -0x2fffe = 0xfd0002)
	bfc	*-0x2fffc	; 753C 0001 (displacement -0x30000 = 0xfd0000)

	br	*		; 5FDE
	br	*+2		; 41C0
	expect	1351
	br	*+255
	endexpect
	br	*+256		; 4FDE
	br	*+258		; 75C0 00FE (displacement 0x0000fe)
	br	*+0x10002	; 75C0 FFFE (displacement 0x00fffe)
	br	*+0x10004	; 75D0 0000 (displacement 0x010000)
	br	*+0x20002	; 75D0 FFFE (displacement 0x01fffe)
	br	*+0x20004	; 75C2 0000 (displacement 0x020000)
	br	*+0x30002	; 75C2 FFFE (displacement 0x02fffe)
	br	*+0x30004	; 75D2 0000 (displacement 0x030000)
	expect	1351
	br	*-253
	endexpect
	br	*-254		; 51C0
	br	*-256		; 75DE FEFE (displacement -260     = 0xfffefc)
	br	*-0xfffa	; 75DE 0003 (displacement -0xfffe  = 0xff0002)
	br	*-0xfffc	; 75DE 0001 (displacement -0x10000 = 0xff0000)
	br	*-0x1fffa	; 75CE 0003 (displacement -0x1fffe = 0xfe0002)
	br	*-0x1fffc	; 75CE 0001 (displacement -0x20000 = 0xfe0000)
	br	*-0x2fffa	; 75DC 0003 (displacement -0x2fffe = 0xfd0002)
	br	*-0x2fffc	; 75DC 0001 (displacement -0x30000 = 0xfd0000)

	blt	*		; 5F9E
	blt	*+2		; 4180
	expect	1351
	blt	*+255
	endexpect
	blt	*+256		; 4F9E
	blt	*+258		; 7580 00FE (displacement 0x0000fe)
	blt	*+0x10002	; 7580 FFFE (displacement 0x00fffe)
	blt	*+0x10004	; 7590 0000 (displacement 0x010000)
	blt	*+0x20002	; 7590 FFFE (displacement 0x01fffe)
	blt	*+0x20004	; 7582 0000 (displacement 0x020000)
	blt	*+0x30002	; 7582 FFFE (displacement 0x02fffe)
	blt	*+0x30004	; 7592 0000 (displacement 0x030000)
	expect	1351
	blt	*-253
	endexpect
	blt	*-254		; 5180
	blt	*-256		; 759E FEFE (displacement -260     = 0xfffefc)
	blt	*-0xfffa	; 759E 0003 (displacement -0xfffe  = 0xff0002)
	blt	*-0xfffc	; 759E 0001 (displacement -0x10000 = 0xff0000)
	blt	*-0x1fffa	; 758E 0003 (displacement -0x1fffe = 0xfe0002)
	blt	*-0x1fffc	; 758E 0001 (displacement -0x20000 = 0xfe0000)
	blt	*-0x2fffa	; 759C 0003 (displacement -0x2fffe = 0xfd0002)
	blt	*-0x2fffc	; 759C 0001 (displacement -0x30000 = 0xfd0000)

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
