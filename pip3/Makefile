# Build of PIP.ABS for HDOS 3.0
# Requires HDOS 3.0 assembler and compatible ACMs.

ASM = asm

# TODO: complete list of dependent ACMs not practical
DEPS = pip.acm pipcmds.acm pipcopy.acm piplist.acm pipsubs.acm pipswi.acm

all: pip.abs

clobber:
	rm -f *.abs *.lst

%.abs: %.asm $(DEPS)
	vhdos $(ASM) dk0:$*,dk0:$*=dk0:$*,sy2:,sy3:/err

