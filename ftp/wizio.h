/*
 * Low-level I/O on WIZnet chips (W5500)
 */

extern int wzget1(); /* (char bsb, int off) */
extern int wzget2(); /* (char bsb, int off) */
extern void wzrd(); /* (char bsb, int off, char *buf, int len) */

extern void wzput1(); /* (char bsb, int off, char val) */
extern void wzput2(); /* (char bsb, int off, int val) */
extern void wzwr(); /* (char bsb, int off, char *buf, int len) */

extern int wzcmd(); /* (char bsb, char cmd) returns socket SR */
extern int wzsts(); /* (char bsb, char bits) returns socket IR */
extern int wzist(); /* (char bsb, char bits) returns socket IR */
