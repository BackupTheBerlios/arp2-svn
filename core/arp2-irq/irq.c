
/*
 * arp2-irq kernel module for user-level interrupts for the ARP2
 * project.  Based on Peter Chubb's "User mode drivers: part 1,
 * interrupt handling" patch and Dmitry A. Fedorov's irq device.
 *
 * Unlike Peter's patch, this is version is implemented as a
 * stand-alone kernel module (which has it's limitations, but does not
 * require a patched kernel tree) and it can handle shared interrupts
 * by using pcode to acknowledge interrupts and to shut down devices.
 *
 * The pcode is a subset of Knuth's MMIX ISA. mmix-knuth-gcc can be
 * used to compile the pcode in case it's too complicated to write it
 * by hand.
 *
 * Resources:
 *
 * http://lwn.net/Articles/66829/
 * http://lwn.net/Articles/127293/
 * http://www.gelato.unsw.edu.au/IA64wiki/UserLevelDrivers
 *
 * http://lwn.net/Articles/127294/
 * http://lwn.net/Articles/127295/
 *
 * http://www.xml.com/ldd/chapter/book/ch09.html
 */

/*
PCI methods:

MapMemory(addr, size)
MapIO(start, size)

MapResource(nr)
Unmap(addr)

SetDMAMask(mask)

(configbytes by mapping sysfs)

*/

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
//#include <linux/moduleparam.h>
//#include <linux/stat.h>

#include "pcode.h"

#define PROC_BASE_DIR "driver/arp2-irq"
#define ARP2_ERR      KERN_ERR "arp2-irq: "
#define ARP2_INFO     KERN_INFO "arp2-irq: "

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Blom");

struct pci_device_list {
    struct pci_device_list* next;
    struct pci_dev* pci_dev;
    char name[16];
} *pci_device_list = NULL;

static struct proc_dir_entry* root = NULL;

struct irq_proc {
    unsigned long     irq;
    wait_queue_head_t q;
    atomic_t          count;
    char              devname[9 + TASK_COMM_LEN];
};
 
static irqreturn_t int_handler(int irq, void* _idp, struct pt_regs* regs)
{
  struct irq_proc* idp = (struct irq_proc*) _idp;
  bool handled;
 
  BUG_ON(idp->irq != irq);

  // Execute pcode to acknowledge the interrupt
  // (here)
  // disable_irq_nosync(irq);
  handled = true;

  atomic_inc(&idp->count);
  wake_up(&idp->q);

  return IRQ_RETVAL(handled);
}



static int int_open(struct inode* inode, struct file* file)
{
  struct irq_proc* ip;
  struct proc_dir_entry* ent = PDE(inode);
  int error;

  ip = kmalloc(sizeof (*ip), GFP_KERNEL);

  if (ip == NULL) {
    return -ENOMEM;
  }
	
  memset(ip, 0, sizeof(*ip));

  snprintf(ip->devname, sizeof (ip->devname), "arp2-irq/%s", current->comm);
  ip->devname[sizeof (ip->devname) - 1] = 0;
  init_waitqueue_head(&ip->q);
  atomic_set(&ip->count, 0);
  ip->irq = (unsigned long) ent->data;
	
  error = request_irq(ip->irq,
		      int_handler,
		      SA_INTERRUPT,
		      ip->devname,
		      ip);
  if (error < 0) {
    kfree(ip);
    return error;
  }

  file->private_data = (void*) ip;

  return 0;
}


static int int_release(struct inode* inode, struct file* file)
{
  struct irq_proc* ip = (struct irq_proc*) file->private_data;
  (void) inode;

  // Execute pcode to shut down the interrupt
  // (here)

  free_irq(ip->irq, ip);
  file->private_data = NULL;
  kfree(ip);

  return 0;
}


static int __init arp2_irq_init(void) {
  struct pci_dev* device = NULL;

  printk(ARP2_INFO "arp2_irq_init()\n");

  root = proc_mkdir(PROC_BASE_DIR, NULL);

  if (root == NULL) {
    printk(ARP2_ERR "Unable to create \"" PROC_BASE_DIR "/\".\n");
    return -EAGAIN;
  }


  for_each_pci_dev(device) {
    struct proc_dir_entry* entry;
    struct pci_device_list* pl;

    pl = kmalloc(sizeof (*pl), GFP_KERNEL);
    
    if (pl == NULL) {
      pci_dev_put(device);
      return -ENOMEM;
    }

    pl->next    = pci_device_list;
    pl->pci_dev = device;

    sprintf(pl->name, "%04x:%02x:%02x.%d", 
	    pci_domain_nr(device->bus),
	    device->bus->number, 
	    PCI_SLOT(device->devfn), 
	    PCI_FUNC(device->devfn));
      
    entry = create_proc_entry(pl->name, 0600, root);

    if (entry == NULL) {
      printk(ARP2_ERR "Unable to create \"%s\".\n", pl->name);
      pci_dev_put(device);
      kfree(pl);
    }
    else {
      entry->data = device;
      entry->read_proc = NULL;
      entry->write_proc = NULL;
      entry->proc_fops = NULL; //&proc_fops;
      
      // Add to list
      pci_device_list = pl;
    }
  }

  return 0;
}


static void __exit arp2_irq_exit(void) {
  if (root != NULL) { 
    struct pci_device_list* pl = pci_device_list;

    while (pl != NULL) {
      struct pci_device_list* next = pl->next;

      remove_proc_entry(pl->name, root);
      pci_dev_put(pl->pci_dev);
      kfree(pl);

      pl = next;
    }
  }

  remove_proc_entry(PROC_BASE_DIR, NULL);

  printk(ARP2_INFO "arp2_irq_exit()\n");
}

module_init(arp2_irq_init);
module_exit(arp2_irq_exit);
