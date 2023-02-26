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
	call	scansw
	mvi	a,EC.IS
	jc	error
	lhld	args
	lxi	d,patn
	call	netparse
 if DEBUG
	mov	a,c
	call	hexout
	mvi	a,':'
	SCALL	.SCOUT
	lxi	h,patn
	mvi	b,14
	call	hxd0
	mvi	a,EC.FNR
 else
	mvi	a,EC.IFN
 endif
	jc	error
	lxi	h,patn+3
	mov	a,m
	ora	a
	jnz	gotit
	mvi	b,11
	mvi	a,'?'
fillq:	mov	m,a
	inx	h
	dcr	b
	jnz	fillq
gotit:	lxi	h,patn
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
	lhld	count
	inx	h
	shld	count
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
	jmp	exit
done:	lhld	count
	mov	a,h
	ora	l
	jnz	exit
	call	$TYPTX
	db	'No file',ENL
exit:	xra	a
	SCALL	.EXIT

nwcall:	push	h
	lhld	netdrv
	xthl
	mvi	a,DC.DSF
	ret

; scan for switches, replacing any with blanks
; HL = commandline
scansw:
	mov	a,m
	ora	a
	rz
	cpi	'/'
	jz	ssw0
	inx	h
	jmp	scansw
ssw0:	mvi	m,' '
	inx	h
	mov	a,m
	ani	5fh
	cpi	'B'	; /BRIEF
	jz	sswB
	cpi	'F'	; /FULL
	jz	sswF
	; TODO: /SYSTEM /FLAG:f /NOFLAG:f /... ?
	stc
	ret
ssw1:	; TODO: fill with blanks... until ???
	jmp	scansw

sswB:	sta	brief
	jmp	ssw1
sswF:	sta	full
	jmp	ssw1

func:	db	.SERF

patn:	db	'DD0'
	db	'FILENAME'
	db	'TYP'

brief:	db	0
full:	db	0
count:	dw	0
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
