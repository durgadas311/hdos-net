; Unix(Linux) "sum" algorithm on CP/M
; Accepts only one, unambiguous, filename.
; Does not handle text EOF ^Z or byte-count.

	.8080

scall	macro	??p
	db	255,??p
	endm

exit	equ	0
scout	equ	2
print	equ	3
read	equ	4
openr	equ	34
close	equ	38
error	equ	57q

ec$eof	equ	1

sysv$sum equ	0	; TODO: commandline option

CR	equ	13
LF	equ	10
EOF	equ	26

BUFLEN	equ	256

	cseg
parms:	db	CR,LF,'Requires filenam','e'+200q
opnerr:	db	CR,LF,'File not foun','d'+200q

 if sysv$sum
s:	db	0,0,0,0	; 32-bit integer, LE
chksum:	db	0,0,0,0	; final result, LE
 else	; BSD sum
chksum:	dw	0
 endif
totbyt:	db	0,0,0,0	; total bytes, LE

zzz:	db	'SY0',0,0,0

; add A to 32-bit sum in HL
add32a:
	add	m
	mov	m,a
	rnc
	inx	h
	mvi	a,0
	adc	m
	mov	m,a
	rnc
	inx	h
	mvi	a,0
	adc	m
	mov	m,a
	rnc
	inx	h
	mvi	a,0
	adc	m
	mov	m,a
	ret	; CY indicates overflow

; add DE to 32-bit sum in HL
add32de:
	mov	a,e
	add	m
	mov	m,a
	inx	h
	mov	a,d
	adc	m
	mov	m,a
	rnc
	inx	h
	mvi	a,0
	adc	m
	mov	m,a
	rnc
	inx	h
	mvi	a,0
	adc	m
	mov	m,a
	ret	; CY indicates overflow

; TODO: support things like A:=B:*.asm[u]
start:
	lxi	h,0
	dad	sp	; cmdbuf
	lxi	sp,stack
	mvi	a,1
	lxi	d,zzz
	scall	openr
	jc	err	; No such file
loop:
	mvi	a,1
	lxi	b,BUFLEN
	lxi	d,buf
	scall	read
	jnc	loop0
	cpi	ec$eof
	jz	done	; TODO: differentiate?
	jmp	err
loop0:
	lxi	h,buf
	mvi	b,0	; a.k.a. 256
 if sysv$sum
sumloop:
	mov	a,m
	inx	h
	push	h
	push	b
	lxi	h,s
	call	add32a	; s += buf[i];
	pop	b
	pop	h
	dcr	b
	jnz	sumloop
 else	; BSD sum
sumloop:
	xchg
	lhld	chksum
	xchg
	; checksum = (checksum >> 1) + ((checksum & 1) << 15);
	call	rar$de
	mov	a,e
	add	m
	mov	e,a
	mvi	a,0
	adc	d
	mov	d,a
	xchg
	shld	chksum	; checksum += ch; (mod 16-bit)
	xchg
	inx	h
	dcr	b
	jnz	sumloop
 endif
	lxi	h,totbyt
	lxi	d,BUFLEN
	call	add32de	; total_bytes += bytes_read;
	jmp	loop
done:
	mvi	a,1
	scall	close
 if sysv$sum
	lhld	s+2	; HL = ((s & 0xffffffff) >> 16)
	xchg
	lhld	s	; DE = (s & 0xffff)
	xchg
	dad	d	; (s & 0xffff) + ((s & 0xffffffff) >> 16) [CY]
	shld	r	; r = (s & 0xffff) + ((s & 0xffffffff) >> 16);
	mvi	a,0
	adc	a
	sta	r+2	; with CY
	lhld	r
	xchg
	lhld	r+2
	xchg
	dad	d	; checksum = (r & 0xffff) + (r >> 16);
 else	; BSD sum
	lhld	chksum
 endif
	mvi	c,1	; zero-fill
	call	dec16
	call	space
	; ... print size, w/o "human_readable" junk.
	; round up to next 1K-byte block...
	lxi	h,totbyt
	lxi	d,1023	; TODO: SYSV sum uses 512...
	call	add32de
	; TODO: handle more than 16M?
	lhld	totbyt+1	; divide by 1024... >> (2+8)
	call	shr$hl
	call	shr$hl
	mvi	c,0	; blank-fill
	call	dec16
	call	crlf
	xra	a
	jmp	exet

err:	mvi	h,LF
	scall	error
	mvi	a,1
exet:	scall	exit
	jmp	$

; print HL in decimal, c=zero suppression/print
dec16:
	; remainder in HL
	lxi	d,10000
	call	div16
	lxi	d,1000
	call	div16
	lxi	d,100
	call	div16
	lxi	d,10
	call	div16
	mov	a,l
	adi	'0'
	scall	scout
	ret

; divide HL/DE, remainder in HL
; assert(HL/DE < 10) (one digit)
div16:	mvi	b,0
dv0:	ora	a
	call	dsbc$d
	inr	b
	jnc	dv0
	dad	d
	dcr	b
	jnz	dv1
	inr	c
	dcr	c
	jnz	dv1
	mvi	a,' '
	jmp	dv2
dv1:	mvi	c,1
	mvi	a,'0'
	add	b
dv2:	push	h	; save remainder
	push	b
	scall	scout
	pop	b
	pop	h
	ret

dsbc$d:
	mov	a,l
	sbb	e
	mov	l,a
	mov	a,h
	sbb	d
	mov	h,a
	ret

shr$hl:
	ora	a	; clear CY
	mov	a,h
	rar
	mov	h,a
	mov	a,l
	rar
	mov	l,a
	ret

rar$de:
	mov	a,e
	rar		; bit 0 into CY
	mov	a,d
	rar
	mov	d,a
	mov	a,e
	rar
	mov	e,a
	ret

space:	mvi	a,' '
chrout:	scall	scout
	ret

crlf:	mvi	a,CR
	scall	scout
	mvi	a,LF
	scall	scout
	ret

; end of program code
stack	equ	$+128

buf	equ	stack

	end	start
