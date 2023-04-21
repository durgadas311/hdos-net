/*
 */

struct fname {
	struct fname *next;
	char fn[11];
};

#ifndef INTERNAL
extern fget(); /* (char *lf, char *fcb) */
extern fput(); /* (char *lf, char *fcb) */
extern mget(); /* (char *pat) */
extern mput(); /* (char *pat) */
extern int afn(); /* (char *fn) */
extern int getfcb(); /* (char *fn, char *fcb) */
extern stfile(); /* (char *str, char *ff) */
extern memcpy(); /* (char *dst, char *src, int len) */
extern int llist(); /* (char *pat) */
extern int flist(); /* (char *pat) */
extern ffree(); /* (void) */

extern int remdrv;
extern char sid;
extern char dev[4];
extern struct fname *list;
#endif
