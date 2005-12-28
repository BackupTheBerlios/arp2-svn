/*
 *
 *    Copyright 2000-2001 Sistina Software, Inc.
 *    Portions Copyright 2001 The OpenGFS Project.
 *
 *    This is free software released under the GNU General Public License.
 *    There is no warranty for this software.  See the file COPYING for
 *    details.
 *
 *    See the file AUTHORS for a list of contributors.
 *
 *    This file was maintained by:
 *      Kenneth W. Preslan <kpreslan@sistina.com>
 *
 */

#define EXPORT_SYMTAB
#include <linux/module.h>

/*
 * Don't even think about doing modversion here.
 * So, it doesn't matter what the types are.
 */
int __divdi3(void);
EXPORT_SYMBOL(__divdi3);

int __moddi3(void);
EXPORT_SYMBOL(__moddi3);

int __udivdi3(void);
EXPORT_SYMBOL(__udivdi3);

int __umoddi3(void);
EXPORT_SYMBOL(__umoddi3);

MODULE_DESCRIPTION("divdi3 Module");
MODULE_AUTHOR("The OpenGFS Project");
MODULE_LICENSE("GPL");
