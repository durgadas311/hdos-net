# An ftp-like utilitiy for HDOS using H8xSPI/WIZnet.
export HDOSDrive_dk0 = $(PWD)

%.mac: %.c
	vhdos c -m1 dk0:$<

%.rel: %.mac
	vhdos m80 =dk0:$*
