		ifndef	__reg80f91inc
__reg80f91inc	equ	1
		save
		listing	off	; no listing over this file

;****************************************************************************
;*                                                                          *
;*   AS(L) - File REG80F91.INC                                              *
;*                                                                          *
;*   Contains Bit & Register Definitions for Zilog eZ80F91                  *
;*                                                                          *
;****************************************************************************

		include	"spi.inc"
		include "uart.inc"
		include "i2c.inc"
		include	"gpio.inc"
		include "rtc.inc"
		include "flash.inc"

;----------------------------------------------------------------------------
; Memory Limits

RAMSTART	equ	0e000h		; assumes RAM_ADDR_U = 00h
RAMEND		equ	RAMSTART+1fffh

EMAC_RAMSTART	equ	0c000h
EMAC_RAMEND	equ	EMAC_RAMSTART+1fffh

;----------------------------------------------------------------------------
; Product ID

ZDI_ID_L	port	00h		; eZ80 Product ID Low Byte Register (r)
ZDI_ID_H	port	01h		; eZ80 Product ID High Byte Register (r)
ZDI_ID_REV	port	02h		; eZ80 Product ID Revision Register (r)

;----------------------------------------------------------------------------
; Interrupt Priority

INT_P0		port	10h		; Interrupt Priority Register - Byte 0 (r/w)
INT_P1		port	11h		; Interrupt Priority Register - Byte 1 (r/w)
INT_P2		port	12h		; Interrupt Priority Register - Byte 2 (r/w)
INT_P3		port	13h		; Interrupt Priority Register - Byte 3 (r/w)
INT_P4		port	14h		; Interrupt Priority Register - Byte 4 (r/w)
INT_P5		port	15h		; Interrupt Priority Register - Byte 5 (r/w)

;----------------------------------------------------------------------------
; Ethernet Media Access Controller

EMAC_TEST	port	20h		; EMAC Test Register (r/w)
TEST_FIFO	equ	1 << 6		;  FIFO test mode enabled
TxRx_SEL	equ	1 << 5		;  Select the Transmit FIFO when FIFO test mode is enabled
SSTC		equ	1 << 4		;  Short Cut Slot Timer Counter
SIMR		equ	1 << 3		;  Simulation Reset
FRC_OVR_ERR	equ	1 << 2		;  Force Overrun error in Receive FIFO
FRC_UND_ERR	equ	1 << 1		;  Force Underrun error in Transmit FIFO
LPBK		equ	1 << 0		;  EMAC Transmit interface is looped back into EMAC Receive interface
EMAC_CFG1	port	21h		; EMAC Configuration Register 1 (r/w)
PADEN		equ	1 << 7		;  EMAC pads all short frames by adding zeroes to the end of the data field
ADPADN		equ	1 << 6		;  EMAC pads all short frames to 64 bytes and append a valid CRC. This bit is ignored if PADEN is cleared to 0
VLPAD		equ	1 << 5		;  Enable frame detection by comparing the two bytes following the source address with 0x8100 (VLAN Protocol ID) and pad accordingly.
CRCEN		equ	1 << 4		;  Append CRC to every frame regardless of padding options
FULLD		equ	1 << 3		;  Enable FULL-DUPLEX mode
FLCHK		equ	1 << 2		;  Both Transmit and Receive frame lengths are compared to the length/type field.
HUGEN		equ	1 << 1		;  Allow unlimited sized frames to be received
DCRCC		equ	1 << 0		;  Four bytes of proprietary header, ignored by CRC, exists on the front of IEEE 802.3 frames
EMAC_CFG2	port	22h		; EMAC Configuration Register 2 (r/w)
BPNB		equ	1 << 7		;  No Backoff
NOBO		equ	1 << 6		;  No exponential backfill
LCOL_S		equ	0		;  # of bytes after SFD for late collision
LCOL_M		equ	3fh << LCOL_S
EMAC_CFG3	port	23h		; EMAC Configuration Register 3 (r/w)
LONGP		equ	1 << 7		;  The EMAC only allows Receive packets that contain preamble fields less than 12 bytes in length
PUREP		equ	1 << 6		;  The EMAC verifies the content of the preamble to ensure that it contains a value of 55h and that it is error-free
XSDFR		equ	1 << 5		;  The EMAC defers to the carrier indefinitely as per the IEEE 802.3 specification
BITMD		equ	1 << 4		;  Enable 10 Mbps ENDEC mode
RETRY_S		equ	0		;  number of retransmission after collision
RETRY_M		equ	15 << RETRY_S
EMAC_CFG4	port	24h		; EMAC Configuration Register 4 (r/w)
TPCF		equ	1 << 6		;  Transmit pause control frame
THDF		equ	1 << 5		;  Enable back pressure
PARF		equ	1 << 4		;  Receive broken frames
RxFC		equ	1 << 3		;  EMAC acts upon pause control frames received
TxFC		equ	1 << 2		;  PAUSE control frames are allowed to be transmitted
TPAUSE		equ	1 << 1		;  Force a pause condition
RxEN		equ	1 << 0		;  EMAC receiver enabled
EMAC_STAD_0	port	25h		; EMAC Station Address - Byte 0 (r/w)
EMAC_STAD_1	port	26h		; EMAC Station Address - Byte 1 (r/w)
EMAC_STAD_2	port	27h		; EMAC Station Address - Byte 2 (r/w)
EMAC_STAD_3	port	28h		; EMAC Station Address - Byte 3 (r/w)
EMAC_STAD_4	port	29h		; EMAC Station Address - Byte 4 (r/w)
EMAC_STAD_5	port	2ah		; EMAC Station Address - Byte 5 (r/w)
EMAC_TPTV_L	port	2bh		; EMAC Transmit Pause Timer Value - Low Byte (r/w)
EMAC_TPTV_H	port	2ch		; EMAC Transmit Pause Timer Value - High Byte (r/w)
EMAC_IPGT	port	2dh		; EMAC Inter-Packet Gap (r/w)
EMAC_IPGR1	port	2eh		; EMAC Non-Back-Back IPG (r/w)
EMAC_IPGR2	port	2fh		; EMAC Non-Back-Back IPG (r/w)
EMAC_MAXF_L	port	30h		; EMAC Maximum Frame Length - Low Byte (r/w)
EMAC_MAXF_H	port	31h		; EMAC Maximum Frame Length - High Byte (r/w)
EMAC_AFR	port	32h		; EMAC Address Filter Register (r/w)
PROM		equ	1 << 3		;  Enable Promiscuous Mode
MC		equ	1 << 2		;  Accept any multicast message
QMC		equ	1 << 1		;  Accept only qualified multicast (QMC) messages
BC		equ	1 << 0		;  Accept broadcast messages
EMAC_HTBL_0	port	33h		; EMAC Hash Table - Byte 0 (r/w)
EMAC_HTBL_1	port	34h		; EMAC Hash Table - Byte 1 (r/w)
EMAC_HTBL_2	port	35h		; EMAC Hash Table - Byte 2 (r/w)
EMAC_HTBL_3	port	36h		; EMAC Hash Table - Byte 3 (r/w)
EMAC_HTBL_4	port	37h		; EMAC Hash Table - Byte 4 (r/w)
EMAC_HTBL_5	port	38h		; EMAC Hash Table - Byte 5 (r/w)
EMAC_HTBL_6	port	39h		; EMAC Hash Table - Byte 6 (r/w)
EMAC_HTBL_7	port	3ah		; EMAC Hash Table - Byte 7 (r/w)
EMAC_MIIMGT	port	3bh		; EMAC MII Management Register (r/w)
LCTLD		equ	1 << 7		;  Rising edge causes the CTLD control data to be transmitted to external PHY if MII is not busy.
RSTAT		equ	1 << 6		;  Rising edge causes status to be read from external PHY
SCINC		equ	1 << 5		;  Scan PHY address increments upon SCAN cycle
SCAN		equ	1 << 4		;  Perform continuous Read cycles via MII management
SPRE		equ	1 << 3		;  Suppress the MDO preamble
CLKS_S		equ	0		;  MDIO Clock Divisor
CLKS_M		equ	7 << CLKS_S
EMAC_CTLD_L	port	3ch		; EMAC PHY Configuration Data - Low Byte (r/w)
EMAC_CTLD_H	port	3dh		; EMAC PHY Configuration Data - High Byte (r/w)
EMAC_RGAD	port	3eh		; EMAC PHY Register Address Register (r/w)
EMAC_FIAD	port	3fh		; EMAC PHY Unit Select Address Register (r/w)
EMAC_PTMR	port	40h		; EMAC Transmit Polling Timer Register (r/w)
EMAC_RST	port	41h		; EMAC Reset Control Register (r/w)
SRST		equ	1 << 5		;  Software Reset
HRTFN		equ	1 << 4		;  Reset Transmit function
HRRFN		equ	1 << 3		;  Reset Receive function
HRTMC		equ	1 << 2		;  Reset EMAC Transmit Control function
HRRMC		equ	1 << 1		;  Reset EMAC Receive Control function
HRMGT		equ	1 << 0		;  Reset EMAC Management function
EMAC_TLBP_L	port	42h		; EMAC Transmit Lower Boundary Pointer - Low Byte (r/w)
EMAC_TLBP_H	port	43h		; EMAC Transmit Lower Boundary Pointer - High Byte (r/w)
EMAC_BP_L	port	44h		; EMAC Boundary Pointer - Low Byte (r/w)
EMAC_BP_H	port	45h		; EMAC Boundary Pointer - High Byte (r/w)
EMAC_BP_U	port	46h		; EMAC Boundary Pointer - Upper Byte (r/w)
EMAC_RHBP_L	port	47h		; EMAC Receive High Boundary Pointer - Low Byte (r/w)
EMAC_RHBP_H	port	48h		; EMAC Receive High Boundary Pointer - High Byte (r/w)
EMAC_RRP_L	port	49h		; EMAC Receive Read Pointer - Low Byte (r/w)
EMAC_RRP_H	port	4ah		; EMAC Receive Read Pointer - High Byte (r/w)
EMAC_BUFSZ	port	4bh		; EMAC Buffer Size Register (r/w)
BUFSZ_S		equ	6		;  EMAC Rx/Tx buffer size
BUFSZ_M		equ	3 << BUFSZ_S
BUFSZ_256B	equ	0 << BUFSZ_S
BUFSZ_128B	equ	1 << BUFSZ_S
BUFSZ_64B	equ	2 << BUFSZ_S
BUFSZ_32B	equ	3 << BUFSZ_S
TPCF_LEV_S	equ	0		;  Transmit Pause Control Frame level
TPCF_LEV_M	equ	3fh << TPCF_LEV_S
EMAC_IEN	port	4ch		; EMAC Interrupt Enable Register (r/w)
TxFSMERR	equ	1 << 7		;  Enable Transmit State Machine Error Interrupt
MGTDONE		equ	1 << 6		;  Enable MII Management Done Interrupt
Rx_CF		equ	1 << 5		;  Enable Receive Control Frame Interrupt
Rx_PCF		equ	1 << 4		;  Enable Receive Pause Control Frame Interrupt
Rx_DONE		equ	1 << 3		;  Enable Receive Done Interrupt
Rx_OVR		equ	1 << 2		;  Enable Receive Overrun Interrupt
Tx_CF		equ	1 << 1		;  Enable Transmit Control Frame Interrupt
Tx_DONE		equ	1 << 0		;  Enable Transmit Done Interrupt
EMAC_ISTAT	port	4dh		; EMAC Interrupt Status Register (r/w)
TxFSMERR_STAT	equ	1 << 7		;  Internal error occured in the EMAC Transmit path
MGTDONE_STAT	equ	1 << 6		;  The MII Management has completed
Rx_CF_STAT	equ	1 << 5		;  Receive Control Frame Interrupt
Rx_PCF_STAT	equ	1 << 4		;  Receive Pause Control Frame Interrupt
Rx_DONE_STAT	equ	1 << 3		;  Receive Done Interrupt
Rx_OVR_STAT	equ	1 << 2		;  Receive Overrun Interrupt
Tx_CF_STAT	equ	1 << 1		;  Transmit Control Frame Interrupt
Tx_DONE_STAT	equ	1 << 0		;  Transmit Done Interrupt
EMAC_PRSD_L	port	4eh		; EMAC PHY Read Status Data - Low Byte (r/w)
EMAC_PRSD_H	port	4fh		; EMAC PHY Read Status Data - High Byte (r/w)
EMAC_MIISTAT	port	50h		; EMAC MII Status Register (r/w)
BUSY		equ	1 << 7		;  MII management operation in progress
MIILF		equ	1 << 6		;  Local copy of PHY Link fail bit
NVALID		equ	1 << 5		;  MII Scan result is not valid
RDADR_S		equ	0		;  Denotes PHY addressed in current scan cycle
RDADR_M		equ	1fh << RDADR_S
EMAC_RWP_L	port	51h		; EMAC Receive Write Pointer - Low Byte (r/w)
EMAC_RWP_H	port	52h		; EMAC Receive Write Pointer - High Byte (r/w)
EMAC_TRP_L	port	53h		; EMAC Transmit Read Pointer - Low Byte (r/w)
EMAC_TRP_H	port	54h		; EMAC Transmit ReadPointer - High Byte (r/w)
EMAC_BLKSLFT_L	port	55h		; EMAC Receive Blocks Left Register - Low Byte (r/w)
EMAC_BLKSLFT_H	port	56h		; EMAC Receive Blocks Left Register - High Byte (r/w)
EMAC_FDATA_L	port	57h		; EMAC FIFO Data - Low Byte (r/w)
EMAC_FDATA_H	port	58h		; EMAC FIFO Data - High Byte (r/w)
EMAC_FFLAGS	port	59h		; EMAC FIFO Flags Register (r/w)
TFF		equ	1 << 7		;  Transmit FIFO full
TFAE		equ	1 << 5		;  Transmit FIFO almost empty
TFE		equ	1 << 4		;  Transmit FIFO empty
RFF		equ	1 << 3		;  Receive FIFO full
RFAF		equ	1 << 2		;  Receive FIFO almost full
RFAE		equ	1 << 1		;  Receive FIFO almost empty
RFE		equ	1 << 0		;  Receive FIFO empty

IVEC_EMAC_Rx	label	40h		; EMAC Rx interrupt vector
IVEC_EMAC_Tx	label	44h		; EMAC Tx interrupt vector
IVEC_EMAC_SYS	label	48h		; EMAC system interrupt vector

;----------------------------------------------------------------------------
; PLL

PLL_DIV_L	port	5ch		; PLL Divider Register - Low Byte (w)
PLL_DIV_H	port	5dh		; PLL Divider Register - High Byte (w)
PLL_CTL0	port	5eh		; PLL Control Register 0 (r/w)
CHRP_CTL1_S	equ	6		;  Charge Pump Current
CHRP_CTL1_M	equ	3 << CHRP_CTL1_S
CHRP_CTL1_100u	equ	0 << CHRP_CTL1_S ;  100 uA
CHRP_CTL1_500u	equ	1 << CHRP_CTL1_S ;  500 uA
CHRP_CTL1_1m	equ	2 << CHRP_CTL1_S ;  1 mA
CHRP_CTL1_1m5	equ	3 << CHRP_CTL1_S ;  1.5 mA
LDS_CTL1_S	equ	2		;  Lock Criteria
LDS_CTL1_M	equ	3 << LDS_CTL1_S
LDS_CTL1_8_20	equ	0 << LDS_CTL1_S	;  8 cycles a 20 ns
LDS_CTL1_16_20	equ	1 << LDS_CTL1_S	;  16 cycles a 20 ns
LDS_CTL1_8_400	equ	2 << LDS_CTL1_S	;  8 cycles a 400 ns
LDS_CTL1_16_400	equ	3 << LDS_CTL1_S	;  16 cycles a 400 ns
CLK_MUX_S	equ	0		;  System Clock Source
CLK_MUX_M	equ	3 << CLK_MUX_S
CLK_MUX_EXT	equ	0 << CLK_MUX_S	;   External Crystal Oscillator
CLK_MUX_PLL	equ	1 << CLK_MUX_S	;   PLL
CLK_MUX_RTC	equ	2 << CLK_MUX_S	;   RTC Crystal Oscillator
PLL_CTL1	port	5fh		; PLL Control Register 1 (r/w)
LCK_STATUS	equ	1 << 5		;  PLL is currently locked
INT_LOCK	equ	1 << 4		;  Interrupt generated when PLL enters LOCK mode
INT_UNLOCK	equ	1 << 3		;  Interrupt generated when PLL goes out of lock
INT_LOCK_EN	equ	1 << 2		;  Interrupt generation for PLL locked condition
INT_UNLOCK_EN	equ	1 << 1		;  Interrupt generation for PLL unlocked condition
PLL_ENABLE	equ	1 << 0		;  PLL Enable

IVEC_PLL	label	04ch		; PLL interrupt vector

;----------------------------------------------------------------------------
; Timer

TMR0_CTL	port	60h		; Timer 0 Control Register (r/w)
BRK_STOP	equ	1 << 7		;  The timer stops operation and holds count value during debug break points
CLK_SEL_S	equ	5		;  Clock Select
CLK_SEL_M	equ	3 << CLK_SEL_S
CLK_SEL_SYSCLK	equ	0 << CLK_SEL_S	;   System clock divided by the prescaler
CLK_SEL_RTC	equ	1 << CLK_SEL_S	;   Real Time Clock Input
CLK_SEL_EC_FALL	equ	2 << CLK_SEL_S	;   Event Count (ECx) input - falling edge
CLK_SEL_EC_RISE	equ	3 << CLK_SEL_S	;   Event Count (ECx) input - rising edge
CLK_DIV_S	equ	3		;  Clock Divider
CLK_DIV_M	equ	3 << CLK_DIV_S
CLK_DIV_4	equ	0 << CLK_DIV_S
CLK_DIV_16	equ	1 << CLK_DIV_S
CLK_DIV_64	equ	2 << CLK_DIV_S
CLK_DIV_256	equ	3 << CLK_DIV_S
TIM_CONT	equ	1 << 2		;  Continuous mode
RLD		equ	1 << 1		;  Reload
TIM_EN		equ	1 << 0		;  Timer Enable
TMR0_IER	port	61h		; Timer 0 Interrupt Enable Register (r/w)
IRQ_OC3_EN	equ	1 << 6		;  Enable interrupt requests for OC3
IRQ_OC2_EN	equ	1 << 5		;  Enable interrupt requests for OC2
IRQ_OC1_EN	equ	1 << 4		;  Enable interrupt requests for OC1
IRQ_OC0_EN	equ	1 << 3		;  Enable interrupt requests for OC0
IRQ_ICB_EN	equ	1 << 2		;  Enable interrupt requests for ICx
IRQ_ICA_EN	equ	1 << 1		;  Enable interrupt requests for ICA or PWM power trip
IRQ_EOC_EN	equ	1 << 0		;  Enable interrupt on end-of-count
TMR0_IIR	port	62h		; Timer 0 Interrupt Identification Register (r/w)
OC3		equ	1 << 6		;  OC3 interrupt occured
OC2		equ	1 << 5		;  OC2 interrupt occured
OC1		equ	1 << 4		;  OC1 interrupt occured
OC0		equ	1 << 3		;  OC0 interrupt occured
ICB		equ	1 << 2		;  ICx interrupt occured
ICA		equ	1 << 1		;  ICA or PWM power trip interrupt occured
EOC		equ	1 << 0		;  End-of-count interrupt occured
TMR0_DR_L	port	63h		; Timer 0 Data Register Low (r)
TMR0_RR_L	port	63h		; Timer 0 Reload Register Low (w)
TMR0_DR_H	port	64h		; Timer 0 Data Register High (r)
TMR0_RR_H	port	64h		; Timer 0 Reload Register High (w)

IVEC_TIMER0	label	54h		; Timer 0 Interrupt Vector

TMR1_CTL	port	65h		; Timer 1 Control Register (r/w)
TMR1_IER	port	66h		; Timer 1 Interrupt Enable Register (r/w)
TMR1_IIR	port	67h		; Timer 1 Interrupt Identification Register (r/w)
TMR1_DR_L	port	68h		; Timer 1 Data Register Low (r)
TMR1_RR_L	port	68h		; Timer 1 Reload Register Low (w)
TMR1_DR_H	port	69h		; Timer 1 Data Register High (r)
TMR1_RR_H	port	69h		; Timer 1 Reload Register High (w)
TMR1_CAP_CTL	port	6ah		; Timer 1 Input Capture Control Register (r/w)
CAP_EDGE_B_S	equ	2		;  Capture Mode B
CAP_EDGE_B_M	equ	3 << CAP_EDGE_B_S
CAP_EDGE_B_NONE	equ	0 << CAP_EDGE_B_S ; No capture
CAP_EDGE_B_FALL	equ	1 << CAP_EDGE_B_S ; Capture on falling edge
CAP_EDGE_B_RISE	equ	2 << CAP_EDGE_B_S ; Capture on risig edge
CAP_EDGE_B_BOTH	equ	3 << CAP_EDGE_B_S ; Capture on both edges
CAP_EDGE_A_S	equ	0		;  Capture Mode A
CAP_EDGE_A_M	equ	3 << CAP_EDGE_B_S
CAP_EDGE_A_NONE	equ	0 << CAP_EDGE_B_S ; No capture
CAP_EDGE_A_FALL	equ	1 << CAP_EDGE_B_S ; Capture on falling edge
CAP_EDGE_A_RISE	equ	2 << CAP_EDGE_B_S ; Capture on risig edge
CAP_EDGE_A_BOTH	equ	3 << CAP_EDGE_B_S ; Capture on both edges
TMR1_CAPA_L	port	6bh		; Timer 1 Capture Value A Register - Low Byte (r/w)
TMR1_CAPA_H	port	6ch		; Timer 1 Capture Value A Register - High Byte (r/w)
TMR1_CAPB_L	port	6dh		; Timer 1 Capture Value B Register - Low Byte (r/w)
TMR1_CAPB_H	port	6eh		; Timer 1 Capture Value B Register - High Byte (r/w)

IVEC_TIMER1	label	58h		; Timer 1 Interrupt Vector

TMR2_CTL	port	6fh		; Timer 2 Control Register (r/w)
TMR2_IER	port	70h		; Timer 2 Interrupt Enable Register (r/w)
TMR2_IIR	port	71h		; Timer 2 Interrupt Identification Register (r/w)
TMR2_DR_L	port	72h		; Timer 2 Data Register Low (r)
TMR2_RR_L	port	72h		; Timer 2 Reload Register Low (w)
TMR2_DR_H	port	73h		; Timer 2 Data Register High (r)
TMR2_RR_H	port	73h		; Timer 2 Reload Register High (w)

IVEC_TIMER2	label	5ch		; Timer 2 Interrupt Vector

TMR3_CTL	port	74h		; Timer 3 Control Register (r/w)
TMR3_IER	port	75h		; Timer 3 Interrupt Enable Register (r/w)
TMR3_IIR	port	76h		; Timer 3 Interrupt Identification Register (r/w)
TMR3_DR_L	port	77h		; Timer 3 Data Register Low (r)
TMR3_RR_L	port	77h		; Timer 3 Reload Register Low (w)
TMR3_DR_H	port	78h		; Timer 3 Data Register High (r)
TMR3_RR_H	port	78h		; Timer 3 Reload Register High (w)
TMR3_CAP_CTL	port	7bh		; Timer 3 Input Capture Control Register (r/w)
TMR3_CAPA_L	port	7ch		; Timer 3 Capture Value A Register - Low Byte (r/w)
TMR3_CAPA_H	port	7dh		; Timer 3 Capture Value A Register - High Byte (r/w)
TMR3_CAPB_L	port	7eh		; Timer 3 Capture Value B Register - Low Byte (r/w)
TMR3_CAPB_H	port	7fh		; Timer 3 Capture Value B Register - High Byte (r/w)
TMR3_OC_CTL1	port	80h		; Timer 3 Output Compare Control Register 1 (r/w)
OC3_INIT	equ	1 << 5		;  OC3 initial pin state
OC2_INIT	equ	1 << 4		;  OC2 initial pin state
OC1_INIT	equ	1 << 3		;  OC1 initial pin state
OC0_INIT	equ	1 << 2		;  OC0 initial pin state
MAST_MODE	equ	1 << 1		;  OC pins all mimic OC0
OC_EN		equ	1 << 0		;  OUTPUT COMPARE mode is enabled
TMR3_OC_CTL2	port	81h		; Timer 3 Output Compare Control Register 2 (r/w)
OC3_MODE_S	equ	6		;  What to happen when compare event takes place
OC3_MODE_M	equ	3 << OC3_MODE_S
OC3_MODE_INIT	equ	0 << OC3_MODE_S	;  Set pin to OCx_INIT
OC3_MODE_CLR	equ	1 << OC3_MODE_S ;  Clear pin
OC3_MODE_SET	equ	2 << OC3_MODE_S ;  Set pin
OC3_MODE_TGL	equ	3 << OC3_MODE_S ;  Toggle pin
OC2_MODE_S	equ	4
OC2_MODE_M	equ	3 << OC2_MODE_S
OC2_MODE_INIT	equ	0 << OC2_MODE_S
OC2_MODE_CLR	equ	1 << OC2_MODE_S
OC2_MODE_SET	equ	2 << OC2_MODE_S
OC2_MODE_TGL	equ	3 << OC2_MODE_S
OC1_MODE_S	equ	2
OC1_MODE_M	equ	3 << OC1_MODE_S
OC1_MODE_INIT	equ	0 << OC1_MODE_S
OC1_MODE_CLR	equ	1 << OC1_MODE_S
OC1_MODE_SET	equ	2 << OC1_MODE_S
OC1_MODE_TGL	equ	3 << OC1_MODE_S
OC0_MODE_S	equ	0
OC0_MODE_M	equ	3 << OC0_MODE_S
OC0_MODE_INIT	equ	0 << OC0_MODE_S
OC0_MODE_CLR	equ	1 << OC0_MODE_S
OC0_MODE_SET	equ	2 << OC0_MODE_S
OC0_MODE_TGL	equ	3 << OC0_MODE_S
TMR3_OC0_L	port	82h		; Timer 3 Output Compare 0 Value Register - Low Byte (r/w)
TMR3_OC0_H	port	83h		; Timer 3 Output Compare 0 Value Register - High Byte (r/w)
TMR3_OC1_L	port	84h		; Timer 3 Output Compare 1 Value Register - Low Byte (r/w)
TMR3_OC1_H	port	85h		; Timer 3 Output Compare 1 Value Register - High Byte (r/w)
TMR3_OC2_L	port	86h		; Timer 3 Output Compare 2 Value Register - Low Byte (r/w)
TMR3_OC2_H	port	87h		; Timer 3 Output Compare 2 Value Register - High Byte (r/w)
TMR3_OC3_L	port	88h		; Timer 3 Output Compare 3 Value Register - Low Byte (r/w)
TMR3_OC3_H	port	89h		; Timer 3 Output Compare 3 Value Register - High Byte (r/w)

IVEC_TIMER3	label	60h		; Timer 3 Interrupt Vector

PWM_CTL1	port	79h		; PWM Control Register 1 (r/w)
PAIR_EN		equ	1 << 7		;  Global enable of the PWM and -PWM output pairs
PT_EN		equ	1 << 6		;  Enable power-trip feature
MM_EN		equ	1 << 5		;  Enable Master mode
PWM3_EN		equ	1 << 4		;  Enable PWM generator 3
PWM2_EN		equ	1 << 3		;  Enable PWM generator 2
PWM1_EN		equ	1 << 2		;  Enable PWM generator 1
PWM0_EN		equ	1 << 1		;  Enable PWM generator 0
MPWM_EN		equ	1 << 0		;  Enable Multi-PWM mode
PWM_CTL2	port	7ah		; PWM Control Register 2 (r/w)
AON_EN_S	equ	6		;  AND/OR Features on -PWM
AON_EN_M	equ	3 << AON_EN_S
AON_EN_N	equ	0 << AON_EN_S	;   neither
AON_EN_A	equ	1 << AON_EN_S	;   AND only
AON_EN_O	equ	2 << AON_EN_S	;   OR only
AON_EN_AO	equ	3 << AON_EN_S	;   AND/OR
AO_EN_S		equ	4		;  AND/OR Features on PWM
AO_EN_M		equ	3 << AO_EN_S
AO_EN_N		equ	0 << AO_EN_S	;   neither
AO_EN_A		equ	1 << AO_EN_S	;   AND only
AO_EN_O		equ	2 << AO_EN_S	;   OR only
AO_EN_AO	equ	3 << AO_EN_S	;   AND/OR
PWM_DLY_S	equ	0		;  Delay between falling edge of -PWM and rising edge of PWM
PWM_DLY_M	equ	15 << PWM_DLY_S
PWM_CTL3	port	7bh		; PWM Control Register 3 (r/w)
PT_IC3_EN	equ	1 << 7		;  Power trip enabled on IC3
PT_IC2_EN	equ	1 << 6		;  Power trip enabled on IC2
PT_IC1_EN	equ	1 << 5		;  Power trip enabled on IC1
PT_IC0_EN	equ	1 << 4		;  Power trip enabled on IC0
PT_TRI		equ	1 << 3		;  All PWM trip levels are defined by PT_LVL and PT_LVL_N
PT_LVL		equ	1 << 2		;  After power trip, PWMx outputs are set to zero
PT_LVL_N	equ	1 << 1		;  After power trip, -PWMx outputs are set to zero
PTD		equ	1 << 0		;  This bit is set after power trip event
PWM0R_L		port	7ch		; PWM 0 Rising-Edge Register - Low Byte (r/w)
PWM0R_H		port	7dh		; PWM 0 Rising-Edge Register - High Byte (r/w)
PWM1R_L		port	7eh		; PWM 1 Rising-Edge Register - Low Byte (r/w)
PWM1R_H		port	7fh		; PWM 1 Rising-Edge Register - High Byte (r/w)
PWM2R_L		port	80h		; PWM 2 Rising-Edge Register - Low Byte (r/w)
PWM2R_H		port	81h		; PWM 2 Rising-Edge Register - High Byte (r/w)
PWM3R_L		port	82h		; PWM 3 Rising-Edge Register - Low Byte (r/w)
PWM3R_H		port	83h		; PWM 3 Rising-Edge Register - High Byte (r/w)
PWM0F_L		port	84h		; PWM 0 Falling-Edge Register - Low Byte (r/w)
PWM0F_H		port	85h		; PWM 0 Falling-Edge Register - High Byte (r/w)
PWM1F_L		port	86h		; PWM 1 Falling-Edge Register - Low Byte (r/w)
PWM1F_H		port	87h		; PWM 1 Falling-Edge Register - High Byte (r/w)
PWM2F_L		port	88h		; PWM 2 Falling-Edge Register - Low Byte (r/w)
PWM2F_H		port	89h		; PWM 2 Falling-Edge Register - High Byte (r/w)
PWM3F_L		port	8ah		; PWM 3 Falling-Edge Register - Low Byte (r/w)
PWM3F_H		port	8bh		; PWM 3 Falling-Edge Register - High Byte (r/w)

;----------------------------------------------------------------------------
; Watchdog Timer

WDT_CTL		port	93h		; Watchdog Timer Control Register (r/w)
WDT_EN		equ	1 << 7		; Watchdog Timer Enable
NMI_OUT		equ	1 << 6		;  NMI instead of reset
RST_FLAG	equ	1 << 5		;  Reset caused by WDT
NMI_FLAG	equ	1 << 4		;  NMI caused by WDT
WDT_CLK_S	equ	2		;  WDT Clock Source
WDT_CLK_M	equ	3 << WDT_CLK_S
WDT_CLK_SYS	equ	0 << WDT_CLK_S	;   System Clock
WDT_CLK_RTC	equ	1 << WDT_CLK_S	;   RTC Clock
WDT_CLK_RC	equ	2 << WDT_CLK_S	;   Internal RC Oscillator
WDT_PERIOD_S	equ	0		;  WDT Period
WDT_PERIOD_M	equ	3 << WDT_PERIOD_S
WDT_PERIOD_27	equ	0 << WDT_PERIOD_S ; WDT period is 2**27 (system) clock cycles
WDT_PERIOD_25	equ	1 << WDT_PERIOD_S ; WDT period is 2**25 (system) clock cycles
WDT_PERIOD_22	equ	2 << WDT_PERIOD_S ; WDT period is 2**22 (system) clock cycles
WDT_PERIOD_18	equ	3 << WDT_PERIOD_S ; WDT period is 2**18 (system) clock cycles

WDT_RR		port	94h		; Watchdog Timer Reset Register (w)

;----------------------------------------------------------------------------
; General-Purpose Input/Output Ports

		__defgpio "A",96h

IVEC_PA0	label	80h		; Port A 0 Interrupt Vector
IVEC_PA1	label	84h		; Port A 1 Interrupt Vector
IVEC_PA2	label	88h		; Port A 2 Interrupt Vector
IVEC_PA3	label	8ch		; Port A 3 Interrupt Vector
IVEC_PA4	label	90h		; Port A 4 Interrupt Vector
IVEC_PA5	label	94h		; Port A 5 Interrupt Vector
IVEC_PA6	label	98h		; Port A 6 Interrupt Vector
IVEC_PA7	label	9ch		; Port A 7 Interrupt Vector

		__defgpio "B",9ah

IVEC_PB0	label	0a0h		; Port B 0 Interrupt Vector
IVEC_PB1	label	0a4h		; Port B 1 Interrupt Vector
IVEC_PB2	label	0a8h		; Port B 2 Interrupt Vector
IVEC_PB3	label	0ach		; Port B 3 Interrupt Vector
IVEC_PB4	label	0b0h		; Port B 4 Interrupt Vector
IVEC_PB5	label	0b4h		; Port B 5 Interrupt Vector
IVEC_PB6	label	0b8h		; Port B 6 Interrupt Vector
IVEC_PB7	label	0bch		; Port B 7 Interrupt Vector

		__defgpio "C",9eh

IVEC_PC0	label	0c0h		; Port C 0 Interrupt Vector
IVEC_PC1	label	0c4h		; Port C 1 Interrupt Vector
IVEC_PC2	label	0c8h		; Port C 2 Interrupt Vector
IVEC_PC3	label	0cch		; Port C 3 Interrupt Vector
IVEC_PC4	label	0d0h		; Port C 4 Interrupt Vector
IVEC_PC5	label	0d4h		; Port C 5 Interrupt Vector
IVEC_PC6	label	0d8h		; Port C 6 Interrupt Vector
IVEC_PC7	label	0dch		; Port C 7 Interrupt Vector

		__defgpio "D",0a2h

IVEC_PD0	label	0e0h		; Port D 0 Interrupt Vector
IVEC_PD1	label	0e4h		; Port D 1 Interrupt Vector
IVEC_PD2	label	0e8h		; Port D 2 Interrupt Vector
IVEC_PD3	label	0ech		; Port D 3 Interrupt Vector
IVEC_PD4	label	0f0h		; Port D 4 Interrupt Vector
IVEC_PD5	label	0f4h		; Port D 5 Interrupt Vector
IVEC_PD6	label	0f8h		; Port D 6 Interrupt Vector
IVEC_PD7	label	0fch		; Port D 7 Interrupt Vector

PA_ALT0		port	0a6h		; Port A Alternate Register 0 (w)
PB_ALT0		port	0a7h		; Port B Alternate Register 0 (w)
PC_ALT0		port	0ceh		; Port C Alternate Register 0 (w)
PD_ALT0		port	0cfh		; Port D Alternate Register 0 (w)

;----------------------------------------------------------------------------
; Chip Select/Wait State Generator

CS0_LBR		port	0a8h		; Chip Select 0 Lower Bound Register (r/w)
CS0_UBR		port	0a9h		; Chip Select 0 Upper Bound Register (r/w)
CS0_CTL		port	0aah		; Chip Select 0 Control Register (r/w)
CS_WAIT_S	equ	5		;  # of wait states
CS_WAIT_M	equ	7 << CS_WAIT_S
CS_IO		equ	1 << 4		; Match I/O range
CS_EN		equ	1 << 3		; Enable Chip Select
CS0_BMC		equ	0f0h		; Chip Select 0 Bus Mode Control Register (r/w)
BUS_MODE_S	equ	6		;  Bus Mode
BUS_MODE_M	equ	3 << BUS_MODE_S
BUS_MODE_EZ80	equ	0 << BUS_MODE_S ;   eZ80 Mode
BUS_MODE_Z80	equ	1 << BUS_MODE_S	;   Z80 Mode
BUS_MODE_INTEL	equ	2 << BUS_MODE_S	;   Intel Mode
BUS_MODE_MOTO	equ	3 << BUS_MODE_S	;   Motorola Mode
AD_MUX		equ	1 << 5		;  Multiplexed Address/Data
BUS_CYCLE_S	equ	0		;  Length of bus cycle
BUS_CYCLE_M	equ	15 << BUS_CYCLE_S

CS1_LBR		port	0abh		; Chip Select 1 Lower Bound Register (r/w)
CS1_UBR		port	0ach		; Chip Select 1 Upper Bound Register (r/w)
CS1_CTL		port	0adh		; Chip Select 1 Control Register (r/w)
CS1_BMC		equ	0f1h		; Chip Select 1 Bus Mode Control Register (r/w)

CS2_LBR		port	0aeh		; Chip Select 2 Lower Bound Register (r/w)
CS2_UBR		port	0afh		; Chip Select 2 Upper Bound Register (r/w)
CS2_CTL		port	0b0h		; Chip Select 2 Control Register (r/w)
CS2_BMC		equ	0f2h		; Chip Select 2 Bus Mode Control Register (r/w)

CS3_LBR		port	0b1h		; Chip Select 3 Lower Bound Register (r/w)
CS3_UBR		port	0b2h		; Chip Select 3 Upper Bound Register (r/w)
CS3_CTL		port	0b3h		; Chip Select 3 Control Register (r/w)
CS3_BMC		equ	0f3h		; Chip Select 3 Bus Mode Control Register (r/w)

;----------------------------------------------------------------------------
; SPI

SPI_BRG_L	port	0b8h		; SPI Baud Rate Generator Register - Low Byte
SPI_BRG_H	port	0b9h		; SPI Baud Rate Generator Register - High Byte
		__defspi "",0bah

IVEC_SPI	label	7ch		; SPI Interrupt Vector

;----------------------------------------------------------------------------
; Infrared

IR_CTL		port	0bfh		; Infrared Encoder/Decoder Control  (r/w)
MIN_PULSE_S	equ	4		;  Minimum receive pulse width control
MIN_PULSE_M	equ	15 << MIN_PULSE_S
LOOP_BACK	equ	1 << 2		;  Internal LOOP BACK mode
IR_RxEN		equ	1 << 1		;  IR_RxD data is passed to UART0 RxD
IR_EN		equ	1 << 0		;  Endec is disabled

;----------------------------------------------------------------------------
; UART 0

		__defuart "0",0c0h
TCIE		equ	1 << 4		;  UARTx_IER Transmission complete interrupt enable
FSTS		equ	1 << 7		;  UARTx_IIR FIFO is enabled
INTSTS_TC	equ	5 << INTSTS_S	;  UARTx_IIR Transmission Complete
POLARITY	equ	1 << 6		;  UARTx_MCR Invert Polarity of TxD and RxD signals
MDM		equ	1 << 5		;  UARTx_MCR Multidrop Mode enable

IVEC_UART0	label	70h		; UART0 Interrupt Vector

;----------------------------------------------------------------------------
; I2C

		__defi2c "",0c8h

IVEC_I2C	label	78h		; I2C Interrupt Vector

;----------------------------------------------------------------------------
; UART 1

		__defuart "1",0d0h

IVEC_UART1	label	74h		; UART1 Interrupt Vector

;----------------------------------------------------------------------------
; Low-Power Control

CLK_PPD1	port	0dbh		; Clock Peripheral Power-Down Register 1 (r/w)
GPIO_D_OFF	equ	1 << 7		;  Power down system clock to GPIO Port D
GPIO_C_OFF	equ	1 << 6		;  Power down system clock to GPIO Port C
GPIO_B_OFF	equ	1 << 5		;  Power down system clock to GPIO Port B
GPIO_A_OFF	equ	1 << 4		;  Power down system clock to GPIO Port A
SPI_OFF		equ	1 << 3		;  Power down system clock to SPI
I2C_OFF		equ	1 << 2		;  Power down system clock to I2C
UART1_OFF	equ	1 << 1		;  Power down system clock to UART1
UART0_OFF	equ	1 << 0		;  Power down system clock to UART0 and IrDA endec

CLK_PPD2	port	0dch		; Clock Peripheral Power-Down Register 2 (r/w)
PHI_OFF		equ	1 << 7		;  Disable PHI Clock output
VBO_OFF		equ	1 << 6		;  Disable voltage brownout detection circuit
TIMER3_OFF	equ	1 << 3		;  Power down system clock to TIMER3
TIMER2_OFF	equ	1 << 2		;  Power down system clock to TIMER2
TIMER1_OFF	equ	1 << 1		;  Power down system clock to TIMER1
TIMER0_OFF	equ	1 << 0		;  Power down system clock to TIMER0

;----------------------------------------------------------------------------
; Real-Time Clock

		__defrtc
DAY_SAV		equ	1 << 2		;  [RTC_CTL] Daylight Savings Time selected

IVEC_RTC	label	6ch		; RTC Interrupt Vector

;----------------------------------------------------------------------------
; Flash Controller

		__defflash

IVEC_FLASH	label	50h

;----------------------------------------------------------------------------

		restore
		endif			; __reg80f91inc
