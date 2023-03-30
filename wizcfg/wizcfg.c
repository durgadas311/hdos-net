/*
 * HDOS version of WIZCFG, written in C
 */

#include "dk1:w5500.h"
#include "dk1:wizio.h"
#include "dk1:nvram.h"
#include "sy0:printf.h"

char direct = 0;
char cmd;
int count = 0;

char nvbuf[512];

static int getnv() {
	if (nvget(nvbuf, 0, 512) != 0) {
		return 1;
	}
	if (vcksum(nvbuf) != 0) {
		return 1;
	}
	return 0;
}

static int setnv() {
	scksum(nvbuf);
	if (nvput(nvbuf, 0, 512) != 0) {
		return 1;
	}
	return 0;
}

static setword(off, val)
int off;
int val;
{
	nvbuf[off] = (val >> 8);
	nvbuf[off + 1] = val;
}

static char hxdigit(chr)
char chr;
{
	if (chr >= '0' && chr <= '9') {
		return chr - '0';
	} else if (chr >= 'A' && chr <= 'F') {
		return (chr - 'A') + 10;
	} else {
		return 255;
	}
}

static char hxbyte(str, end)
char *str;
char **end;
{
	int x, y, z;

	z = 0;
	x = 0;
	while ((y = hxdigit(str[x])) < 16) {
		z = (z << 4) + y;
		++x;
	}
	if (x > 0) {
		*end = str + x;
		return z;
	}
	*end = 0;
	return 0; /* special case - not hex number */
}

static int isdigit(c)
char c;
{
	return (c >= '0' && c <= '9');
}

static int dcword(str, end)
char *str;
char **end;
{
	int x, z;

	z = 0;
	x = 0;
	while (isdigit(str[x])) {
		z = (z * 10) + (str[x] - '0');
		++x;
	}
	if (x > 0) {
		*end = str + x;
		return z;
	}
	*end = 0;
	return 0; /* special case - not decimal number */
}

static int wizcfg() {
	int x, y, z;

	/* configure W5500 from nvbuf[] */
	wzwr(0, CR_GAR, nvbuf + CR_GAR, 18);
	wzput1(0, CR_PMAG, nvbuf[CR_PMAG]);
	z = 1;
	y = 32;
	for (x = 0; x < 8; ++x) {
		if (nvbuf[y + SN_PRT] == 0x31) {
			wzclose(z);
			wzput1(z, SN_MR, 1);
			wzput1(z, SN_KA, nvbuf[y + SN_KANV]);
			wzwr(z, SN_PRT, nvbuf + y + SN_PRT, 2);
			wzwr(z, SN_DIPR, nvbuf + y + SN_DIPR, 6);
		}
		y += 32;
		z += 4;
	}
	return 0;
}

static int getip(str, off)
char *str;
int off;
{
	int x;
	int y;
	char *s, *e;

	s = str;
	for (x = 0; x < 4; ++x) {
		y = dcword(s, &e);
		if (e == 0 || y > 255) return 1;
		if (x == 3) {
			if (*e != 0) return 1;
		} else {
			if (*e != '.') return 1;
		}
		nvbuf[off + x] = y;
		s = e + 1;
	}
	return 0;
}

/* always bsb=0? */
static int ipadr(p1, reg)
char *p1;
char reg;
{
	/* parse and set IP addr */
	if (getip(p1, reg) != 0) return 1;
	if (direct) {
		wzwr(0, reg, nvbuf + reg, 4);
	}
	return 0;
}

/* always bsb=0? */
static int macadr(p1, reg)
char *p1;
char reg;
{
	/* parse and set MAC addr */
	int x;
	char y;
	char *s, *e;

	s = p1;
	for (x = 0; x < 6; ++x) {
		y = hxbyte(s, &e);
		if (e == 0) return 1;
		if (x == 5) {
			if (*e != 0) return 1;
		} else {
			if (*e != ':') return 1;
		}
		nvbuf[reg + x] = y;
		s = e + 1;
	}
	if (direct) {
		wzwr(0, reg, nvbuf + reg, 6);
	}
	return 0;
}

/* always bsb=0? */
static int nid(p1, reg)
char *p1;
char reg;
{
	/* parse and set node ID */
	char nid;
	char *end;

	nid = hxbyte(p1, &end);
	if (end == 0 || *end != 0) return 1;
	if (direct) {
		wzput1(0, reg, nid);
	} else {
		nvbuf[reg] = nid;
	}
	return 0;
}

static int sockx() {
	/* delete socket config */
	int ix;

	if (direct) {
		ix = (cmd - '0') * 4 + 1; /* bsb */
		wzput1(ix, SN_PRT, 0);
	} else {
		ix = (cmd - '0' + 1) * 32;
		nvbuf[ix + SN_PRT] = 0;
	}
	return 0;
}

static int sock(p1, p2, p3, p4)
char *p1;
char *p2;
char *p3;
char *p4; /* may be NULL */
{
	/* parse and set socket config */
	int ix;
	char *s, *e;
	int n;
	int w;
	char bsb;

	/* build in nvbuf[] first */
	ix = (cmd - '0' + 1) * 32;
	n = hxbyte(p1, &e) | 0x3100;
	if (e == 0 || *e != 0) return 1;
	if (getip(p2, ix + SN_DIPR) != 0) return 1;
	w = dcword(p3, &e);
	if (e == 0 || *e != 0) return 1;
	setword(ix + SN_DPRT, w);
	if (p4 != 0) {
		w = dcword(p4, &e);
		if (e == 0 || *e != 0) return 1;
		w = w / 5;
	} else {
		w = 0;
	}
	nvbuf[ix + SN_KANV] = w;
	setword(ix + SN_PRT, n);
	if (direct) {
		bsb = (cmd - '0') * 4 + 1;
		wzput1(bsb, SN_MR, 1);
		wzput2(bsb, SN_PRT, n);
		wzwr(bsb, SN_DIPR, nvbuf + ix + SN_DIPR, 6);
		wzput1(bsb, SN_KA, w);
	}
	return 0;
}

static shnid(off, str)
int off;
char *str;
{
	printf("%s%02xH\n", str, nvbuf[off]);
}

static ship(off, str)
int off;
char *str;
{
	printf("%s%u.%u.%u.%u\n", str,
		nvbuf[off], nvbuf[off+1], nvbuf[off+2], nvbuf[off+3]);
}

static shmac(off, str)
int off;
char *str;
{
	printf("%s%02x.%02x.%02x.%02x.%02x.%02x\n", str,
		nvbuf[off], nvbuf[off+1], nvbuf[off+2],
		nvbuf[off+3], nvbuf[off+4], nvbuf[off+5]);
}

static xxshow() {
	/* show common register block, in nvbuf */
	shnid(CR_PMAG, "Node ID:  ");
	ship(CR_SIPR,  "IP Addr:  ");
	ship(CR_GAR,   "Gateway:  ");
	ship(CR_SUBR,  "Subnet:   ");
	shmac(CR_SHAR, "MAC:      ");
}

static xxshsk(ix, off, kao)
int ix;
int off;
int kao;
{
	int pt, ka;

	if (nvbuf[off + SN_PRT] != 0x31) {
		return 1;
	}
	++count;
	pt = (nvbuf[off + SN_DPRT] << 8) |
		nvbuf[off + SN_DPRT + 1];
	ka = nvbuf[off + kao] * 5;
	printf("Socket %d: %02xH %u.%u.%u.%u %d %d\n",
		ix, nvbuf[off + SN_PRT + 1],
		nvbuf[off + SN_DIPR],
		nvbuf[off + SN_DIPR + 1],
		nvbuf[off + SN_DIPR + 2],
		nvbuf[off + SN_DIPR + 3],
		pt, ka);
	return 0;
}

static wzshow() {
	int x, y;

	wzrd(0, 0, nvbuf, 64);
	xxshow();
	count = 0;
	y = 1;
	for (x = 0; x < 8; ++x) {
		wzrd(y, 0, nvbuf, 64);
		xxshsk(x, 0, SN_KA);
		y += 4;
	}
	if (count == 0) {
		printf("No Sockets Configured\n");
	}
	return 0;
}

static nvshow() {
	int x, y;

	xxshow();
	count = 0;
	y = 32;
	for (x = 0; x < 8; ++x) {
		xxshsk(x, y, SN_KANV);
		y += 32;
	}
	if (count == 0) {
		printf("No Sockets Configured\n");
	}
	return 0;
}

static show() {
	if (direct) {
		wzshow();
	} else {
		nvshow();
	}
	return 0;
}

help() {
	printf("WIZCFG version 1.5c\n");
	printf("Usage: WIZCFG {G|I|S} ipadr\n");
	printf("       WIZCFG M macadr\n");
	printf("       WIZCFG N cid\n");
	printf("       WIZCFG {0..7} sid ipadr port [keep]\n");
	printf("       WIZCFG {0..7} X\n");
	printf("       WIZCFG R\n");
	printf("Sets network config in NVRAM\n");
	printf("Prefix cmd with W to set WIZ850io directly\n");
	printf("R cmd sets WIZ850io from NVRAM\n");
	exit(1);
}

int main(argc, argv)
int argc;
char **argv;
{
	int x, e;

	x = 1;
	if (x < argc && argv[x][0] == 'W' && argv[x][1] == 0) {
		direct = 1;
		++x;
	}
	if (!direct) {
		if (getnv() != 0) {
			printf("NVRAM read failure\n");
			exit(1);
		}
	}
	if (x >= argc) {
		show();
		exit(0);
	}
	if (x < argc && argv[x][0] == 'R' && argv[x][1] == 0) {
		/* the only command with no arguments */
		wizcfg();
		exit(0);
	}
	if (x < argc && argv[x][0] == 'H' && argv[x][1] == 0) {
		/* the only other command with no arguments */
		help(); /* does not return */
	}
	e = 1; /* assume error */
	if (x < argc && argv[x][0] != 0 && argv[x][1] == 0 && x + 1 < argc) {
		/* valid 1-char command */
		cmd = argv[x][0];
		switch (cmd) {
		case 'G':
			e = ipadr(argv[x + 1], CR_GAR);
			break;
		case 'I':
			e = ipadr(argv[x + 1], CR_SIPR);
			break;
		case 'S':
			e = ipadr(argv[x + 1], CR_SUBR);
			break;
		case 'M':
			e = macadr(argv[x + 1], CR_SHAR);
			break;
		case 'N':
			e = nid(argv[x + 1], CR_PMAG);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			if (x + 4 < argc) {
				e = sock(argv[x + 1], argv[x + 2], argv[x + 3], argv[x + 4]);
			} else if (x + 3 < argc) {
				e = sock(argv[x + 1], argv[x + 2], argv[x + 3], 0);
			} else if (argv[x + 1][0] == 'X') {
				e = sockx();
			}
			break;
		}
	}
	if (e) {
		help(); /* does not return */
	}
	if (!direct) {
		if (setnv() != 0) {
			printf("NVRAM write failure\n");
			exit(1);
		}
	}
	return 0;
}
