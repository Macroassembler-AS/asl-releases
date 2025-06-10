        cpu     68040
        supmode on
        pmmu    on
	fpu	on

        move16  (a1)+,(a3)+
        move16  (a4)+,$1234
        move16  $12345,(a5)+
        move16  (a6),$12345
        move16  $1234,(a7)

	cinva	nc
        cinva   dc
        cinva   ic
        cinva   dc/ic
	cinvl	nc,(a1)
	cinvl	dc,(a1)
	cinvl	ic,(a1)
        cinvl   dc/ic,(a1)
	cinvp	nc,(a2)
	cinvp	dc,(a2)
	cinvp	ic,(a2)
        cinvp   dc/ic,(a2)
	cpusha	nc
        cpusha  dc
        cpusha  ic
        cpusha  ic/dc
	cpushl	nc,(a3)
	cpushl	dc,(a3)
	cpushl	ic,(a3)
        cpushl  dc/ic,(a3)
	cpushp	nc,(a4)
	cpushp	dc,(a4)
	cpushp	ic,(a4)
        cpushp  dc/ic,(a4)

        pflushn (a2)
        pflush  (a3)
        pflushan
        pflusha

        ptestw  (a2)
        ptestr  (a4)

	fsneg	fp3
	fdneg	fp3
	fsneg	fp4,fp3
	fdneg	fp4,fp3
	fsneg.x	(a0),fp3
	fdneg.x	(a0),fp3
