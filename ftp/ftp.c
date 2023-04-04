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

static cbye() { run = 0; }

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
	if (end == 0 || *end != 0) {
		sid = 255;
		printf("Invalid server node ID \"%s\"\n", str);
		return 0;
	}
#if 0
	if (nconn(nid) != 0) {
		printf("Server unknown\n");
		return 0;
	}
#endif
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
#if 0
	ndisc();
#endif
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
#if 0
	ninit();
#endif
	if (argc > 1) {
		copen(argv[1]);
	}
	while (run) {
		docmd();
	}
	cclose();
#if 0
	ndown();
#endif
}
