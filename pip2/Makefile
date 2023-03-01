# Build of PIP.ABS for HDOS 2.0

ASM = asm

all: pip.abs

clobber:
	rm -f *.abs *.lst

%.abs: %.asm
	vhdos $(ASM) dk0:$*,dk0:$*=dk0:$*,sy2:/err
