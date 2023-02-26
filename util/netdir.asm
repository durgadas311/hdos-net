; HDOS-NET DELETE networked file
;
DEBUG	equ	0

	aseg
	include	ascii.acm
	include	hosdef.acm
	include	dddef.acm
	include	ecdef.acm
	include	hrom.acm
	include	cpnet.acm

	extrn	netdev,netparse
	cseg
start:
	lxi	h,0
	dad	sp
	shld	args
	; TODO: check for empty...
	lxi	sp,stack
	call	netdev
	shld	netdrv
	lhld	args
	lxi	d,patn
	call	netparse
 if DEBUG
	push	psw
	mov	a,c
	call	hexout
	mvi	a,':'
	SCALL	.SCOUT
	lxi	h,patn
	mvi	b,14
	call	hxd0
	pop	psw
	mvi	a,EC.FNR
 else
	mvi	a,EC.IFN
 endif
	jc	error

	lxi	h,patn
loop:
	lxi	d,dirbuf
	lda	func
	mov	c,a
	call	nwcall
	jnc	ok
	cpi	EC.EOF
	jz	done
	jmp	error
ok:	mvi	a,.SERN
	sta	func
	lxi	h,dirbuf
	mvi	b,11
nmloop:	mov	a,m
	ora	a
	jnz	nm0
	mvi	a,' '
nm0:	SCALL	.SCOUT
	mov	a,b
	cpi	4
	jnz	nm1
	mvi	a,'.'
	SCALL	.SCOUT
nm1:	inx	h
	dcr	b
	jnz	nmloop
	; TODO: more data
	mvi	a,NL
	SCALL	.SCOUT
	jmp	loop

error:	push	psw
	call	$TYPTX
	db	NL,'Error:',' '+200Q
	pop	psw
	mvi	h,NL
	SCALL	.ERROR
	;jmp	done
done:	xra	a
	SCALL	.EXIT

nwcall:	push	h
	lhld	netdrv
	xthl
	mvi	a,DC.DSF
	ret

func:	db	.SERF

patn:	db	'DD0'
	db	'FILENAME'
	db	'TYP'

dirbuf:	ds	23

args:	dw	0
netdrv:	dw	0

 if DEBUG
hxd0:	mvi	a,' '
	call	conout
hxd:	mov	a,m
	call	hexout
	inx	h
	dcr	b
	jnz	hxd0
	ret

hexout:	push	psw
	rlc
	rlc
	rlc
	rlc
	call	hexdig
	pop	psw
hexdig:	ani	0fh
	adi	90h
	daa
	aci	40h
	daa
conout:	SCALL	.SCOUT
	ret
 endif

	ds	64
stack:	ds	0

	end
