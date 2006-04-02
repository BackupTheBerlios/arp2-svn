/*
 * hvc_console.h
 * Copyright (C) 2005 IBM Corporation
 *
 * Author(s):
 * 	Ryan S. Arnold <rsa@us.ibm.com>
 *
 * hvc_console header information:
 *      moved here from include/asm-ppc64/hvconsole.h
 *      and drivers/char/hvc_console.c
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

#ifndef HVC_CONSOLE_H
#define HVC_CONSOLE_H

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/kobject.h>

/*
 * This is the max number of console adapters that can/will be found as
 * console devices on first stage console init.  Any number beyond this range
 * can't be used as a console device but is still a valid tty device.
 */
#define MAX_NR_HVC_CONSOLES	16

/*
 * This is a design shortcoming, the number '16' is a vio required buffer
 * size.  This should be changeable per architecture, but hvc_struct relies
 * upon it and that struct is used by all hvc_console backend drivers.  This
 * needs to be fixed.
 */
#define N_OUTBUF	16
#define N_INBUF		16

#define __ALIGNED__ __attribute__((__aligned__(sizeof(long))))

/* implemented by a low level driver */
struct hv_ops {
	int (*get_chars)(uint32_t vtermno, char *buf, int count);
	int (*put_chars)(uint32_t vtermno, const char *buf, int count);
};

struct hvc_struct {
	spinlock_t lock;
	int index;
	struct tty_struct *tty;
	unsigned int count;
	int do_wakeup;
	char outbuf[N_OUTBUF] __ALIGNED__;
	int n_outbuf;
	uint32_t vtermno;
	struct hv_ops *ops;
	int irq_requested;
	int irq;
	struct list_head next;
	struct kobject kobj; /* ref count & hvc_struct lifetime */
};

/* Register a vterm and a slot index for use as a console (console_init) */
extern int hvc_instantiate(uint32_t vtermno, int index, struct hv_ops *ops);

/* register a vterm for hvc tty operation (module_init or hotplug add) */
extern struct hvc_struct * __devinit hvc_alloc(uint32_t vtermno, int irq,
						 struct hv_ops *ops);
/* remove a vterm from hvc tty operation (modele_exit or hotplug remove) */
extern int __devexit hvc_remove(struct hvc_struct *hp);

#endif // HVC_CONSOLE_H
