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

int nwcid()
{
	return wzget1(0, CR_PMAG);
}

int nwstat()
{
	return 0;
}

static int wzopen(bsb)
char bsb;
{
	char sr;
	int x;

	sr = wzget1(bsb, SN_SR);
	if (sr != SS_ESTAB) {
		if (sr != SS_INIT) {
			sr = wzcmd(bsb, SC_OPEN);
			if (sr != SS_INIT) return -1;
		}
		wzcmd(bsb, SC_CONN);
		sr = x = wzist(bsb, (SI_CON|SI_TIMEOUT|SI_DISCON));
		if (x == -1 || (sr & SI_CON) == 0) return -1;
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
 * Take up to 'len' bytes from recv fifo into 'buf'.
 * issues RECV command.
 * Returns number of bytes taken.
 */
int rcvall(bsb, buf, len)
char bsb;
char *buf;
int len;
{
	int rsr;
	int rxrd;
	char fif;
	char ir;

	fif = bsb + 2; /* RX is just +2 from SK */
	rsr = wzget2(bsb, SN_RXRSR);
	if (rsr > len) rsr = len;
	rxrd = wzget2(bsb, SN_RXRD);
	wzrd(fif, rxrd, buf, rsr);
	rxrd += rsr;
	wzput2(bsb, SN_RXRD, rxrd);
	rcvend(bsb);
	return rsr;
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
	if (rsr < len) return 0; /* not enough data, yet */
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
		/* TODO: timeout? */
		ret = rcvdat(bsb, buf, CPN_DAT + 1 + add, 0);
	} while (ret == 0);
	return ret; /* TODO: what if ret < CPN_DAT + 1 + add */
}

int sndend(bsb)
char bsb;
{
	char ir;
	int x;

	wzcmd(bsb, SC_SEND);
	ir = x = wzist(bsb, (SI_SEND_OK|SI_TIMEOUT|SI_DISCON));
	if (x == -1 || (ir & SI_SEND_OK) == 0) return -1;
	return 0;
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

	fif = bsb + 1; /* TX is just +1 from SK */
	txwr = wzget2(bsb, SN_TXWR);
	/* TODO: check for overrun? not possible? */
	wzwr(fif, txwr, buf, len);
	txwr += len;
	wzput2(bsb, SN_TXWR, txwr);
	if (last) {
		if (sndend(bsb) != 0)
			return -1;
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
	/* TODO: any init/setup on W5500? flush recv? */
	return snddat(bsb, buf, CPN_DAT + 1 + add, 0);
}
