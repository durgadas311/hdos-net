/*
 * Network file operations.
 *
 * Uses CP/M-style FCBs (36-byte).
 */

extern int ncreat(); /* (char *fcb) */
extern int nopen(); /* (char *fcb) */
extern int nclose(); /* (char *fcb) */
extern int nread(); /* (char *fcb, char *buf, int len) */
extern int nwrite(); /* (char *fcb, char *buf, int len) */

extern int nfirst(); /* (char *fcb, char *buf) returns dirent idx */
extern int nnext(); /* (char *fcb, char *buf) returns dirent idx */

extern int ndelete(); /* (char *fcb) */
extern int nrename(); /* (char *new, char *old) */

extern int nsize(); /* (char *fcb) */
