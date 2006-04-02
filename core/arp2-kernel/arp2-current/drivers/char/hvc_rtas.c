/*
 * IBM RTAS driver interface to hvc_console.c
 *
 * (C) Copyright IBM Corporation 2001-2005
 * (C) Copyright Red Hat, Inc. 2005
 *
 * Author(s): Maximino Augilar <IBM STI Design Center>
 *          : Ryan S. Arnold <rsa@us.ibm.com>
 *          : Utz Bacher <utz.bacher@de.ibm.com>
 *          : David Woodhouse <dwmw2@infradead.org>
 *
 *    inspired by drivers/char/hvc_console.c
 *    written by Anton Blanchard and Paul Mackerras
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
#include <linux/delay.h>
#include <asm/rtas.h>
#include <asm/irq.h>
#include "hvc_console.h"

static uint32_t hvc_rtas_vtermno = 0;
struct hvc_struct *hvc_rtas_dev;

#define RTASCONS_PUT_ATTEMPTS  16

static int rtascons_put_char_token = -1;
static int rtascons_get_char_token = -1;
static int rtascons_put_delay;

static inline int hvc_rtas_write_console(uint32_t vtermno, const char *buf, int count)
{
       int result = 0;
       int attempts = RTASCONS_PUT_ATTEMPTS;
       int done = 0;

       /* if there is more than one character to be displayed, wait a bit */
       for (; done < count && attempts; udelay(rtascons_put_delay)) {
               attempts--;
               result = rtas_call(rtascons_put_char_token, 1, 1, NULL, buf[done]);

               if (!result) {
			attempts = RTASCONS_PUT_ATTEMPTS;
			done++;
		}
       }
	/* the calling routine expects to receive the number of bytes sent */
	return done?:result;
}

static inline int rtascons_get_char(void)
{
       int result;

       if (rtas_call(rtascons_get_char_token, 0, 2, &result))
               result = -1;

       return result;
}

static int hvc_rtas_read_console(uint32_t vtermno, char *buf, int count)
{
	unsigned long got;
	int c;	
	int i;

	for (got = 0, i = 0; i < count; i++) {
	
		if (( c = rtascons_get_char() ) != -1) {
			buf[i] = c;
			++got;
		}
		else
			break;
	}
	return got;
}

static struct hv_ops hvc_rtas_get_put_ops = {
	.get_chars = hvc_rtas_read_console,
	.put_chars = hvc_rtas_write_console,
};

static int hvc_rtas_init(void)
{
	struct hvc_struct *hp;

	if (rtascons_put_char_token == -1)
		rtascons_put_char_token = rtas_token("put-term-char");
	if (rtascons_put_char_token == -1)
		return -EIO;

	if (rtascons_get_char_token == -1)
		rtascons_get_char_token = rtas_token("get-term-char");
	if (rtascons_get_char_token == -1)
		return -EIO;

	if (__onsim())
		rtascons_put_delay = 0;
	else
		rtascons_put_delay = 100;

	BUG_ON(hvc_rtas_dev);

	/* Allocate an hvc_struct for the console device we instantiated
	 * earlier.  Save off hp so that we can return it on exit */
	hp = hvc_alloc(hvc_rtas_vtermno, NO_IRQ, &hvc_rtas_get_put_ops);
	if (IS_ERR(hp))
		return PTR_ERR(hp);
	hvc_rtas_dev = hp;
	return 0;
}
module_init(hvc_rtas_init);

/* This will tear down the tty portion of the driver */
static void __exit hvc_rtas_exit(void)
{
	struct hvc_struct *hp_safe;
	/* Hopefully this isn't premature */
	if (!hvc_rtas_dev)
		return;

	hp_safe = hvc_rtas_dev;
	hvc_rtas_dev = NULL;

	/* Really the fun isn't over until the worker thread breaks down and the
	 * tty cleans up */
	hvc_remove(hp_safe);
}
module_exit(hvc_rtas_exit); /* before drivers/char/hvc_console.c */

/* This will happen prior to module init.  There is no tty at this time? */
static int hvc_rtas_console_init(void)
{
	rtascons_put_char_token = rtas_token("put-term-char");
	if (rtascons_put_char_token == -1)
		return -EIO;
	rtascons_get_char_token = rtas_token("get-term-char");
	if (rtascons_get_char_token == -1)
		return -EIO;

	hvc_instantiate(hvc_rtas_vtermno, 0, &hvc_rtas_get_put_ops );
	return 0;
}
console_initcall(hvc_rtas_console_init);
