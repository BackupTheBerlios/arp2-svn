
/*
 * arp2-irq kernel module for user-level interrupts for the ARP2
 * project.  Based on Peter Chubb's "User mode drivers: part 1,
 * interrupt handling" patch.
 *
 * Unlike Peter's patch, this is version is implemented as a
 * stand-alone kernel module (which has it's limitations, but does not
 * require a patched kernel tree) and it can handle shared interrupts
 * by using pcode to acknowledge interrupts and to shut down devices.
 *
 * The pcode is a subset of Knuth's MMIX ISA. mmix-knuth-gcc can be
 * used to compile the pcode in case it's too complicated to write it
 * by hand.
 */


#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
//#include <linux/moduleparam.h>
//#include <linux/stat.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Blom");


#define PROC_BASE_DIR "driver/arp2-irq"
#define ARP2_ERR      KERN_ERR "arp2-irq: "
#define ARP2_INFO     KERN_INFO "arp2-irq: "


static struct proc_dir_entry* root = NULL;

static int __init arp2_irq_init(void) {
  int i;
  char name[40];

  printk(ARP2_INFO "arp2_irq_init()\n");

  root = proc_mkdir(PROC_BASE_DIR, NULL);

  if (root == NULL) {
    printk(ARP2_ERR "Unable to create \"" PROC_BASE_DIR "/\".\n");
    return -EAGAIN;
  }


  for (i = 0; i < NR_IRQS; i++) {
    struct proc_dir_entry *entry;

    sprintf(name, "%d", i);
      
    entry = create_proc_entry(name, 0600, root);

    if (entry == NULL) {
      printk(ARP2_ERR "Unable to create \"%s\".\n", name);
    }
    else {
      entry->data = (void *)(unsigned long) i;
      entry->read_proc = NULL;
      entry->write_proc = NULL;
      entry->proc_fops = NULL; //&proc_fops;	
    }
  }

  return 0;
}


static void __exit arp2_irq_exit(void) {
  if (root != NULL) { 
    int i;
    char name[40];

    for (i = 0; i < NR_IRQS; i++) {
      sprintf(name, "%d", i);
      
      remove_proc_entry(name, root);
    }
  }

  remove_proc_entry(PROC_BASE_DIR, NULL);

  printk(KERN_INFO "arp2_irq_exit()\n");
}

module_init(arp2_irq_init);
module_exit(arp2_irq_exit);
