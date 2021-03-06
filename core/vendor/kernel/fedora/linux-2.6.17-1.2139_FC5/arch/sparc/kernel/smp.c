/* smp.c: Sparc SMP support.
 *
 * Copyright (C) 1996 David S. Miller (davem@caip.rutgers.edu)
 * Copyright (C) 1998 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 * Copyright (C) 2004 Keith M Wesolowski (wesolows@foobazco.org)
 */

#include <asm/head.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/threads.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/cache.h>
#include <linux/delay.h>

#include <asm/ptrace.h>
#include <asm/atomic.h>

#include <asm/irq.h>
#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/oplib.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/cpudata.h>

volatile int smp_processors_ready = 0;
int smp_num_cpus = 1;
volatile unsigned long cpu_callin_map[NR_CPUS] __initdata = {0,};
unsigned char boot_cpu_id = 0;
unsigned char boot_cpu_id4 = 0; /* boot_cpu_id << 2 */
int smp_activated = 0;
volatile int __cpu_number_map[NR_CPUS];
volatile int __cpu_logical_map[NR_CPUS];

cpumask_t cpu_online_map = CPU_MASK_NONE;
cpumask_t phys_cpu_present_map = CPU_MASK_NONE;
cpumask_t smp_commenced_mask = CPU_MASK_NONE;

/* The only guaranteed locking primitive available on all Sparc
 * processors is 'ldstub [%reg + immediate], %dest_reg' which atomically
 * places the current byte at the effective address into dest_reg and
 * places 0xff there afterwards.  Pretty lame locking primitive
 * compared to the Alpha and the Intel no?  Most Sparcs have 'swap'
 * instruction which is much better...
 */

/* Used to make bitops atomic */
unsigned char bitops_spinlock = 0;

void __init smp_store_cpu_info(int id)
{
	int cpu_node;

	cpu_data(id).udelay_val = loops_per_jiffy;

	cpu_find_by_mid(id, &cpu_node);
	cpu_data(id).clock_tick = prom_getintdefault(cpu_node,
						     "clock-frequency", 0);
	cpu_data(id).prom_node = cpu_node;
	cpu_data(id).mid = cpu_get_hwmid(cpu_node);

	/* this is required to tune the scheduler correctly */
	/* is it possible to have CPUs with different cache sizes? */
	if (id == boot_cpu_id) {
		int cache_line,cache_nlines;
		cache_line = 0x20;
		cache_line = prom_getintdefault(cpu_node, "ecache-line-size", cache_line);
		cache_nlines = 0x8000;
		cache_nlines = prom_getintdefault(cpu_node, "ecache-nlines", cache_nlines);
		max_cache_size = cache_line * cache_nlines;
	}
	if (cpu_data(id).mid < 0)
		panic("No MID found for CPU%d at node 0x%08d", id, cpu_node);
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	extern void smp4m_smp_done(void);
	unsigned long bogosum = 0;
	int cpu, num;

	for (cpu = 0, num = 0; cpu < NR_CPUS; cpu++)
		if (cpu_online(cpu)) {
			num++;
			bogosum += cpu_data(cpu).udelay_val;
		}

	printk("Total of %d processors activated (%lu.%02lu BogoMIPS).\n",
		num, bogosum/(500000/HZ),
		(bogosum/(5000/HZ))%100);

	BUG_ON(sparc_cpu_model != sun4m);
	smp4m_smp_done();
}

void cpu_panic(void)
{
	printk("CPU[%d]: Returns from cpu_idle!\n", smp_processor_id());
	panic("SMP bolixed\n");
}

struct linux_prom_registers smp_penguin_ctable __initdata = { 0 };

void smp_send_reschedule(int cpu)
{
	/* See sparc64 */
}

void smp_send_stop(void)
{
}

void smp_flush_cache_all(void)
{
	xc0((smpfunc_t) BTFIXUP_CALL(local_flush_cache_all));
	local_flush_cache_all();
}

void smp_flush_tlb_all(void)
{
	xc0((smpfunc_t) BTFIXUP_CALL(local_flush_tlb_all));
	local_flush_tlb_all();
}

void smp_flush_cache_mm(struct mm_struct *mm)
{
	if(mm->context != NO_CONTEXT) {
		cpumask_t cpu_mask = mm->cpu_vm_mask;
		cpu_clear(smp_processor_id(), cpu_mask);
		if (!cpus_empty(cpu_mask))
			xc1((smpfunc_t) BTFIXUP_CALL(local_flush_cache_mm), (unsigned long) mm);
		local_flush_cache_mm(mm);
	}
}

void smp_flush_tlb_mm(struct mm_struct *mm)
{
	if(mm->context != NO_CONTEXT) {
		cpumask_t cpu_mask = mm->cpu_vm_mask;
		cpu_clear(smp_processor_id(), cpu_mask);
		if (!cpus_empty(cpu_mask)) {
			xc1((smpfunc_t) BTFIXUP_CALL(local_flush_tlb_mm), (unsigned long) mm);
			if(atomic_read(&mm->mm_users) == 1 && current->active_mm == mm)
				mm->cpu_vm_mask = cpumask_of_cpu(smp_processor_id());
		}
		local_flush_tlb_mm(mm);
	}
}

void smp_flush_cache_range(struct vm_area_struct *vma, unsigned long start,
			   unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (mm->context != NO_CONTEXT) {
		cpumask_t cpu_mask = mm->cpu_vm_mask;
		cpu_clear(smp_processor_id(), cpu_mask);
		if (!cpus_empty(cpu_mask))
			xc3((smpfunc_t) BTFIXUP_CALL(local_flush_cache_range), (unsigned long) vma, start, end);
		local_flush_cache_range(vma, start, end);
	}
}

void smp_flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
			 unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (mm->context != NO_CONTEXT) {
		cpumask_t cpu_mask = mm->cpu_vm_mask;
		cpu_clear(smp_processor_id(), cpu_mask);
		if (!cpus_empty(cpu_mask))
			xc3((smpfunc_t) BTFIXUP_CALL(local_flush_tlb_range), (unsigned long) vma, start, end);
		local_flush_tlb_range(vma, start, end);
	}
}

void smp_flush_cache_page(struct vm_area_struct *vma, unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;

	if(mm->context != NO_CONTEXT) {
		cpumask_t cpu_mask = mm->cpu_vm_mask;
		cpu_clear(smp_processor_id(), cpu_mask);
		if (!cpus_empty(cpu_mask))
			xc2((smpfunc_t) BTFIXUP_CALL(local_flush_cache_page), (unsigned long) vma, page);
		local_flush_cache_page(vma, page);
	}
}

void smp_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;

	if(mm->context != NO_CONTEXT) {
		cpumask_t cpu_mask = mm->cpu_vm_mask;
		cpu_clear(smp_processor_id(), cpu_mask);
		if (!cpus_empty(cpu_mask))
			xc2((smpfunc_t) BTFIXUP_CALL(local_flush_tlb_page), (unsigned long) vma, page);
		local_flush_tlb_page(vma, page);
	}
}

void smp_reschedule_irq(void)
{
	set_need_resched();
}

void smp_flush_page_to_ram(unsigned long page)
{
	/* Current theory is that those who call this are the one's
	 * who have just dirtied their cache with the pages contents
	 * in kernel space, therefore we only run this on local cpu.
	 *
	 * XXX This experiment failed, research further... -DaveM
	 */
#if 1
	xc1((smpfunc_t) BTFIXUP_CALL(local_flush_page_to_ram), page);
#endif
	local_flush_page_to_ram(page);
}

void smp_flush_sig_insns(struct mm_struct *mm, unsigned long insn_addr)
{
	cpumask_t cpu_mask = mm->cpu_vm_mask;
	cpu_clear(smp_processor_id(), cpu_mask);
	if (!cpus_empty(cpu_mask))
		xc2((smpfunc_t) BTFIXUP_CALL(local_flush_sig_insns), (unsigned long) mm, insn_addr);
	local_flush_sig_insns(mm, insn_addr);
}

extern unsigned int lvl14_resolution;

/* /proc/profile writes can call this, don't __init it please. */
static DEFINE_SPINLOCK(prof_setup_lock);

int setup_profiling_timer(unsigned int multiplier)
{
	int i;
	unsigned long flags;

	/* Prevent level14 ticker IRQ flooding. */
	if((!multiplier) || (lvl14_resolution / multiplier) < 500)
		return -EINVAL;

	spin_lock_irqsave(&prof_setup_lock, flags);
	for_each_possible_cpu(i) {
		load_profile_irq(i, lvl14_resolution / multiplier);
		prof_multiplier(i) = multiplier;
	}
	spin_unlock_irqrestore(&prof_setup_lock, flags);

	return 0;
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	extern void smp4m_boot_cpus(void);
	int i, cpuid, ncpus, extra;

	BUG_ON(sparc_cpu_model != sun4m);
	printk("Entering SMP Mode...\n");

	ncpus = 1;
	extra = 0;
	for (i = 0; !cpu_find_by_instance(i, NULL, &cpuid); i++) {
		if (cpuid == boot_cpu_id)
			continue;
		if (cpuid < NR_CPUS && ncpus++ < max_cpus)
			cpu_set(cpuid, phys_cpu_present_map);
		else
			extra++;
	}
	if (max_cpus >= NR_CPUS && extra)
		printk("Warning: NR_CPUS is too low to start all cpus\n");

	smp_store_cpu_info(boot_cpu_id);

	smp4m_boot_cpus();
}

void __devinit smp_prepare_boot_cpu(void)
{
	int cpuid = hard_smp_processor_id();

	if (cpuid >= NR_CPUS) {
		prom_printf("Serious problem, boot cpu id >= NR_CPUS\n");
		prom_halt();
	}
	if (cpuid != 0)
		printk("boot cpu id != 0, this could work but is untested\n");

	current_thread_info()->cpu = cpuid;
	cpu_set(cpuid, cpu_online_map);
	cpu_set(cpuid, phys_cpu_present_map);
}

int __devinit __cpu_up(unsigned int cpu)
{
	extern int smp4m_boot_one_cpu(int);
	int ret;

	ret = smp4m_boot_one_cpu(cpu);

	if (!ret) {
		cpu_set(cpu, smp_commenced_mask);
		while (!cpu_online(cpu))
			mb();
	}
	return ret;
}

void smp_bogo(struct seq_file *m)
{
	int i;
	
	for_each_online_cpu(i) {
		seq_printf(m,
			   "Cpu%dBogo\t: %lu.%02lu\n",
			   i,
			   cpu_data(i).udelay_val/(500000/HZ),
			   (cpu_data(i).udelay_val/(5000/HZ))%100);
	}
}

void smp_info(struct seq_file *m)
{
	int i;

	seq_printf(m, "State:\n");
	for_each_online_cpu(i)
		seq_printf(m, "CPU%d\t\t: online\n", i);
}
