
/*
 * arp2-irq kernel module for user-level interrupts for the ARP2
 * project.  Based on Peter Chubb's "User mode drivers: part 1,
 * interrupt handling" patch.
 *
 * Unlike Peter's patch, this is version is implemented as a kernel
 * module and it can handle shared interrupts by using pcode to
 * acknowledge interrupts and to shut down devices.
 *
 * The pcode is a subset of Knuth's MMIX ISA. mmix-knuth-gcc can be
 * used to compile the pcode in case it's too complicated to write it
 * by hand.
 */


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
//#include <linux/moduleparam.h>
//#include <linux/stat.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Blom");


static int __init arp2_irq_init(void)
{
  printk(KERN_INFO "arp2_irq_init()\n");
  return 0;
}


static void __exit arp2_irq_exit(void)
{
	printk(KERN_INFO "arp2_irq_exit()\n");
}

module_init(arp2_irq_init);
module_exit(arp2_irq_exit);
