/*
 *  linux/arch/m32r/kernel/setup_m32104ut.c
 *
 *  Setup routines for M32104UT Board
 *
 *  Copyright (c) 2002-2005  Hiroyuki Kondo, Hirokazu Takata,
 *                           Hitoshi Yamamoto, Mamoru Sakugawa,
 *                           Naoto Sugai, Hayato Fujiwara
 */

#include <linux/config.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>

#include <asm/system.h>
#include <asm/m32r.h>
#include <asm/io.h>

#define irq2port(x) (M32R_ICU_CR1_PORTL + ((x - 1) * sizeof(unsigned long)))

icu_data_t icu_data[NR_IRQS];

static void disable_m32104ut_irq(unsigned int irq)
{
	unsigned long port, data;

	port = irq2port(irq);
	data = icu_data[irq].icucr|M32R_ICUCR_ILEVEL7;
	outl(data, port);
}

static void enable_m32104ut_irq(unsigned int irq)
{
	unsigned long port, data;

	port = irq2port(irq);
	data = icu_data[irq].icucr|M32R_ICUCR_IEN|M32R_ICUCR_ILEVEL6;
	outl(data, port);
}

static void mask_and_ack_m32104ut(unsigned int irq)
{
	disable_m32104ut_irq(irq);
}

static void end_m32104ut_irq(unsigned int irq)
{
	enable_m32104ut_irq(irq);
}

static unsigned int startup_m32104ut_irq(unsigned int irq)
{
	enable_m32104ut_irq(irq);
	return (0);
}

static void shutdown_m32104ut_irq(unsigned int irq)
{
	unsigned long port;

	port = irq2port(irq);
	outl(M32R_ICUCR_ILEVEL7, port);
}

static struct hw_interrupt_type m32104ut_irq_type =
{
	.typename = "M32104UT-IRQ",
	.startup = startup_m32104ut_irq,
	.shutdown = shutdown_m32104ut_irq,
	.enable = enable_m32104ut_irq,
	.disable = disable_m32104ut_irq,
	.ack = mask_and_ack_m32104ut,
	.end = end_m32104ut_irq
};

void __init init_IRQ(void)
{
	static int once = 0;

	if (once)
		return;
	else
		once++;

#if defined(CONFIG_SMC91X)
	/* INT#0: LAN controller on M32104UT-LAN (SMC91C111)*/
	irq_desc[M32R_IRQ_INT0].status = IRQ_DISABLED;
	irq_desc[M32R_IRQ_INT0].handler = &m32104ut_irq_type;
	irq_desc[M32R_IRQ_INT0].action = 0;
	irq_desc[M32R_IRQ_INT0].depth = 1;
	icu_data[M32R_IRQ_INT0].icucr = M32R_ICUCR_IEN | M32R_ICUCR_ISMOD11; /* "H" level sense */
	disable_m32104ut_irq(M32R_IRQ_INT0);
#endif  /* CONFIG_SMC91X */

	/* MFT2 : system timer */
	irq_desc[M32R_IRQ_MFT2].status = IRQ_DISABLED;
	irq_desc[M32R_IRQ_MFT2].handler = &m32104ut_irq_type;
	irq_desc[M32R_IRQ_MFT2].action = 0;
	irq_desc[M32R_IRQ_MFT2].depth = 1;
	icu_data[M32R_IRQ_MFT2].icucr = M32R_ICUCR_IEN;
	disable_m32104ut_irq(M32R_IRQ_MFT2);

#ifdef CONFIG_SERIAL_M32R_SIO
	/* SIO0_R : uart receive data */
	irq_desc[M32R_IRQ_SIO0_R].status = IRQ_DISABLED;
	irq_desc[M32R_IRQ_SIO0_R].handler = &m32104ut_irq_type;
	irq_desc[M32R_IRQ_SIO0_R].action = 0;
	irq_desc[M32R_IRQ_SIO0_R].depth = 1;
	icu_data[M32R_IRQ_SIO0_R].icucr = M32R_ICUCR_IEN;
	disable_m32104ut_irq(M32R_IRQ_SIO0_R);

	/* SIO0_S : uart send data */
	irq_desc[M32R_IRQ_SIO0_S].status = IRQ_DISABLED;
	irq_desc[M32R_IRQ_SIO0_S].handler = &m32104ut_irq_type;
	irq_desc[M32R_IRQ_SIO0_S].action = 0;
	irq_desc[M32R_IRQ_SIO0_S].depth = 1;
	icu_data[M32R_IRQ_SIO0_S].icucr = M32R_ICUCR_IEN;
	disable_m32104ut_irq(M32R_IRQ_SIO0_S);
#endif /* CONFIG_SERIAL_M32R_SIO */
}

#if defined(CONFIG_SMC91X)

#define LAN_IOSTART     0x300
#define LAN_IOEND       0x320
static struct resource smc91x_resources[] = {
	[0] = {
		.start  = (LAN_IOSTART),
		.end    = (LAN_IOEND),
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = M32R_IRQ_INT0,
		.end    = M32R_IRQ_INT0,
		.flags  = IORESOURCE_IRQ,
	}
};

static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};
#endif

static int __init platform_init(void)
{
#if defined(CONFIG_SMC91X)
	platform_device_register(&smc91x_device);
#endif
	return 0;
}
arch_initcall(platform_init);
