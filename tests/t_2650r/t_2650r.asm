	cpu	2650

	org	$0000
	lodr	r2,$0020	; ($0002+$001E) MOD 8K -> $0020
	lodr	r2,$0000	; ($0004+$FFFC) MOD 8K -> $0000
	lodr	r2,$1fff	; ($0006+$FFF9) MOD 8K -> $1FFF
	expect	1330
	lodr	r2,$00ff	; out of range
	endexpect

	org	$1ff0
	lodr	r2,$1fe0	; ($1FF2+$FFEE) MOD 8K -> $1FE0
	lodr	r2,$1ff8	; ($1FF4+$0004) MOD 8K -> $1FF8
	lodr	r2,$0010	; ($1FF6+$001A) MOD 8K -> $0010

	; ZBRR/ZBRS may be seen as a relative branch with an implicit source address of 0:

	zbrr	$0000
	zbrr	$0010
	expect	1370
	zbrr	$0040
	endexpect
        expect	1370
	zbrr	$1fbf
        endexpect
	zbrr	$1fc0
	zbrr	$1fff
