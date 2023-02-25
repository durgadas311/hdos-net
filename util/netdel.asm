; HDOS-NET DELETE networked file
;
	aseg
	include	ascii.acm
	include	hosdef.acm
	include	dddef.acm
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
	lxi	d,defblk
	mvi	c,.DELET
	call	nwcall
	jnc	done
	push	psw
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

defblk:	db	0,0,0
	db	0,0,0

args:	dw	0
netdrv:	dw	0

	ds	64
stack:	ds	0

	end
