; HDOS-NET DELETE networked file
;
	aseg
	include	ascii.acm
	include	hosdef.acm
	include	dddef.acm
	include	ecdef.acm
	include	hrom.acm
	include	cpnet.acm

	extrn	netdev
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
	call	parse
	jc	error

	lxi	h,patn
	lxi	d,defblk
loop:
	lxi	b,dirbuf
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

parse:	; TODO...
	xra	a
	ret

func:	db	.SERF

patn:	db	'NW0:????????.???'

defblk:	db	0,0,0
	db	0,0,0

dirbuf:	ds	23

args:	dw	0
netdrv:	dw	0

	ds	64
stack:	ds	0

	end
