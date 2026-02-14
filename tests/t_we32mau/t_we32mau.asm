	page	0
	cpu	we32200
	fpu	on

	; ABS internally always takes one source an one destination
	; operand.  If the operation is in-place, only one arg needs to be written:

	mfabss1	%f1		; 32 F1 30 00 00 = SPOP 0x000030f1 (ABS %f1,,%s1)
	mfabss	%f1
	mfabsd1	%f1		; 32 F5 30 00 00 = SPOP 0x000030f5 (ABS %f1,,%d1)
	mfabsd	%f1
	mfabsx1	%f1		; 32 F9 30 00 00 = SPOP 0x000030f9 (ABS %f1,,%x1)
	mfabsx	%f1
	mfabss1	(%r4)		; 23 7C 32 00 00 54 54 = SPOPS2 0x0000327c (ABS ms,,ms), (%r4), (%r4)
	mfabss	(%r4)
	mfabsd1	(%r4)		; 03 FD 32 00 00 54 54 = SPOPD2 0x000032fd (ABS md,,md), (%r4), (%r4)
	mfabsd	(%r4)
	mfabsx1	(%r4)		; 07 7E 33 00 00 54 54 = SPOPT2 0x0000337e (ABS mt,,mt), (%r4), (%r4)
	mfabsx	(%r4)

	mfabss2	%f1,%f2		; 32 F2 30 00 00 = SPOP 0x000030f2 (ABS %f1,,%s2)
	mfabss	%f1,%f2
	mfabsd2	%f1,%f2		; 32 F6 30 00 00 = SPOP 0x000030f6 (ABS %f1,,%d2)
	mfabsd	%f1,%f2
	mfabsx2	%f1,%f2		; 32 FA 30 00 00 = SPOP 0x000030fa (ABS %f1,,%x2)
	mfabsx	%f1,%f2
	mfabss2	(%r4),%f2	; 22 72 32 00 00 54 = SPOPRS 0x00003272 (ABS ms,,%s2), (%r4)
	mfabss	(%r4),%f2
	mfabsd2	(%r4),%f2	; 02 F6 32 00 00 54 = SPOPRD 0x000032f6 (ABS md,,%d2), (%r4)
	mfabsd	(%r4),%f2
	mfabsx2	(%r4),%f2	; 06 7A 33 00 00 54 = SPOPRT 0x0000337a (ABS mt,,%x2), (%r4)
	mfabsx	(%r4),%f2
	mfabss2	%f1,(%r4)	; 33 FC 30 00 00 54 = SPOPWS 0x000030fc (ABS %f1,,ms), (%r4)
	mfabss	%f1,(%r4)
	mfabsd2	%f1,(%r4)	; 13 FD 30 00 00 54 = SPOPWD 0x000030fd (ABS %f1,,md), (%r4)
	mfabsd	%f1,(%r4)
	mfabsx2	%f1,(%r4)	; 17 FE 30 00 00 54 = SPOPWT 0x000030fe (ABS %f1,,mt), (%r4)
	mfabsx	%f1,(%r4)
	mfabss2	(%r3),(%r4)	; 23 7C 32 00 00 53 54 = SPOPS2 0x0000327c (ABS ms,,ms), (%r3), (%r4)
	mfabss	(%r3),(%r4)
	mfabsd2	(%r3),(%r4)	; 03 FD 32 00 00 53 54 = SPOPD2 0x000032fd (ABS ms,,md), (%r3), (%r4)
	mfabsd	(%r3),(%r4)
	mfabsx2	(%r3),(%r4)	; 07 7E 33 00 00 53 54 = SPOPT2 0x0000337e (ABS ms,,mt), (%r3), (%r4)
	mfabsx	(%r3),(%r4)

	; Similar with ADD, just 2/3 instead of 1/2 arguments:

	mfadds2	%f1,%f2		; 32 A2 08 00 00 = SPOP 0x000008a2 (ADD %f1,%f1,%s2)
	mfadds	%f1,%f2
	mfaddd2	%f1,%f2		; 32 A6 08 00 00 = SPOP 0x000008a6 (ADD %f1,%f1,%d2)
	mfaddd	%f1,%f2
	mfaddx2	%f1,%f2		; 32 AA 08 00 00 = SPOP 0x000008aa (ADD %f1,%f1,%x2)
	mfaddx	%f1,%f2
	mfadds2	(%r1),%f2	; 22 22 0A 00 00 51 = SPOPRS 0x00000a22 (ADD ms,%f2,%s2), (%r1)
	mfadds	(%r1),%f2
	mfaddd2	(%r1),%f2	; 02 A6 0A 00 00 51 = SPOPRD 0x00000aa6 (ADD md,%f2,%d2), (%r1)
	mfaddd	(%r1),%f2
	mfaddx2	(%r1),%f2	; 06 2A 0B 00 00 51 = SPOPRT 0x00000b2a (ADD mt,%f2,%x2), (%r1)
	mfaddx	(%r1),%f2
	mfadds2	%f1,(%r2)	; 23 CC 08 00 00 52 52 = SPOPS2 0x000008cc (ADD %f1,ms,ms), (%r2), (%r2)
	mfadds	%f1,(%r2)
	mfaddd2	%f1,(%r2)	; 03 DD 08 00 00 52 52 = SPOPD2 0x000008dd (ADD %f1,md,md), (%r2), (%r2)
	mfaddd	%f1,(%r2)
	mfaddx2	%f1,(%r2)	; 07 EE 08 00 00 52 52 = SPOPT2 0x000008ee (ADD %f1,mt,mt), (%r2), (%r2)
	mfaddx	%f1,(%r2)
	mfadds2	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 23 3C 0A 00 00 51 52 = SPOPS2 0x00000a3c (ADD ms,%f3,ms), (%r1), (%r2)
	mfadds	(%r1),(%r2)
	mfaddd2	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2); 03 BD 0A 00 00 51 52 = SPOPD2 0x00000abd (ADD md,%f3,md), (%r1), (%r2)
	mfaddd	(%r1),(%r2)
	mfaddx2	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2); 07 3E 0B 00 00 51 52 = SPOPT2 0x00000b3e (ADD mt,%f3,mt), (%r1), (%r2)
	mfaddx	(%r1),(%r2)

	; Due to 2 instructions, PC-relative distances differ in 2nd instruction:
	mfadds2	.+0x100,.+0x100	; 22 73 1E 00 00 AF 00 01 = SPOPRS 0x00001e73 (MOVE ms,,%s3), 0x100(%pc); 23 3C 0A 00 00 AF F8 00 AF F8 00 = SPOPS2 0x00000a3c (ADD ms,%f3,ms), 0xf8(pc), 0xf8(pc)
	mfaddd2	.+0x100,.+0x100	; 02 F7 1E 00 00 AF 00 01 = SPOPRD 0x00001ef7 (MOVE md,,%d3), 0x100(%pc); 03 BD 0A 00 00 AF F8 00 AF F8 00 = SPOPD2 0x00000abd (ADD md,%f3,md), 0xf8(pc), 0xf8(pc)
	mfaddx2	.+0x100,.+0x100	; 06 7B 1F 00 00 AF 00 01 = SPOPRT 0x00001f7b (MOVE mt,,%x3), 0x100(%pc); 07 3E 0B 00 00 AF F8 00 AF F8 00 = SPOPT2 0x00000b3e (ADD mt,%f3,mt), 0xf8(pc), 0xf8(pc)

	mfadds3	%f1,%f2,%f5	; 32 A1 08 04 00 = SPOP 0x000408a1 (ADD %f1,%f2,%s5)
	mfadds	%f1,%f2,%f5
	mfaddd3	%f1,%f2,%f5	; 32 A5 08 04 00 = SPOP 0x000408a5 (ADD %f1,%f2,%d5)
	mfaddd	%f1,%f2,%f5
	mfaddx3	%f1,%f2,%f5	; 32 A9 08 04 00 = SPOP 0x000408a9 (ADD %f1,%f2,%x5)
	mfaddx	%f1,%f2,%f5
	mfadds3	(%r1),%f2,%f5	; 22 21 0A 04 00 51 = SPOPRS 0x00040a21 (ADD ms,%f2,%s5), (%r1)
	mfadds	(%r1),%f2,%f5
	mfaddd3	(%r1),%f2,%f5	; 02 A5 0A 04 00 51 = SPOPRD 0x00040aa5 (ADD md,%f2,%d5), (%r1)
	mfaddd	(%r1),%f2,%f5
	mfaddx3	(%r1),%f2,%f5	; 06 29 0B 04 00 51 = SPOPRT 0x00040b29 (ADD mt,%f2,%x5), (%r1)
	mfaddx	(%r1),%f2,%f5
	mfadds3	%f1,(%r2),%f5	; 22 C1 08 04 00 52 = SPOPRS 0x000408c1 (ADD %f1,ms,%s5), (%r2)
	mfadds	%f1,(%r2),%f5
	mfaddd3	%f1,(%r2),%f5	; 02 D5 0A 04 00 52 = SPOPRD 0x00040ad5 (ADD %f1,md,%d5), (%r2)
	mfaddd	%f1,(%r2),%f5
	mfaddx3	%f1,(%r2),%f5	; 06 E9 0B 04 00 52 = SPOPRT 0x00040be9 (ADD %f1,mt,%x5), (%r2)
	mfaddx	%f1,(%r2),%f5
	mfadds3	(%r1),(%r2),%f5	; 22 71 1E 04 00 52 = SPOPRS 0x00041e71 (MOVE ms,,%s5) (%r2) ; 22 11 0A 0C 00 51 = SPOPRS 0x000c0a11 (ADD ms,%f5,%s5), (%r1)
	mfadds	(%r1),(%r2),%f5
	mfaddd3	(%r1),(%r2),%f5	; 02 F5 1E 04 00 52 = SPOPRD 0x00041ef5 (MOVE md,,%d5) (%r2) ; 02 95 0A 0C 00 51 = SPOPRD 0x000c0a95 (ADD md,%f5,%d5), (%r1)
	mfaddd	(%r1),(%r2),%f5
	mfaddx3	(%r1),(%r2),%f5	; 06 79 1F 04 00 52 = SPOPRT 0x00041f79 (MOVE mt,,%x5) (%r2) ; 06 19 0B 0C 00 51 = SPOPRT 0x000c0b19 (ADD mt,%f5,%x5), (%r1)
	mfaddx	(%r1),(%r2),%f5
	mfadds3	%f1,%f2,(%r5)	; 33 AC 08 00 00 55 = SPOPWS 0x000008ac (ADD %f1,%f2,ms), (%r5)
	mfadds	%f1,%f2,(%r5)
	mfaddd3	%f1,%f2,(%r5)	; 13 AD 08 00 00 55 = SPOPWD 0x000008ad (ADD %f1,%f2,md), (%r5)
	mfaddd	%f1,%f2,(%r5)
	mfaddx3	%f1,%f2,(%r5)	; 17 AE 08 00 00 55 = SPOPWT 0x000008ae (ADD %f1,%f2,mt), (%r5)
	mfaddx	%f1,%f2,(%r5)
	mfadds3	(%r1),%f2,(%r5)	; 23 2C 0A 00 00 51 55 = SPOPS2 0x00000a2c (ADD ms,%f2,ms), (%r1), (%r5)
	mfadds	(%r1),%f2,(%r5)
	mfaddd3	(%r1),%f2,(%r5)	; 03 AD 0A 00 00 51 55 = SPOPD2 0x00000aad (ADD md,%f2,md), (%r1), (%r5)
	mfaddd	(%r1),%f2,(%r5)
	mfaddx3	(%r1),%f2,(%r5)	; 07 2E 0B 00 00 51 55 = SPOPT2 0x00000b2e (ADD mt,%f2,mt), (%r1), (%r5)
	mfaddx	(%r1),%f2,(%r5)
	mfadds3	%f1,(%r2),(%r5)	; 23 CC 08 00 00 52 55 = SPOPS2 0x000008cc (ADD %f2,ms,ms), (%r2), (%r5)
	mfadds	%f1,(%r2),(%r5)
	mfaddd3	%f1,(%r2),(%r5)	; 03 DD 08 00 00 52 55 = SPOPD2 0x000008dd (ADD %f2,md,md), (%r2), (%r5)
	mfaddd	%f1,(%r2),(%r5)
	mfaddx3	%f1,(%r2),(%r5)	; 07 EE 08 00 00 52 55 = SPOPT2 0x000008ee (ADD %f2,mt,mt), (%r2), (%r5)
	mfaddx	%f1,(%r2),(%r5)
	mfadds3	(%r1),(%r2),(%r5)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2) ; 23 3C 0A 00 00 51 55 = SPOPS2 0x00000a3c (ADD ms,%f3,ms), (%r1), (%r5)
	mfadds	(%r1),(%r2),(%r5)
	mfaddd3	(%r1),(%r2),(%r5)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2) ; 03 BD 0A 00 00 51 55 = SPOPD2 0x00000abd (ADD ms,%f3,ms), (%r1), (%r5)
	mfaddd	(%r1),(%r2),(%r5)
	mfaddx3	(%r1),(%r2),(%r5)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2) ; 07 3E 0B 00 00 51 55 = SPOPT2 0x00000b3e (ADD ms,%f3,ms), (%r1), (%r5)
	mfaddx	(%r1),(%r2),(%r5)

	mfatans1 %f1		; 32 F1 78 00 00 = SPOP 0x000078f1 (ATAN %f1,,%s1)
	mfatans	%f1
	mfatand1 %f1		; 32 F5 78 00 00 = SPOP 0x000078f5 (ATAN %f1,,%d1)
	mfatand	%f1
	mfatanx1 %f1		; 32 F9 78 00 00 = SPOP 0x000078f9 (ATAN %f1,,%x1)
	mfatanx	%f1
	mfatans1 (%r4)		; 23 7C 7A 00 00 54 54 = SPOPS2 0x00007a7c (ATAN ms,,ms), (%r4), (%r4)
	mfatans	(%r4)
	mfatand1 (%r4)		; 03 FD 7A 00 00 54 54 = SPOPD2 0x00007afd (ATAN md,,md), (%r4), (%r4)
	mfatand	(%r4)
	mfatanx1 (%r4)		; 07 7E 7B 00 00 54 54 = SPOPT2 0x00007b7e (ATAN mt,,mt), (%r4), (%r4)
	mfatanx	(%r4)

	mfatans2 %f1,%f2	; 32 F2 78 00 00 = SPOP 0x000078f2 (ATAN %f1,,%s2)
	mfatans	%f1,%f2
	mfatand2 %f1,%f2	; 32 F6 78 00 00 = SPOP 0x000078f6 (ATAN %f1,,%d2)
	mfatand	%f1,%f2
	mfatanx2 %f1,%f2	; 32 FA 78 00 00 = SPOP 0x000078fa (ATAN %f1,,%x2)
	mfatanx	%f1,%f2
	mfatans2 (%r4),%f2	; 22 72 7A 00 00 54 = SPOPRS 0x00007a72 (ATAN ms,,%s2), (%r4)
	mfatans	(%r4),%f2
	mfatand2 (%r4),%f2	; 02 F6 7A 00 00 54 = SPOPRD 0x00007af6 (ATAN md,,%d2), (%r4)
	mfatand	(%r4),%f2
	mfatanx2 (%r4),%f2	; 06 7A 7B 00 00 54 = SPOPRT 0x00007b7a (ATAN mt,,%x2), (%r4)
	mfatanx	(%r4),%f2
	mfatans2 %f1,(%r4)	; 33 FC 78 00 00 54 = SPOPWS 0x000078fc (ATAN %f1,,ms), (%r4)
	mfatans	%f1,(%r4)
	mfatand2 %f1,(%r4)	; 13 FD 78 00 00 54 = SPOPWD 0x000078fd (ATAN %f1,,md), (%r4)
	mfatand	%f1,(%r4)
	mfatanx2 %f1,(%r4)	; 17 FE 78 00 00 54 = SPOPWT 0x000078fe (ATAN %f1,,mt), (%r4)
	mfatanx	%f1,(%r4)
	mfatans2 (%r3),(%r4)	; 23 7C 7A 00 00 53 54 = SPOPS2 0x00007a7c (ATAN ms,,ms), (%r3), (%r4)
	mfatans	(%r3),(%r4)
	mfatand2 (%r3),(%r4)	; 03 FD 7A 00 00 53 54 = SPOPD2 0x00007afd (ATAN ms,,md), (%r3), (%r4)
	mfatand	(%r3),(%r4)
	mfatanx2 (%r3),(%r4)	; 07 7E 7B 00 00 53 54 = SPOPT2 0x00007b7e (ATAN ms,,mt), (%r3), (%r4)
	mfatanx	(%r3),(%r4)

	mfcmps	%f1,%f2		; 32 AF 28 00 00 = SPOP 0x000028af (CMP %f1,%f2,)
	mfcmpd	%f1,%f2		; 32 AF 28 00 00 = SPOP 0x000028af (CMP %f1,%f2,)
	mfcmpx	%f1,%f2		; 32 AF 28 00 00 = SPOP 0x000028af (CMP %f1,%f2,)
	mfcmps	(%r1),%f2	; 22 2F 2A 00 00 51 = SPOPRS 0x00002a2f (CMP ms,%f2,), (%r1)
	mfcmpd	(%r1),%f2	; 02 AF 2A 00 00 51 = SPOPRD 0x00002aaf (CMP md,%f2,), (%r1)
	mfcmpx	(%r1),%f2	; 06 2F 2B 00 00 51 = SPOPRT 0x00002b2f (CMP mt,%f2,), (%r1)
	mfcmps	%f1,(%r2)	; 22 CF 28 00 00 52 = SPOPRS 0x000028cf (CMP %f1,ms,), (%r2)
	mfcmpd	%f1,(%r2)	; 02 DF 28 00 00 52 = SPOPRD 0x000028df (CMP %f1,md,), (%r2)
	mfcmpx	%f1,(%r2)	; 06 EF 28 00 00 52 = SPOPRT 0x000028ef (CMP %f1,mt,), (%r2)
	mfcmps	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 22 3F 2A 00 00 51 SPOPRS 0x00002a3f (CMP ms,%f3,), (%r1)
	mfcmpd	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRS 0x00001ef7 (MOVE ms,,%d3), (%r2); 02 BF 2A 00 00 51 SPOPRD 0x00002abf (CMP md,%f3,), (%r1)
	mfcmpx	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRS 0x00001f7b (MOVE ms,,%x3), (%r2); 06 3F 2B 00 00 51 SPOPRT 0x00002b3f (CMP mt,%f3,), (%r1)

	mfcmpts	%f1,%f2		; 32 AF 2C 00 00 = SPOP 0x00002caf (CMPE %f1,%f2,)
	mfcmptd	%f1,%f2		; 32 AF 2C 00 00 = SPOP 0x00002caf (CMPE %f1,%f2,)
	mfcmptx	%f1,%f2		; 32 AF 2C 00 00 = SPOP 0x00002caf (CMPE %f1,%f2,)
	mfcmpts	(%r1),%f2	; 22 2F 2E 00 00 51 = SPOPRS 0x00002e2f (CMPE ms,%f2,), (%r1)
	mfcmptd	(%r1),%f2	; 02 AF 2E 00 00 51 = SPOPRD 0x00002eaf (CMPE md,%f2,), (%r1)
	mfcmptx	(%r1),%f2	; 06 2F 2F 00 00 51 = SPOPRT 0x00002f2f (CMPE mt,%f2,), (%r1)
	mfcmpts	%f1,(%r2)	; 22 CF 2C 00 00 52 = SPOPRS 0x00002ccf (CMPE %f1,ms,), (%r2)
	mfcmptd	%f1,(%r2)	; 02 DF 2C 00 00 52 = SPOPRD 0x00002cdf (CMPE %f1,md,), (%r2)
	mfcmptx	%f1,(%r2)	; 06 EF 2C 00 00 52 = SPOPRT 0x00002cef (CMPE %f1,mt,), (%r2)
	mfcmpts	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 22 3F 2E 00 00 51 SPOPRS 0x00002e3f (CMPE ms,%f3,), (%r1)
	mfcmptd	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRS 0x00001ef7 (MOVE ms,,%d3), (%r2); 02 BF 2E 00 00 51 SPOPRD 0x00002ebf (CMPE md,%f3,), (%r1)
	mfcmptx	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRS 0x00001f7b (MOVE ms,,%x3), (%r2); 06 3F 2F 00 00 51 SPOPRT 0x00002f3f (CMPE mt,%f3,), (%r1)

	mfcoss1	%f1		; 32 F1 74 00 00 = SPOP 0x000074f1 (COS %f1,,%s1)
	mfcoss	%f1
	mfcosd1	%f1		; 32 F5 74 00 00 = SPOP 0x000074f5 (COS %f1,,%d1)
	mfcosd	%f1
	mfcosx1	%f1		; 32 F9 74 00 00 = SPOP 0x000074f9 (COS %f1,,%x1)
	mfcosx	%f1
	mfcoss1	(%r4)		; 23 7C 76 00 00 54 54 = SPOPS2 0x0000767c (COS ms,,ms), (%r4), (%r4)
	mfcoss	(%r4)
	mfcosd1	(%r4)		; 03 FD 76 00 00 54 54 = SPOPD2 0x000076fd (COS md,,md), (%r4), (%r4)
	mfcosd	(%r4)
	mfcosx1	(%r4)		; 07 7E 77 00 00 54 54 = SPOPT2 0x0000777e (COS mt,,mt), (%r4), (%r4)
	mfcosx	(%r4)

	mfcoss2	%f1,%f2		; 32 F2 74 00 00 = SPOP 0x000074f2 (COS %f1,,%s2)
	mfcoss	%f1,%f2
	mfcosd2	%f1,%f2		; 32 F6 74 00 00 = SPOP 0x000074f6 (COS %f1,,%d2)
	mfcosd	%f1,%f2
	mfcosx2	%f1,%f2		; 32 FA 74 00 00 = SPOP 0x000074fa (COS %f1,,%x2)
	mfcosx	%f1,%f2
	mfcoss2	(%r4),%f2	; 22 72 76 00 00 54 = SPOPRS 0x00007672 (COS ms,,%s2), (%r4)
	mfcoss	(%r4),%f2
	mfcosd2	(%r4),%f2	; 02 F6 76 00 00 54 = SPOPRD 0x000076f6 (COS md,,%d2), (%r4)
	mfcosd	(%r4),%f2
	mfcosx2	(%r4),%f2	; 06 7A 77 00 00 54 = SPOPRT 0x0000777a (COS mt,,%x2), (%r4)
	mfcosx	(%r4),%f2
	mfcoss2	%f1,(%r4)	; 33 FC 74 00 00 54 = SPOPWS 0x000074fc (COS %f1,,ms), (%r4)
	mfcoss	%f1,(%r4)
	mfcosd2	%f1,(%r4)	; 13 FD 74 00 00 54 = SPOPWD 0x000074fd (COS %f1,,md), (%r4)
	mfcosd	%f1,(%r4)
	mfcosx2	%f1,(%r4)	; 17 FE 74 00 00 54 = SPOPWT 0x000074fe (COS %f1,,mt), (%r4)
	mfcosx	%f1,(%r4)
	mfcoss2	(%r3),(%r4)	; 23 7C 76 00 00 53 54 = SPOPS2 0x0000767c (COS ms,,ms), (%r3), (%r4)
	mfcoss	(%r3),(%r4)
	mfcosd2	(%r3),(%r4)	; 03 FD 76 00 00 53 54 = SPOPD2 0x000076fd (COS ms,,md), (%r3), (%r4)
	mfcosd	(%r3),(%r4)
	mfcosx2	(%r3),(%r4)	; 07 7E 77 00 00 53 54 = SPOPT2 0x0000777e (COS ms,,mt), (%r3), (%r4)
	mfcosx	(%r3),(%r4)

	mfdivs2	%f1,%f2		; 32 A2 10 00 00 = SPOP 0x000010a2 (DIV %f1,%f1,%s2)
	mfdivs	%f1,%f2
	mfdivd2	%f1,%f2		; 32 A6 10 00 00 = SPOP 0x000010a6 (DIV %f1,%f1,%d2)
	mfdivd	%f1,%f2
	mfdivx2	%f1,%f2		; 32 AA 10 00 00 = SPOP 0x000010aa (DIV %f1,%f1,%x2)
	mfdivx	%f1,%f2
	mfdivs2	(%r1),%f2	; 22 22 12 00 00 51 = SPOPRS 0x00001222 (DIV ms,%f2,%s2), (%r1)
	mfdivs	(%r1),%f2
	mfdivd2	(%r1),%f2	; 02 A6 12 00 00 51 = SPOPRD 0x000012a6 (DIV md,%f2,%d2), (%r1)
	mfdivd	(%r1),%f2
	mfdivx2	(%r1),%f2	; 06 2A 13 00 00 51 = SPOPRT 0x0000132a (DIV mt,%f2,%x2), (%r1)
	mfdivx	(%r1),%f2
	mfdivs2	%f1,(%r2)	; 23 CC 10 00 00 52 52 = SPOPS2 0x000010cc (DIV %f1,ms,ms), (%r2), (%r2)
	mfdivs	%f1,(%r2)
	mfdivd2	%f1,(%r2)	; 03 DD 10 00 00 52 52 = SPOPD2 0x000010dd (DIV %f1,md,md), (%r2), (%r2)
	mfdivd	%f1,(%r2)
	mfdivx2	%f1,(%r2)	; 07 EE 10 00 00 52 52 = SPOPT2 0x000010ee (DIV %f1,mt,mt), (%r2), (%r2)
	mfdivx	%f1,(%r2)
	mfdivs2	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 23 3C 12 00 00 51 52 = SPOPS2 0x0000123c (DIV ms,%f3,ms), (%r1), (%r2)
	mfdivs	(%r1),(%r2)
	mfdivd2	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2); 03 BD 12 00 00 51 52 = SPOPD2 0x000012bd (DIV md,%f3,md), (%r1), (%r2)
	mfdivd	(%r1),(%r2)
	mfdivx2	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2); 07 3E 13 00 00 51 52 = SPOPT2 0x0000133e (DIV mt,%f3,mt), (%r1), (%r2)
	mfdivx	(%r1),(%r2)

	mfdivs2	.+0x100,.+0x100	; 22 73 1E 00 00 AF 00 01 = SPOPRS 0x00001e73 (MOVE ms,,%s3), 0x100(%pc); 23 3C 12 00 00 AF F8 00 AF F8 00 = SPOPS2 0x0000123c (DIV ms,%f3,ms), 0xf8(pc), 0xf8(pc)
	mfdivd2	.+0x100,.+0x100	; 02 F7 1E 00 00 AF 00 01 = SPOPRD 0x00001ef7 (MOVE md,,%d3), 0x100(%pc); 03 BD 12 00 00 AF F8 00 AF F8 00 = SPOPD2 0x000012bd (DIV md,%f3,md), 0xf8(pc), 0xf8(pc)
	mfdivx2	.+0x100,.+0x100	; 06 7B 1F 00 00 AF 00 01 = SPOPRT 0x00001f7b (MOVE mt,,%x3), 0x100(%pc); 07 3E 13 00 00 AF F8 00 AF F8 00 = SPOPT2 0x0000133e (DIV mt,%f3,mt), 0xf8(pc), 0xf8(pc)

	mfdivs3	%f1,%f2,%f5	; 32 A1 10 04 00 = SPOP 0x000410a1 (DIV %f1,%f2,%s5)
	mfdivs	%f1,%f2,%f5
	mfdivd3	%f1,%f2,%f5	; 32 A5 10 04 00 = SPOP 0x000410a5 (DIV %f1,%f2,%d5)
	mfdivd	%f1,%f2,%f5
	mfdivx3	%f1,%f2,%f5	; 32 A9 10 04 00 = SPOP 0x000410a9 (DIV %f1,%f2,%x5)
	mfdivx	%f1,%f2,%f5
	mfdivs3	(%r1),%f2,%f5	; 22 21 12 04 00 51 = SPOPRS 0x00041221 (DIV ms,%f2,%s5), (%r1)
	mfdivs	(%r1),%f2,%f5
	mfdivd3	(%r1),%f2,%f5	; 02 A5 12 04 00 51 = SPOPRD 0x000412a5 (DIV md,%f2,%d5), (%r1)
	mfdivd	(%r1),%f2,%f5
	mfdivx3	(%r1),%f2,%f5	; 06 29 13 04 00 51 = SPOPRT 0x00041329 (DIV mt,%f2,%x5), (%r1)
	mfdivx	(%r1),%f2,%f5
	mfdivs3	%f1,(%r2),%f5	; 22 C1 10 04 00 52 = SPOPRS 0x000410c1 (DIV %f1,ms,%s5), (%r2)
	mfdivs	%f1,(%r2),%f5
	mfdivd3	%f1,(%r2),%f5	; 02 D5 12 04 00 52 = SPOPRD 0x000412d5 (DIV %f1,md,%d5), (%r2)
	mfdivd	%f1,(%r2),%f5
	mfdivx3	%f1,(%r2),%f5	; 06 E9 13 04 00 52 = SPOPRT 0x000413e9 (DIV %f1,mt,%x5), (%r2)
	mfdivx	%f1,(%r2),%f5
	mfdivs3	(%r1),(%r2),%f5	; 22 71 1E 04 00 52 = SPOPRS 0x00041e71 (MOVE ms,,%s5) (%r2) ; 22 11 12 0C 00 51 = SPOPRS 0x000c1211 (DIV ms,%f5,%s5), (%r1)
	mfdivs	(%r1),(%r2),%f5
	mfdivd3	(%r1),(%r2),%f5	; 02 F5 1E 04 00 52 = SPOPRD 0x00041ef5 (MOVE md,,%d5) (%r2) ; 02 95 12 0C 00 51 = SPOPRD 0x000c1295 (DIV md,%f5,%d5), (%r1)
	mfdivd	(%r1),(%r2),%f5
	mfdivx3	(%r1),(%r2),%f5	; 06 79 1F 04 00 52 = SPOPRT 0x00041f79 (MOVE mt,,%x5) (%r2) ; 06 19 13 0C 00 51 = SPOPRT 0x000c1319 (DIV mt,%f5,%x5), (%r1)
	mfdivx	(%r1),(%r2),%f5
	mfdivs3	%f1,%f2,(%r5)	; 33 AC 10 00 00 55 = SPOPWS 0x000010ac (DIV %f1,%f2,ms), (%r5)
	mfdivs	%f1,%f2,(%r5)
	mfdivd3	%f1,%f2,(%r5)	; 13 AD 10 00 00 55 = SPOPWD 0x000010ad (DIV %f1,%f2,md), (%r5)
	mfdivd	%f1,%f2,(%r5)
	mfdivx3	%f1,%f2,(%r5)	; 17 AE 10 00 00 55 = SPOPWT 0x000010ae (DIV %f1,%f2,mt), (%r5)
	mfdivx	%f1,%f2,(%r5)
	mfdivs3	(%r1),%f2,(%r5)	; 23 2C 12 00 00 51 55 = SPOPS2 0x0000122c (DIV ms,%f2,ms), (%r1), (%r5)
	mfdivs	(%r1),%f2,(%r5)
	mfdivd3	(%r1),%f2,(%r5)	; 03 AD 12 00 00 51 55 = SPOPD2 0x000012ad (DIV md,%f2,md), (%r1), (%r5)
	mfdivd	(%r1),%f2,(%r5)
	mfdivx3	(%r1),%f2,(%r5)	; 07 2E 13 00 00 51 55 = SPOPT2 0x0000132e (DIV mt,%f2,mt), (%r1), (%r5)
	mfdivx	(%r1),%f2,(%r5)
	mfdivs3	%f1,(%r2),(%r5)	; 23 CC 10 00 00 52 55 = SPOPS2 0x000010cc (DIV %f2,ms,ms), (%r2), (%r5)
	mfdivs	%f1,(%r2),(%r5)
	mfdivd3	%f1,(%r2),(%r5)	; 03 DD 10 00 00 52 55 = SPOPD2 0x000010dd (DIV %f2,md,md), (%r2), (%r5)
	mfdivd	%f1,(%r2),(%r5)
	mfdivx3	%f1,(%r2),(%r5)	; 07 EE 10 00 00 52 55 = SPOPT2 0x000010ee (DIV %f2,mt,mt), (%r2), (%r5)
	mfdivx	%f1,(%r2),(%r5)
	mfdivs3	(%r1),(%r2),(%r5)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2) ; 23 3C 12 00 00 51 55 = SPOPS2 0x0000123c (DIV ms,%f3,ms), (%r1), (%r5)
	mfdivs	(%r1),(%r2),(%r5)
	mfdivd3	(%r1),(%r2),(%r5)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2) ; 03 BD 12 00 00 51 55 = SPOPD2 0x000012bd (DIV ms,%f3,ms), (%r1), (%r5)
	mfdivd	(%r1),(%r2),(%r5)
	mfdivx3	(%r1),(%r2),(%r5)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2) ; 07 3E 13 00 00 51 55 = SPOPT2 0x0000133e (DIV ms,%f3,ms), (%r1), (%r5)
	mfdivx	(%r1),(%r2),(%r5)

	mfmuls2	%f1,%f2		; 32 A2 18 00 00 = SPOP 0x000018a2 (MUL %f1,%f1,%s2)
	mfmuls	%f1,%f2
	mfmuld2	%f1,%f2		; 32 A6 18 00 00 = SPOP 0x000018a6 (MUL %f1,%f1,%d2)
	mfmuld	%f1,%f2
	mfmulx2	%f1,%f2		; 32 AA 18 00 00 = SPOP 0x000018aa (MUL %f1,%f1,%x2)
	mfmulx	%f1,%f2
	mfmuls2	(%r1),%f2	; 22 22 1A 00 00 51 = SPOPRS 0x00001a22 (MUL ms,%f2,%s2), (%r1)
	mfmuls	(%r1),%f2
	mfmuld2	(%r1),%f2	; 02 A6 1A 00 00 51 = SPOPRD 0x00001aa6 (MUL md,%f2,%d2), (%r1)
	mfmuld	(%r1),%f2
	mfmulx2	(%r1),%f2	; 06 2A 1B 00 00 51 = SPOPRT 0x00001b2a (MUL mt,%f2,%x2), (%r1)
	mfmulx	(%r1),%f2
	mfmuls2	%f1,(%r2)	; 23 CC 18 00 00 52 52 = SPOPS2 0x000018cc (MUL %f1,ms,ms), (%r2), (%r2)
	mfmuls	%f1,(%r2)
	mfmuld2	%f1,(%r2)	; 03 DD 18 00 00 52 52 = SPOPD2 0x000018dd (MUL %f1,md,md), (%r2), (%r2)
	mfmuld	%f1,(%r2)
	mfmulx2	%f1,(%r2)	; 07 EE 18 00 00 52 52 = SPOPT2 0x000018ee (MUL %f1,mt,mt), (%r2), (%r2)
	mfmulx	%f1,(%r2)
	mfmuls2	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 23 3C 1A 00 00 51 52 = SPOPS2 0x00001a3c (MUL ms,%f3,ms), (%r1), (%r2)
	mfmuls	(%r1),(%r2)
	mfmuld2	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2); 03 BD 1A 00 00 51 52 = SPOPD2 0x00001abd (MUL md,%f3,md), (%r1), (%r2)
	mfmuld	(%r1),(%r2)
	mfmulx2	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2); 07 3E 1B 00 00 51 52 = SPOPT2 0x00001b3e (MUL mt,%f3,mt), (%r1), (%r2)
	mfmulx	(%r1),(%r2)

	mfmuls2	.+0x100,.+0x100	; 22 73 1E 00 00 AF 00 01 = SPOPRS 0x00001e73 (MOVE ms,,%s3), 0x100(%pc); 23 3C 1A 00 00 AF F8 00 AF F8 00 = SPOPS2 0x00001a3c (MUL ms,%f3,ms), 0xf8(pc), 0xf8(pc)
	mfmuld2	.+0x100,.+0x100	; 02 F7 1E 00 00 AF 00 01 = SPOPRD 0x00001ef7 (MOVE md,,%d3), 0x100(%pc); 03 BD 1A 00 00 AF F8 00 AF F8 00 = SPOPD2 0x00001abd (MUL md,%f3,md), 0xf8(pc), 0xf8(pc)
	mfmulx2	.+0x100,.+0x100	; 06 7B 1F 00 00 AF 00 01 = SPOPRT 0x00001f7b (MOVE mt,,%x3), 0x100(%pc); 07 3E 1B 00 00 AF F8 00 AF F8 00 = SPOPT2 0x00001b3e (MUL mt,%f3,mt), 0xf8(pc), 0xf8(pc)

	mfmuls3	%f1,%f2,%f5	; 32 A1 18 04 00 = SPOP 0x000418a1 (MUL %f1,%f2,%s5)
	mfmuls	%f1,%f2,%f5
	mfmuld3	%f1,%f2,%f5	; 32 A5 18 04 00 = SPOP 0x000418a5 (MUL %f1,%f2,%d5)
	mfmuld	%f1,%f2,%f5
	mfmulx3	%f1,%f2,%f5	; 32 A9 18 04 00 = SPOP 0x000418a9 (MUL %f1,%f2,%x5)
	mfmulx	%f1,%f2,%f5
	mfmuls3	(%r1),%f2,%f5	; 22 21 1A 04 00 51 = SPOPRS 0x00041a21 (MUL ms,%f2,%s5), (%r1)
	mfmuls	(%r1),%f2,%f5
	mfmuld3	(%r1),%f2,%f5	; 02 A5 1A 04 00 51 = SPOPRD 0x00041aa5 (MUL md,%f2,%d5), (%r1)
	mfmuld	(%r1),%f2,%f5
	mfmulx3	(%r1),%f2,%f5	; 06 29 1B 04 00 51 = SPOPRT 0x00041b29 (MUL mt,%f2,%x5), (%r1)
	mfmulx	(%r1),%f2,%f5
	mfmuls3	%f1,(%r2),%f5	; 22 C1 18 04 00 52 = SPOPRS 0x000418c1 (MUL %f1,ms,%s5), (%r2)
	mfmuls	%f1,(%r2),%f5
	mfmuld3	%f1,(%r2),%f5	; 02 D5 1A 04 00 52 = SPOPRD 0x00041ad5 (MUL %f1,md,%d5), (%r2)
	mfmuld	%f1,(%r2),%f5
	mfmulx3	%f1,(%r2),%f5	; 06 E9 1B 04 00 52 = SPOPRT 0x00041be9 (MUL %f1,mt,%x5), (%r2)
	mfmulx	%f1,(%r2),%f5
	mfmuls3	(%r1),(%r2),%f5	; 22 71 1E 04 00 52 = SPOPRS 0x00041e71 (MOVE ms,,%s5) (%r2) ; 22 11 1A 0C 00 51 = SPOPRS 0x000c1a11 (MUL ms,%f5,%s5), (%r1)
	mfmuls	(%r1),(%r2),%f5
	mfmuld3	(%r1),(%r2),%f5	; 02 F5 1E 04 00 52 = SPOPRD 0x00041ef5 (MOVE md,,%d5) (%r2) ; 02 95 1A 0C 00 51 = SPOPRD 0x000c1a95 (MUL md,%f5,%d5), (%r1)
	mfmuld	(%r1),(%r2),%f5
	mfmulx3	(%r1),(%r2),%f5	; 06 79 1F 04 00 52 = SPOPRT 0x00041f79 (MOVE mt,,%x5) (%r2) ; 06 19 1B 0C 00 51 = SPOPRT 0x000c1b19 (MUL mt,%f5,%x5), (%r1)
	mfmulx	(%r1),(%r2),%f5
	mfmuls3	%f1,%f2,(%r5)	; 33 AC 18 00 00 55 = SPOPWS 0x000018ac (MUL %f1,%f2,ms), (%r5)
	mfmuls	%f1,%f2,(%r5)
	mfmuld3	%f1,%f2,(%r5)	; 13 AD 18 00 00 55 = SPOPWD 0x000018ad (MUL %f1,%f2,md), (%r5)
	mfmuld	%f1,%f2,(%r5)
	mfmulx3	%f1,%f2,(%r5)	; 17 AE 18 00 00 55 = SPOPWT 0x000018ae (MUL %f1,%f2,mt), (%r5)
	mfmulx	%f1,%f2,(%r5)
	mfmuls3	(%r1),%f2,(%r5)	; 23 2C 1A 00 00 51 55 = SPOPS2 0x00001a2c (MUL ms,%f2,ms), (%r1), (%r5)
	mfmuls	(%r1),%f2,(%r5)
	mfmuld3	(%r1),%f2,(%r5)	; 03 AD 1A 00 00 51 55 = SPOPD2 0x00001aad (MUL md,%f2,md), (%r1), (%r5)
	mfmuld	(%r1),%f2,(%r5)
	mfmulx3	(%r1),%f2,(%r5)	; 07 2E 1B 00 00 51 55 = SPOPT2 0x00001b2e (MUL mt,%f2,mt), (%r1), (%r5)
	mfmulx	(%r1),%f2,(%r5)
	mfmuls3	%f1,(%r2),(%r5)	; 23 CC 18 00 00 52 55 = SPOPS2 0x000018cc (MUL %f2,ms,ms), (%r2), (%r5)
	mfmuls	%f1,(%r2),(%r5)
	mfmuld3	%f1,(%r2),(%r5)	; 03 DD 18 00 00 52 55 = SPOPD2 0x000018dd (MUL %f2,md,md), (%r2), (%r5)
	mfmuld	%f1,(%r2),(%r5)
	mfmulx3	%f1,(%r2),(%r5)	; 07 EE 18 00 00 52 55 = SPOPT2 0x000018ee (MUL %f2,mt,mt), (%r2), (%r5)
	mfmulx	%f1,(%r2),(%r5)
	mfmuls3	(%r1),(%r2),(%r5)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2) ; 23 3C 1A 00 00 51 55 = SPOPS2 0x00001a3c (MUL ms,%f3,ms), (%r1), (%r5)
	mfmuls	(%r1),(%r2),(%r5)
	mfmuld3	(%r1),(%r2),(%r5)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2) ; 03 BD 1A 00 00 51 55 = SPOPD2 0x00001abd (MUL ms,%f3,ms), (%r1), (%r5)
	mfmuld	(%r1),(%r2),(%r5)
	mfmulx3	(%r1),(%r2),(%r5)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2) ; 07 3E 1B 00 00 51 55 = SPOPT2 0x00001b3e (MUL ms,%f3,ms), (%r1), (%r5)
	mfmulx	(%r1),(%r2),(%r5)

	mfnegs1	%f1		; 32 F1 5C 00 00 = SPOP 0x00005cf1 (NEG %f1,,%s1)
	mfnegs	%f1
	mfnegd1	%f1		; 32 F5 5C 00 00 = SPOP 0x00005cf5 (NEG %f1,,%d1)
	mfnegd	%f1
	mfnegx1	%f1		; 32 F9 5C 00 00 = SPOP 0x00005cf9 (NEG %f1,,%x1)
	mfnegx	%f1
	mfnegs1	(%r4)		; 23 7C 5E 00 00 54 54 = SPOPS2 0x00005e7c (NEG ms,,ms), (%r4), (%r4)
	mfnegs	(%r4)
	mfnegd1	(%r4)		; 03 FD 5E 00 00 54 54 = SPOPD2 0x00005efd (NEG md,,md), (%r4), (%r4)
	mfnegd	(%r4)
	mfnegx1	(%r4)		; 07 7E 5F 00 00 54 54 = SPOPT2 0x00005f7e (NEG mt,,mt), (%r4), (%r4)
	mfnegx	(%r4)

	mfnegs2	%f1,%f2		; 32 F2 5C 00 00 = SPOP 0x00005cf2 (NEG %f1,,%s2)
	mfnegs	%f1,%f2
	mfnegd2	%f1,%f2		; 32 F6 5C 00 00 = SPOP 0x00005cf6 (NEG %f1,,%d2)
	mfnegd	%f1,%f2
	mfnegx2	%f1,%f2		; 32 FA 5C 00 00 = SPOP 0x00005cfa (NEG %f1,,%x2)
	mfnegx	%f1,%f2
	mfnegs2	(%r4),%f2	; 22 72 5E 00 00 54 = SPOPRS 0x00005e72 (NEG ms,,%s2), (%r4)
	mfnegs	(%r4),%f2
	mfnegd2	(%r4),%f2	; 02 F6 5E 00 00 54 = SPOPRD 0x00005ef6 (NEG md,,%d2), (%r4)
	mfnegd	(%r4),%f2
	mfnegx2	(%r4),%f2	; 06 7A 5F 00 00 54 = SPOPRT 0x00005f7a (NEG mt,,%x2), (%r4)
	mfnegx	(%r4),%f2
	mfnegs2	%f1,(%r4)	; 33 FC 5C 00 00 54 = SPOPWS 0x00005cfc (NEG %f1,,ms), (%r4)
	mfnegs	%f1,(%r4)
	mfnegd2	%f1,(%r4)	; 13 FD 5C 00 00 54 = SPOPWD 0x00005cfd (NEG %f1,,md), (%r4)
	mfnegd	%f1,(%r4)
	mfnegx2	%f1,(%r4)	; 17 FE 5C 00 00 54 = SPOPWT 0x00005cfe (NEG %f1,,mt), (%r4)
	mfnegx	%f1,(%r4)
	mfnegs2	(%r3),(%r4)	; 23 7C 5E 00 00 53 54 = SPOPS2 0x00005e7c (NEG ms,,ms), (%r3), (%r4)
	mfnegs	(%r3),(%r4)
	mfnegd2	(%r3),(%r4)	; 03 FD 5E 00 00 53 54 = SPOPD2 0x00005efd (NEG ms,,md), (%r3), (%r4)
	mfnegd	(%r3),(%r4)
	mfnegx2	(%r3),(%r4)	; 07 7E 5F 00 00 53 54 = SPOPT2 0x00005f7e (NEG ms,,mt), (%r3), (%r4)
	mfnegx	(%r3),(%r4)

	mfrems2	%f1,%f2		; 32 A2 14 00 00 = SPOP 0x000014a2 (REM %f1,%f1,%s2)
	mfrems	%f1,%f2
	mfremd2	%f1,%f2		; 32 A6 14 00 00 = SPOP 0x000014a6 (REM %f1,%f1,%d2)
	mfremd	%f1,%f2
	mfremx2	%f1,%f2		; 32 AA 14 00 00 = SPOP 0x000014aa (REM %f1,%f1,%x2)
	mfremx	%f1,%f2
	mfrems2	(%r1),%f2	; 22 22 16 00 00 51 = SPOPRS 0x00001622 (REM ms,%f2,%s2), (%r1)
	mfrems	(%r1),%f2
	mfremd2	(%r1),%f2	; 02 A6 16 00 00 51 = SPOPRD 0x000016a6 (REM md,%f2,%d2), (%r1)
	mfremd	(%r1),%f2
	mfremx2	(%r1),%f2	; 06 2A 17 00 00 51 = SPOPRT 0x0000172a (REM mt,%f2,%x2), (%r1)
	mfremx	(%r1),%f2
	mfrems2	%f1,(%r2)	; 23 CC 14 00 00 52 52 = SPOPS2 0x000014cc (REM %f1,ms,ms), (%r2), (%r2)
	mfrems	%f1,(%r2)
	mfremd2	%f1,(%r2)	; 03 DD 14 00 00 52 52 = SPOPD2 0x000014dd (REM %f1,md,md), (%r2), (%r2)
	mfremd	%f1,(%r2)
	mfremx2	%f1,(%r2)	; 07 EE 14 00 00 52 52 = SPOPT2 0x000014ee (REM %f1,mt,mt), (%r2), (%r2)
	mfremx	%f1,(%r2)
	mfrems2	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 23 3C 16 00 00 51 52 = SPOPS2 0x0000163c (REM ms,%f3,ms), (%r1), (%r2)
	mfrems	(%r1),(%r2)
	mfremd2	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2); 03 BD 16 00 00 51 52 = SPOPD2 0x000016bd (REM md,%f3,md), (%r1), (%r2)
	mfremd	(%r1),(%r2)
	mfremx2	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2); 07 3E 17 00 00 51 52 = SPOPT2 0x0000173e (REM mt,%f3,mt), (%r1), (%r2)
	mfremx	(%r1),(%r2)

	mfrems2	.+0x100,.+0x100	; 22 73 1E 00 00 AF 00 01 = SPOPRS 0x00001e73 (MOVE ms,,%s3), 0x100(%pc); 23 3C 16 00 00 AF F8 00 AF F8 00 = SPOPS2 0x0000163c (REM ms,%f3,ms), 0xf8(pc), 0xf8(pc)
	mfremd2	.+0x100,.+0x100	; 02 F7 1E 00 00 AF 00 01 = SPOPRD 0x00001ef7 (MOVE md,,%d3), 0x100(%pc); 03 BD 16 00 00 AF F8 00 AF F8 00 = SPOPD2 0x000016bd (REM md,%f3,md), 0xf8(pc), 0xf8(pc)
	mfremx2	.+0x100,.+0x100	; 06 7B 1F 00 00 AF 00 01 = SPOPRT 0x00001f7b (MOVE mt,,%x3), 0x100(%pc); 07 3E 17 00 00 AF F8 00 AF F8 00 = SPOPT2 0x0000173e (REM mt,%f3,mt), 0xf8(pc), 0xf8(pc)

	mfrems3	%f1,%f2,%f5	; 32 A1 14 04 00 = SPOP 0x000414a1 (REM %f1,%f2,%s5)
	mfrems	%f1,%f2,%f5
	mfremd3	%f1,%f2,%f5	; 32 A5 14 04 00 = SPOP 0x000414a5 (REM %f1,%f2,%d5)
	mfremd	%f1,%f2,%f5
	mfremx3	%f1,%f2,%f5	; 32 A9 14 04 00 = SPOP 0x000414a9 (REM %f1,%f2,%x5)
	mfremx	%f1,%f2,%f5
	mfrems3	(%r1),%f2,%f5	; 22 21 16 04 00 51 = SPOPRS 0x00041621 (REM ms,%f2,%s5), (%r1)
	mfrems	(%r1),%f2,%f5
	mfremd3	(%r1),%f2,%f5	; 02 A5 16 04 00 51 = SPOPRD 0x000416a5 (REM md,%f2,%d5), (%r1)
	mfremd	(%r1),%f2,%f5
	mfremx3	(%r1),%f2,%f5	; 06 29 17 04 00 51 = SPOPRT 0x00041729 (REM mt,%f2,%x5), (%r1)
	mfremx	(%r1),%f2,%f5
	mfrems3	%f1,(%r2),%f5	; 22 C1 14 04 00 52 = SPOPRS 0x000414c1 (REM %f1,ms,%s5), (%r2)
	mfrems	%f1,(%r2),%f5
	mfremd3	%f1,(%r2),%f5	; 02 D5 16 04 00 52 = SPOPRD 0x000416d5 (REM %f1,md,%d5), (%r2)
	mfremd	%f1,(%r2),%f5
	mfremx3	%f1,(%r2),%f5	; 06 E9 17 04 00 52 = SPOPRT 0x000417e9 (REM %f1,mt,%x5), (%r2)
	mfremx	%f1,(%r2),%f5
	mfrems3	(%r1),(%r2),%f5	; 22 71 1E 04 00 52 = SPOPRS 0x00041e71 (MOVE ms,,%s5) (%r2) ; 22 11 16 0C 00 51 = SPOPRS 0x000c1611 (REM ms,%f5,%s5), (%r1)
	mfrems	(%r1),(%r2),%f5
	mfremd3	(%r1),(%r2),%f5	; 02 F5 1E 04 00 52 = SPOPRD 0x00041ef5 (MOVE md,,%d5) (%r2) ; 02 95 16 0C 00 51 = SPOPRD 0x000c1695 (REM md,%f5,%d5), (%r1)
	mfremd	(%r1),(%r2),%f5
	mfremx3	(%r1),(%r2),%f5	; 06 79 1F 04 00 52 = SPOPRT 0x00041f79 (MOVE mt,,%x5) (%r2) ; 06 19 17 0C 00 51 = SPOPRT 0x000c1719 (REM mt,%f5,%x5), (%r1)
	mfremx	(%r1),(%r2),%f5
	mfrems3	%f1,%f2,(%r5)	; 33 AC 14 00 00 55 = SPOPWS 0x000014ac (REM %f1,%f2,ms), (%r5)
	mfrems	%f1,%f2,(%r5)
	mfremd3	%f1,%f2,(%r5)	; 13 AD 14 00 00 55 = SPOPWD 0x000014ad (REM %f1,%f2,md), (%r5)
	mfremd	%f1,%f2,(%r5)
	mfremx3	%f1,%f2,(%r5)	; 17 AE 14 00 00 55 = SPOPWT 0x000014ae (REM %f1,%f2,mt), (%r5)
	mfremx	%f1,%f2,(%r5)
	mfrems3	(%r1),%f2,(%r5)	; 23 2C 16 00 00 51 55 = SPOPS2 0x0000162c (REM ms,%f2,ms), (%r1), (%r5)
	mfrems	(%r1),%f2,(%r5)
	mfremd3	(%r1),%f2,(%r5)	; 03 AD 16 00 00 51 55 = SPOPD2 0x000016ad (REM md,%f2,md), (%r1), (%r5)
	mfremd	(%r1),%f2,(%r5)
	mfremx3	(%r1),%f2,(%r5)	; 07 2E 17 00 00 51 55 = SPOPT2 0x0000172e (REM mt,%f2,mt), (%r1), (%r5)
	mfremx	(%r1),%f2,(%r5)
	mfrems3	%f1,(%r2),(%r5)	; 23 CC 14 00 00 52 55 = SPOPS2 0x000014cc (REM %f2,ms,ms), (%r2), (%r5)
	mfrems	%f1,(%r2),(%r5)
	mfremd3	%f1,(%r2),(%r5)	; 03 DD 14 00 00 52 55 = SPOPD2 0x000014dd (REM %f2,md,md), (%r2), (%r5)
	mfremd	%f1,(%r2),(%r5)
	mfremx3	%f1,(%r2),(%r5)	; 07 EE 14 00 00 52 55 = SPOPT2 0x000014ee (REM %f2,mt,mt), (%r2), (%r5)
	mfremx	%f1,(%r2),(%r5)
	mfrems3	(%r1),(%r2),(%r5)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2) ; 23 3C 16 00 00 51 55 = SPOPS2 0x0000163c (REM ms,%f3,ms), (%r1), (%r5)
	mfrems	(%r1),(%r2),(%r5)
	mfremd3	(%r1),(%r2),(%r5)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2) ; 03 BD 16 00 00 51 55 = SPOPD2 0x000016bd (REM ms,%f3,ms), (%r1), (%r5)
	mfremd	(%r1),(%r2),(%r5)
	mfremx3	(%r1),(%r2),(%r5)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2) ; 07 3E 17 00 00 51 55 = SPOPT2 0x0000173e (REM ms,%f3,ms), (%r1), (%r5)
	mfremx	(%r1),(%r2),(%r5)

	mfrnds1	%f1		; 32 F1 38 00 00 = SPOP 0x000038f1 (RTOI %f1,,%s1)
	mfrnds	%f1
	mfrndd1	%f1		; 32 F5 38 00 00 = SPOP 0x000038f5 (RTOI %f1,,%d1)
	mfrndd	%f1
	mfrndx1	%f1		; 32 F9 38 00 00 = SPOP 0x000038f9 (RTOI %f1,,%x1)
	mfrndx	%f1
	mfrnds1	(%r4)		; 23 7C 3A 00 00 54 54 = SPOPS2 0x00003a7c (RTOI ms,,ms), (%r4), (%r4)
	mfrnds	(%r4)
	mfrndd1	(%r4)		; 03 FD 3A 00 00 54 54 = SPOPD2 0x00003afd (RTOI md,,md), (%r4), (%r4)
	mfrndd	(%r4)
	mfrndx1	(%r4)		; 07 7E 3B 00 00 54 54 = SPOPT2 0x00003b7e (RTOI mt,,mt), (%r4), (%r4)
	mfrndx	(%r4)

	mfrnds2	%f1,%f2		; 32 F2 38 00 00 = SPOP 0x000038f2 (RTOI %f1,,%s2)
	mfrnds	%f1,%f2
	mfrndd2	%f1,%f2		; 32 F6 38 00 00 = SPOP 0x000038f6 (RTOI %f1,,%d2)
	mfrndd	%f1,%f2
	mfrndx2	%f1,%f2		; 32 FA 38 00 00 = SPOP 0x000038fa (RTOI %f1,,%x2)
	mfrndx	%f1,%f2
	mfrnds2	(%r4),%f2	; 22 72 3A 00 00 54 = SPOPRS 0x00003a72 (RTOI ms,,%s2), (%r4)
	mfrnds	(%r4),%f2
	mfrndd2	(%r4),%f2	; 02 F6 3A 00 00 54 = SPOPRD 0x00003af6 (RTOI md,,%d2), (%r4)
	mfrndd	(%r4),%f2
	mfrndx2	(%r4),%f2	; 06 7A 3B 00 00 54 = SPOPRT 0x00003b7a (RTOI mt,,%x2), (%r4)
	mfrndx	(%r4),%f2
	mfrnds2	%f1,(%r4)	; 33 FC 38 00 00 54 = SPOPWS 0x000038fc (RTOI %f1,,ms), (%r4)
	mfrnds	%f1,(%r4)
	mfrndd2	%f1,(%r4)	; 13 FD 38 00 00 54 = SPOPWD 0x000038fd (RTOI %f1,,md), (%r4)
	mfrndd	%f1,(%r4)
	mfrndx2	%f1,(%r4)	; 17 FE 38 00 00 54 = SPOPWT 0x000038fe (RTOI %f1,,mt), (%r4)
	mfrndx	%f1,(%r4)
	mfrnds2	(%r3),(%r4)	; 23 7C 3A 00 00 53 54 = SPOPS2 0x00003a7c (RTOI ms,,ms), (%r3), (%r4)
	mfrnds	(%r3),(%r4)
	mfrndd2	(%r3),(%r4)	; 03 FD 3A 00 00 53 54 = SPOPD2 0x00003afd (RTOI ms,,md), (%r3), (%r4)
	mfrndd	(%r3),(%r4)
	mfrndx2	(%r3),(%r4)	; 07 7E 3B 00 00 53 54 = SPOPT2 0x00003b7e (RTOI ms,,mt), (%r3), (%r4)
	mfrndx	(%r3),(%r4)

	mfsins1	%f1		; 32 F1 70 00 00 = SPOP 0x000030f1 (SIN %f1,,%s1)
	mfsins	%f1
	mfsind1	%f1		; 32 F5 70 00 00 = SPOP 0x000030f5 (SIN %f1,,%d1)
	mfsind	%f1
	mfsinx1	%f1		; 32 F9 70 00 00 = SPOP 0x000030f9 (SIN %f1,,%x1)
	mfsinx	%f1
	mfsins1	(%r4)		; 23 7C 72 00 00 54 54 = SPOPS2 0x0000327c (SIN ms,,ms), (%r4), (%r4)
	mfsins	(%r4)
	mfsind1	(%r4)		; 03 FD 72 00 00 54 54 = SPOPD2 0x000032fd (SIN md,,md), (%r4), (%r4)
	mfsind	(%r4)
	mfsinx1	(%r4)		; 07 7E 73 00 00 54 54 = SPOPT2 0x0000337e (SIN mt,,mt), (%r4), (%r4)
	mfsinx	(%r4)

	mfsins2	%f1,%f2		; 32 F2 70 00 00 = SPOP 0x000070f2 (SIN %f1,,%s2)
	mfsins	%f1,%f2
	mfsind2	%f1,%f2		; 32 F6 70 00 00 = SPOP 0x000070f6 (SIN %f1,,%d2)
	mfsind	%f1,%f2
	mfsinx2	%f1,%f2		; 32 FA 70 00 00 = SPOP 0x000070fa (SIN %f1,,%x2)
	mfsinx	%f1,%f2
	mfsins2	(%r4),%f2	; 22 72 72 00 00 54 = SPOPRS 0x00007272 (SIN ms,,%s2), (%r4)
	mfsins	(%r4),%f2
	mfsind2	(%r4),%f2	; 02 F6 72 00 00 54 = SPOPRD 0x000072f6 (SIN md,,%d2), (%r4)
	mfsind	(%r4),%f2
	mfsinx2	(%r4),%f2	; 06 7A 73 00 00 54 = SPOPRT 0x0000737a (SIN mt,,%x2), (%r4)
	mfsinx	(%r4),%f2
	mfsins2	%f1,(%r4)	; 33 FC 70 00 00 54 = SPOPWS 0x000070fc (SIN %f1,,ms), (%r4)
	mfsins	%f1,(%r4)
	mfsind2	%f1,(%r4)	; 13 FD 70 00 00 54 = SPOPWD 0x000070fd (SIN %f1,,md), (%r4)
	mfsind	%f1,(%r4)
	mfsinx2	%f1,(%r4)	; 17 FE 70 00 00 54 = SPOPWT 0x000070fe (SIN %f1,,mt), (%r4)
	mfsinx	%f1,(%r4)
	mfsins2	(%r3),(%r4)	; 23 7C 72 00 00 53 54 = SPOPS2 0x0000727c (SIN ms,,ms), (%r3), (%r4)
	mfsins	(%r3),(%r4)
	mfsind2	(%r3),(%r4)	; 03 FD 72 00 00 53 54 = SPOPD2 0x000072fd (SIN ms,,md), (%r3), (%r4)
	mfsind	(%r3),(%r4)
	mfsinx2	(%r3),(%r4)	; 07 7E 73 00 00 53 54 = SPOPT2 0x0000737e (SIN ms,,mt), (%r3), (%r4)
	mfsinx	(%r3),(%r4)

	mfsqrs1	%f1		; 32 F1 34 00 00 = SPOP 0x000034f1 (SQRT %f1,,%s1)
	mfsqrs	%f1
	mfsqrd1	%f1		; 32 F5 34 00 00 = SPOP 0x000034f5 (SQRT %f1,,%d1)
	mfsqrd	%f1
	mfsqrx1	%f1		; 32 F9 34 00 00 = SPOP 0x000034f9 (SQRT %f1,,%x1)
	mfsqrx	%f1
	mfsqrs1	(%r4)		; 23 7C 36 00 00 54 54 = SPOPS2 0x0000367c (SQRT ms,,ms), (%r4), (%r4)
	mfsqrs	(%r4)
	mfsqrd1	(%r4)		; 03 FD 35 00 00 54 54 = SPOPD2 0x000036fd (SQRT md,,md), (%r4), (%r4)
	mfsqrd	(%r4)
	mfsqrx1	(%r4)		; 07 7E 37 00 00 54 54 = SPOPT2 0x0000377e (SQRT mt,,mt), (%r4), (%r4)
	mfsqrx	(%r4)

	mfsqrs2	%f1,%f2		; 32 F2 34 00 00 = SPOP 0x000034f2 (SQRT %f1,,%s2)
	mfsqrs	%f1,%f2
	mfsqrd2	%f1,%f2		; 32 F6 34 00 00 = SPOP 0x000034f6 (SQRT %f1,,%d2)
	mfsqrd	%f1,%f2
	mfsqrx2	%f1,%f2		; 32 FA 34 00 00 = SPOP 0x000034fa (SQRT %f1,,%x2)
	mfsqrx	%f1,%f2
	mfsqrs2	(%r4),%f2	; 22 72 36 00 00 54 = SPOPRS 0x00003672 (SQRT ms,,%s2), (%r4)
	mfsqrs	(%r4),%f2
	mfsqrd2	(%r4),%f2	; 02 F6 36 00 00 54 = SPOPRD 0x000036f6 (SQRT md,,%d2), (%r4)
	mfsqrd	(%r4),%f2
	mfsqrx2	(%r4),%f2	; 06 7A 37 00 00 54 = SPOPRT 0x0000377a (SQRT mt,,%x2), (%r4)
	mfsqrx	(%r4),%f2
	mfsqrs2	%f1,(%r4)	; 33 FC 34 00 00 54 = SPOPWS 0x000034fc (SQRT %f1,,ms), (%r4)
	mfsqrs	%f1,(%r4)
	mfsqrd2	%f1,(%r4)	; 13 FD 34 00 00 54 = SPOPWD 0x000034fd (SQRT %f1,,md), (%r4)
	mfsqrd	%f1,(%r4)
	mfsqrx2	%f1,(%r4)	; 17 FE 34 00 00 54 = SPOPWT 0x000034fe (SQRT %f1,,mt), (%r4)
	mfsqrx	%f1,(%r4)
	mfsqrs2	(%r3),(%r4)	; 23 7C 36 00 00 53 54 = SPOPS2 0x0000367c (SQRT ms,,ms), (%r3), (%r4)
	mfsqrs	(%r3),(%r4)
	mfsqrd2	(%r3),(%r4)	; 03 FD 36 00 00 53 54 = SPOPD2 0x000036fd (SQRT ms,,md), (%r3), (%r4)
	mfsqrd	(%r3),(%r4)
	mfsqrx2	(%r3),(%r4)	; 07 7E 37 00 00 53 54 = SPOPT2 0x0000377e (SQRT ms,,mt), (%r3), (%r4)
	mfsqrx	(%r3),(%r4)

	mfsubs2	%f1,%f2		; 32 A2 0C 00 00 = SPOP 0x00000ca2 (SUB %f1,%f1,%s2)
	mfsubs	%f1,%f2
	mfsubd2	%f1,%f2		; 32 A6 0C 00 00 = SPOP 0x00000ca6 (SUB %f1,%f1,%d2)
	mfsubd	%f1,%f2
	mfsubx2	%f1,%f2		; 32 AA 0C 00 00 = SPOP 0x00000caa (SUB %f1,%f1,%x2)
	mfsubx	%f1,%f2
	mfsubs2	(%r1),%f2	; 22 22 0E 00 00 51 = SPOPRS 0x00000e22 (SUB ms,%f2,%s2), (%r1)
	mfsubs	(%r1),%f2
	mfsubd2	(%r1),%f2	; 02 A6 0E 00 00 51 = SPOPRD 0x00000ea6 (SUB md,%f2,%d2), (%r1)
	mfsubd	(%r1),%f2
	mfsubx2	(%r1),%f2	; 06 2A 0F 00 00 51 = SPOPRT 0x00000f2a (SUB mt,%f2,%x2), (%r1)
	mfsubx	(%r1),%f2
	mfsubs2	%f1,(%r2)	; 23 CC 0C 00 00 52 52 = SPOPS2 0x00000ecc (SUB %f1,ms,ms), (%r2), (%r2)
	mfsubs	%f1,(%r2)
	mfsubd2	%f1,(%r2)	; 03 DD 0C 00 00 52 52 = SPOPD2 0x00000edd (SUB %f1,md,md), (%r2), (%r2)
	mfsubd	%f1,(%r2)
	mfsubx2	%f1,(%r2)	; 07 EE 0C 00 00 52 52 = SPOPT2 0x00000eee (SUB %f1,mt,mt), (%r2), (%r2)
	mfsubx	%f1,(%r2)
	mfsubs2	(%r1),(%r2)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2); 23 3C 0E 00 00 51 52 = SPOPS2 0x00000e3c (SUB ms,%f3,ms), (%r1), (%r2)
	mfsubs	(%r1),(%r2)
	mfsubd2	(%r1),(%r2)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2); 03 BD 0E 00 00 51 52 = SPOPD2 0x00000ebd (SUB md,%f3,md), (%r1), (%r2)
	mfsubd	(%r1),(%r2)
	mfsubx2	(%r1),(%r2)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2); 07 3E 0F 00 00 51 52 = SPOPT2 0x00000f3e (SUB mt,%f3,mt), (%r1), (%r2)
	mfsubx	(%r1),(%r2)

	mfsubs2	.+0x100,.+0x100	; 22 73 1E 00 00 AF 00 01 = SPOPRS 0x00001e73 (MOVE ms,,%s3), 0x100(%pc); 23 3C 0E 00 00 AF F8 00 AF F8 00 = SPOPS2 0x00000e3c (SUB ms,%f3,ms), 0xf8(pc), 0xf8(pc)
	mfsubd2	.+0x100,.+0x100	; 02 F7 1E 00 00 AF 00 01 = SPOPRD 0x00001ef7 (MOVE md,,%d3), 0x100(%pc); 03 BD 0E 00 00 AF F8 00 AF F8 00 = SPOPD2 0x00000ebd (SUB md,%f3,md), 0xf8(pc), 0xf8(pc)
	mfsubx2	.+0x100,.+0x100	; 06 7B 1F 00 00 AF 00 01 = SPOPRT 0x00001f7b (MOVE mt,,%x3), 0x100(%pc); 07 3E 0F 00 00 AF F8 00 AF F8 00 = SPOPT2 0x00000f3e (SUB mt,%f3,mt), 0xf8(pc), 0xf8(pc)

	mfsubs3	%f1,%f2,%f5	; 32 A1 0C 04 00 = SPOP 0x00040ca1 (SUB %f1,%f2,%s5)
	mfsubs	%f1,%f2,%f5
	mfsubd3	%f1,%f2,%f5	; 32 A5 0C 04 00 = SPOP 0x00040ca5 (SUB %f1,%f2,%d5)
	mfsubd	%f1,%f2,%f5
	mfsubx3	%f1,%f2,%f5	; 32 A9 0C 04 00 = SPOP 0x00040ca9 (SUB %f1,%f2,%x5)
	mfsubx	%f1,%f2,%f5
	mfsubs3	(%r1),%f2,%f5	; 22 21 0E 04 00 51 = SPOPRS 0x00040e21 (SUB ms,%f2,%s5), (%r1)
	mfsubs	(%r1),%f2,%f5
	mfsubd3	(%r1),%f2,%f5	; 02 A5 0E 04 00 51 = SPOPRD 0x00040ea5 (SUB md,%f2,%d5), (%r1)
	mfsubd	(%r1),%f2,%f5
	mfsubx3	(%r1),%f2,%f5	; 06 29 0F 04 00 51 = SPOPRT 0x00040f29 (SUB mt,%f2,%x5), (%r1)
	mfsubx	(%r1),%f2,%f5
	mfsubs3	%f1,(%r2),%f5	; 22 C1 0C 04 00 52 = SPOPRS 0x00040ec1 (SUB %f1,ms,%s5), (%r2)
	mfsubs	%f1,(%r2),%f5
	mfsubd3	%f1,(%r2),%f5	; 02 D5 0E 04 00 52 = SPOPRD 0x00040ed5 (SUB %f1,md,%d5), (%r2)
	mfsubd	%f1,(%r2),%f5
	mfsubx3	%f1,(%r2),%f5	; 06 E9 0F 04 00 52 = SPOPRT 0x00040fe9 (SUB %f1,mt,%x5), (%r2)
	mfsubx	%f1,(%r2),%f5
	mfsubs3	(%r1),(%r2),%f5	; 22 71 1E 04 00 52 = SPOPRS 0x00041e71 (MOVE ms,,%s5) (%r2) ; 22 11 0E 0C 00 51 = SPOPRS 0x000c0e11 (SUB ms,%f5,%s5), (%r1)
	mfsubs	(%r1),(%r2),%f5
	mfsubd3	(%r1),(%r2),%f5	; 02 F5 1E 04 00 52 = SPOPRD 0x00041ef5 (MOVE md,,%d5) (%r2) ; 02 95 0E 0C 00 51 = SPOPRD 0x000c0e95 (SUB md,%f5,%d5), (%r1)
	mfsubd	(%r1),(%r2),%f5
	mfsubx3	(%r1),(%r2),%f5	; 06 79 1F 04 00 52 = SPOPRT 0x00041f79 (MOVE mt,,%x5) (%r2) ; 06 19 0F 0C 00 51 = SPOPRT 0x000c0f19 (SUB mt,%f5,%x5), (%r1)
	mfsubx	(%r1),(%r2),%f5
	mfsubs3	%f1,%f2,(%r5)	; 33 AC 0C 00 00 55 = SPOPWS 0x00000cac (SUB %f1,%f2,ms), (%r5)
	mfsubs	%f1,%f2,(%r5)
	mfsubd3	%f1,%f2,(%r5)	; 13 AD 0C 00 00 55 = SPOPWD 0x00000cad (SUB %f1,%f2,md), (%r5)
	mfsubd	%f1,%f2,(%r5)
	mfsubx3	%f1,%f2,(%r5)	; 17 AE 0C 00 00 55 = SPOPWT 0x00000cae (SUB %f1,%f2,mt), (%r5)
	mfsubx	%f1,%f2,(%r5)
	mfsubs3	(%r1),%f2,(%r5)	; 23 2C 0E 00 00 51 55 = SPOPS2 0x00000e2c (SUB ms,%f2,ms), (%r1), (%r5)
	mfsubs	(%r1),%f2,(%r5)
	mfsubd3	(%r1),%f2,(%r5)	; 03 AD 0E 00 00 51 55 = SPOPD2 0x00000ead (SUB md,%f2,md), (%r1), (%r5)
	mfsubd	(%r1),%f2,(%r5)
	mfsubx3	(%r1),%f2,(%r5)	; 07 2E 0F 00 00 51 55 = SPOPT2 0x00000f2e (SUB mt,%f2,mt), (%r1), (%r5)
	mfsubx	(%r1),%f2,(%r5)
	mfsubs3	%f1,(%r2),(%r5)	; 23 CC 0C 00 00 52 55 = SPOPS2 0x00000ccc (SUB %f2,ms,ms), (%r2), (%r5)
	mfsubs	%f1,(%r2),(%r5)
	mfsubd3	%f1,(%r2),(%r5)	; 03 DD 0C 00 00 52 55 = SPOPD2 0x00000cdd (SUB %f2,md,md), (%r2), (%r5)
	mfsubd	%f1,(%r2),(%r5)
	mfsubx3	%f1,(%r2),(%r5)	; 07 EE 0C 00 00 52 55 = SPOPT2 0x00000cee (SUB %f2,mt,mt), (%r2), (%r5)
	mfsubx	%f1,(%r2),(%r5)
	mfsubs3	(%r1),(%r2),(%r5)	; 22 73 1E 00 00 52 = SPOPRS 0x00001e73 (MOVE ms,,%s3), (%r2) ; 23 3C 0E 00 00 51 55 = SPOPS2 0x00000e3c (SUB ms,%f3,ms), (%r1), (%r5)
	mfsubs	(%r1),(%r2),(%r5)
	mfsubd3	(%r1),(%r2),(%r5)	; 02 F7 1E 00 00 52 = SPOPRD 0x00001ef7 (MOVE md,,%d3), (%r2) ; 03 BD 0E 00 00 51 55 = SPOPD2 0x00000ebd (SUB ms,%f3,ms), (%r1), (%r5)
	mfsubd	(%r1),(%r2),(%r5)
	mfsubx3	(%r1),(%r2),(%r5)	; 06 7B 1F 00 00 52 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r2) ; 07 3E 0F 00 00 51 55 = SPOPT2 0x00000f3e (SUB ms,%f3,ms), (%r1), (%r5)
	mfsubx	(%r1),(%r2),(%r5)

	expect	1350
	mmov10d	%f1,%f2
	endexpect
	mmov10d	(%r1),%f2		; 06 76 47 00 00 51 = SPOPRT 0x00004776 (DTOF mt,,%d2), (%r1)
	expect	1350
	mmov10d	%f1,(%r2)
	endexpect
	mmov10d	(%r1),(%r2)		; 06 7B 47 00 00 51 = SPOPRT 0x0000477b (DTOF mt,,%x3), (%r1) ; 13 FD 1D 00 00 52 = SPOPWD 0x00001dfd (MOVE %f3,,md), (%r2)

	expect	1350
	mmov10s	%f1,%f2
	endexpect
	mmov10s	(%r1),%f2		; 06 72 47 00 00 51 = SPOPRT 0x00004772 (DTOF mt,,%s2), (%r1)
	expect	1350
	mmov10s	%f1,(%r2)
	endexpect
	mmov10s	(%r1),(%r2)		; 06 7B 47 00 00 51 = SPOPRT 0x0000477b (DTOF mt,,%x3), (%r1) ; 33 FC 1D 00 00 52 = SPOPWS 0x00001dfc (MOVE %f3,,ms), (%r2)

	expect	1350
	mmov10x	%f1,%f2
	endexpect
	mmov10x	(%r1),%f2		; 06 7a 47 00 00 51 = SPOPRT 0x0000477a (DTOF mt,,%x2), (%r1)
	expect	1350
	mmov10x	%f1,(%r2)
	endexpect
	mmov10x	(%r1),(%r2)		; 07 7E 47 00 00 51 52 = SPOPT2 0x0000477e (DTOF mt,,mt), (%r1), (%r2)

	expect	1350
	mmovd10	%f1,%f2
	endexpect
	expect	1350
	mmovd10	(%r1),%f2
	endexpect
	mmovd10	%f1,(%r2)		; 17 FE 48 00 00 52 = SPOPWT 0x000048fe (FTOD %f1,,mt), (%r2)
	mmovd10	(%r1),(%r2)		; 02 FB 1E 00 00 51 = SPOPRD 0x00001efb (MOVE md,,%f3), (%r1); 17 FE 49 00 00 52 = SPOPWT 0x000049fe (FTOD %f3,,mt), (%r1)

	mmovdd	%f1,%f2			; 32 F6 1C 00 00 = SPOP 0x00001cf6 (MOVE %f1,,%d2)
	mmovdd	(%r1),%f2		; 02 F6 1E 00 00 51 = SPOPRD 0x00001ef6 (MOVE md,,%d2), (%r1)
	mmovdd	%f1,(%r2)		; 13 FD 1C 00 00 52 = SPOPWD 0x00001cfd (MOVE %f1,,md), (%r2)
	mmovdd	(%r1),(%r2)		; 03 FD 1E 00 00 51 52 = SPOPD2 0x00001efd (MOVE md,,md), (%r1), (%r2)

	mmovds	%f1,%f2			; 32 F2 1C 00 00 = SPOP 0x00001cf2 (MOVE %f1,,%s2)
	mmovds	(%r1),%f2		; 02 F2 1E 00 00 51 = SPOPRD 0x00001ef2 (MOVE md,,%s2), (%r1)
	mmovds	%f1,(%r2)		; 33 FC 1C 00 00 52 = SPOPWS 0x00001cfc (MOVE %f1,,ms), (%r2)
	mmovds	(%r1),(%r2)		; 02 FB 1E 00 00 51 = SPOPRD 0x00001efb (MOVE md,,%x3), (%r1); 33 FC 1D 00 00 52 = SPOPWS 0x00001dfc (MOVE %f3,,ms), (%r2)

	expect	1350
	mmovdw	%f1,%f2
	endexpect
	expect	1350
	mmovdw	(%r1),%f2
	endexpect
	mmovdw	%f1,(%r2)		; 33 FC 3C 00 00 52 = SPOPWS 0x000048fe (FTOI %f1,,ms), (%r2)
	mmovdw	(%r1),(%r2)		; 02 FB 1E 00 00 51 = SPOPRD 0x00001efb (MOVE md,,%f3), (%r1); 33 FC 3D 00 00 52 = SPOPWS 0x00003dfc (FTOI %f3,,ms), (%r2)

	mmovdx	%f1,%f2			; 32 FA 1C 00 00 = SPOP 0x00001cfa (MOVE %f1,,%x2)
	mmovdx	(%r1),%f2		; 02 FA 1E 00 00 51 = SPOPRD 0x00001efa (MOVE md,,%x2), (%r1)
	mmovdx	%f1,(%r2)		; 17 FE 1C 00 00 52 = SPOPWT 0x00001cfe (MOVE %f1,,mt), (%r2)
	mmovdx	(%r1),(%r2)		; 02 FB 1E 00 00 51 = SPOPRD 0x00001efb (MOVE md,,%x3), (%r1); 17 FE 1D 00 00 52 = SPOPWT 0x00001dfe (%f3,,mt), (%r2)

	expect	1350
	mmovfa	%f1
	endexpect
	mmovfa	(%r1)			; 33 FC 23 00 00 51 = SPOPWS 0x000023fc (RDASR ,,ms), (%r1)

	expect	1350
	mmovfd	%f1
	endexpect
	mmovfd	(%r1)			; 33 FC 67 00 00 51 = SPOPWS 0x000067fc (SDT(?) ,,ms), (%r1)

	expect	1350
	mmovs10	%f1,%f2
	endexpect
	expect	1350
	mmovs10	(%r1),%f2
	endexpect
	mmovs10	%f1,(%r2)		; 17 FE 48 00 00 52 = SPOPWT 0x000048fe (FTOD %f1,,mt), (%r2)
	mmovs10 (%r1),(%r2)		; 22 7B 1E 00 00 51 = SPOPRS 0x00001e7b (MOVE ms,,%f3), (%r1); 17 FE 49 00 00 52 = SPOPWT 0x000049fe (FTOD %f3,,mt), (%r1)

	mmovsd	%f1,%f2			; 32 F6 1C 00 00 = SPOP 0x00001cf6 (MOVE %f1,,%d2)
	mmovsd	(%r1),%f2		; 22 76 1E 00 00 51 = SPOPRS 0x00001e76 (MOVE ms,,%d2), (%r1)
	mmovsd	%f1,(%r2)		; 13 FD 1C 00 00 52 = SPOPWD 0x00001cfd (%f1,,md), (%r2)
	mmovsd	(%r1),(%r2)		; 22 7B 1E 00 00 51 = SPOPRS 0x00001e7b (MOVE ms,,%f3), (%r1); 13 FD 1D 00 00 52 = SPOPWD 0x00001dfd (MOVE %f3,,md), (%r2)

	mmovss	%f1,%f2			; 32 F2 1C 00 00 = SPOP 0x00001cf2 (MOVE %f1,,%s2)
	mmovss	(%r1),%f2		; 22 72 1E 00 00 51 = SPOPRS 0x00001e72 (MOVE ms,,%s2), (%r1)
	mmovss	%f1,(%r2)		; 33 FC 1C 00 00 52 = SPOPWS 0x00001cfc (MOVE %f1,,ms), (%r2)
	mmovss	(%r1),(%r2)		; 23 7C 1E 00 00 51 52 = SPOPS2 0x00001e7c (ms,,ms), (%r1), (%r2)

	expect	1350
	mmovsw	%f1,%f2
	endexpect
	expect	1350
	mmovsw	(%r1),%f2
	endexpect
	mmovsw	%f1,(%r2)		; 33 FC 3C 00 00 52 = SPOPWS 0x00003cfc (FTOI %f1,,ms), (%r2)
	mmovsw	(%r1),(%r2)		; 23 7C 3E 00 00 51 52 = SPOPS2 0x00003e7c (FTOI ms,,ms), (%r1), (%r2)

	mmovsx	%f1,%f2			; 32 FA 1C 00 00 = SPOP 0x001cfa (MOVE %f1,,%x2)
	mmovsx	(%r1),%f2		; 22 7A 1E 00 00 51 = SPOPRS 0x00001e7a (MOVE ms,,%x2), (%r1)
	mmovsx	%f1,(%r2)		; 17 FE 1C 00 00 52 = SPOPWT 0x00001cfe (MOVE %f1,,mt), (%r2)
	mmovsx	(%r1),(%r2)		; 22 7B 1E 00 00 51 = SPOPRS 0x00001e7b (MOVE ms,,%x3), (%r1); 17 FE 1D 00 00 52 = SPOPWT 0x00001dfe (%f3,,mt), (%r2)

	expect	1350
	mmovta	%f1
	endexpect
	mmovta	(%r1)			; 22 7F 26 00 00 51 = SPOPRS 0x0000267f (WRASR ms,,), (%r1)

	expect	1350
	mmovtd	%f1
	endexpect
	mmovtd	(%r1)			; 22 76 42 00 00 51 = SPOPRS 0x00004276 (LDR ms,,), (%r1)

        expect  1350
        mmovwd %f1,%f2
        endexpect
        mmovwd (%r1),%f2		; 22 76 42 00 00 51 = SPOPRS 0x00004276 (ITOF ms,,%d2), (%r1)
        expect  1350
        mmovwd %f1,(%r2)
        endexpect
        mmovwd (%r1),(%r2)		; 22 7B 42 00 00 51 = SPOPRS 0x0000427b (ITOF ms,,%x3), (%r1); 13 FD 1D 00 00 52 = SPOPWD 0x00001dfd (MOVE %f3,,md), (%r2)

        expect  1350
        mmovws %f1,%f2
        endexpect
        mmovws (%r1),%f2		; 22 72 42 00 00 51 = SPOPRS 0x00004272 (ITOF ms,,%s2), (%r1)
        expect  1350
        mmovws %f1,(%r2)
        endexpect
        mmovws (%r1),(%r2)		; 23 7C 42 00 00 51 52 = SPOPS2 0x0000427x (ITOF ms,,ms), (%r1), (%r2)

        expect  1350
        mmovwx %f1,%f2
        endexpect
        mmovwx (%r1),%f2		; 22 7A 42 00 00 51 = SPOPRS 0x0000427a (ITOF ms,,%x2), (%r1)
        expect  1350
        mmovwx %f1,(%r2)
        endexpect
        mmovwx (%r1),(%r2)		; 22 7B 42 00 00 51 = SPOPRS 0x0000427b (ITOF ms,,%x3); 17 FE 1D 00 00 52 = SPOPWT 0x00001dfe (MOVE %f3,,mt), (%r2)

	expect	1350
	mmovx10	%f1,%f2
	endexpect
	expect	1350
	mmovx10	(%r1),%f2
	endexpect
	mmovx10	%f1,(%r2)		; 17 FE 48 00 00 52 = SPOPWT 0x000048fe (FTOD %f1,,mt), (%r2)
	mmovx10	(%r1),(%r2)		; 07 7E 4B 00 00 51 52 = SPOPT2 0x00004b7e (FTOD mt,,mt), (%r1), (%r2)

	mmovxd	%f1,%f2			; 32 F6 1C 00 00 = SPOP 0x00001cf6 (MOVE %f1,,%d2)
	mmovxd	(%r1),%f2		; 06 76 1F 00 00 51 = SPOPRT 0x00001f76 (MOVE mt,,%d2), (%r1)
	mmovxd	%f1,(%r2)		; 13 FD 1C 00 00 52 = SPOPWD 0x00001cfd (MOVE %f1,,md), (%r2)
	mmovxd	(%r1),(%r2)		; 06 7B 1F 00 00 51 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r1); 13 FD 1D 00 00 52 = SPOPWD 0x00001dfd (%f3,,md), (%r2)

	mmovxs	%f1,%f2			; 32 F2 1C 00 00 = SPOP 0x00001cf2 (MOVE %f1,,%s2)
	mmovxs	(%r1),%f2		; 06 72 1F 00 00 51 = SPOPRT 0x00001f72 (MOVE mt,,%s2), (%r1)
	mmovxs	%f1,(%r2)		; 33 FC 1C 00 00 52 = SPOPWS 0x00001cfc (%f1,,ms), (%r2)
	mmovxs	(%r1),(%r2)		; 06 7B 1F 00 00 51 = SPOPRT 0x00001f7b (MOVE mt,,%x3), (%r1); 33 FC 1D 00 00 52 = SPOPWS 0x00001dfc (%f3,,ms), (%r2)

	expect	1350
	mmovxw	%f1,%f2
	endexpect
	expect	1350
	mmovxw	(%r1),%f2
	endexpect
	mmovxw	%f1,(%r2)		; 33 FC 3C 00 00 52 = SPOPWS 0x00003cfc (FTOI %f1,,ms), (%r2)
	mmovxw	(%r1),(%r2)		; 06 7B 1F 00 00 51 = SPOPRT 0x00001f7b (MOVE mt,,%f3), (%r1); 33 FC 3D 00 00 52 = SPOPWS 0x00003dfc (FTOI %f3,,ms), (%r1)

	mmovxx	%f1,%f2			; 32 FA 1C 00 00 = SPOP 0x00001cfa (MOVE %f1,,%x2)
	mmovxx	(%r1),%f2		; 06 7A 1F 00 00 51 = SPOPRT 0x00001f7a (MOVE mt,,%x2), (%r1)
	mmovxx	%f1,(%r2)		; 17 FE 1C 00 00 52 = SPOPWT 0x00001cfe (MOVE %f1,,mt), (%r2)
	mmovxx	(%r1),(%r2)		; 07 7E 1F 00 00 51 52 = SPOPT2 0x00001f7e (mt,,mtt), (%r1), (%r2)

	.flt	1.0			; 3F 80 00 00
	.double	100.5			; 40 59 20 00 00 00 00 00
