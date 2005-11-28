/*
 * include/asm-ppc/pc_serial.h
 *
 * This is basically a copy of include/asm-i386/serial.h.
 * It is used on platforms which have an ISA bus and thus are likely
 * to have PC-style serial ports at the legacy I/O port addresses.
 * It also includes the definitions for the fourport, accent, boca
 * and hub6 multiport serial cards, although I have never heard of
 * anyone using any of those on a PPC platform.  -- paulus
 */

#include <linux/config.h>

/*
 * This assumes you have a 1.8432 MHz clock for your UART.
 *
 * It'd be nice if someone built a serial card with a 24.576 MHz
 * clock, since the 16550A is capable of handling a top speed of 1.5
 * megabits/second; but this requires the faster clock.
 */
#define BASE_BAUD ( 1843200 / 16 )

#ifdef CONFIG_SERIAL_MANY_PORTS
#define RS_TABLE_SIZE  64
#else
#define RS_TABLE_SIZE  4
#endif

#define SERIAL_PORT_DFNS /* */
