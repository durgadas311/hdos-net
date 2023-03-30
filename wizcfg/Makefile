# A wizcfg utility for HDOS using H8xSPI/WIZnet.
VHDOS_SY0 = $(shell VHDOSShow=sy0 vhdos)
export HDOSDrive_dk0 = $(PWD)
export HDOSDrive_dk1 = $(PWD)/../lib

all: wizcfg.abs

%.mac: %.c
	vhdos c -x -m1 dk0:$<

$(VHDOS_SY0)/printf.mac: $(VHDOS_SY0)/printf.c
	vhdos c -m1 sy0:printf

$(VHDOS_SY0)/printf.rel: $(VHDOS_SY0)/printf.mac
	vhdos m80 =sy0:printf

%.rel: %.mac
	vhdos m80 =dk0:$*

$(HDOSDrive_dk1)/%.rel: $(HDOSDrive_dk1)/%.c
	cd $(HDOSDrive_dk1); make

wizcfg.abs: wizcfg.rel $(HDOSDrive_dk1)/wizio.rel $(HDOSDrive_dk1)/nvram.rel $(VHDOS_SY0)/printf.rel
	vhdos l80 dk0:wizcfg,dk1:wizio,dk1:nvram,sy0:printf,sy0:clibrary,dk0:wizcfg/n/e
