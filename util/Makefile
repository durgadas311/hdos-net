export HDOSDrive_dk0 = $(PWD)

all: sum.abs

%.rel: %.asm
	vhdos m80 =dk0:$*.asm

%.abs: %.rel
	vhdos l80 dk0:$*,dk0:$*/n/e

ship: ../bin/sum.abs
	@cd ../bin; sum *.abs >../checksums

../bin/%: %
	cp $< $@
