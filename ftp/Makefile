# An ftp-like utilitiy for HDOS using H8xSPI/WIZnet.
SY0 = $(shell VHDOSShow=sy0 vhdos)
DK0 = $(PWD)
DK1 = $(PWD)/../lib

export HDOSDrive_dk0 = $(DK0)
export HDOSDrive_dk1 = $(DK1)

all: ftp.abs

ftp.abs: ftp.rel cnetfile.rel wizcpnet.rel # TODO...

%.mac: %.c
	vhdos c -x -m1 dk0:$<

%.rel: %.mac
	vhdos m80 =dk0:$*

$(DK1)/%.rel: $(DK1)/%.c
	cd $(DK1); make

ftp.abs: ftp.rel cnetfile.rel wizcpnet.rel $(DK1)/wizio.rel $(DK1)/nvram.rel $(SY0)/printf.rel
	vhdos l80 dk0:ftp,dk0:cnetfile,dk0:wizcpnet,dk1:wizio,dk1:nvram,sy0:printf,sy0:stdlib/s,sy0:clibrary,dk0:ftp/n/e
