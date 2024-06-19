	cpu	vax-11/780
	page	0

	; NOTE: To avoid rounding errors when converting decimal fractions
	; to binary, compute fractions from integer values

	; negative numbers not supported at all
	expect	1315
	movf	s^#-1.0,r4
	endexpect

	; zero is not supported either (could use clr)
	expect	1315
	movf	s^#0.0,r4
	endexpect

	; just too small
	expect	1315
	movf	s^#(1.0/4.0),r4
	endexpect

	; valid values encoding to 0..63
	movf	s^#(1.0/2.0),r4
	movf	s^#(9.0/16.0),r4
	movf	s^#(5.0/8.0),r4
	movf	s^#(11.0/16.0),r4
	movf	s^#(3.0/4.0),r4
	movf	s^#(13.0/16.0),r4
	movf	s^#(7.0/8.0),r4
	movf	s^#(15.0/16.0),r4
	movf	s^#1.0,r4
	movf	s^#(9.0/8.0),r4
	movf	s^#(5.0/4.0),r4
	movf	s^#(11.0/8.0),r4
	movf	s^#(3.0/2.0),r4
	movf	s^#(13.0/8.0),r4
	movf	s^#(7.0/4.0),r4
	movf	s^#(15.0/8.0),r4
	movf	s^#2.0,r4
	movf	s^#(9.0/4.0),r4
	movf	s^#(5.0/2.0),r4
	movf	s^#(11.0/4.0),r4
	movf	s^#3.0,r4
	movf	s^#(13.0/4.0),r4
	movf	s^#(7.0/2.0),r4
	movf	s^#(15.0/4.0),r4
	movf	s^#4.0,r4
	movf	s^#(9.0/2.0),r4
	movf	s^#5.0,r4
	movf	s^#(11.0/2.0),r4
	movf	s^#6.0,r4
	movf	s^#(13.0/2.0),r4
	movf	s^#7.0,r4
	movf	s^#(15.0/2.0),r4
	movf	s^#8.0,r4
	movf	s^#9.0,r4
	movf	s^#10.0,r4
	movf	s^#11.0,r4
	movf	s^#12.0,r4
	movf	s^#13.0,r4
	movf	s^#14.0,r4
	movf	s^#15.0,r4
	movf	s^#16.0,r4
	movf	s^#18.0,r4
	movf	s^#20.0,r4
	movf	s^#22.0,r4
	movf	s^#24.0,r4
	movf	s^#26.0,r4
	movf	s^#28.0,r4
	movf	s^#30.0,r4
	movf	s^#32.0,r4
	movf	s^#36.0,r4
	movf	s^#40.0,r4
	movf	s^#44.0,r4
	movf	s^#48.0,r4
	movf	s^#52.0,r4
	movf	s^#56.0,r4
	movf	s^#60.0,r4
	movf	s^#64.0,r4
	movf	s^#72.0,r4
	movf	s^#80.0,r4
	movf	s^#88.0,r4
	movf	s^#96.0,r4
	movf	s^#104.0,r4
	movf	s^#112.0,r4
	movf	s^#120.0,r4

	; too large
	expect	1320
	movf	s^#120.1,r4
	endexpect

	; in range, but not in list of possible values:
	expect	1985
	movf	s^#17.0,r4
	endexpect
