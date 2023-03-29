/*
 * Low-level I/O on WIZnet chips (W5500)
 */

extern int wzget1(); /* (char bsb, char off) */
extern int wzget2(); /* (char bsb, char off) */
extern wzrd(); /* (char bsb, int off, char *buf, int len) */

extern wzput1(); /* (char bsb, char off, char val) */
extern wzput2(); /* (char bsb, char off, int val) */
extern wzwr(); /* (char bsb, int off, char *buf, int len) */

extern int wzcmd(); /* (char bsb, char cmd) returns socket SR */
extern int wzsts(); /* (char bsb, char bits) returns socket IR */
extern int wzist(); /* (char bsb, char bits) returns socket IR */
