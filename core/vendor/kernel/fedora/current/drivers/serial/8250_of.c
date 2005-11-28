#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <asm/serial.h>
#include <asm/prom.h>

#if 0
#define DBG(fmt...) printk(KERN_DEBUG fmt)
#else
#define DBG(fmt...) do { } while (0)
#endif

/*
 * This function can be used by platforms to "find" legacy serial ports.
 * It works for "serial" nodes under an "isa" node, and will try to
 * respect the "ibm,aix-loc" property if any. It works with up to 8
 * ports.
 */

#define MAX_LEGACY_SERIAL_PORTS	8
static int ports_probed = 0;

static struct plat_serial8250_port serial_ports[MAX_LEGACY_SERIAL_PORTS+1];
static unsigned int old_serial_count;

void __init generic_find_legacy_serial_ports(u64 *physport,
		unsigned int *default_speed)
{
	struct device_node *np;
	u32 *sizeprop;

	struct isa_reg_property {
		u32 space;
		u32 address;
		u32 size;
	};

	DBG(" -> generic_find_legacy_serial_port()\n");
	ports_probed = 1;

	*physport = 0;
	if (default_speed)
		*default_speed = 0;

	np = of_find_node_by_path("/");
	if (!np)
		return;

	/* First fill our array */
	for (np = NULL; (np = of_find_node_by_type(np, "serial"));) {
		struct device_node *isa, *pci;
		struct isa_reg_property *reg;
		unsigned long phys_size, addr_size;
		u64 io_base;
		u32 *rangesp;
		u32 *interrupts, *clk, *spd;
		char *typep;
		int index, rlen, rentsize;

		/* Ok, first check if it's under an "isa" parent */
		isa = of_get_parent(np);
		if (!isa || strcmp(isa->name, "isa")) {
			DBG("%s: no isa parent found\n", np->full_name);
			continue;
		}

		/* Now look for an "ibm,aix-loc" property that gives us ordering
		 * if any...
		 */
	 	typep = (char *)get_property(np, "ibm,aix-loc", NULL);

		/* Get the ISA port number */
		reg = (struct isa_reg_property *)get_property(np, "reg", NULL);
		if (reg == NULL)
			goto next_port;
		/* We assume the interrupt number isn't translated ... */
		interrupts = (u32 *)get_property(np, "interrupts", NULL);
		/* get clock freq. if present */
		clk = (u32 *)get_property(np, "clock-frequency", NULL);
		/* get default speed if present */
		spd = (u32 *)get_property(np, "current-speed", NULL);
		/* Default to locate at end of array */
		index = old_serial_count; /* end of the array by default */

		/* If we have a location index, then use it */
		if (typep && *typep == 'S') {
			index = simple_strtol(typep+1, NULL, 0) - 1;
			/* if index is out of range, use end of array instead */
			if (index >= MAX_LEGACY_SERIAL_PORTS)
				index = old_serial_count;
			/* if our index is still out of range, that mean that
			 * array is full, we could scan for a free slot but that
			 * make little sense to bother, just skip the port
			 */
			if (index >= MAX_LEGACY_SERIAL_PORTS)
				goto next_port;
			if (index >= old_serial_count)
				old_serial_count = index + 1;
			/* Check if there is a port who already claimed our slot */
			if (serial_ports[index].iobase != 0) {
				/* if we still have some room, move it, else override */
				if (old_serial_count < MAX_LEGACY_SERIAL_PORTS) {
					DBG("Moved legacy port %d -> %d\n", index,
					    old_serial_count);
					serial_ports[old_serial_count++] =
						serial_ports[index];
				} else {
					DBG("Replacing legacy port %d\n", index);
				}
			}
		}
		if (index >= MAX_LEGACY_SERIAL_PORTS)
			goto next_port;
		if (index >= old_serial_count)
			old_serial_count = index + 1;

		/* Now fill the entry */
		memset(&serial_ports[index], 0, sizeof(struct plat_serial8250_port));
		serial_ports[index].uartclk = (clk && *clk) ? *clk : BASE_BAUD * 16;
		serial_ports[index].iobase = reg->address;
		serial_ports[index].irq = interrupts ? interrupts[0] : 0;
		serial_ports[index].flags = ASYNC_BOOT_AUTOCONF;

		DBG("Added legacy port, index: %d, port: %x, irq: %d, clk: %d\n",
		    index,
		    serial_ports[index].iobase,
		    serial_ports[index].irq,
		    serial_ports[index].uartclk);

		/* Get phys address of IO reg for port 1 */
		if (index != 0)
			goto next_port;

		pci = of_get_parent(isa);
		if (!pci) {
			DBG("%s: no pci parent found\n", np->full_name);
			goto next_port;
		}

		rangesp = (u32 *)get_property(pci, "ranges", &rlen);
		if (rangesp == NULL) {
			of_node_put(pci);
			goto next_port;
		}
		rlen /= 4;

		/* we need the #size-cells of the PCI bridge node itself */
		phys_size = 1;
		sizeprop = (u32 *)get_property(pci, "#size-cells", NULL);
		if (sizeprop != NULL)
			phys_size = *sizeprop;
		/* we need the parent #addr-cells */
		addr_size = prom_n_addr_cells(pci);
		rentsize = 3 + addr_size + phys_size;
		io_base = 0;
		for (;rlen >= rentsize; rlen -= rentsize,rangesp += rentsize) {
			if (((rangesp[0] >> 24) & 0x3) != 1)
				continue; /* not IO space */
			io_base = rangesp[3];
			if (addr_size == 2)
				io_base = (io_base << 32) | rangesp[4];
		}
		if (io_base != 0) {
			*physport = io_base + reg->address;
			if (default_speed && spd)
				*default_speed = *spd;
		}
		of_node_put(pci);
	next_port:
		of_node_put(isa);
	}

	DBG(" <- generic_find_legacy_serial_port()\n");
}

static struct platform_device serial_device = {
	.name	= "serial8250",
	.id	= PLAT8250_DEV_PLATFORM,
	.dev	= {
		.platform_data = serial_ports,
	},
};

static int __init serial_dev_init(void)
{
	u64 phys;
	unsigned int spd;

	if (!ports_probed)
		generic_find_legacy_serial_ports(&phys, &spd);
	return platform_device_register(&serial_device);
}
arch_initcall(serial_dev_init);
