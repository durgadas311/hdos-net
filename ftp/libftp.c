/*
 * Library for HDOS using CP/NET.
 */

#include "dk1:netfile.h"
#include "sy0:printf.h"
#define INTERNAL
#include "libftp.h"

int remdrv = 0;
char sid = 255;
char dev[4] = { "SY0" };
struct fname *list = 0;

static struct fname *tail = 0;
static char dskbuf[512];
static char gfn[16];
static char fcb[36];

memcpy(dst, src, len)
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

/*
 * Builds an FCB from a filename string.
 * handles wildcards, in which case returns 1.
 * otherwise (simple filename) returns 0.
 */
int getfcb(fn, fcb)
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

int flist(pat)
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

ffree() {
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

int llist(pat)
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

stfile(str, ff)
char *str;
char *ff; /* 11 byte (8+3) filename field */
{
	int x;

	for (x = 0; x < 8 && ff[x] != ' '; ++x) {
		*str++ = ff[x];
	}
	x = 8;
	if (ff[x] != ' ') {
		*str++ = '.';
		for (; x < 11 && ff[x] != ' '; ++x) {
			*str++ = ff[x];
		}
	}
	*str++ = 0;
}

/* Copy (download) 'fcb' to 'lf' */
fget(lf, fcb)
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
	stfile(gfn, fcb + 1);
	printf("[%02x]%c:%-12s -> %s...", sid, remdrv + 'A', gfn, dskbuf);
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
fput(lf, fcb)
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
	stfile(gfn, fcb + 1);
	printf("%-16s -> [%02x]%c:%s...", dskbuf, sid, remdrv + 'A', gfn);
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

mget(pat)
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
		stfile(gfn, fcb + 1);
		if (fget(gfn, fcb) == -1) {
			break;
		}
	}
getout:
	ffree();
}

mput(pat)
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
		stfile(gfn, fcb + 1);
		if (fput(gfn, fcb) == -1) {
			break;
		}
	}
getout:
	ffree();
}
