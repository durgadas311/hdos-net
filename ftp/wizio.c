/*
 * Low-level I/O on WIZnet chips (W5500)
 */
#asm
WZSCS	equ	00000001b	; H8xSPI /CS for WIZnet
spi	equ	40h		; H8xSPI base port
spi$ctl	equ	spi+1
spi$rd	equ	spi+0
spi$wr	equ	spi+0
#endasm

#include "w5500.h"

/*
 * Read byte from register 'bsb','off'.
 */
int wzget1(bsb, off)
char bsb;
char off;
{
#asm
	lxi	h,2
	dad	sp
	mvi	a,WZSCS
	out	spi$ctl
	xra	a
	out	spi$wr	; off < 0x0100
	mov	a,m	; off
	out	spi$wr	; off
	inx	h
	inx	h
	mov	a,m	; bsb
	add	a
	add	a
	add	a	; read op, bulk xfer
	out	spi$wr	; cmd
	in	spi$rd	; dummy
	in	spi$rd
	mov	l,a
	xra	a
	mov	h,a
	out	spi$ctl
#endasm
}

/*
 * Read word from register 'bsb','off', big-endian.
 */
int wzget2(bsb, off)
char bsb;
char off;
{
#asm
	lxi	h,2
	dad	sp
	mvi	a,WZSCS
	out	spi$ctl
	xra	a
	out	spi$wr	; off < 0x0100
	mov	a,m	; off
	out	spi$wr	; off
	inx	h
	inx	h
	mov	a,m	; bsb
	add	a
	add	a
	add	a	; read op, bulk xfer
	out	spi$wr	; cmd
	in	spi$rd	; dummy
	in	spi$rd
	mov	h,a
	in	spi$rd
	mov	l,a
	xra	a
	out	spi$ctl
#endasm
}

/*
 * Read 'len' bytes into 'buf' from registers/fifo 'bsb','off'.
 */
wzrd(bsb, off, buf, len)
char bsb;
int off;
char *buf;
int len;
{
#asm
	lxi	h,6
	dad	sp
	mov	e,m
	inx	h
	mov	d,m	; off
	inx	h
	mvi	a,WZSCS
	out	spi$ctl
	mov	a,d
	out	spi$wr	; off(hi)
	mov	a,e
	out	spi$wr	; off(lo)
	mov	a,m	; bsb
	add	a
	add	a
	add	a	; read op, bulk xfer
	out	spi$wr	; cmd
	in	spi$rd	; dummy
	lxi	h,2
	dad	sp
	mov	c,m
	inx	h
	mov	b,m	; len
	inx	h
	mov	e,m
	inx	h
	mov	d,m	; buf
wzrd0:	in	spi$rd
	stax	d
	inx	d
	dcx	b
	mov	a,b
	ora	c
	jnz	wzrd0
	xra	a
	out	spi$ctl
#endasm
}

/*
 * Write byte 'val' to register 'bsb','off'.
 */
wzput1(bsb, off, val)
char bsb;
char off;
char val;
{
#asm
	lxi	h,2
	dad	sp
	mov	c,m	; val
	inx	h
	inx	h
	mvi	a,WZSCS
	out	spi$ctl
	xra	a
	out	spi$wr	; off < 0x0100
	mov	a,m	; off
	out	spi$wr	; off
	inx	h
	inx	h
	mov	a,m	; bsb
	add	a
	inr	a
	add	a
	add	a	; write op, bulk xfer
	out	spi$wr	; cmd
	mov	a,c	; val
	out	spi$wr
	xra	a
	out	spi$ctl
#endasm
}

/*
 * Write word 'val' to register 'bsb','off'.
 * Value is written big-endian.
 */
wzput2(bsb, off, val)
char bsb;
char off;
int val;
{
#asm
	lxi	h,2
	dad	sp
	mov	c,m	; val LE
	inx	h
	mov	b,m	; val
	inx	h
	mvi	a,WZSCS
	out	spi$ctl
	xra	a
	out	spi$wr	; off < 0x0100
	mov	a,m	; off
	out	spi$wr	; off
	inx	h
	inx	h
	mov	a,m	; bsb
	add	a
	inr	a
	add	a
	add	a	; write op, bulk xfer
	out	spi$wr	; cmd
	mov	a,b	; val BE
	out	spi$wr
	mov	a,c	; val
	out	spi$wr
	xra	a
	out	spi$ctl
#endasm
}

/*
 * Write 'len' bytes from 'buf' to registers/fifo 'bsb','off'.
 */
wzwr(bsb, off, buf, len)
char bsb;
int off;
char *buf;
int len;
{
#asm
	lxi	h,6
	dad	sp
	mov	e,m	; off LE
	inx	h
	mov	d,m	; off
	inx	h
	mvi	a,WZSCS
	out	spi$ctl
	mov	a,d
	out	spi$wr	; off(hi) BE
	mov	a,e
	out	spi$wr	; off(lo)
	mov	a,m	; bsb
	add	a
	inr	a
	add	a
	add	a	; write op, bulk xfer
	out	spi$wr	; cmd
	lxi	h,2
	dad	sp
	mov	c,m
	inx	h
	mov	b,m	; len
	inx	h
	mov	e,m
	inx	h
	mov	d,m	; buf
wzwr0:	ldax	d
	out	spi$wr
	inx	d
	dcx	b
	mov	a,b
	ora	c
	jnz	wzwr0
	xra	a
	out	spi$ctl
#endasm
}

/*
 * Send 'cmd' to socket CR register, wait for completion.
 * Returns: contents of socket SR after command.
 */
int wzcmd(bsb, cmd)
char bsb;
char cmd;
{
	wzput1(bsb, SN_CR, cmd);
	while (wzget1(bsb, SN_CR) != 0);
	return wzget1(bsb, SN_SR);
}

/*
 * Read socket IR register.
 * if any of 'bits' are set then reset those bits.
 * Returns: original contents of socket IR.
 */
int wzsts(bsb, bits)
char bsb;
char bits;
{
	char ir;

	ir = wzget1(bsb, SN_IR);
	if ((ir & bits) != 0) {
		wzput1(bsb, SN_IR, bits);
	}
	return ir;
}

/*
 * Wait for socket IR register to have at least one 'bits' set.
 * See wzsts() for side-effects.
 * Returns: original contents of socket IR, or -1 on timeout.
 */
int wzist(bsb, bits)
char bsb;
char bits;
{
	int to;
	char ir;

	to = 32000; /* TODO: tune this for C vs ASM */
	while (to != 0) {
		ir = wzsts(bsb, bits);
		if ((ir & bits) != 0) break;
		--to;
	}
	if (to == 0) return -1;
	return ir;
}
