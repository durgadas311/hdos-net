; HDOS-NET network status display
	aseg
	include	ascii.acm
	include	hosdef.acm
	include	hrom.acm
	include	dddef.acm
	include	devdef.acm
	include	cpnet.acm

	extrn	netdev
	cseg
start:
	call	$TYPTX
	db	NL,'HDOS/NET Status'
	db	NL,'==============='
	db	ENL
	; TODO: ensure device driver loaded...
	call	netcfg
	shld	cfgtbl
	mov	c,m	; network status
	inx	h
	mov	b,m	; node ID
	inx	h
	call	$TYPTX
	db	'Requester ID =',' '+200Q
	mov	a,b
	call	hexout
	call	$TYPTX
	db	NL,'Network Status Byte =',' '+200Q
	mov	a,c
	call	hexout
	call	$TYPTX
	db	NL,'Device status:'
	db	ENL
	mvi	b,8
	mvi	c,0
loop:	mov	a,m
	inr	a
	jz	skip
	inr	c
	call	$TYPTX
	db	'  Drive N','W'+200Q	; hard-coded dev name, again
	mvi	a,8
	sub	b
	adi	'0'
	SCALL	.SCOUT
	mvi	a,':'
	SCALL	.SCOUT
	call	$TYPTX
	db	' = Drive',' '+200Q
	mov	e,m
	inx	h
	mov	a,m
	ani	5fh
	SCALL	.SCOUT
	inx	h
	mov	a,m
	ani	5fh
	SCALL	.SCOUT
	inx	h
	mov	a,m
	SCALL	.SCOUT
	mvi	a,':'
	SCALL	.SCOUT
	call	$TYPTX
	db	' on Network Server ID =',' '+200Q
	mov	a,e
	call	hexout
	mvi	a,NL
	SCALL	.SCOUT
	jmp	loop0
skip:	inx	h
	inx	h
	inx	h
loop0:	inx	h
	dcr	b
	jnz	loop
	mov	a,c
	ora	a
	jnz	done
	call	$TYPTX
	db	'  None Mapped',ENL
	jmp	done

netcfg:	lxi	h,NWDVD	; must hard-code this, else require parameter.
	call	netdev
	mvi	c,.NTCFG
	mvi	a,DC.DSF
	pchl

done:	xra	a
	SCALL	.EXIT

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

cfgtbl:	dw	0

	end
