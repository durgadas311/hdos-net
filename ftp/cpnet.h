/*
 * CP/NET operations
 */

extern int nwinit(); /* (void) */
extern int nwdown(); /* (void) */

extern int nwopen(); /* (char sid) - opens socket to SID, returns bsb (handle) */
extern int nwclose(); /* (char bsb) - closes socket */

extern int rcvhdr(); /* (char bsb, char *buf) */
extern int rcvdat(); /* (char bsb, char *buf, int len, bool last) */

extern int sndhdr(); /* (char bsb, char *buf) */
extern int snddat(); /* (char bsb, char *buf, int len, bool last) */
