/*
 * PCI Backend Xenbus Setup - handles setup with frontend and xend
 *
 *   Author: Ryan Wilson <hap9@epoch.ncsc.mil>
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <xen/xenbus.h>
#include <xen/evtchn.h>
#include "pciback.h"

#define INVALID_EVTCHN_IRQ  (-1)

static struct pciback_device *alloc_pdev(struct xenbus_device *xdev)
{
	struct pciback_device *pdev;

	pdev = kzalloc(sizeof(struct pciback_device), GFP_KERNEL);
	if (pdev == NULL)
		goto out;
	dev_dbg(&xdev->dev, "allocated pdev @ 0x%p\n", pdev);

	pdev->xdev = xdev;
	xdev->data = pdev;

	spin_lock_init(&pdev->dev_lock);

	pdev->sh_area = NULL;
	pdev->sh_info = NULL;
	pdev->evtchn_irq = INVALID_EVTCHN_IRQ;
	pdev->be_watching = 0;

	INIT_WORK(&pdev->op_work, pciback_do_op, pdev);

	if (pciback_init_devices(pdev)) {
		kfree(pdev);
		pdev = NULL;
	}
      out:
	return pdev;
}

static void free_pdev(struct pciback_device *pdev)
{
	if (pdev->be_watching)
		unregister_xenbus_watch(&pdev->be_watch);

	/* Ensure the guest can't trigger our handler before removing devices */
	if (pdev->evtchn_irq != INVALID_EVTCHN_IRQ)
		unbind_from_irqhandler(pdev->evtchn_irq, pdev);

	/* If the driver domain started an op, make sure we complete it or
	 * delete it before releasing the shared memory */
	cancel_delayed_work(&pdev->op_work);
	flush_scheduled_work();

	if (pdev->sh_info)
		xenbus_unmap_ring_vfree(pdev->xdev, pdev->sh_area);

	pciback_release_devices(pdev);

	pdev->xdev->data = NULL;
	pdev->xdev = NULL;

	kfree(pdev);
}

static int pciback_do_attach(struct pciback_device *pdev, int gnt_ref,
			     int remote_evtchn)
{
	int err = 0;
	int evtchn;
	struct vm_struct *area;

	dev_dbg(&pdev->xdev->dev,
		"Attaching to frontend resources - gnt_ref=%d evtchn=%d\n",
		gnt_ref, remote_evtchn);

	area = xenbus_map_ring_valloc(pdev->xdev, gnt_ref);
	if (IS_ERR(area)) {
		err = PTR_ERR(area);
		goto out;
	}
	pdev->sh_area = area;
	pdev->sh_info = area->addr;

	err = xenbus_bind_evtchn(pdev->xdev, remote_evtchn, &evtchn);
	if (err)
		goto out;

	err = bind_evtchn_to_irqhandler(evtchn, pciback_handle_event,
					SA_SAMPLE_RANDOM, "pciback", pdev);
	if (err < 0) {
		xenbus_dev_fatal(pdev->xdev, err,
				 "Error binding event channel to IRQ");
		goto out;
	}
	pdev->evtchn_irq = err;
	err = 0;

	dev_dbg(&pdev->xdev->dev, "Attached!\n");
      out:
	return err;
}

static int pciback_attach(struct pciback_device *pdev)
{
	int err = 0;
	int gnt_ref, remote_evtchn;
	char *magic = NULL;

	spin_lock(&pdev->dev_lock);

	/* Make sure we only do this setup once */
	if (xenbus_read_driver_state(pdev->xdev->nodename) !=
	    XenbusStateInitialised)
		goto out;

	/* Wait for frontend to state that it has published the configuration */
	if (xenbus_read_driver_state(pdev->xdev->otherend) !=
	    XenbusStateInitialised)
		goto out;

	dev_dbg(&pdev->xdev->dev, "Reading frontend config\n");

	err = xenbus_gather(XBT_NULL, pdev->xdev->otherend,
			    "pci-op-ref", "%u", &gnt_ref,
			    "event-channel", "%u", &remote_evtchn,
			    "magic", NULL, &magic, NULL);
	if (err) {
		/* If configuration didn't get read correctly, wait longer */
		xenbus_dev_fatal(pdev->xdev, err,
				 "Error reading configuration from frontend");
		goto out;
	}

	if (magic == NULL || strcmp(magic, XEN_PCI_MAGIC) != 0) {
		xenbus_dev_fatal(pdev->xdev, -EFAULT,
				 "version mismatch (%s/%s) with pcifront - "
				 "halting pciback",
				 magic, XEN_PCI_MAGIC);
		goto out;
	}

	err = pciback_do_attach(pdev, gnt_ref, remote_evtchn);
	if (err)
		goto out;

	dev_dbg(&pdev->xdev->dev, "Connecting...\n");

	err = xenbus_switch_state(pdev->xdev, XenbusStateConnected);
	if (err)
		xenbus_dev_fatal(pdev->xdev, err,
				 "Error switching to connected state!");

	dev_dbg(&pdev->xdev->dev, "Connected? %d\n", err);
      out:
	spin_unlock(&pdev->dev_lock);

	if (magic)
		kfree(magic);

	return err;
}

static void pciback_frontend_changed(struct xenbus_device *xdev,
				     enum xenbus_state fe_state)
{
	struct pciback_device *pdev = xdev->data;

	dev_dbg(&xdev->dev, "fe state changed %d\n", fe_state);

	switch (fe_state) {
	case XenbusStateInitialised:
		pciback_attach(pdev);
		break;

	case XenbusStateClosing:
		xenbus_switch_state(xdev, XenbusStateClosing);
		break;

	case XenbusStateClosed:
		dev_dbg(&xdev->dev, "frontend is gone! unregister device\n");
		device_unregister(&xdev->dev);
		break;

	default:
		break;
	}
}

static int pciback_publish_pci_root(struct pciback_device *pdev,
				    unsigned int domain, unsigned int bus)
{
	unsigned int d, b;
	int i, root_num, len, err;
	char str[64];

	dev_dbg(&pdev->xdev->dev, "Publishing pci roots\n");

	err = xenbus_scanf(XBT_NULL, pdev->xdev->nodename,
			   "root_num", "%d", &root_num);
	if (err == 0 || err == -ENOENT)
		root_num = 0;
	else if (err < 0)
		goto out;

	/* Verify that we haven't already published this pci root */
	for (i = 0; i < root_num; i++) {
		len = snprintf(str, sizeof(str), "root-%d", i);
		if (unlikely(len >= (sizeof(str) - 1))) {
			err = -ENOMEM;
			goto out;
		}

		err = xenbus_scanf(XBT_NULL, pdev->xdev->nodename,
				   str, "%x:%x", &d, &b);
		if (err < 0)
			goto out;
		if (err != 2) {
			err = -EINVAL;
			goto out;
		}

		if (d == domain && b == bus) {
			err = 0;
			goto out;
		}
	}

	len = snprintf(str, sizeof(str), "root-%d", root_num);
	if (unlikely(len >= (sizeof(str) - 1))) {
		err = -ENOMEM;
		goto out;
	}

	dev_dbg(&pdev->xdev->dev, "writing root %d at %04x:%02x\n",
		root_num, domain, bus);

	err = xenbus_printf(XBT_NULL, pdev->xdev->nodename, str,
			    "%04x:%02x", domain, bus);
	if (err)
		goto out;

	err = xenbus_printf(XBT_NULL, pdev->xdev->nodename,
			    "root_num", "%d", (root_num + 1));

      out:
	return err;
}

static int pciback_export_device(struct pciback_device *pdev,
				 int domain, int bus, int slot, int func)
{
	struct pci_dev *dev;
	int err = 0;

	dev_dbg(&pdev->xdev->dev, "exporting dom %x bus %x slot %x func %x\n",
		domain, bus, slot, func);

	dev = pcistub_get_pci_dev_by_slot(pdev, domain, bus, slot, func);
	if (!dev) {
		err = -EINVAL;
		xenbus_dev_fatal(pdev->xdev, err,
				 "Couldn't locate PCI device "
				 "(%04x:%02x:%02x.%01x)! "
				 "perhaps already in-use?",
				 domain, bus, slot, func);
		goto out;
	}

	err = pciback_add_pci_dev(pdev, dev);
	if (err)
		goto out;

	/* TODO: It'd be nice to export a bridge and have all of its children
	 * get exported with it. This may be best done in xend (which will
	 * have to calculate resource usage anyway) but we probably want to
	 * put something in here to ensure that if a bridge gets given to a
	 * driver domain, that all devices under that bridge are not given
	 * to other driver domains (as he who controls the bridge can disable
	 * it and stop the other devices from working).
	 */
      out:
	return err;
}

static int pciback_setup_backend(struct pciback_device *pdev)
{
	/* Get configuration from xend (if available now) */
	int domain, bus, slot, func;
	int err = 0;
	int i, num_devs;
	char dev_str[64];

	spin_lock(&pdev->dev_lock);

	/* It's possible we could get the call to setup twice, so make sure
	 * we're not already connected.
	 */
	if (xenbus_read_driver_state(pdev->xdev->nodename) !=
	    XenbusStateInitWait)
		goto out;

	dev_dbg(&pdev->xdev->dev, "getting be setup\n");

	err = xenbus_scanf(XBT_NULL, pdev->xdev->nodename, "num_devs", "%d",
			   &num_devs);
	if (err != 1) {
		if (err >= 0)
			err = -EINVAL;
		xenbus_dev_fatal(pdev->xdev, err,
				 "Error reading number of devices");
		goto out;
	}

	for (i = 0; i < num_devs; i++) {
		int l = snprintf(dev_str, sizeof(dev_str), "dev-%d", i);
		if (unlikely(l >= (sizeof(dev_str) - 1))) {
			err = -ENOMEM;
			xenbus_dev_fatal(pdev->xdev, err,
					 "String overflow while reading "
					 "configuration");
			goto out;
		}

		err = xenbus_scanf(XBT_NULL, pdev->xdev->nodename, dev_str,
				   "%x:%x:%x.%x", &domain, &bus, &slot, &func);
		if (err < 0) {
			xenbus_dev_fatal(pdev->xdev, err,
					 "Error reading device configuration");
			goto out;
		}
		if (err != 4) {
			err = -EINVAL;
			xenbus_dev_fatal(pdev->xdev, err,
					 "Error parsing pci device "
					 "configuration");
			goto out;
		}

		err = pciback_export_device(pdev, domain, bus, slot, func);
		if (err)
			goto out;
	}

	err = pciback_publish_pci_roots(pdev, pciback_publish_pci_root);
	if (err) {
		xenbus_dev_fatal(pdev->xdev, err,
				 "Error while publish PCI root buses "
				 "for frontend");
		goto out;
	}

	err = xenbus_switch_state(pdev->xdev, XenbusStateInitialised);
	if (err)
		xenbus_dev_fatal(pdev->xdev, err,
				 "Error switching to initialised state!");

      out:
	spin_unlock(&pdev->dev_lock);

	if (!err)
		/* see if pcifront is already configured (if not, we'll wait) */
		pciback_attach(pdev);

	return err;
}

static void pciback_be_watch(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	struct pciback_device *pdev =
	    container_of(watch, struct pciback_device, be_watch);

	switch (xenbus_read_driver_state(pdev->xdev->nodename)) {
	case XenbusStateInitWait:
		pciback_setup_backend(pdev);
		break;

	default:
		break;
	}
}

static int pciback_xenbus_probe(struct xenbus_device *dev,
				const struct xenbus_device_id *id)
{
	int err = 0;
	struct pciback_device *pdev = alloc_pdev(dev);

	if (pdev == NULL) {
		err = -ENOMEM;
		xenbus_dev_fatal(dev, err,
				 "Error allocating pciback_device struct");
		goto out;
	}

	/* wait for xend to configure us */
	err = xenbus_switch_state(dev, XenbusStateInitWait);
	if (err)
		goto out;

	/* watch the backend node for backend configuration information */
	err = xenbus_watch_path(dev, dev->nodename, &pdev->be_watch,
				pciback_be_watch);
	if (err)
		goto out;
	pdev->be_watching = 1;

	/* We need to force a call to our callback here in case
	 * xend already configured us!
	 */
	pciback_be_watch(&pdev->be_watch, NULL, 0);

      out:
	return err;
}

static int pciback_xenbus_remove(struct xenbus_device *dev)
{
	struct pciback_device *pdev = dev->data;

	if (pdev != NULL)
		free_pdev(pdev);

	return 0;
}

static struct xenbus_device_id xenpci_ids[] = {
	{"pci"},
	{{0}},
};

static struct xenbus_driver xenbus_pciback_driver = {
	.name 			= "pciback",
	.owner 			= THIS_MODULE,
	.ids 			= xenpci_ids,
	.probe 			= pciback_xenbus_probe,
	.remove 		= pciback_xenbus_remove,
	.otherend_changed 	= pciback_frontend_changed,
};

int __init pciback_xenbus_register(void)
{
	return xenbus_register_backend(&xenbus_pciback_driver);
}

void __exit pciback_xenbus_unregister(void)
{
	xenbus_unregister_driver(&xenbus_pciback_driver);
}
