/*
 * ftp-like utility for HDOS using CP/NET.
 */

#include "dk1:netfile.h"
#include "sy0:printf.h"
#include "libftp.h"

static int run = 1;
static char cmdbuf[128];
static char fcb[36] = {0};
static char gfn[16];

/* assumes s1 is already lower-case */
static int stricmp(s1, s2)
char *s1;
char *s2;
{
	while (*s1 && *s2) {
		if (*s1 != tolower(*s2)) break;
		++s1;
		++s2;
	}
	return *s2 - *s1;
}

/* anything other than "y" or "yes" returns -1 */
static int getyn() {
	int c;
	int x;

	x = 0;
	while ((c = getchar()) != -1 && c != '\n') {
		if (x < 4) gfn[x++] = toupper(c);
	}
	if (x != 1 && x != 3) return -1;
	if (gfn[0] != 'Y') return -1;
	if (x > 1 && gfn[1] != 'E' && gfn[2] != 'S') return -1;
	return 0;
}

static getline(cc, cv)
int *cc;
char **cv;
{
	int c;
	int x;
	char *s;

	x = 0;
	cv[x] = 0;
	s = cmdbuf;
	while ((c = getchar()) != -1 && c != '\n') {
		if (c == ' ') {
			if (cv[x] != 0) {
				*s++ = 0;
				++x;
				cv[x] = 0;
			}
			continue;
		}
		if (cv[x] == 0) {
			cv[x] = s;
		}
		*s++ = c;
	}
	*s++ = 0;
	if (cv[x] != 0) {
		++x;
		cv[x] = 0;
	}
	*cc = x;
}

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

/*
 * Print a file name from a CP/M FCB.
 * prints in a 13-char field, 5 files per line.
 * call with NULL when done, to end last line.
 */
static prfile(de)
char *de;
{
	int x, y;
	static col = 0;

	if (de == 0) {
		if (col != 0) putchar('\n');
		col = 0;
		return 0;
	}
	if (col >= 5) {
		putchar('\n');
		col = 0;
	} else if (col != 0) {
		putchar(' ');
		putchar(' ');
	}
	stfile(gfn, de + 1);
	printf("%-13s", gfn);
	++col;
}

static cbye() { run = 0; }

static copen(argc, argv)
int argc;
char **argv;
{
	char *end;
	char nid;

	if (sid != 255) {
		printf("Already connected\n");
		return 0;
	}
	nid = hxbyte(argv[1], &end);
	if (end == 0 || *end != 0 || nid >= 255) {
		printf("Invalid server node ID \"%s\"\n", argv[1]);
		return 0;
	}
	if (nconn(nid) != 0) {
		printf("Server %02x unknown\n", nid);
		return 0;
	}
	sid = nid;
	remdrv = 0;
	printf("Connected to %02x\n", sid);
}

static cclose() {
	if (sid == 255) return;
	ndisc();
	printf("Disconnected from %02x\n", sid);
	sid = 255;
}

static cstatus() {
	if (sid == 255) {
		printf("Not connected, Local %s:\n", dev);
		return 0;
	}
	if (nstat() != 0) {
		printf("Lost connection to %02x\n", sid);
		sid = 255;
	} else {
		printf("Server %02x, Remote %c:, Local %s:\n",
			sid, 'A' + remdrv, dev);
	}
}

static ccd(argc, argv)
int argc;
char **argv;
{
	char d;
	char *drv;

	drv = argv[1];
	if (isalpha(drv[0]) && (drv[1] == ':' || drv[1] == 0)) {
		d = toupper(drv[0]) - 'A';
		if (d < 16) {
			remdrv = d;
			return 0;
		}
	}
	printf("Invalid CP/NET drive\n");
}

static clcd(argc, argv)
int argc;
char **argv;
{
	char *drv;

	drv = argv[1];
	if (isalpha(drv[0]) && isalpha(drv[1]) &&
			isdigit(drv[2]) &&
			(drv[3] == ':' || drv[3] == 0)) {
		dev[0] = toupper(drv[0]);
		dev[1] = toupper(drv[1]);
		dev[2] = drv[2];
		dev[3] = 0; /* just in case */
		/* TODO: confirm that it exists... */
		return 0;
	}
	printf("Invalid HDOS device name\n");
}

static cdir(argc, argv)
int argc;
char **argv;
{
	int e;
	struct fname *f;

	if (argc < 2) {
		getfcb("*.*", fcb);
	} else {
		getfcb(argv[1], fcb);
	}
	e = flist(fcb);
	if (e == -1) goto getout;
	if (e == 0) {
		printf("No file\n");
		return;
	}
	for (f = list; f != 0; f = f->next) {
		prfile(f->fn - 1);
	}
	prfile(0);
getout:
	ffree();
}

static cldir(argc, argv)
int argc;
char **argv;
{
	int e;
	struct fname *f;

	if (argc < 2) {
		getfcb("*.*", fcb);
	} else {
		getfcb(argv[1], fcb);
	}
	e = llist(fcb);
	if (e == -1) goto getout;
	if (e == 0) {
		printf("No file\n");
		return;
	}
	for (f = list; f != 0; f = f->next) {
		prfile(f->fn - 1);
	}
	prfile(0);
getout:
	ffree();
}

/* return file size in KB, or -1 on error */
static int fsize(fcb)
char *fcb;
{
	int sz;

	if (nsize(fcb) != 0) {
		printf("Failed to get remote file size\n");
		return -1;
	}
	sz = (fcb[33] >> 3) | (fcb[34] << 5) | (fcb[35] << 13);
	if ((fcb[33] & 7) != 0) ++sz;
	return sz;
}

static int msize(pat)
char *pat;
{
	int sz, z;
	int c;
	struct fname *f;

	c = flist(pat);
	if (c == -1) goto getout;
	if (c == 0) {
		printf("No files\n");
		return 0;
	}
	sz = 0;
	for (f = list; f != 0; f = f->next) {
		memcpy(fcb + 1, f->fn, sizeof(f->fn));
		fcb[0] = remdrv + 1;
		fcb[12] = 0;
		fcb[32] = 0;
		z = fsize(fcb);
		if (z == -1) goto getout;
		sz += z;
	}
	printf("Total: %d files, %uK\n", c, sz);
getout:
	ffree();
}

static csize(argc, argv)
int argc;
char **argv;
{
	int sz;

	if (getfcb(argv[1], fcb) != 0) {
		msize(fcb);
		return 0;
	}
	sz = fsize(fcb);
	if (sz == -1) return 0;
	printf("%s: %uK\n", argv[1], sz);
}

/* argv[1] is remote spec, argv[2] is local */
static cget(argc, argv)
int argc;
char **argv;
{
	char *f;

	f = argv[1];
	if (argc > 2) {
		if (afn(argv[2]) || getfcb(argv[1], fcb) != 0) {
			printf("Wildcards not allowed\n");
			return;
		}
		f = argv[2];
	} else if (getfcb(argv[1], fcb) != 0) {
		mget(fcb);
		return;
	}
	fget(f, fcb);
}

/* argv[1] is local spec, argv[2] is remote */
static cput(argc, argv)
int argc;
char **argv;
{
	if (argc > 2) {
		if (afn(argv[1]) || getfcb(argv[2], fcb) != 0) {
			printf("Wildcards not allowed\n");
			return;
		}
	} else if (getfcb(argv[1], fcb) != 0) {
		mput(fcb);
		return;
	}
	fput(argv[1], fcb);
}

static cdelete(argc, argv)
int argc;
char **argv;
{
	if (getfcb(argv[1], fcb) != 0) {
		printf("Confirm: DELETE [%02x]%c:%s ? ",
			sid, remdrv + 'A', argv[1]);
		if (getyn() != 0) {
			printf("Delete aborted\n");
			return;
		}
	}
	if (ndelete(fcb) != 0) {
		printf("Failed to delete\n");
	}
}

static chelp() {
	printf("Usage: ftp [<sid>]\n");
	printf("Commands:\n");
	printf("help		print this\n");
	printf("open <sid>	open new connection to server\n");
	printf("close		close current connection\n");
	printf("cd <drive>	change remote drive A:-P:\n");
	printf("dir [<afn>]	list remote files\n");
	printf("lcd <device>	change local drive SY0:,...\n");
	printf("ldir [<afn>]	list local files\n");
	printf("get <rf> [<lf>]	get remote file(s) [to local file]\n");
	printf("put <lf> [<rf>]	put local file(s) [to remote file]\n");
	printf("delete <afn>	delete remote file(s)\n");
	printf("size <afn>	show remote file(s) size\n");
	printf("status		show ftp status\n");
	printf("quit		disconnect and exit ftp\n");
	printf("Where: <afn> = Ambiguous File Name (wildcards)\n");
	printf("get and put also allow wildcards\n");
}

#define F_CON	1
#define F_ARG	2

static struct cmds {
	char cmd[8];
	char flg;
	int (*fnc)();
} cmdtbl[20] = {
	{"quit", 0, cbye},
	{"help", 0, chelp},
	{"status", 0, cstatus},
	{"open", F_ARG, copen},
	{"close", F_CON, cclose},
	{"cd", F_CON|F_ARG, ccd},
	{"dir", F_CON, cdir},
	{"size", F_CON|F_ARG, csize},
	{"lcd", F_ARG, clcd},
	{"ldir", 0, cldir},
	{"get", F_CON|F_ARG, cget},
	{"put", F_CON|F_ARG, cput},
	{"delete", F_CON|F_ARG, cdelete},
	{0,0, 0}
};

static int cmdc;
static char *cmdv[8];

static docmd() {
	struct cmds *cmd;

	printf("ftp> ");
	getline(&cmdc, cmdv);
	if (cmdc < 1) return 0;
	for (cmd = cmdtbl; cmd->cmd[0] != 0; ++cmd) {
		if (stricmp(cmd->cmd, cmdv[0]) == 0) break;
	}
	if (cmd->cmd[0] == 0) {
		printf("Unknown command\n");
		return;
	}
	if ((cmd->flg & F_CON) && sid == 255) {
		printf("Not connected\n");
		return;
	}
	if ((cmd->flg & F_ARG) && cmdc < 2) {
		printf("Missing argument(s)\n");
		return;
	}
	(*cmd->fnc)(cmdc, cmdv);
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
	int x;

	printf("HDOS FTP-Lite version 0.9\n");
	setctlc();
	ninit();
	if (argc > 1) {
		copen(argc, argv);
	}
	while (run) {
		docmd();
	}
	cclose();
	ndown();
}
