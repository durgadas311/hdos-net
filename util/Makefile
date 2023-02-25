# Utilities for HDOS Network

.PRECIOUS: %.rel

all: netstat.abs netdel.abs netdir.abs

%.rel: %.asm
	zmac -i -c -8 -s -n -I../inc --od . --oo rel,lst $*.asm
	@test -s $@

%.abs: %.rel netutil.rel
	ld80 -o $*.abs -O abs $*.rel netutil.rel
