; Utility to alter device maps.
;
; Usage: netmap NW<x>:=<dev>:[<nid>]
;        netmap NW<x>:=NONE
; Where:
;	<dev> = remote device specification, two letters followed by a digit.
;	<x>   = local device unit, 0-7.
;	<nid> = server node ID, hex 00-FE.
; TODO: any way to avoid hard-coding "NW"?

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
	lxi	h,0
	dad	sp
	shld	args
	call	netcfg	; drive is loaded, if not already
	shld	cfgtbl
	; parse the commandline
	lhld	args
skipb:	mov	a,m
	cpi	' '
	jnz	begin
	inx	h
	jmp	skipb
begin:
	push	h
	call	devspec
	pop	h
	jc	error
	xchg
	lxi	h,devloc
	lxi	b,3
	call	$MOVE
	xchg
	inx	h	; past ':'
	mov	a,m
	inx	h
	cpi	'='
	jnz	error
	lxi	d,none
	lxi	b,4
	call	strncmp
	jz	nomap
	push	h
	call	devspec
	pop	h
	jc	error
	xchg
	lxi	h,devmap+1
	lxi	b,3
	call	$MOVE
	xchg
	inx	h	; past ':'
	mov	a,m	; possible '['
	inx	h
	ora	a
	jz	nid0
	cpi	'['
	jnz	error
	mov	a,m	; hex digit
	inx	h
	call	hexin
	jc	error
	mov	c,a
	mov	a,m	; hex digit or term
	inx	h
	cpi	']'
	jz	gothex
	ora	a
	jz	gothex
	call	hexin
	jc	error
	mov	b,a
	mov	a,c
	add	a
	add	a
	add	a
	add	a
	ora	b
	mov	c,a
gothex:	mov	a,c
nid0:	sta	devmap+0
	; now validate some things
	; TODO: try not to hard-code this
nomap:	lhld	devloc
	lxi	d,NWDVD
	call	$CDEHL
	jnz	error2
	lda	devloc+2	; unit
	sui	'0'
	add	a
	inr	a
	add	a	; unit * 4 + 2
	lhld	cfgtbl
	call	@DADA
	lxi	d,devmap
	lxi	b,4
	call	$MOVE
	call	$TYPTX
	db	ENL
done:	xra	a
	SCALL	.EXIT

error2:	call	$TYPTX
	db	NL,'Invalid network device name',ENL
	jmp	done

error:	call	$TYPTX
	db	NL,'Syntax error',ENL
	jmp	done

netcfg:	lxi	h,NWDVD	; must hard-code this, else require parameter.
	call	netdev
	mvi	c,.NTCFG
	mvi	a,DC.DSF
	pchl

hexin:	call	toupper
	sui	'0'
	rc
	cpi	10
	cmc
	rnc
	sui	'A'-'9'-1
	rc
	cpi	16
	cmc
	ret

strncmp:
	push	h
	xchg	; input in DE, const in HL
snc1:	ldax	d
	call	toupper
	cmp	m
	jnz	snc0
	inx	h
	inx	d
	dcx	b
	mov	a,b
	ora	c
	jnz	snc1
snc0:	pop	h
	ret	; ZR/NZ based on compare

devspec:
	mov	a,m
	call	isletter
	rc
	inx	h
	mov	a,m
	call	isletter
	rc
	inx	h
	mov	a,m
	cpi	'0'
	rc
	cpi	'7'+1
	cmc
	rc
	inx	h
	mov	a,m
	cpi	':'
	rz
	stc
	ret

isletter:
	ani	5fh
	cpi	'A'
	rc
	cpi	'Z'+1
	cmc
	ret

toupper:
	cpi	'a'
	rc
	cpi	'z'+1
	rnc
	ani	5fh
	ret

none:	db	'NONE'
devloc:	db	0,0,0
devmap:	db	0ffh,0,0,0

cfgtbl:	dw	0
args:	dw	0

	end
