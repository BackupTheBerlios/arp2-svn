/*
 * PCI Stub Driver - Grabs devices in backend to be exported later
 *
 *   Author: Ryan Wilson <hap9@epoch.ncsc.mil>
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/kref.h>
#include <asm/atomic.h>
#include "pciback.h"

static char *pci_devs_to_hide = NULL;
module_param_named(hide, pci_devs_to_hide, charp, 0444);

struct pcistub_device_id {
	struct list_head slot_list;
	int domain;
	unsigned char bus;
	unsigned int devfn;
};
static LIST_HEAD(pcistub_device_ids);
static DEFINE_SPINLOCK(device_ids_lock);

struct pcistub_device {
	struct kref kref;
	struct list_head dev_list;
	spinlock_t lock;

	struct pci_dev *dev;
	struct pciback_device *pdev;	/* non-NULL if struct pci_dev is in use */
};
/* Access to pcistub_devices & seized_devices lists and the initialize_devices
 * flag must be locked with pcistub_devices_lock
 */
static DEFINE_SPINLOCK(pcistub_devices_lock);
static LIST_HEAD(pcistub_devices);

/* wait for device_initcall before initializing our devices
 * (see pcistub_init_devices_late)
 */
static int initialize_devices = 0;
static LIST_HEAD(seized_devices);

static struct pcistub_device *pcistub_device_alloc(struct pci_dev *dev)
{
	struct pcistub_device *psdev;

	dev_dbg(&dev->dev, "pcistub_device_alloc\n");

	psdev = kzalloc(sizeof(*psdev), GFP_ATOMIC);
	if (!psdev)
		return NULL;

	psdev->dev = pci_dev_get(dev);
	if (!psdev->dev) {
		kfree(psdev);
		return NULL;
	}

	kref_init(&psdev->kref);
	spin_lock_init(&psdev->lock);

	return psdev;
}

/* Don't call this directly as it's called by pcistub_device_put */
static void pcistub_device_release(struct kref *kref)
{
	struct pcistub_device *psdev;

	psdev = container_of(kref, struct pcistub_device, kref);

	dev_dbg(&psdev->dev->dev, "pcistub_device_release\n");

	/* Clean-up the device */
	pciback_reset_device(psdev->dev);
	pciback_config_free(psdev->dev);
	kfree(pci_get_drvdata(psdev->dev));
	pci_set_drvdata(psdev->dev, NULL);

	pci_dev_put(psdev->dev);

	kfree(psdev);
}

static inline void pcistub_device_get(struct pcistub_device *psdev)
{
	kref_get(&psdev->kref);
}

static inline void pcistub_device_put(struct pcistub_device *psdev)
{
	kref_put(&psdev->kref, pcistub_device_release);
}

static struct pci_dev *pcistub_device_get_pci_dev(struct pciback_device *pdev,
						  struct pcistub_device *psdev)
{
	struct pci_dev *pci_dev = NULL;
	unsigned long flags;

	pcistub_device_get(psdev);

	spin_lock_irqsave(&psdev->lock, flags);
	if (!psdev->pdev) {
		psdev->pdev = pdev;
		pci_dev = psdev->dev;
	}
	spin_unlock_irqrestore(&psdev->lock, flags);

	if (!pci_dev)
		pcistub_device_put(psdev);

	return pci_dev;
}

struct pci_dev *pcistub_get_pci_dev_by_slot(struct pciback_device *pdev,
					    int domain, int bus,
					    int slot, int func)
{
	struct pcistub_device *psdev;
	struct pci_dev *found_dev = NULL;
	unsigned long flags;

	spin_lock_irqsave(&pcistub_devices_lock, flags);

	list_for_each_entry(psdev, &pcistub_devices, dev_list) {
		if (psdev->dev != NULL
		    && domain == pci_domain_nr(psdev->dev->bus)
		    && bus == psdev->dev->bus->number
		    && PCI_DEVFN(slot, func) == psdev->dev->devfn) {
			found_dev = pcistub_device_get_pci_dev(pdev, psdev);
			break;
		}
	}

	spin_unlock_irqrestore(&pcistub_devices_lock, flags);
	return found_dev;
}

struct pci_dev *pcistub_get_pci_dev(struct pciback_device *pdev,
				    struct pci_dev *dev)
{
	struct pcistub_device *psdev;
	struct pci_dev *found_dev = NULL;
	unsigned long flags;

	spin_lock_irqsave(&pcistub_devices_lock, flags);

	list_for_each_entry(psdev, &pcistub_devices, dev_list) {
		if (psdev->dev == dev) {
			found_dev = pcistub_device_get_pci_dev(pdev, psdev);
			break;
		}
	}

	spin_unlock_irqrestore(&pcistub_devices_lock, flags);
	return found_dev;
}

void pcistub_put_pci_dev(struct pci_dev *dev)
{
	struct pcistub_device *psdev, *found_psdev = NULL;
	unsigned long flags;

	spin_lock_irqsave(&pcistub_devices_lock, flags);

	list_for_each_entry(psdev, &pcistub_devices, dev_list) {
		if (psdev->dev == dev) {
			found_psdev = psdev;
			break;
		}
	}

	spin_unlock_irqrestore(&pcistub_devices_lock, flags);

	/* Cleanup our device
	 * (so it's ready for the next domain)
	 */
	pciback_reset_device(found_psdev->dev);
	pciback_config_reset(found_psdev->dev);

	spin_lock_irqsave(&found_psdev->lock, flags);
	found_psdev->pdev = NULL;
	spin_unlock_irqrestore(&found_psdev->lock, flags);

	pcistub_device_put(found_psdev);
}

static int __devinit pcistub_match_one(struct pci_dev *dev,
				       struct pcistub_device_id *pdev_id)
{
	/* Match the specified device by domain, bus, slot, func and also if
	 * any of the device's parent bridges match.
	 */
	for (; dev != NULL; dev = dev->bus->self) {
		if (pci_domain_nr(dev->bus) == pdev_id->domain
		    && dev->bus->number == pdev_id->bus
		    && dev->devfn == pdev_id->devfn)
			return 1;
	}

	return 0;
}

static int __devinit pcistub_match(struct pci_dev *dev)
{
	struct pcistub_device_id *pdev_id;
	unsigned long flags;
	int found = 0;

	spin_lock_irqsave(&device_ids_lock, flags);
	list_for_each_entry(pdev_id, &pcistub_device_ids, slot_list) {
		if (pcistub_match_one(dev, pdev_id)) {
			found = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&device_ids_lock, flags);

	return found;
}

static int __devinit pcistub_init_device(struct pci_dev *dev)
{
	struct pciback_dev_data *dev_data;
	int err = 0;

	dev_dbg(&dev->dev, "initializing...\n");

	/* The PCI backend is not intended to be a module (or to work with
	 * removable PCI devices (yet). If it were, pciback_config_free()
	 * would need to be called somewhere to free the memory allocated
	 * here and then to call kfree(pci_get_drvdata(psdev->dev)).
	 */
	dev_data = kmalloc(sizeof(*dev_data), GFP_ATOMIC);
	if (!dev_data) {
		err = -ENOMEM;
		goto out;
	}
	pci_set_drvdata(dev, dev_data);

	dev_dbg(&dev->dev, "initializing config\n");
	err = pciback_config_init(dev);
	if (err)
		goto out;

	/* HACK: Force device (& ACPI) to determine what IRQ it's on - we
	 * must do this here because pcibios_enable_device may specify
	 * the pci device's true irq (and possibly its other resources)
	 * if they differ from what's in the configuration space.
	 * This makes the assumption that the device's resources won't
	 * change after this point (otherwise this code may break!)
	 */
	dev_dbg(&dev->dev, "enabling device\n");
	err = pci_enable_device(dev);
	if (err)
		goto config_release;

	/* Now disable the device (this also ensures some private device
	 * data is setup before we export)
	 */
	dev_dbg(&dev->dev, "reset device\n");
	pciback_reset_device(dev);

	return 0;

      config_release:
	pciback_config_free(dev);

      out:
	pci_set_drvdata(dev, NULL);
	kfree(dev_data);
	return err;
}

/*
 * Because some initialization still happens on
 * devices during fs_initcall, we need to defer
 * full initialization of our devices until
 * device_initcall.
 */
static int __init pcistub_init_devices_late(void)
{
	struct pcistub_device *psdev;
	unsigned long flags;
	int err = 0;

	pr_debug("pciback: pcistub_init_devices_late\n");

	spin_lock_irqsave(&pcistub_devices_lock, flags);

	while (!list_empty(&seized_devices)) {
		psdev = container_of(seized_devices.next,
				     struct pcistub_device, dev_list);
		list_del(&psdev->dev_list);

		spin_unlock_irqrestore(&pcistub_devices_lock, flags);

		err = pcistub_init_device(psdev->dev);
		if (err) {
			dev_err(&psdev->dev->dev,
				"error %d initializing device\n", err);
			kfree(psdev);
			psdev = NULL;
		}

		spin_lock_irqsave(&pcistub_devices_lock, flags);

		if (psdev)
			list_add_tail(&psdev->dev_list, &pcistub_devices);
	}

	initialize_devices = 1;

	spin_unlock_irqrestore(&pcistub_devices_lock, flags);

	return 0;
}

static int __devinit pcistub_seize(struct pci_dev *dev)
{
	struct pcistub_device *psdev;
	unsigned long flags;
	int initialize_devices_copy;
	int err = 0;

	psdev = pcistub_device_alloc(dev);
	if (!psdev)
		return -ENOMEM;

	/* initialize_devices has to be accessed under a spin lock. But since
	 * it can only change from 0 -> 1, if it's already 1, we don't have to
	 * worry about it changing. That's why we can take a *copy* of
	 * initialize_devices and wait till we're outside of the lock to
	 * check if it's 1 (don't ever check if it's 0 outside of the lock)
	 */
	spin_lock_irqsave(&pcistub_devices_lock, flags);

	initialize_devices_copy = initialize_devices;

	if (!initialize_devices_copy) {
		dev_dbg(&dev->dev, "deferring initialization\n");
		list_add(&psdev->dev_list, &seized_devices);
	}

	spin_unlock_irqrestore(&pcistub_devices_lock, flags);

	if (initialize_devices_copy) {
		/* don't want irqs disabled when calling pcistub_init_device */
		err = pcistub_init_device(psdev->dev);
		if (err)
			goto out;

		list_add(&psdev->dev_list, &pcistub_devices);
	}

      out:
	if (err)
		pcistub_device_put(psdev);

	return err;
}

static int __devinit pcistub_probe(struct pci_dev *dev,
				   const struct pci_device_id *id)
{
	int err = 0;

	dev_dbg(&dev->dev, "probing...\n");

	if (pcistub_match(dev)) {

		if (dev->hdr_type != PCI_HEADER_TYPE_NORMAL
		    && dev->hdr_type != PCI_HEADER_TYPE_BRIDGE) {
			dev_err(&dev->dev, "can't export pci devices that "
				"don't have a normal (0) or bridge (1) "
				"header type!\n");
			err = -ENODEV;
			goto out;
		}

		dev_info(&dev->dev, "seizing device\n");
		err = pcistub_seize(dev);
	} else
		/* Didn't find the device */
		err = -ENODEV;

      out:
	return err;
}

static void pcistub_remove(struct pci_dev *dev)
{
	struct pcistub_device *psdev, *found_psdev = NULL;
	unsigned long flags;

	dev_dbg(&dev->dev, "removing\n");

	spin_lock_irqsave(&pcistub_devices_lock, flags);

	list_for_each_entry(psdev, &pcistub_devices, dev_list) {
		if (psdev->dev == dev) {
			found_psdev = psdev;
			break;
		}
	}

	spin_unlock_irqrestore(&pcistub_devices_lock, flags);

	if (found_psdev) {
		dev_dbg(&dev->dev, "found device to remove - in use? %p\n",
			found_psdev->pdev);

		if (found_psdev->pdev) {
			printk(KERN_WARNING "pciback: ****** removing device "
			       "%s while still in-use! ******\n",
			       pci_name(found_psdev->dev));
			printk(KERN_WARNING "pciback: ****** driver domain may "
			       "still access this device's i/o resources!\n");
			printk(KERN_WARNING "pciback: ****** shutdown driver "
			       "domain before binding device\n");
			printk(KERN_WARNING "pciback: ****** to other drivers "
			       "or domains\n");

			pciback_release_pci_dev(found_psdev->pdev,
						found_psdev->dev);
		}

		spin_lock_irqsave(&pcistub_devices_lock, flags);
		list_del(&found_psdev->dev_list);
		spin_unlock_irqrestore(&pcistub_devices_lock, flags);

		/* the final put for releasing from the list */
		pcistub_device_put(found_psdev);
	}
}

static struct pci_device_id pcistub_ids[] = {
	{
	 .vendor = PCI_ANY_ID,
	 .device = PCI_ANY_ID,
	 .subvendor = PCI_ANY_ID,
	 .subdevice = PCI_ANY_ID,
	 },
	{0,},
};

/*
 * Note: There is no MODULE_DEVICE_TABLE entry here because this isn't
 * for a normal device. I don't want it to be loaded automatically.
 */

static struct pci_driver pciback_pci_driver = {
	.name = "pciback",
	.id_table = pcistub_ids,
	.probe = pcistub_probe,
	.remove = pcistub_remove,
};

static inline int str_to_slot(const char *buf, int *domain, int *bus,
			      int *slot, int *func)
{
	int err;

	err = sscanf(buf, " %x:%x:%x.%x", domain, bus, slot, func);
	if (err == 4)
		return 0;
	else if (err < 0)
		return -EINVAL;

	/* try again without domain */
	*domain = 0;
	err = sscanf(buf, " %x:%x.%x", bus, slot, func);
	if (err == 3)
		return 0;

	return -EINVAL;
}

static int pcistub_device_id_add(int domain, int bus, int slot, int func)
{
	struct pcistub_device_id *pci_dev_id;
	unsigned long flags;

	pci_dev_id = kmalloc(sizeof(*pci_dev_id), GFP_KERNEL);
	if (!pci_dev_id)
		return -ENOMEM;

	pci_dev_id->domain = domain;
	pci_dev_id->bus = bus;
	pci_dev_id->devfn = PCI_DEVFN(slot, func);

	pr_debug("pciback: wants to seize %04x:%02x:%02x.%01x\n",
		 domain, bus, slot, func);

	spin_lock_irqsave(&device_ids_lock, flags);
	list_add_tail(&pci_dev_id->slot_list, &pcistub_device_ids);
	spin_unlock_irqrestore(&device_ids_lock, flags);

	return 0;
}

static int pcistub_device_id_remove(int domain, int bus, int slot, int func)
{
	struct pcistub_device_id *pci_dev_id, *t;
	int devfn = PCI_DEVFN(slot, func);
	int err = -ENOENT;
	unsigned long flags;

	spin_lock_irqsave(&device_ids_lock, flags);
	list_for_each_entry_safe(pci_dev_id, t, &pcistub_device_ids, slot_list) {

		if (pci_dev_id->domain == domain
		    && pci_dev_id->bus == bus && pci_dev_id->devfn == devfn) {
			/* Don't break; here because it's possible the same
			 * slot could be in the list more than once
			 */
			list_del(&pci_dev_id->slot_list);
			kfree(pci_dev_id);

			err = 0;

			pr_debug("pciback: removed %04x:%02x:%02x.%01x from "
				 "seize list\n", domain, bus, slot, func);
		}
	}
	spin_unlock_irqrestore(&device_ids_lock, flags);

	return err;
}

static ssize_t pcistub_slot_add(struct device_driver *drv, const char *buf,
				size_t count)
{
	int domain, bus, slot, func;
	int err;

	err = str_to_slot(buf, &domain, &bus, &slot, &func);
	if (err)
		goto out;

	err = pcistub_device_id_add(domain, bus, slot, func);

      out:
	if (!err)
		err = count;
	return err;
}

DRIVER_ATTR(new_slot, S_IWUSR, NULL, pcistub_slot_add);

static ssize_t pcistub_slot_remove(struct device_driver *drv, const char *buf,
				   size_t count)
{
	int domain, bus, slot, func;
	int err;

	err = str_to_slot(buf, &domain, &bus, &slot, &func);
	if (err)
		goto out;

	err = pcistub_device_id_remove(domain, bus, slot, func);

      out:
	if (!err)
		err = count;
	return err;
}

DRIVER_ATTR(remove_slot, S_IWUSR, NULL, pcistub_slot_remove);

static ssize_t pcistub_slot_show(struct device_driver *drv, char *buf)
{
	struct pcistub_device_id *pci_dev_id;
	size_t count = 0;
	unsigned long flags;

	spin_lock_irqsave(&device_ids_lock, flags);
	list_for_each_entry(pci_dev_id, &pcistub_device_ids, slot_list) {
		if (count >= PAGE_SIZE)
			break;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "%04x:%02x:%02x.%01x\n",
				   pci_dev_id->domain, pci_dev_id->bus,
				   PCI_SLOT(pci_dev_id->devfn),
				   PCI_FUNC(pci_dev_id->devfn));
	}
	spin_unlock_irqrestore(&device_ids_lock, flags);

	return count;
}

DRIVER_ATTR(slots, S_IRUSR, pcistub_slot_show, NULL);

static int __init pcistub_init(void)
{
	int pos = 0;
	int err = 0;
	int domain, bus, slot, func;
	int parsed;

	if (pci_devs_to_hide && *pci_devs_to_hide) {
		do {
			parsed = 0;

			err = sscanf(pci_devs_to_hide + pos,
				     " (%x:%x:%x.%x) %n",
				     &domain, &bus, &slot, &func, &parsed);
			if (err != 4) {
				domain = 0;
				err = sscanf(pci_devs_to_hide + pos,
					     " (%x:%x.%x) %n",
					     &bus, &slot, &func, &parsed);
				if (err != 3)
					goto parse_error;
			}

			err = pcistub_device_id_add(domain, bus, slot, func);
			if (err)
				goto out;

			/* if parsed<=0, we've reached the end of the string */
			pos += parsed;
		} while (parsed > 0 && pci_devs_to_hide[pos]);
	}

	/* If we're the first PCI Device Driver to register, we're the
	 * first one to get offered PCI devices as they become
	 * available (and thus we can be the first to grab them)
	 */
	err = pci_register_driver(&pciback_pci_driver);
	if (err < 0)
		goto out;

	driver_create_file(&pciback_pci_driver.driver, &driver_attr_new_slot);
	driver_create_file(&pciback_pci_driver.driver,
			   &driver_attr_remove_slot);
	driver_create_file(&pciback_pci_driver.driver, &driver_attr_slots);

      out:
	return err;

      parse_error:
	printk(KERN_ERR "pciback: Error parsing pci_devs_to_hide at \"%s\"\n",
	       pci_devs_to_hide + pos);
	return -EINVAL;
}

#ifndef MODULE
/*
 * fs_initcall happens before device_initcall
 * so pciback *should* get called first (b/c we 
 * want to suck up any device before other drivers
 * get a chance by being the first pci device
 * driver to register)
 */
fs_initcall(pcistub_init);
#endif

static int __init pciback_init(void)
{
#ifdef MODULE
	int err;

	err = pcistub_init();
	if (err < 0)
		return err;
#endif

	pcistub_init_devices_late();
	pciback_xenbus_register();

	return 0;
}

static void __exit pciback_cleanup(void)
{
	pciback_xenbus_unregister();

	driver_remove_file(&pciback_pci_driver.driver, &driver_attr_new_slot);
	driver_remove_file(&pciback_pci_driver.driver,
			   &driver_attr_remove_slot);
	driver_remove_file(&pciback_pci_driver.driver, &driver_attr_slots);

	pci_unregister_driver(&pciback_pci_driver);
}

module_init(pciback_init);
module_exit(pciback_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
