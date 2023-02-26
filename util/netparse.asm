; HDOS-NET Utility routines
;
	aseg
	include	ascii.acm
	include	hosdef.acm
	include	devdef.acm
	include	hrom.acm
	include	cpnet.acm

	public	netparse
	cseg

; HL = commandline (input string)
; DE = destination file block
; [DD[U]:]FILENAME.TYP - allowing for wildcards in FILENAME.TYP
; Default DDU: is "NW0:". Converts unit to binary.
; Returns: CY on error, A=0 for unambiguous file name
netparse:
;        private void setupFile(String name, byte[] nam) {
;                int t = 0;
;                int x = 0;
;                char c;
;                char b = '\0'; 
;                while (t < name.length() && name.charAt(t) != '.' && x < 8) {
;                        c = name.charAt(t++);
;                        if (c == '*') {
;                                b = '?';
;                                break;
;                        }
;                        nam[x++] = (byte)Character.toUpperCase(c);
;                }
;                while (x < 8) {
;                        nam[x++] = (byte)b;
;                }
;                while (t < name.length() && name.charAt(t) != '.') {
;                        ++t;
;                }
;                if (t < name.length() && name.charAt(t) == '.') {
;                        ++t;
;                }
;                b = '\0'; 
;                while (t < name.length() && name.charAt(t) != '.' && x < 11) { 
;                        c = name.charAt(t++);
;                        if (c == '*') {
;                                b = '?';
;                                break;
;                        }
;                        nam[x++] = (byte)Character.toUpperCase(c);
;                }
;                while (x < 11) { 
;                        nam[x++] = (byte)b;
;                }
;        }
skipb:	mov	a,m
	cpi	' '
	jnz	begin
	inx	h
	jmp	skipb
begin:	call	isdelim
	stc
	rz
	push	h
	push	d
	mov	a,m
	inx	h
	call	isletter
	jc	nodev
	stax	d
	inx	d
	mov	a,m
	inx	h
	call	isletter
	jc	nodev
	stax	d
	inx	d
	mov	a,m	; octal digit or ':'
	inx	h
	cpi	':'
	jz	defunit
	sui	'0'
	jc	nodev
	cpi	8
	jnc	nodev
	stax	d
	inx	d
	mov	a,m	; must be ':'
	inx	h
	cpi	':'
	jnz	nodev
cont1:
	pop	b	; discard ptrs and continue
	pop	b
cont2:
	mvi	c,0	; afn flag
	mvi	b,8
name0:	mov	a,m
	inx	h
	cpi	'.'	; error if first...
	jz	break1
	cpi	'*'
	jz	doast
	cpi	'?'	; must check to avoid error
	jnz	name1
	inr	c
	jmp	name3
name1:	call	isdelim
	jz	done1
	call	isalnum
	rc
name3:	stax	d
	inx	d
	dcr	b
	jnz	name0
	; must be '.' or end
name4:	mov	a,m
	inx	h
	cpi	'.'
	jz	break2
	call	isdelim
	rc	; error
	jmp	done2
break1:	xra	a	; fill rest with 0
break0:	call	fill
break2:	mvi	b,3
ext0:	mov	a,m
	inx	h
	cpi	'*'
	jz	doast2
	cpi	'?'	; must check to avoid error
	jnz	ext1
	inr	c
	jmp	ext3
ext1:	call	isdelim
	jz	break4
	call	isalnum
	rc	; error
ext3:	stax	d
	inx	d
	dcr	b
	jnz	ext0
	; must be end
ext4:	mov	a,m
	inx	h
	call	isdelim
	rc
break4:	xra	a
break3:	call	fill
done:	mov	a,c
	ora	a
	ret

; end in name, fill and fill ext
done1:	xra	a
	call	fill
done2:	mvi	b,3
	call	fill
	jmp	done

; B might be 0 already
fill:	inr	b
fill0:	dcr	b
	rz
	stax	d
	inx	d
	jmp	fill0

doast:	mvi	a,'?'
	inr	c
	call	fill
	mov	a,m
	inx	h
	; must be '.' or end
	cpi	'.'
	jz	break2
	call	isdelim
	mvi	b,3
	jz	break4
	stc
	ret

doast2:	mvi	a,'?'
	inr	c
	jmp	break3

defunit:	; already saw ':'
	xra	a
	stax	d
	inx	d
	jmp	cont1

; RESET, store default device/unit
nodev:	pop	h	; dest ptr
	lxi	b,NWDVD
	mov	m,c
	inx	h
	mov	m,b
	inx	h
	mvi	m,0
	inx	h
	xchg
	pop	h	; src ptr
	jmp	cont2

; CY if not A-Z,0-9
isalnum:	; also toupper
	cpi	'0'
	rc
	cpi	'9'+1
	cmc
	rnc
; CY if not A-Z
isletter:
	ani	5fh
	cpi	'A'
	rc
	cpi	'Z'+1
	cmc
	ret

; TODO: what about '/' for options?
; CY if not a valid delimiter, ZR if delimiter
isdelim:
	ora	a
	rz
	cpi	' '
	rz
	cpi	','
	rz
	stc
	ret

	end
