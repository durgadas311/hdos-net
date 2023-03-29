# An ftp-like utilitiy for HDOS using H8xSPI/WIZnet.
export HDOSDrive_dk0 = $(PWD)
export HDOSDrive_dk1 = $(PWD)/../lib

all: wizcfg.abs

%.mac: %.c
	vhdos c -m1 dk0:$<

%.rel: %.mac
	vhdos m80 =dk0:$*

wizcfg.abs: wizcfg.rel $(HDOSDrive_dk1)/wizio.rel $(HDOSDrive_dk1)/nvram.rel
	vhdos l80 dk0:wizcfg,dk1:wizio,dk1:nvram,dk0:wizcfg/n/e
