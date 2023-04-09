# An ftp-like utilitiy for HDOS using H8xSPI/WIZnet.
SY0 = $(shell VHDOSShow=sy0 vhdos)
DK0 = $(PWD)
DK1 = $(dir $(PWD))lib

export HDOSDrive_dk0 = $(DK0)
export HDOSDrive_dk1 = $(DK1)

all: ftp.abs ftppip.abs

%.mac: %.c
	vhdos c -x -m1 dk0:$<

%.rel: %.mac
	vhdos m80 =dk0:$*

$(DK1)/%.rel: __FRC__
	cd $(DK1); make $@

ftp.abs: ftp.rel libftp.rel $(DK1)/libwznet.rel $(SY0)/printf.rel
	vhdos l80 dk0:ftp,dk0:libftp,dk1:libwznet,sy0:printf,sy0:stdlib/s,sy0:clibrary,dk0:ftp/n/e

ftppip.abs: ftppip.rel libftp.rel $(DK1)/libwznet.rel $(SY0)/printf.rel
	vhdos l80 dk0:ftppip,dk0:libftp,dk1:libwznet,sy0:printf,sy0:stdlib/s,sy0:clibrary,dk0:ftppip/n/e

ship: ../bin/ftp.abs ../bin/ftppip.abs
	@cd ../bin; sum *.abs >../checksums

../bin/%: %
	cp $< $@

__FRC__:
