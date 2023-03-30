/*
 * NVRAM as in SPI device like 25LC512
 */

#include "h8xspi.h"

#asm
NVRD	equ	3	; chip READ command
NVWR	equ	2	; chip WRITE command
RDSR	equ	5	; chip READ STATUS REG command
WREN	equ	6	; chip WRITE ENABLE command

WIP	equ	1	; chip WRITE IN PROGRESS status bit

NVSCS	equ	2	; chip select bit
#endasm

#asm
; BC=len, DE=buf
cks32:	dw	0,0
ccks32:	lxi	h,0
	shld	cks32
	shld	cks32+2
cks0:	ldax	d
	inx	d
	lxi	h,cks32
	add	m
	mov	m,a
	inx	h
	jnc	cks1
	inr	m
	inx	h
	jnz	cks1
	inr	m
	inx	h
	jnz	cks1
	inr	m
cks1:	dcx	b
	mov	a,b
	ora	c
	jnz	cks0
	ret
#endasm

/*
 * Verify checksum in 512B block.
 * Returns 0 if OK.
 */
int vcksum(buf)
char *buf;
{
#asm
	lxi	h,2
	dad	sp
	mov	e,m
	inx	h
	mov	d,m	; buf[512]
	lxi	b,508
	call	ccks32	; returns DE=buf+508
	lxi	h,cks32
	mvi	c,0
	mvi	b,4
vck0:	ldax	d
	ora	c
	mov	c,a
	ldax	d
	cmp	m
	jnz	vck1
	inx	h
	inx	d
	dcr	b
	jnz	vck0
	mov	a,c	; was it 00000000?
	ora	a
	jz	vck1
	lxi	h,0	; checksum OK
	ret
vck1:	lxi	h,1	; wrong checksum
#endasm
}

/*
 * Compute and set checksum in 512B block
 */
scksum(buf)
char *buf;
{
#asm
	lxi	h,2
	dad	sp
	mov	e,m
	inx	h
	mov	d,m	; buf[512]
	lxi	b,508
	call	ccks32	; returns DE=buf+508
	lxi	h,cks32
	mvi	b,4
sck0:	mov	a,m
	stax	d
	inx	h
	inx	d
	dcr	b
	jnz	sck0
#endasm
}

nvget(buf, adr, len)
char *buf;
int adr;
int len;
{
 /* (char *buf, int adr, int len) */
#asm
	lxi	h,2
	dad	sp
	mov	c,m
	inx	h
	mov	b,m	; len
	inx	h
	mov	e,m
	inx	h
	mov	d,m	; adr (NVRAM addr)
	inx	h
	mvi	a,NVSCS
	out	spi$ctl
	mvi	a,NVRD
	out	spi$wr
	mov	a,d
	out	spi$wr
	mov	a,e
	out	spi$wr
	in	spi$rd	; dummy
	mov	e,m
	inx	h
	mov	d,m	; buffer
nvg0:	in	spi$rd
	stax	d
	inx	d
	dcx	b
	mov	a,b
	ora	c
	jnz	nvg0
	xra	a
	out	spi$ctl
#endasm
	return 0;
}

static wrpage(buf, adr)
char *buf;
int adr;
{
#asm
; TODO: don't wait for FRAM, only SEEPROM (25LC512)
wrp0:	mvi	a,NVSCS
	out	spi$ctl
	mvi	a,RDSR
	out	spi$wr
	in	spi$rd	; dummy
	in	spi$rd	; status register
	push	psw
	xra	a
	out	spi$ctl
	pop	psw
	ani	WIP
	jnz	wrp0
	mvi	a,NVSCS
	out	spi$ctl
	mvi	a,WREN
	out	spi$wr
	xra	a
	out	spi$ctl
	; now ready to write 128 bytes
	lxi	h,2
	dad	sp
	mov	c,m
	inx	h
	mov	b,m	; adr
	inx	h
	mov	e,m
	inx	h
	mov	d,m	; buf
	mvi	a,NVSCS
	out	spi$ctl
	mvi	a,NVWR
	out	spi$wr
	mov	a,b
	out	spi$wr
	mov	a,c
	out	spi$wr
	mvi	b,128
wrp1:	ldax	d
	out	spi$wr
	inx	d
	dcr	b
	jnz	wrp1
	xra	a
	out	spi$ctl
#endasm
}

nvput(buf, adr, len)
char *buf;
int adr;
int len;
{
	/* TODO: force 128B alignment/length */
	while (len > 0) {
		wrpage(buf, adr);
		buf += 128;
		adr += 128;
		len -= 128;
	}
	return 0;
}
