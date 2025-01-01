; /*--------------------------------------------------*/
; /*                                                  */
; /* Module GENSCSI.SS                                */
; /*                                                  */
; /* SCRIPTS routines for generic SCSI operations.    */
; /*                                                  */
; /* Adapted from Symbios Logic                       */
; /*    Software Development Kit                      */
; /*                                                  */
; /* Project: A Programmer's Guide to SCSI            */
; /* Copyright (C) 1997, Brian Sawert.                */
; /* All rights reserved.                             */
; /* Syntax adapted to AS to serve as a test case     */
; /*                                                  */
; /*--------------------------------------------------*/


; /*--------------------------------------------------*/
; /*                                                  */
; /* This script is a generic skeleton for issuing    */
; /* SCSI commands. It handles arbitration, message   */
; /* in and out, status and data in.                  */
; /*                                                  */
; /* Uses table indirect mode for command, data, and  */
; /* other buffers.                                   */
; /*                                                  */
; /*--------------------------------------------------*/


;---------- set architecture for 53C825
	cpu	sym53c825


;---------- set constant values
err_cmd_complete	equ	0x00000000
err_not_msgout		equ	0x00000001
err_bad_reselect	equ	0x00000002
err_bad_phase		equ	0x00000004


;---------- set up table definitions

scsi_id		db	0x33, 0x00, 0x00, 0x00
msgout_buf	db	0x80, 0x00
cmd_buf		db	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
stat_buf	db	?
msgin_buf	db	2 dup (?)
exmsgin_buf	db	4 dup (?)
datain_buf	db	0x40 dup (?)

GEN_SCRIPT:	align	4

;---------- start SCSI target selection
start_scsi:

; select device from encoded SCSI ID
; set ATN for message out after select
	SELECT ATN FROM scsi_id, REL(bad_reselect)

;---------- send identify message
; exit if not message out phase
	INT err_not_msgout, WHEN NOT MSG_OUT

; send identify message
	MOVE FROM msgout_buf, WHEN MSG_OUT
	JUMP REL(handle_phase)

;---------- send SCSI command
send_cmd:

; send command block to target
	MOVE FROM cmd_buf, WHEN CMD
	JUMP REL(handle_phase)

;---------- get SCSI status
get_status:

; read status byte from data bus
	MOVE FROM stat_buf, WHEN STATUS
	JUMP REL(handle_phase)

;---------- get SCSI message input
get_msgin:

; read message byte from data bus
	MOVE FROM msgin_buf, WHEN MSG_IN
	CLEAR ACK

; handle Command Complete message
	JUMP REL(cmd_complete), IF 0x00

; handle Disconnect message
	JUMP REL(wait_disconnect), IF 0x04

; handle extended message
	JUMP REL(ext_msgin), IF 0x01
	JUMP REL(handle_phase)

;---------- handle extended message
ext_msgin:

; read extended message from data bus
	MOVE FROM exmsgin_buf, WHEN MSG_IN
	CLEAR ACK
	JUMP REL(handle_phase)

;---------- get data input
get_datain:

; read data from bus
	MOVE FROM datain_buf, WHEN DATA_IN
	JUMP REL(handle_phase)

;---------- handle SCSI phases
handle_phase:

; jump to appropriate handler for phase
	JUMP REL(get_status), WHEN STATUS
	JUMP REL(get_msgin), WHEN MSG_IN
	JUMP REL(get_datain), WHEN DATA_IN
	JUMP REL(send_cmd), WHEN COMMAND
; unhandled phase
	INT err_bad_phase

;---------- SCSI command execution complete
cmd_complete:

; command complete - wait for disconnect
	MOVE SCNTL2 & 0x7F to SCNTL2
	CLEAR ACK
	WAIT DISCONNECT
	INT err_cmd_complete

;---------- handle invalid select or reselect
bad_reselect:

; unhandled reselect
	INT err_bad_reselect

;---------- handle disconnect before reselect
wait_disconnect:

	MOVE SCNTL2 & 0x7F to SCNTL2
	CLEAR ACK
	WAIT DISCONNECT

; clear DMA and SCSI fifos
	MOVE CTEST3 | 0x04 to CTEST3
	MOVE STEST3 | 0x02 to STEST3

; wait for reselect
	WAIT RESELECT REL(bad_reselect)

; expect identify message
	MOVE FROM msgin_buf, WHEN MSG_IN
	CLEAR ACK

; shortcut to update sync and wide options
	SELECT FROM scsi_id, REL(handle_phase)

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"

;---------- entry point for general SCSI script
	end	start_scsi
