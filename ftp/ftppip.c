/*
 * PIP-like utility for transferrinf files between HDOS and CP/NET.
 */

#include "dk1:netfile.h"
#include "sy0:printf.h"
#include "libftp.h"

static char fcb[36] = {0};

static char rid;
static char *rf = 0;
static char *lf = 0;

static char hxdigit(chr)
char chr;
{
	if (chr >= '0' && chr <= '9') {
		return chr - '0';
	} else if (chr >= 'A' && chr <= 'F') {
		return (chr - 'A') + 10;
	} else if (chr >= 'a' && chr <= 'f') {
		return (chr - 'a') + 10;
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

static copen(nid)
char nid;
{
	if (sid != 255) {
		printf("Already connected\n");
		return 0;
	}
	if (nconn(nid) != 0) {
		printf("Server %02x unknown\n", nid);
		return 0;
	}
	sid = nid;
	printf("Connected to %02x\n", sid);
}

static cclose() {
	if (sid == 255) return;
	ndisc();
	printf("Disconnected from %02x\n", sid);
	sid = 255;
}

static cget()
{
	if (getfcb(rf, fcb) != 0) {
		/* TODO: error if lf != NULL? */
		mget(fcb);
		return;
	}
	if (lf) {
		fget(lf, fcb);
	} else {
		fget(rf, fcb);
	}
}

static cput()
{
	if (getfcb(lf, fcb) != 0) {
		/* TODO: error if rf != NULL? */
		mput(fcb);
		return;
	}
	if (rf) {
		fget(rf, fcb);
	} else {
		fget(lf, fcb);
	}
}

static help() {
	printf("Usage: ftppip [sid]drv:rem-file=dev:loc-file\n");
	printf("       ftppip dev:loc-file=[sid]drv:rem-file\n");
	printf("Where [sid] is optional if [00]\n");
	printf("Wildcards in source file require no destination file\n");
}

static char get = 0; /* get or put? */

/* try to parse a local file expression */
static int lparse(str)
char *str;
{
	if (isalpha(str[0]) && isalpha(str[1]) &&
			isdigit(str[2]) && str[3] == ':') {
		dev[0] = toupper(str[0]);
		dev[1] = toupper(str[1]);
		dev[2] = str[2];
		dev[3] = 0; /* just in case */
		str += 4;
	} else {
		/* "dev:" syntax error */
		return -1;
	}
	lf = str; /* might be "", caller checks in context */
	return 0;
}

/* try to parse a remote file expression */
static int rparse(str)
char *str;
{
	char *e;
	char nid;
	char drv;

	nid = 0;
	if (*str == '[') {
		nid = hxbyte(str + 1, &e);
		if (e == 0 || *e != ']') {
			/* "[sid]" syntax error */
			return -1;
		}
		str = e + 1;
	}
	drv = toupper(*str) - 'A';
	if (drv > 15 || str[1] != ':') {
		/* "drv:" syntax error */
		return -1;
	}
	str += 2;
	rf = str; /* might be "", caller checks in context */
	remdrv = drv;
	rid = nid;
	return 0;
}

static int parse(argc, argv)
int argc;
char **argv;
{
	char *s;

	s = argv[1];
	while (*s && *s != '=') ++s;
	if (!*s) {
		return -1;
	}
	*s++ = 0;
	/* parse source file expression */
	if (rparse(s) == 0) {
		if (!*rf) return -1;
		get = 1;
	} else if (lparse(s) == 0) {
		if (!*lf) return -1;
		get = 0;
	} else {
		return -1;
	}
	/* now parse destination expression */
	if (get && lparse(argv[1]) == 0) {
		if (!*lf) lf = 0;
		return 0;
	} else if (!get && rparse(argv[1]) == 0) {
		if (!*rf) rf = 0;
		return 0;
	}
	return -1;
}

static abort() {
	printf("Ctl-C\n");
	cclose();
	ndown();
#asm
	db	255,7	; SCALL CLRCO
#endasm
	exit(1);
}

static setctlc() {
	abort;
#asm
	mvi	a,3
	db	255,33	; SCALL CTLC
#endasm
}

main(argc, argv)
int argc;
char **argv;
{
	printf("HDOS FTP-PIP version 0.7\n");
	if (argc != 2 || parse(argc, argv) != 0) {
		help();
		exit(1);
	}
	setctlc();
	ninit();
	copen(rid);
	if (get) {
		cget();
	} else {
		cput();
	}
	cclose();
	ndown();
}
