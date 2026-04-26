	; SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
	cpu	sym53c895
	page	0

	nop				; 80000000 00000000
	disconnect			; 48000000 00000000

	expect	1110
	jump
	endexpect
	jump	0x12345678
	jump	rel($+0x345678)
	jump	0x12345678, when carry
	jump	rel($+0x345678), when carry
	jump	0x12345678, if atn
	jump	rel($+0x345678), if atn
	jump	0x12345678, when not carry
	jump	rel($+0x345678), when not carry
	jump	0x12345678, if not atn
	jump	rel($+0x345678), if not atn
	jump	0x12345678, if data_out
	jump	rel($+0x345678), if data_out
	jump	0x12345678, when data_out
	jump	rel($+0x345678), when data_out
	jump	0x12345678, if data_in
	jump	rel($+0x345678), if data_in
	jump	0x12345678, when data_in
	jump	rel($+0x345678), when data_in
	jump	0x12345678, if cmd
	jump	rel($+0x345678), if command
	jump	0x12345678, when cmd
	jump	rel($+0x345678), when command
	jump	0x12345678, if status
	jump	rel($+0x345678), if status
	jump	0x12345678, when status
	jump	rel($+0x345678), when status
	jump	0x12345678, if msg_out
	jump	rel($+0x345678), if message_out
	jump	0x12345678, when msg_out
	jump	rel($+0x345678), when message_out
	jump	0x12345678, if msg_in
	jump	rel($+0x345678), if message_in
	jump	0x12345678, when msg_in
	jump	rel($+0x345678), when message_in
	jump	0x12345678, if msg_in and 0xaa and mask 0x55
	jump	rel($+0x345678), if msg_in and 0xaa and mask 0x55
	jump	0x12345678, when msg_in and 0xaa and mask 0x55
	jump	rel($+0x345678), when msg_in and 0xaa and mask 0x55
	call	0x12345678, when msg_in and 0xaa and mask 0x55
	call	rel($+0x345678), when msg_in and 0xaa and mask 0x55
	return
	return	when msg_in
	return	when 0xaa and mask 0x55
	int	0, when msg_in
	int	0, when 0xaa and mask 0x55
	intfly
	intfly	0, when msg_in
	intfly	0, when 0xaa and mask 0x55

	chmov	from , with msg_out
	chmov	0x123456, ptr 0x55, when msg_out

	set	ack
	set	atn and target
	set	ack and atn and target and carry
	clear	ack
	clear	atn and target
	clear	ack and atn and target and carry

	load	dstat,3,0x12345678
	load	dstat,3,dsarel(0x345678)
	store	dstat,3,0x12345678
	store	dstat,3,dsarel(0x345678)

	move	0x20 to dstat
	move	scratchb + 0x20 to scratchb
	move	from 0x12345678, with msg_out
	move	from 0x12345678, when msg_out
	move	memory 0x123456, 0x12345678, 0x23456789
	move	memory no flush 0x123456, 0x12345678, 0x23456789
	move	0x123456, 0x12345678, when msg_in
	move	0x123456, ptr 0x12345678 , with msg_in

	select	7, 0x87654321
	select	atn 7, 0x87654321
	select	from 0x123456, 0x87654321
	select	atn from 0x123456, 0x87654321
	select	atn from 0x123456, rel($+12345)
	reselect from 0x123456, 0x87654321

	wait	disconnect
	wait	select 0x12345678
	wait	select rel($+123456)
	wait	reselect 0x12345678
	wait	reselect rel($+123456)
