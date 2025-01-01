                cpu     AM29240
                emulated class,convert,sqrt
                supmode on

                page	0
                relaxed	on

                include reg29k

                add	r128,r129,r130
                add	r129,r130,131
                add	r130,r131
                add	r131,132
                ; andere Op's dito...

                dadd	r132,r133,r134
                dadd	r133,r134
                ; andere Op's dito

                aseq	134,r135,r136
                aseq	135,r136,137

                call	r136,$+0x048d0
                call	r137,0x30000
                call	r138,r139
                calli	r139,r140
                jmp	$
                jmpi	r141
                jmpf	r142,$
                jmpfi	r143,r144
                jmpfdec r144,$
                jmpt	r145,$
                jmpti	r146,r147

                load    0,27,r160,r161
                loadl	27,r161,r162
                loadm	27,r162,163
                loadset 13,r163,r164
                store	13,r164,165
                storel	13,r165,r166
                storem  7,r166,167

                class   r147,r148,2
                sqrt	r147,r148,2
                sqrt	r149,2

                clz	r148,r149
                clz	r149,150

                const	r150,151
                const	r151,-152
                const	r152,0x153154
                consth	r153,500
                constn	r154,0xffff1234
                constn	r155,-5
                constn	r156,0123

                convert r157,r158,1,2,2,1
                exhws	r158,r159

                halt
                iret

                inv
                iretinv	2

                emulate 20,lr10,lr11

                mfsr	r148,lru
                mtsr	ipc,r148
                mtsrim	lru,0xa55a
                mftlb	r159,r160
                mttlb	r160,r161

                add	lr122,lr110,0
                addc	gr10,gr30

                add	gr20,gr40,gr60
                assume	rbp:0b0000000000000010
		add	gr20,gr40,gr60

		; register symbols

reg_r223	reg	r223
reg_gr52	equ	gr52
reg_lr119	reg	lr119

		add	r223,gr52,lr119
		add	reg_r223,reg_gr52,reg_lr119
		add	r223,gr52,42
		add	reg_r223,reg_gr52,42

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

		include "../t_dx/t_dn.inc"
		include "../t_dx/t_db.inc"
		include "../t_dx/t_dw.inc"
		include "../t_dx/t_dd.inc"
		include "../t_dx/t_dq.inc"
		include "../t_dx/t_dt.inc"
		include "../t_dx/t_do.inc"
