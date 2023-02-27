; HDOS-NET Utility routines
;
	aseg
	include	ascii.acm
	include	hosdef.acm
	include	devdef.acm
	include	hrom.acm
	include	cpnet.acm

	public	netdev
	cseg

; HL = device name (e.g. H='W' L='N' for "NW")
; Returns: HL = DEV.JMP
; Aborts if no network
netdev:	shld	dev
	lxi	h,dev
	SCALL	.LOADD	; HL=AIO.DTA if scucess
	jc	nonet
	inx	h
	inx	h	; DEV.RES
	mov	a,m
	inx	h	; DEV.JMP
	ani	DR.IM
	jz	nonet
	ret

nonet:	call	$TYPTX
	db	NL,'HDOS/NET has not been loaded','.'+200Q
done:	xra	a
	SCALL	.EXIT

dev:	db	'xx:',0
