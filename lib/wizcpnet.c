/*
 * CP/NET operations on WIZnet (W5500)
 */

#include "wizio.h"
#include "w5500.h"
#include "cpnet.h"
#asm
	NAME	('wizcpnet')
#endasm

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

/* "0" for OK, else not connected (...) */
int nwstat(bsb)
char bsb;
{
	char sr;

	sr = wzget1(bsb, SN_SR);
	if (sr == SS_ESTAB) return 0;
	/* TODO: other status checks? */
	return -1;
}

static int wzopen(bsb)
char bsb;
{
	char sr;
	int x;

	wzput1(bsb, SN_IR, (SI_CON|SI_TIMEOUT|SI_DISCON)); /* clear left-overs */
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

/*
 * Assuming the same anomaly as with SN_TXWR:
 * cannot read current value of SN_RXRD until
 * RECV command is executed.
 */
static int rxoff = 0;

int rcvend(bsb)
char bsb;
{
	int rxrd;

	rxrd = wzget2(bsb, SN_RXRD) + rxoff;
	wzput2(bsb, SN_RXRD, rxrd);
	rxoff = 0;
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
	rsr = wzget2(bsb, SN_RXRSR) - rxoff;
	if (rsr > len) rsr = len;
	rxrd = wzget2(bsb, SN_RXRD) + rxoff;
	wzrd(fif, rxrd, buf, rsr);
	rxoff += rsr;
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
	rsr = wzget2(bsb, SN_RXRSR) - rxoff;
	if (rsr < len) return 0; /* not enough data, yet */
	rxrd = wzget2(bsb, SN_RXRD) + rxoff;
	wzrd(fif, rxrd, buf, len);
	rxoff += len;
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

	rxoff = 0; /* paranoia */
	/* TODO: any init/setup on W5500? */
	do {
		if (nwstat(bsb) != 0) return -1;
		/* TODO: timeout? */
		ret = rcvdat(bsb, buf, CPN_DAT + 1 + add, 0);
	} while (ret == 0);
	return ret; /* TODO: what if ret < CPN_DAT + 1 + add */
}

/*
 * The W5500 has an odd behavior w.r.t. reading the SN_TXWR register.
 * The value read is effectively SN_RXRD until the SEND command is
 * executed. Intermediate values written to SN_TXWR are not visible.
 * Need to track progress of message to FIFO independent of SN_TXWR,
 * then update SN_TXWR at end.
 *
 * Alternative to 'txoff' would be to try and mirror
 * SN_TXWR and just use it directly (do not wzget2 each time).
 * Hard to say which would be more reliable.
 */
static int txoff = 0;

int sndend(bsb)
char bsb;
{
	int txwr;
	char ir;
	int x;

	txwr = wzget2(bsb, SN_TXWR);
	txwr += txoff;
	wzput2(bsb, SN_TXWR, txwr);
	txoff = 0;
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
	wzwr(fif, txwr + txoff, buf, len);
	txoff += len;
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
	txoff = 0; /* paranoia */
	/* TODO: any init/setup on W5500? flush recv? */
	return snddat(bsb, buf, CPN_DAT + 1 + add, 0);
}
