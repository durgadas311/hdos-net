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
#define SC_OPEN		0x01
#define SC_CONN		0x04
#define SC_CLOS		0x08	/* DISCONNECT, actually */
#define SC_SEND		0x20
#define SC_RECV		0x40	/* completion of recv */

/* socket status/state */
#define SS_CLOSED	0x00
#define SS_INIT		0x13
#define SS_ESTAB	0x17

/* socket IR bits */
#define SI_SEND_OK	0x10
#define SI_TIMEOUT	0x08
#define SI_RECV		0x04
#define SI_DISCON	0x02
#define SI_CON		0x01
