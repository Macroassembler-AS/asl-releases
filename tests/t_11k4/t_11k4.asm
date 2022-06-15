		cpu	68hc11k4
		page	0

		; no windowing

		assume	mmwbr:$00,mmsiz:$00,mm1cr:$00,mm2cr:$00
		;prwins

		adr	phys2cpu($0000)		; unbanked -> $0000
		adr	phys2cpu($1000)		; unbanked -> $1000
		adr	phys2cpu($2000)		; unbanked -> $2000
		adr	phys2cpu($3000)		; unbanked -> $3000
		adr	phys2cpu($4000)		; unbanked -> $4000
		adr	phys2cpu($5000)		; unbanked -> $5000
		adr	phys2cpu($6000)		; unbanked -> $6000
		adr	phys2cpu($7000)		; unbanked -> $7000
		adr	phys2cpu($8000)		; unbanked -> $8000
		adr	phys2cpu($9000)		; unbanked -> $9000
		adr	phys2cpu($a000)		; unbanked -> $a000
		adr	phys2cpu($b000)		; unbanked -> $b000
		adr	phys2cpu($c000)		; unbanked -> $c000
		adr	phys2cpu($d000)		; unbanked -> $d000
		adr	phys2cpu($e000)		; unbanked -> $e000
		adr	phys2cpu($f000)		; unbanked -> $f000

		; window 1 @ $4000, 8 K (starts at $10000 in 'AS address space')
		; window 2 disabled

		assume	mmwbr:$04,mmsiz:$41,mm1cr:$00,mm2cr:$00
		;prwins

		adr	phys2cpu($0000)		; unbanked -> $0000
		adr	phys2cpu($1000)		; unbanked -> $1000
		adr	phys2cpu($2000)		; unbanked -> $2000
		adr	phys2cpu($3000)		; unbanked -> $3000
		adr	phys2cpu($10000)	; banked (window 1) -> $4000
		adr	phys2cpu($11000)	; banked (window 1) -> $5000
		adr	phys2cpu($6000)		; unbanked -> $6000
		adr	phys2cpu($7000)		; unbanked -> $7000
		adr	phys2cpu($8000)		; unbanked -> $8000
		adr	phys2cpu($9000)		; unbanked -> $9000
		adr	phys2cpu($a000)		; unbanked -> $a000
		adr	phys2cpu($b000)		; unbanked -> $b000
		adr	phys2cpu($c000)		; unbanked -> $c000
		adr	phys2cpu($d000)		; unbanked -> $d000
		adr	phys2cpu($e000)		; unbanked -> $e000
		adr	phys2cpu($f000)		; unbanked -> $f000

		; window 1 @ $4000, 8 K (starts at $10000 in 'AS address space')
		; window 2 @ $8000, 16 K (starts at $90000 in 'AS address space')
 
		assume  mmwbr:$84,mmsiz:$e1,mm1cr:$00,mm2cr:$00
		;prwins

		adr	phys2cpu($0000)		; unbanked -> $0000
		adr	phys2cpu($1000)		; unbanked -> $1000
		adr	phys2cpu($2000)		; unbanked -> $2000
		adr	phys2cpu($3000)		; unbanked -> $3000
		adr	phys2cpu($10000)	; banked (window 1) -> $4000
		adr	phys2cpu($11000)	; banked (window 1) -> $5000
		adr	phys2cpu($6000)		; unbanked -> $6000
		adr	phys2cpu($7000)		; unbanked -> $7000
		adr	phys2cpu($90000)	; banked (window 2) -> $8000
		adr	phys2cpu($91000)	; banked (window 2) -> $9000
		adr	phys2cpu($92000)	; banked (window 2) -> $a000
		adr	phys2cpu($93000)	; banked (window 2) -> $b000
		adr	phys2cpu($c000)		; unbanked -> $c000
		adr	phys2cpu($d000)		; unbanked -> $d000
		adr	phys2cpu($e000)		; unbanked -> $e000
		adr	phys2cpu($f000)		; unbanked -> $f000

		; window 1 @ $8000, 8 K (starts at $10000 in 'AS address space')
		; window 2 @ $8000, 32 K (starts at $90000 in 'AS address space')
                ; window 1 overshadows first 8K of window 2

		assume	mmwbr:$88,mmsiz:$f1,mm1cr:$00,mm2cr:$00
		;prwins

		adr	phys2cpu($0000)		; unbanked -> $0000
		adr	phys2cpu($1000)		; unbanked -> $1000
		adr	phys2cpu($2000)		; unbanked -> $2000
		adr	phys2cpu($3000)		; unbanked -> $3000
		adr	phys2cpu($4000)		; unbanked -> $4000
		adr	phys2cpu($5000)		; unbanked -> $5000
		adr	phys2cpu($6000)		; unbanked -> $6000
		adr	phys2cpu($7000)		; unbanked -> $7000
		adr	phys2cpu($10000)	; banked (window 1) -> $8000
		adr	phys2cpu($11000)	; banked (window 1) -> $9000
		adr	phys2cpu($92000)	; banked (window 2) -> $a000
		adr	phys2cpu($93000)	; banked (window 2) -> $b000
		adr	phys2cpu($94000)	; banked (window 2) -> $c000
		adr	phys2cpu($95000)	; banked (window 2) -> $d000
		adr	phys2cpu($96000)	; banked (window 2) -> $e000
		adr	phys2cpu($97000)	; banked (window 2) -> $f000

		; window 1 @ $e000, 8 K (starts at $10000 in 'AS address space')
		; window 2 @ $8000, 32 K (starts at $90000 in 'AS address space')
                ; window 1 overshadows last 8K of window 2

		assume	mmwbr:$8e,mmsiz:$f1,mm1cr:$00,mm2cr:$00
		;prwins

		adr	phys2cpu($0000)		; unbanked -> $0000
		adr	phys2cpu($1000)		; unbanked -> $1000
		adr	phys2cpu($2000)		; unbanked -> $2000
		adr	phys2cpu($3000)		; unbanked -> $3000
		adr	phys2cpu($4000)		; unbanked -> $4000
		adr	phys2cpu($5000)		; unbanked -> $5000
		adr	phys2cpu($6000)		; unbanked -> $6000
		adr	phys2cpu($7000)		; unbanked -> $7000
		adr	phys2cpu($90000)	; banked (window 2) -> $8000
		adr	phys2cpu($91000)	; banked (window 2) -> $9000
		adr	phys2cpu($92000)	; banked (window 2) -> $a000
		adr	phys2cpu($93000)	; banked (window 2) -> $b000
		adr	phys2cpu($94000)	; banked (window 2) -> $c000
		adr	phys2cpu($95000)	; banked (window 2) -> $d000
		adr	phys2cpu($10000)	; banked (window 1) -> $e000
		adr	phys2cpu($11000)	; banked (window 1) -> $f000

		; window 1 @ $a000, 8 K (starts at $10000 in 'AS address space')
		; window 2 @ $8000, 32 K (starts at $90000 in 'AS address space')
                ; window 1 partially overshadows window 2, and 'cuts out' 8K in mid of the 32W window 2

		assume	mmwbr:$8a,mmsiz:$f1,mm1cr:$00,mm2cr:$00
		;prwins

		adr	phys2cpu($0000)		; unbanked -> $0000
		adr	phys2cpu($1000)		; unbanked -> $1000
		adr	phys2cpu($2000)		; unbanked -> $2000
		adr	phys2cpu($3000)		; unbanked -> $3000
		adr	phys2cpu($4000)		; unbanked -> $4000
		adr	phys2cpu($5000)		; unbanked -> $5000
		adr	phys2cpu($6000)		; unbanked -> $6000
		adr	phys2cpu($7000)		; unbanked -> $7000
		adr	phys2cpu($90000)	; banked (window 2) -> $8000
		adr	phys2cpu($91000)	; banked (window 2) -> $9000
		adr	phys2cpu($10000)	; banked (window 1) -> $a000
		adr	phys2cpu($11000)	; banked (window 1) -> $b000
		adr	phys2cpu($94000)	; banked (window 2) -> $c000
		adr	phys2cpu($95000)	; banked (window 2) -> $d000
		adr	phys2cpu($96000)	; banked (window 2) -> $e000
		adr	phys2cpu($97000)	; banked (window 2) -> $f000

		; test symmetry of mappings:

		listing	off
addr		set	$0000
		while	addr<$10000
		if 	phys2cpu(cpu2phys(addr)) <> addr
		error	"mapping error at $\{ADDR}"
		endif
addr		set	addr+$10
		endm
		listing	on
