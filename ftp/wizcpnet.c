/*
 * CP/NET operations on WIZnet (W5500)
 */

#include "dk1:wizio.h"
#include "dk1:w5500.h"
#include "dk1:cpnet.h"

int nwinit()
{
	/* TODO: check if W5500 configured... */
	return 0;
}

int nwdown()
{
	/* TODO: close sockets "we" opened? */
	return 0;
}

static int wzopen(bsb)
char bsb;
{
	char sr;

	sr = wzget1(bsb, SN_SR);
	if (sr != SS_ESTAB) {
		if (sr != SS_INIT) {
			sr = wzcmd(bsb, SC_OPEN);
			if (sr != SS_INIT) return -1;
		}
		sr = wizcmd(bsb, SC_CONN);
		if (sr != SS_ESTAB) return -1;
	}
	return bsb;
}

int nwopen(sid)
char sid;
{
	char bsb;
	int port;

	for (bsb = 1; bsb < 0x20; bsb += 0x04) {
		port = wzget2(bsb, SN_PRT);
		if ((port >> 8) == 0x31 && (port & 0xff) == sid) {
			return wzopen(bsb);
		}
	}
	/* SID not configured */
	return -1;
}

int nwclose(bsb)
char bsb;
{
	char sr;

	sr = wzget1(bsb, SN_SR);
	if (sr != SS_CLOSED) {
		wzcmd(bsb, SC_CLOS);
	}
	return 0;
}

int rcvend(bsb)
char bsb;
{
	wzcmd(bsb, SC_RECV);
	return 0;
}

/*
 * Take 'len' bytes from recv fifo into 'buf'.
 * Caller must ensure there are bytes available.
 * if 'last' then issue RECV command.
 * Returns 'len' on success, 0 if not enough data, -1 if error
 */
int rcvdat(bsb, buf, len, last)
char bsb;
char *buf;
int len;
char last;
{
	int rsr;
	int rxrd;
	char fif;
	char ir;

	fif = bsb + 2; /* RX is just +2 from SK */
	rsr = wzget2(bsb, SN_RXRSR);
	if (rsr < len) return 0; /* not error, just no data */
	rxrd = wzget2(bsb, SN_RXRD);
	wzrd(fif, rxrd, buf, len);
	rxrd += len;
	wzput2(bsb, SN_RXRD, rxrd);
	if (last) {
		rcvend(bsb);
	}
	return len;
}

/* Receives minimal CP/NET message (hdr+1) */
int rcvhdr(bsb, buf, add)
char bsb;
char *buf;
int add;
{
	int ret;

	/* TODO: any init/setup on W5500? */
	do {
		ret = rcvdat(bsb, buf, CPN_DAT + 1 + add, 0);
	} while (ret == 0);
	return ret;
}

/*
 * Put 'len' bytes from 'buf' into send fifo.
 * if 'last' then issue SEND command.
 */
int snddat(bsb, buf, len, last)
char bsb;
char *buf;
int len;
char last;
{
	int txwr;
	char fif;
	char ir;

	fif = bsb + 1; /* TX is just +1 from SK */
	txwr = wzget2(bsb, SN_TXWR);
	wzwr(fif, txwr, buf, len);
	txwr += len;
	wzput2(bsb, SN_TXWR, txwr);
	if (last) {
		wzcmd(bsb, SC_SEND);
		ir = wzist(bsb, (SI_SEND_OK|SI_TIMEOUT|SI_DISCON));
		if ((ir & SI_SEND_OK) == 0) {
			return -1;
		}
	}
	return len; /* or 0? */
}

/*
 * Put CP/NET header from 'buf' into send fifo.
 * Sends minimal CP/NET message (hdr+1).
 */
int sndhdr(bsb, buf, add)
char bsb;
char *buf;
int add;
{
	/* TODO: any init/setup on W5500? */
	return snddat(bsb, buf, CPN_DAT + 1 + add, 0);
}