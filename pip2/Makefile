# Build of PIP.ABS for HDOS 2.0

export HDOSDrive_dk1=${PWD}/../acm

ASM = asm

all: pip.abs

clobber:
	rm -f *.abs *.lst

%.abs: %.asm
	vhdos $(ASM) dk0:$*,dk0:$*=dk0:$*,sy2:,dk1:/err
