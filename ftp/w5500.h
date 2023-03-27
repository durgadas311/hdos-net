/*
 * WIZnet W5500 chip definitions
 */

/* per-socket register offsets */
#define SN_CR	1	/* command register */
#define SN_IR	2	/* interrupt register */
#define SN_SR	3	/* status register */
#define SN_PRT	4	/* port register */
#define SN_TXWR		36
#define SN_RXRSR	38
#define SN_RXRD		40

/* socket commands */
#define SN_OPEN		0x01
#define SN_CONN		0x04
#define SN_CLOS		0x08	/* DISCONNECT, actually */
#define SN_SEND		0x20
#define SN_RECV		0x40	/* completion of recv */

/* socket status/state */
#define SS_CLOSED	0x00
#define SS_INIT		0x13
#define SS_ESTAB	0x17
