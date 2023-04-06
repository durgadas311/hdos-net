/*
 * ftp-like utility for HDOS using CP/NET.
 */

#include "dk1:netfile.h"
#include "sy0:printf.h"

int remdrv = 0;
int run = 1;
char sid = 255;
char dev[4] = { "SY0" };

static struct fname {
	struct fname *next;
	char fn[11];
} *list = 0, *tail = 0;

static char cmdbuf[128];
static char dskbuf[512];
static char fcb[36] = {0};
static char gfn[16];

static memcpy(dst, src, len)
char *dst;
char *src;
int len;
{
	/* also convert NUL to SP */
	while (len-- > 0) {
		if (*src) *dst++ = *src++;
		else { *dst++ = ' '; ++src; }
	}
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
 * Builds an FCB from a filename string.
 * handles wildcards, in which case returns 1.
 * otherwise (simple filename) returns 0.
 */
static int getfcb(fn, fcb)
char *fn;
char *fcb;
{
	char afn, b, c;
	int x, y;

	afn = 0;
	x = 0;
	y = 1;
	b = ' ';
	/* TODO: check for errors like ':' */
	while ((c = fn[x]) != 0 && c != '.' && y < 9) {
		++x;
		if (c == '*') {
			afn = 1;
			b = '?';
			break;
		}
		if (c == '?') afn = 1;
		fcb[y++] = toupper(c);
	}
	while (y < 9) fcb[y++] = b;
	while ((c = fn[x]) != 0 && c != '.') ++x;
	if (c == '.') ++x;
	b = ' ';
	while ((c = fn[x]) != 0 && y < 12) {
		++x;
		if (c == '*') {
			afn = 1;
			b = '?';
			break;
		}
		if (c == '?') afn = 1;
		fcb[y++] = toupper(c);
	}
	while (y < 12) fcb[y++] = b;
	fcb[0] = remdrv + 1;
	fcb[12] = 0;
	fcb[32] = 0;
	return afn;
}

static struct fname *fnew(ff)
char *ff;
{
	struct fname *f;

	f = alloc(sizeof(struct fname));
	if (f == 0) return 0;
	memcpy(f->fn, ff, sizeof(f->fn));
	f->next = 0;
	if (tail == 0) {
		list = tail = f;
	} else {
		tail->next = f;
		tail = f;
	}
	return f;
}

static int flist(pat)
char *pat;
{
	int e, c;
	struct fname *f;

	c = 0;
	e = nfirst(pat, dskbuf);
	while (e != 255) {
		e = (e & 3) * 32;
		f = fnew(dskbuf + e + 1);
		if (f == 0) {
			printf("Out of memory\n");
			return -1;
		}
		++c;
		e = nnext(pat, dskbuf);
	}
	return c;
}

static ffree() {
	struct fname *f;

	tail = 0;
	while ((f = list) != 0) {
		list = list->next;
		free(f);
	}
}

static int match(de, pat)
char *de;
char *pat;
{
	int x;
	char c;

	for (x = 0; x < 11; ++x) {
		if (pat[x] == '?') continue;
		c = de[x];
		if (c == 0) c = ' ';
		if (c != pat[x]) return 0;
	}
	return 1;
}

static int llist(pat)
char *pat;
{
	int c, i;
	struct fname *f;
	int fd;

	sprintf(dskbuf, "%s:direct.sys", dev);
	fd = fopen(dskbuf, "rb");
	if (fd == 0) {
		printf("Directory error\n");
		return -1;
	}
	c = 0;
	while (read(fd, dskbuf, 512) == 512) {
		/* TODO: allow SYS files? */
		for (i = 0; i < 0x1fa; i += 23) {
			if (dskbuf[i] == 0xfe) goto done;
			if (dskbuf[i] == 0xff) continue;
			if (dskbuf[i + 14] & 0x80) continue; /* SYS */
			if (!match(dskbuf + i, pat + 1)) continue;
			f = fnew(dskbuf + i);
			if (f == 0) {
				printf("Out of memory\n");
				c = -1;
				goto done;
			}
			++c;
		}
	}
done:
	fclose(fd);
	return c;
}

static stfile(str, ff, pad)
char *str;
char *ff; /* 11 byte (8+3) filename field */
char pad;
{
	int x;

	for (x = 0; x < 8 && ff[x] != pad; ++x) {
		*str++ = ff[x];
	}
	x = 8;
	if (ff[x] != pad) {
		*str++ = '.';
		for (; x < 11 && ff[x] != pad; ++x) {
			*str++ = ff[x];
		}
	}
	*str++ = 0;
}

/*
 * Print a file name from a CP/M FCB.
 * prints in a 13-char field, 5 files per line.
 */
static prfile(de, pad)
char *de;
char pad;
{
	int x, y;
	static col = 0;

	if (de == 0) {
		/* TODO: end of partial line */
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
	stfile(gfn, de + 1, pad);
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
	strcpy(dev, "SY0");
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
		printf("Not connected\n");
		return 0;
	}
	printf("Server %02x, Remote %c:, Local %s:\n",
		sid, 'A' + remdrv, dev);
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

	if (argc < 2) {
		getfcb("*.*", fcb);
	} else {
		getfcb(argv[1], fcb);
	}
	e = nfirst(fcb, dskbuf);
	if (e == 255) {
		printf("No file\n");
		return 0;
	}
	while (e != 255) {
		e = (e & 3) * 32;
		prfile(dskbuf + e, ' ');
		e = nnext(fcb, dskbuf);
	}
	prfile(0, 0);
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
		prfile(f->fn - 1, ' ');
	}
	prfile(0, 0);
getout:
	ffree();
}

static csize(argc, argv)
int argc;
char **argv;
{
	int sz;

	if (getfcb(argv[1], fcb) != 0) {
		printf("Wildcards not allowed\n");
		return 0;
	}
	if (nsize(fcb) != 0) {
		printf("Failled to get remote file size\n");
		return 0;
	}
	sz = (fcb[33] >> 3) | (fcb[34] << 5) | (fcb[35] << 13);
	if ((fcb[33] & 7) != 0) ++sz;
	printf("%s: %uK\n", argv[1], sz);
}

/* Copy (download) 'fcb' to 'lf' */
static fget(lf, fcb)
char *lf;
char *fcb;
{
	int fd;
	int n;

	if (nopen(fcb) != 0) {
		printf("No remote file\n");
		return -1;
	}
	sprintf(dskbuf, "%s:%s", dev, lf);
	fd = fopen(dskbuf, "wb");
	if (fd == 0) {
		printf("Cannot create local file\n");
		nclose(fcb);
		return -1;
	}
	stfile(gfn, fcb + 1, ' ');
	printf("%-12s -> %s...", gfn, dskbuf);
	while ((n = nread(fcb, dskbuf, 256)) > 0) {
		if (write(fd, dskbuf, 256) == -1) {
			printf("local write error\n");
			n = -1;
			break;
		}
	}
	fclose(fd);
	nclose(fcb);
	if (n == 0) {
		printf("Done\n");
	}
	return n;
}

/* Copy (upload) 'lf' to 'fcb' */
static fput(lf, fcb)
char *lf;
char *fcb;
{
	int fd;
	int n;

	sprintf(dskbuf, "%s:%s", dev, lf);
	fd = fopen(dskbuf, "rb");
	if (fd == 0) {
		printf("No local file\n");
		return -1;
	}
	/* TODO: protect against existing file? */
	if (nopen(fcb) != 0 && ncreat(fcb) != 0) {
		printf("Cannot create remote file\n");
		fclose(fd);
		return -1;
	}
	stfile(gfn, fcb + 1, ' ');
	printf("%-16s -> %s...", dskbuf, gfn);
	while ((n = read(fd, dskbuf, 256)) > 0) {
		if (nwrite(fcb, dskbuf, 256) != 0) {
			printf("remote write error\n");
			n = -1;
			break;
		}
	}
	fclose(fd);
	nclose(fcb);
	if (n == 0) {
		printf("Done\n");
	}
	return n;
}

static mget(pat)
char *pat;
{
	int c;
	struct fname *f;

	c = flist(pat);
	if (c == -1) goto getout;
	if (c == 0) {
		printf("No files\n");
		return;
	}
	for (f = list; f != 0; f = f->next) {
		memcpy(fcb + 1, f->fn, sizeof(f->fn));
		fcb[0] = remdrv + 1;
		fcb[12] = 0;
		fcb[32] = 0;
		stfile(gfn, fcb + 1, ' ');
		if (fget(gfn, fcb) == -1) {
			break;
		}
	}
getout:
	ffree();
}

static mput(pat)
char *pat;
{
	int c;
	struct fname *f;

	c = llist(pat);
	if (c == -1) goto getout;
	if (c == 0) {
		printf("No files\n");
		return;
	}
	for (f = list; f != 0; f = f->next) {
		memcpy(fcb + 1, f->fn, sizeof(f->fn));
		fcb[0] = remdrv + 1;
		fcb[12] = 0;
		fcb[32] = 0;
		stfile(gfn, fcb + 1, ' ');
		if (fput(gfn, fcb) == -1) {
			break;
		}
	}
getout:
	ffree();
}

static cget(argc, argv)
int argc;
char **argv;
{
	char *f;

	if (getfcb(argv[1], fcb) != 0) {
		/* TODO: error if argc > 2? */
		mget(fcb);
		return;
	}
	if (argc > 2) f = argv[2];
	else f = argv[1];
	fget(f, fcb);
}

static cput(argc, argv)
int argc;
char **argv;
{
	char *f;

	if (getfcb(argv[1], fcb) != 0) {
		/* TODO: error if argc > 2? */
		mput(fcb);
		return;
	}
	if (argc > 2) f = argv[2];
	else f = argv[1];
	fput(f, fcb);
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
	printf("delete <rf>	delete remote file(s)\n");
	printf("rename <old> <new>\n		rename remote file\n");
	printf("size <rf>	show remote file size\n");
	printf("status		show ftp status\n");
	printf("quit		disconnect and exit ftp\n");
	printf("Where: <afn> = Ambiguous File Name (wildcards)\n");
	printf("get, put, and delete also allow wildcards\n");
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
	{"lcd", F_CON|F_ARG, clcd},
	{"ldir", F_CON, cldir},
	{"get", F_CON|F_ARG, cget},
	{"put", F_CON|F_ARG, cput},
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
		if (strcmp(cmd->cmd, cmdv[0]) == 0) break;
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

	printf("HDOS FTP-Lite version 0.5\n");
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
