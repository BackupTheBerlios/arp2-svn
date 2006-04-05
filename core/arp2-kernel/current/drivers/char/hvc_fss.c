/*
 * IBM Full System Simulator driver interface to hvc_console.c
 *
 * (C) Copyright IBM Corporation 2001-2005
 * Author(s): Maximino Augilar <IBM STI Design Center>
 *          : Ryan S. Arnold <rsa@us.ibm.com>
 *
 *    inspired by drivers/char/hvc_console.c
 *    written by Anton Blanchard and Paul Mackerras
 *
 * Some code is from the IBM Full System Simulator Group in ARL.
 * Author: Patrick Bohrer <IBM Austin Research Lab>
 *
 * Much of this code was moved here from the IBM Full System Simulator
 * Bogus console driver in order to reuse the framework provided by the hvc
 * console driver. Ryan S. Arnold <rsa@us.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/err.h>
#include <asm/irq.h>
#include "hvc_console.h"

static uint32_t hvc_fss_vtermno = 0;
struct hvc_struct *hvc_fss_dev;

static inline int callthru0(int command)
{
	register int c asm ("r3") = command;

	asm volatile (".long 0x000EAEB0" : "=r" (c): "r" (c));
	return((c));
}

static inline int callthru3(int command, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
	register int c asm ("r3") = command;
	register unsigned long a1 asm ("r4") = arg1;
	register unsigned long a2 asm ("r5") = arg2;
	register unsigned long a3 asm ("r6") = arg3;

	asm volatile (".long 0x000EAEB0" : "=r" (c): "r" (c), "r" (a1), "r" (a2), "r" (a3));
	return((c));
}

static inline int hvc_fss_write_console(uint32_t vtermno, const char *buf, int count)
{
	int ret = 0;
	ret = callthru3(0, (unsigned long)buf,
		(unsigned long)count, (unsigned long)1);
	if (ret != 0) {
		return (count - ret); /* is this right? */
	}

	/* the calling routine expects to receive the number of bytes sent */
	return count;
}

static inline int hvc_fss_read_console(uint32_t vtermno, char *buf, int count)
{
	unsigned long got;
	int c;	
	int i;

	for (got = 0, i = 0; i < count; i++) {
	
		if (( c = callthru0(60) ) != -1) {
			buf[i] = c;
			++got;
		}
		else
			break;
	}
	return got;
}

static struct hv_ops hvc_fss_get_put_ops = {
	.get_chars = hvc_fss_read_console,
	.put_chars = hvc_fss_write_console,
};

static int hvc_fss_init(void)
{
	/* Register a single device with the driver */
	struct hvc_struct *hp;

	if(!__onsim()) {
		return -1;
	}

	if(hvc_fss_dev) {
		return -1; /* This shouldn't happen */
	}

	/* Allocate an hvc_struct for the console device we instantiated
	 * earlier.  Save off hp so that we can return it on exit */
	hp = hvc_alloc(hvc_fss_vtermno, NO_IRQ, &hvc_fss_get_put_ops);
	if (IS_ERR(hp))
		return PTR_ERR(hp);
	hvc_fss_dev = hp;
	return 0;
}
module_init(hvc_fss_init);

/* This will tear down the tty portion of the driver */
static void __exit hvc_fss_exit(void)
{
	struct hvc_struct *hp_safe;
	/* Hopefully this isn't premature */
	if (!hvc_fss_dev)
		return;

	hp_safe = hvc_fss_dev;
	hvc_fss_dev = NULL;

	/* Really the fun isn't over until the worker thread breaks down and the
	 * tty cleans up */
	hvc_remove(hp_safe);
}
module_exit(hvc_fss_exit); /* before drivers/char/hvc_console.c */

/* This will happen prior to module init.  There is no tty at this time? */
static int hvc_fss_console_init(void)
{
	/* Don't register if we aren't running on the simulator */
	if (__onsim()) {
		/* Tell the driver we know of one console device.  We
		 * shouldn't get a collision on the index as long as no-one
		 * else instantiates on hardware they don't have. */
		hvc_instantiate(hvc_fss_vtermno, 0, &hvc_fss_get_put_ops );
	}
	return 0;
}
console_initcall(hvc_fss_console_init);
