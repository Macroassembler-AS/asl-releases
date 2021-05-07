	page	0

	cpu	320c26

	conf	2

	cpu	320c28

	abs
	cmpl
	neg
	rol
	ror
	sfl
	sfr
	zac
	apac
	pac
	spac
	bacc
	cala
	ret
	rfsm
	rtxm
	rxf
	sfsm
	stxm
	sxf
	dint
	eint
	idle
	nop
	pop
	push
	rc
	rhm
	rovm
	rsxm
	rtc
	sc
	shm
	sovm
	ssxm
	stc
	trap

	cnfd
	cnfp

	irp     instr,b,banz,bbnz,bbz,bc,bgez,bgz,bioz,blez,blz,bnc,bnv,bnz,bv,bz,call
	instr	$,*-
	instr	$,*+
	instr	$,*br0-
	instr	$,*0-
	instr	$,*ar0-
	instr	$,*0+
	instr	$,*ar0+
	instr	$,*br0+
	instr	$,*
	instr	$,*-,ar2
	instr	$,*+,ar1
	instr	$,*br0-,ar4
	instr	$,*0-,ar5
	instr	$,*ar0-,ar2
	instr	$,*0+,ar6
	instr	$,*ar0+,ar0
	instr	$,*br0+,ar7
	instr	$,*,ar4
	endm

alladdr	macro	instr
	instr	*-
	instr	*+
	instr	*br0-
	instr	*0-
	instr	*ar0-
	instr	*0+
	instr	*ar0+
	instr	*br0+
	instr	*
	instr	23
	instr	*-,ar2
	instr	*+,ar1
	instr	*br0-,ar4
	instr	*0-,ar5
	instr	*ar0-,ar2
	instr	*0+,ar6
	instr	*ar0+,ar0
	instr	*br0+,ar7
	instr	*,ar4
	endm

	irp	instr,addc,addh,adds,addt,and,lact,or,subb,subc,subh,subs,subt,xor,zalh,zalr,zals,ldp,mar,lph
	alladdr	instr
	endm
	irp	instr,lt,lta,ltd,ltp,lts,mpy,mpya,mpys,mpyu,sph,spl,sqra,sqrs,dmov,tblr,tblw,bitt,lst
	alladdr	instr
	endm
	irp	instr,lst1,popd,pshd,rpt,sst,sst1
	alladdr	instr
	endm

	irp	instr,blkd,blkp,mac,macd
	instr	100,*-
	instr	200,*+
	instr	300,*br0-
	instr	400,*0-
	instr	500,*ar0-
	instr	600,*0+
	instr	700,*ar0+
	instr	800,*br0+
	instr	900,*
	instr	1000,23
	instr	1100,*-,ar2
	instr	1200,*+,ar1
	instr	1300,*br0-,ar4
	instr	1400,*0-,ar5
	instr	1500,*ar0-,ar2
	instr	1600,*0+,ar6
	instr	1700,*ar0+,ar0
	instr	1800,*br0+,ar7
	instr	1900,*,ar4
	endm

	irp	instr,add,lac,sach,sacl,sub,bit
	instr	*-,1
	instr	*+,2
	instr	*br0-,3
	instr	*0-,4
	instr	*ar0-,5
	instr	*0+,6
	instr	*ar0+,7
	instr	*br0+,0
	instr	*,1
	instr	23,2
	instr	*-,3,ar2
	instr	*+,4,ar1
	instr	*br0-,5,ar4
	instr	*0-,6,ar5
	instr	*ar0-,7,ar2
	instr	*0+,1,ar6
	instr	*ar0+,2,ar0
	instr	*br0+,3,ar7
	instr	*,4,ar4
	endm

	irp	instr,in,out
	instr	*-,1
	instr	*+,2
	instr	*br0-,3
	instr	*0-,4
	instr	*ar0-,5
	instr	*0+,6
	instr	*ar0+,7
	instr	*br0+,0
	instr	*,1
	instr	23,2
	instr	*-,3,ar2
	instr	*+,4,ar1
	instr	*br0-,5,ar4
	instr	*0-,6,ar5
	instr	*ar0-,7,ar2
	instr	*0+,1,ar6
	instr	*ar0+,2,ar0
	instr	*br0+,3,ar7
	instr	*,4,ar4
	endm

	irp	instr,addk,lack,subk,adrk,sbrk,rptk,mpyk,spm,cmpr,fort,adlk,andk,lalk,ork,sblk,xork
	instr	1
	endm

	larp	ar2
	larp	2

	irp	instr,lar,sar
	instr	3,*-
	instr	ar3,*+
	instr	4,*br0-
	instr	ar4,*0-
	instr	5,*ar0-
	instr	ar5,*0+
	instr	6,*ar0+
	instr	ar6,*br0+
	instr	7,*
	instr	ar7,23
	instr	0,*-,ar2
	instr	ar0,*+,ar1
	instr	1,*br0-,ar4
	instr	ar1,*0-,ar5
	instr	2,*ar0-,ar2
	instr	ar2,*0+,ar6
	instr	3,*ar0+,ar0
	instr	ar3,*br0+,ar7
	instr	4,*,ar4
	endm

	lark	ar5,57
	lark	2,23

	lrlk	ar4,2
	lrlk	5,7

	ldpk	10
	ldpk	10<<7

	norm	*-
	norm	*+
	norm	*br0-
	norm	*0-
	norm	*ar0-
	norm	*0+
	norm	*ar0+
	norm	*br0+
	norm	*

	data	'a'
	data	'ab'
	data	'abc'	; treated like "abc" due to length
	data	0
	data	65535
	data	-32768
	data	"a"
	data	"ab"
	data	"abc"
	data	"abcd"

	byte	'a'
	byte	'ab'	; treated like "ab" due to length
	byte	'abc'	; treated like "abc" due to length
	byte	0
	byte	255
	byte	-128
	byte	"a"
	byte	"ab"
	byte	"abc"
	byte	"abcd"

	word	'a'
	word	'ab'
	word	'abc'	; treated like "abc" due to length
	word	0
	word	65535
	word	-32768
	word	"a"
	word	"ab"
	word	"abc"
	word	"abcd"

	long	'a'
	long	'ab'
	long	'abc'
	long	'abcd'
	long	0
	long	4294967296
	long	-2147483648
	long	"a"
	long	"ab"
	long	"abc"
	long	"abcd"
	long	"abcde"

	string	'a'
	string	'ab'
	string	0
	string	255
	string	-128
	string	"a"
	string	"ab"
	string	"abc"
	string	"abcd"
	string	"abcde"

	rstring	'a'
	rstring	'ab'
	rstring	0
	rstring	255
	rstring	-128
	rstring	"a"
	rstring	"ab"
	rstring	"abc"
	rstring	"abcd"
	rstring	"abcde"
