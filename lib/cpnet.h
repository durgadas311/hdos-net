/*
 * CP/NET operations and constants
 */

#define CPN_FMT	0
#define CPN_DID	1
#define CPN_SID	2
#define CPN_FNC	3
#define CPN_SIZ	4
#define CPN_DAT	5	/* payload of message, length of hdr */

extern int nwinit(); /* (void) */
extern int nwdown(); /* (void) */

extern int nwstat(); /* (void) */
extern int nwcid(); /* (void) */

extern int nwopen(); /* (char sid) - opens socket to SID, returns bsb (handle) */
extern int nwclose(); /* (char bsb) - closes socket */

extern int rcvhdr(); /* (char bsb, char *buf, int add) */
extern int rcvdat(); /* (char bsb, char *buf, int len, bool last) */
extern int rcvend(); /* (char bsb) */
extern int rcvall(); /* (char bsb, char *buf, int len) */

extern int sndhdr(); /* (char bsb, char *buf, int add) */
extern int snddat(); /* (char bsb, char *buf, int len, bool last) */
extern int sndend(); /* (char bsb) */
