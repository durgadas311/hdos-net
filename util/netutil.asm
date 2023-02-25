; HDOS-NET Utility routines
;
	aseg
	include	ascii.acm
	include	hosdef.acm
	include	devdef.acm
	include	hrom.acm

NWDVD	equ	574eh	; 'NW' not working in zmac

	public	netdev
	cseg

; Returns: HL = DEV.JMP
; Aborts if no network
netdev:	SCALL	.VERS
	cpi	30H
	jnc	hdos3
	; TODO: get device in HDOS 2.0...
	jmp	nonet
hdos3:	lxi	d,NWDVD
	SCALL	.GDA
	jc	nonet
	inx	b
	inx	b	; DEV.RES
	ldax	b
	ani	DR.IM
	jz	nonet
	ret

nonet:	call	$TYPTX
	db	NL,'HDOS/NET has not been loaded','.'+200Q
done:	xra	a
	SCALL	.EXIT
