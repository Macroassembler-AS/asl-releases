	cpu	80196
	page	0

	expect	1325
	extb	101
	endexpect
	extb	102
	extb	104
	expect	1325,1325
	ext	101
	ext	102
	endexpect
	ext	104

	expect	1325,1325
	norml	101,101
	norml	102,101
	endexpect
	norml	104,101

	shlb	101,#1
	shlb	102,#1
	shlb	104,#1
	expect	1325
	shl	101,#1
	endexpect
	shl	102,#1
	shl	104,#1
	expect	1325,1325
	shll	101,#1
	shll	102,#1
	endexpect
	shll	104,#1

	shrb	101,#1
	shrb	102,#1
	shrb	104,#1
	expect	1325
	shr	101,#1
	endexpect
	shr	102,#1
	shr	104,#1
	expect	1325,1325
	shrl	101,#1
	shrl	102,#1
	endexpect
	shrl	104,#1

	shrab	101,#1
	shrab	102,#1
	shrab	104,#1
	expect	1325
	shra	101,#1
	endexpect
	shra	102,#1
	shra	104,#1
	expect	1325,1325
	shral	101,#1
	shral	102,#1
	endexpect
	shral	104,#1

	expect	1325
	mulb	101,101
	endexpect
	mulb	102,101
	mulb	104,101
	expect	1325,1325
	mul	101,102
	mul	102,102
	endexpect
	mul	104,102

	expect	1325
	mulub	101,101
	endexpect
	mulub	102,101
	mulub	104,101
	expect	1325,1325
	mulu	101,102
	mulu	102,102
	endexpect
	mulu	104,102

	expect	1325
	divb	101,102
	endexpect
	divb	102,102
	divb	104,102
	expect	1325,1325
	div	101,102
	div	102,102
	endexpect
	div	104,102

	expect	1325
	divub	101,102
	endexpect
	divub	102,102
	divub	104,102
	expect	1325,1325
	divu	101,102
	divu	102,102
	endexpect
	divu	104,102
