/*
 * ftp-like utility for HDOS using CP/NET.
 */

#include "dk1:netfile.h"
#include "sy0:printf.h"

int remdrv = 0;
int run = 1;
char sid = 255;
char dev[4] = { "SY0" };

static char cmdbuf[128];
static char fcb[36] = {0};

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

static prfile(de)
char *de;
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
	y = 0;
	for (x = 1; x < 9 && de[x] != ' '; ++x) {
		++y;
		putchar(de[x]);
	}
	x = 9;
	if (de[x] != ' ') {
		++y;
		putchar('.');
		for (; x < 12 && de[x] != ' '; ++x) {
			++y;
			putchar(de[x]);
		}
	}
	while (y < 13) {
		++y;
		putchar(' ');
	}
	++col;
}

static cbye() { run = 0; }

extern int foo1, foo2;

static copen(str)
char *str;
{
	char *end;
	char nid;

	if (sid != 255) {
		printf("Already connected\n");
		return 0;
	}
	nid = hxbyte(str, &end);
	if (end == 0 || *end != 0 || nid >= 255) {
		printf("Invalid server node ID \"%s\"\n", str);
		return 0;
	}
	if (nconn(nid) != 0) {
		printf("Server %02x unknown %04x %04x\n", nid, foo1, foo2);
		return 0;
	}
	sid = nid;
	remdrv = 0;
	strcpy(dev, "SY0");
	printf("Connected to %02x\n", sid);
}

static cclose() {
	if (sid == 255) {
		printf("Not connected\n");
		return 0;
	}
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

static ccd(drv)
char *drv;
{
	char d;

	if (isalpha(drv[0]) && (drv[1] == ':' || drv[1] == 0)) {
		d = toupper(drv[0]) - 'A';
		if (d < 16) {
			remdrv = d;
			return 0;
		}
	}
	printf("Invalid CP/NET drive\n");
}

static clcd(drv)
char *drv;
{
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

static cdir(arg)
char *arg;
{
	int e;

	fcb[0] = remdrv + 1;
	strcpy(fcb + 1, "???????????");
	e = nfirst(fcb, cmdbuf);
	if (e == 255) {
		printf("No file\n");
		return 0;
	}
	while (e != 255) {
		e = (e & 3) * 32;
		prfile(cmdbuf + e);
		e = nnext(fcb, cmdbuf);
	}
	prfile(0);
}

static chelp() {
	printf("Usage: ftp [<sid>]\n");
	printf("Commands:\n");
	printf("help		print this\n");
	printf("open <sid>	open new connection to server\n");
	printf("close		close current connection\n");
	printf("cd <drive>	change remote drive A:-P:\n");
	printf("dir [<afn>]	list remote files\n");
	printf("pwd		show remote drive name\n");
	printf("lcd <device>	change local drive SY0:,...\n");
	printf("lpwd		show local drive name\n");
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

static int cmdc;
static char *cmdv[8];

static docmd() {
	printf("ftp> ");
	getline(&cmdc, cmdv);
	if (cmdc < 1) return 0;
	if (strcmp(cmdv[0], "quit") == 0) {
		cbye();
	} else if (strcmp(cmdv[0], "help") == 0) {
		chelp();
	} else if (strcmp(cmdv[0], "status") == 0) {
		cstatus();
	} else if (strcmp(cmdv[0], "close") == 0) {
		cclose();
	} else if (strcmp(cmdv[0], "open") == 0 && cmdc > 1) {
		copen(cmdv[1]);
	} else if (strcmp(cmdv[0], "cd") == 0 && cmdc > 1) {
		ccd(cmdv[1]);
	} else if (strcmp(cmdv[0], "lcd") == 0 && cmdc > 1) {
		clcd(cmdv[1]);
	} else if (strcmp(cmdv[0], "dir") == 0) {
		cdir(0); /* TODO: pass optional arg */
	} else {
		printf("Unknown command\n");
	}
}


main(argc, argv)
int argc;
char **argv;
{
	int x;

	printf("HDOS FTP-Lite version 1.0\n");
	ninit();
	if (argc > 1) {
		copen(argv[1]);
	}
	while (run) {
		docmd();
	}
	cclose();
	ndown();
}
