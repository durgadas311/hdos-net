/*
 * Constants for H8xSPI adapter board
 */
#asm
WZSCS	equ	00000001b	; H8xSPI /CS for WIZnet
NVSCS	equ	00000010b	; H8xSPI /CS for NVRAM 25LC512

spi	equ	40h		; H8xSPI base port
spi$ctl	equ	spi+1
spi$rd	equ	spi+0
spi$wr	equ	spi+0
#endasm
