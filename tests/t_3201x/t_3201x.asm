                cpu     32015

                abs
                apac
                cala
                dint
                eint
                nop
                pac
                pop
                push
                ret
                rovm
                sovm
                spac
                zac

                b       345h
                banz    345h
                bgez    345h
                bgz     345h
                bioz    345h
                blez    345h
                blz     345h
                bnz     345h
                bv      345h
                bz      345h
                call    345h

                addh    *,ar1
                adds    *-,ar0
                and     *+,ar1
                dmov    *,1
                ldp     *-,0
                lst     *+,1
                lt      dat123
                lta     dat45
                ltd     12h
                mar     *,ar1
                mpy     *-,ar0
                or      *+,ar1
                sst     *,1
                subc    *-,0
                subh    *+,1
                subs    dat123
                tblr    dat45
                tblw    12h
                xor     *,ar1
                zalh    *-,ar0
                zals    *+,ar1

		add	*
		add	*,0
		add	*,0,ar0
		add	*,4,ar0
		add	*-
		add	*-,0
		add	*-,0,ar0
                add     *-,4,ar0
		add	*+
		add	*+,0
		add	*+,0,ar0
                add     *+,4,ar0
                lac     *+,ar0
                sach    12h,4
                sacl    12h
                sub     12h,6

                lack    12h
                ldpk    1
                mpyk    123h

                in      *,2,ar1
		in	*,pa2,ar1
                in      45h,4
		in	45h,pa4
                out     *,2,ar1
		out	*,pa2,ar1
                out     45h,4
		out	45h,pa4

                larp    ar1
                larp    1

                lar     ar1,12h
                sar     1,*,ar0

                lark    ar1,12h

