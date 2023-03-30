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
	extern int dcksum();
	if (nvget(nvbuf, 0, 512) != 0) {
		return 1;
	}
	if (vcksum(nvbuf) != 0) {
		return 1;
	}
	return 0;
}

static int setnv() {
	if (scksum(nvbuf) != 0) {
		return 1;
	}
	if (nvput(nvbuf, 0, 512) != 0) {
		return 1;
	}
	return 0;
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

static int ipadr(p1)
char *p1;
{
	/* parse and set IP addr */
	return 0;
}

static int macadr(p1)
char *p1;
{
	/* parse and set MAC addr */
	return 0;
}

static int nid(p1)
char *p1;
{
	/* parse and set node ID */
	return 0;
}

static int sockx() {
	/* delete socket config */
	return 0;
}

static int sock(p1, p2, p3, p4)
char *p1;
char *p2;
char *p3;
char *p4; /* may be NULL */
{
	/* parse and set socket config */
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
		case 'I':
		case 'S':
			e = ipadr(argv[x + 1]);
			break;
		case 'M':
			e = macadr(argv[x + 1]);
			break;
		case 'N':
			e = nid(argv[x + 1]);
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
			printf("NVRAM read failure\n");
			exit(1);
		}
	}
	return 0;
}
