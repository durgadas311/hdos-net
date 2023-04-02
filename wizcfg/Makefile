# A wizcfg utility for HDOS using H8xSPI/WIZnet.
SY0 = $(shell VHDOSShow=sy0 vhdos)
DK0 = $(PWD)
DK1 = $(PWD)/../lib

export HDOSDrive_dk0 = $(DK0)
export HDOSDrive_dk1 = $(DK1)

all: wizcfg.abs

%.mac: %.c
	vhdos c -x -m1 dk0:$<

$(SY0)/printf.mac: $(SY0)/printf.c
	vhdos c -m1 sy0:printf

$(SY0)/printf.rel: $(SY0)/printf.mac
	vhdos m80 =sy0:printf

%.rel: %.mac
	vhdos m80 =dk0:$*

$(DK1)/%.rel: $(DK1)/%.c
	cd $(DK1); make

wizcfg.abs: wizcfg.rel $(DK1)/wizio.rel $(DK1)/nvram.rel $(SY0)/printf.rel
	vhdos l80 dk0:wizcfg,dk1:wizio,dk1:nvram,sy0:printf,sy0:clibrary,dk0:wizcfg/n/e
