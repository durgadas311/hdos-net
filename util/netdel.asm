; HDOS-NET DELETE networked file
;
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
	; check for afn and confirm delete
	lhld	args
	lxi	d,patn
	call	netparse
	mvi	a,EC.IFN
	jc	error
	lda	patn+3
	ora	a
	mvi	a,EC.FNR
	jz	error
	mov	a,c
	ora	a
	jnz	confirm
doit:	lxi	h,patn
	mvi	c,.DELET
	call	nwcall
	jnc	done
error:	push	psw
	call	$TYPTX
	db	NL,'Error:',' '+200Q
	pop	psw
	mvi	h,NL
	SCALL	.ERROR
	;jmp	done
done:	xra	a
	SCALL	.EXIT

confirm:
	call	$TYPTX
	db	'Delet','e'+200Q
	lhld	args	; starts with space
conf0:	mov	a,m
	inx	h
	ora	a
	jz	conf1
	SCALL	.SCOUT
	jmp	conf0
conf1:	call	$TYPTX
	db	' ?',' '+200Q
conf2:	SCALL	.SCIN
	jc	conf2
	ani	5fh
	push	psw
	; flush anything else
conf3:	SCALL	.SCIN
	jnc	conf3
	pop	psw
	cpi	'Y'
	jz	doit
	call	$TYPTX
	db	'Aborted',ENL
	jmp	done

nwcall:	push	h
	lhld	netdrv
	xthl
	mvi	a,DC.DSF
	ret

args:	dw	0
netdrv:	dw	0
patn:	db	'DD0'
	db	'FILENAME'
	db	'TYP'

	ds	64
stack:	ds	0

	end
