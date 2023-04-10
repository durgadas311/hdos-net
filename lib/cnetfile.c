/*
 * Implementation of netfile.h using CP/NET (cpnet.h)
 */
#include "cpnet.h"

#asm
	NAME	('cnetfile')
#endasm

extern char remdrv; /* 0-15 == A:-P: */

static char cursid = 255;
static char curbsb = 0;
static char cid;

static char cpnhdr[CPN_DAT + 2];
static char xbuf[256];
static int ret;
static int siz;

int nconn(sid)
char sid;
{
	int bsb;

	if (curbsb != 0) return -1;
	bsb = nwopen(sid);
	if (bsb <= 0) return -1;
	cursid = sid;
	curbsb = bsb;
	return 0;
}

int ndisc()
{
	int ret;

	if (curbsb == 0) return 0;
	ret = nwclose(curbsb);
	curbsb = 0;
	cursid = 255;
	return ret;
}

/* returns 0 if connected, else -1 */
int nstat() {
	if (curbsb == 0) return -1;
	if (nwstat(curbsb) == 0) return 0;
	ndisc();
	return -1;
}

static setup(fnc, siz)
char fnc;
int siz;
{
	cpnhdr[CPN_FMT] = 0x00;
	cpnhdr[CPN_DID] = cursid;
	cpnhdr[CPN_SID] = cid;
	cpnhdr[CPN_FNC] = fnc;
	cpnhdr[CPN_SIZ] = siz - 1;
	cpnhdr[CPN_DAT] = 0; /* might change before send */
}

static int rcverr(siz)
int siz;
{
	if (siz > 1) {
		rcvall(curbsb, xbuf, siz - 1);
	} else {
		rcvend(curbsb);
		xbuf[0] = 0;
	}
	return cpnhdr[CPN_DAT] | (xbuf[0] << 8);
}

/* response is minimum message size */
static int rcvsmp() {
	if (rcvhdr(curbsb, cpnhdr, 0) == -1) return -1;
	return rcverr(cpnhdr[CPN_SIZ] + 1); /* should be 1 */
}

/* standard response message with only FCB (+1) */
static int rcvfcb(fcb)
char *fcb;
{
	if (rcvhdr(curbsb, cpnhdr, 0) == -1) return -1;
	siz = cpnhdr[CPN_SIZ] + 1;
	if (siz == 37) {
		ret = rcvdat(curbsb, fcb, 36, 1);
		if (ret <= 0) return -1;
		return 0;
	} else { /* error */
		return rcverr(siz);
	}
}

/* response handler for search first/next */
static int rcvdir(buf)
char *buf;
{
	char dc;

	if (rcvhdr(curbsb, cpnhdr, 0) == -1) return -1;
	siz = cpnhdr[CPN_SIZ] + 1;
	if (cpnhdr[CPN_DAT] == 255) {
		return rcverr(siz);
	}
	dc = cpnhdr[CPN_DAT] & 3; /* 0,1,2,3 */
	if (siz == 33) {
		ret = rcvdat(curbsb, buf + (dc * 32), 32, 1);
	} else if (siz == 129) {
		ret = rcvdat(curbsb, buf, 128, 1);
	} else { /* assume error */
		return rcverr(siz);
	}
	if (ret == 0) { /* not enough data... ooops... */
		rcvall(curbsb, xbuf, siz - 1);
		return 255; /* generic error */
	}
	return dc;
}

/* 'fcb' - standard CP/M 2.2 FCB (36 bytes) */
int ncreat(fcb)
char *fcb;
{
	int ret;

	setup(22, 37);
	sndhdr(curbsb, cpnhdr, 0);
	if (snddat(curbsb, fcb, 36, 1) == -1)
		return -1;
	return rcvfcb(fcb);
}

int nopen(fcb)
char *fcb;
{
	setup(15, 45);
	sndhdr(curbsb, cpnhdr, 0);
	snddat(curbsb, fcb, 36, 0);
	if (snddat(curbsb, xbuf, 8, 1) == -1) /* password (not used) */
		return -1;
	return rcvfcb(fcb);
}

int nclose(fcb)
char *fcb;
{
	setup(16, 45);
	sndhdr(curbsb, cpnhdr, 0);
	snddat(curbsb, fcb, 36, 0);
	if (snddat(curbsb, xbuf, 8, 1) == -1) /* password (ignored) */
		return -1;
	return rcvfcb(fcb);
}

int nread(fcb, buf, len)
char *fcb;
char *buf;
int len;
{
	int n;

	n = 0;
	while (len > 0) {
		setup(20, 37);
		sndhdr(curbsb, cpnhdr, 0);
		if (snddat(curbsb, fcb, 36, 1) == -1)
			return -1;
		if (rcvhdr(curbsb, cpnhdr, 0) == -1) return -1;
		siz = cpnhdr[CPN_SIZ] + 1;
		if (siz == 165) {
			ret = rcvdat(curbsb, fcb, 36, 0);
			if (ret > 0)
				ret = rcvdat(curbsb, buf, 128, 1);
			if (ret == 0) {
				rcvall(curbsb, xbuf, siz);
				return -1;
			}
		} else { /* error (siz == 2?) */
			rcverr(siz);
			if (cpnhdr[CPN_DAT] == 1) { /* EOF */
				if (n > 0) {
					for (; len > 0; --len) *buf++ = 0;
				}
				break;
			}
			/* TODO: save full error? */
			return -1;
		}
		buf += 128;
		len -= 128;
		n += 128;
	}
	return n;
}

int nwrite(fcb, buf, len)
char *fcb;
char *buf;
int len;
{
	int siz;

	while (len > 0) {
		setup(21, 165);
		sndhdr(curbsb, cpnhdr, 0);
		snddat(curbsb, fcb, 36, 0);
		if (snddat(curbsb, buf, 128, 1) == -1)
			return -1;
		if (rcvhdr(curbsb, cpnhdr, 0) == -1) return -1;
		siz = cpnhdr[CPN_SIZ] + 1;
		if (siz == 37) {
			ret = rcvdat(curbsb, fcb, 36, 1);
			if (ret == 0) {
				rcvall(curbsb, xbuf, siz);
				return -1;
			}
			if (cpnhdr[CPN_DAT] != 0) {
				return cpnhdr[CPN_DAT];
			}
		} else { /* error (siz == 2?) */
			return rcverr(siz);
		}
		buf += 128;
		len -= 128;
	}
	return 0;
}

/*
 * 'fcb' is not strictly an FCB, but the relevant search portion, R/O.
 * 'buf' must be (at least) 128 bytes, and the only valid entry is
 * the one pointed to by the return value (0,1,2,3).
 */
int nfirst(fcb, buf)
char *fcb;
{
	setup(17, 38);
	cpnhdr[CPN_DAT] = remdrv; /* TODO: check against fcb[0] */
	cpnhdr[CPN_DAT + 1] = 0; /* no user numbers */
	sndhdr(curbsb, cpnhdr, 1);
	if (snddat(curbsb, fcb, 36, 1) == -1)
		return -1;
	return rcvdir(buf);
}

/*
 * 'fcb' not used, currently (must be same as nfirst()).
 * 'buf' must be (at least) 128 bytes, and the only valid entry is
 * the one pointed to by the return value (0,1,2,3).
 */
int nnext(fcb, buf)
char *fcb;
{
	setup(18, 2);
	cpnhdr[CPN_DAT] = remdrv; /* TODO: check against fcb[0] */
	cpnhdr[CPN_DAT + 1] = 0; /* no user numbers */
	sndhdr(curbsb, cpnhdr, 1);
	if (sndend(curbsb) == -1)
		return -1;
	return rcvdir(buf);
}

/* deletes multiple files without warning */
int ndelete(fcb)
char *fcb;
{
	setup(19, 37);
	sndhdr(curbsb, cpnhdr, 0);
	if (snddat(curbsb, fcb, 36, 1) == -1)
		return -1;
	return rcvsmp();
}

int nrename(new, old)
char *new;
char *old;
{
	setup(23, 37);
	sndhdr(curbsb, cpnhdr, 0);
	snddat(curbsb, old, 16, 0);
	if (snddat(curbsb, new, 20, 1) == -1)
		return -1;
	return rcvsmp();
}

/* computes file size and returns in fcb[33..35] */
int nsize(fcb)
char *fcb;
{
	setup(35, 37);
	sndhdr(curbsb, cpnhdr, 0);
	if (snddat(curbsb, fcb, 36, 1) == -1)
		return -1;
	return rcvfcb(fcb);
}

int ninit() {
	int ret;

	ret = nwinit();
	if (ret != 0) return -1;
	cid = nwcid();
	return 0;
}

int ndown() {
	ndisc();
	nwdown();
}
