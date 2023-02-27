# Utilities for HDOS Network

.PRECIOUS: %.rel

LIBS = netdev.rel netparse.rel

all: netstat.abs netmap.abs # netdel.abs netdir.abs

%.rel: %.asm
	zmac -i -c -8 -s -n -I../inc --od . --oo rel,lst $*.asm
	@test -s $@

%.abs: %.rel $(LIBS)
	ld80 -o $*.abs -O abs $*.rel $(addprefix -l ,$(LIBS))
