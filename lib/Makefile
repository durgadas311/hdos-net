# An ftp-like utilitiy for HDOS using H8xSPI/WIZnet.
export HDOSDrive_dk0 = $(PWD)
#export CPMDrive_D = $(PWD)
#export CPMDefault = d:

all: wizio.rel nvram.rel wizcpnet.rel cnetfile.rel

%.mac: %.c
	vhdos c -x -m1 dk0:$<

%.rel: %.mac
	vhdos m80 =dk0:$*

libwznet.rel: all
	vhdos lib dk0:junk=dk0:cnetfile,dk0:wizcpnet,dk0:wizio,dk0:nvram/e
#	vcpm lib 'libwznet=cnetfile,wizcpnet,wizio,nvram'
