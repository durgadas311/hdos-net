/*
 * NVRAM interfaces
 */

extern int vcksum(); /* (char buf[512]) */
extern scksum(); /* (char buf[512]) */

extern nvget(); /* (char *buf, int adr, int len) */
extern nvput(); /* (char *buf, int adr, int len) */
