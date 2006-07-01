/**
 * @file oprofile_files.c
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 *
 * Modified by Aravind Menon for Xen
 * These modifications are:
 * Copyright (C) 2005 Hewlett-Packard Co.	
 */

#include <linux/fs.h>
#include <linux/oprofile.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>

#include "event_buffer.h"
#include "oprofile_stats.h"
#include "oprof.h"

unsigned long fs_buffer_size = 131072;
unsigned long fs_cpu_buffer_size = 8192;
unsigned long fs_buffer_watershed = 32768; /* FIXME: tune */

static ssize_t depth_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_ulong_to_user(backtrace_depth, buf, count, offset);
}


static ssize_t depth_write(struct file * file, char const __user * buf, size_t count, loff_t * offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;

	retval = oprofile_set_backtrace(val);

	if (retval)
		return retval;
	return count;
}


static struct file_operations depth_fops = {
	.read		= depth_read,
	.write		= depth_write
};

 
static ssize_t pointer_size_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_ulong_to_user(sizeof(void *), buf, count, offset);
}


static struct file_operations pointer_size_fops = {
	.read		= pointer_size_read,
};


static ssize_t cpu_type_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_str_to_user(oprofile_ops.cpu_type, buf, count, offset);
}
 
 
static struct file_operations cpu_type_fops = {
	.read		= cpu_type_read,
};
 
 
static ssize_t enable_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_ulong_to_user(oprofile_started, buf, count, offset);
}


static ssize_t enable_write(struct file * file, char const __user * buf, size_t count, loff_t * offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;
 
	if (val)
		retval = oprofile_start();
	else
		oprofile_stop();

	if (retval)
		return retval;
	return count;
}

 
static struct file_operations enable_fops = {
	.read		= enable_read,
	.write		= enable_write,
};


static ssize_t dump_write(struct file * file, char const __user * buf, size_t count, loff_t * offset)
{
	wake_up_buffer_waiter();
	return count;
}


static struct file_operations dump_fops = {
	.write		= dump_write,
};

#ifdef CONFIG_XEN
#define TMPBUFSIZE 512

unsigned int adomains = 0;
long active_domains[MAX_OPROF_DOMAINS];

static ssize_t adomain_write(struct file * file, char const __user * buf, 
			     size_t count, loff_t * offset)
{
	char tmpbuf[TMPBUFSIZE];
	char * startp = tmpbuf;
	char * endp = tmpbuf;
	int i;
	unsigned long val;
	
	if (*offset)
		return -EINVAL;	
	if (!count)
		return 0;
	if (count > TMPBUFSIZE - 1)
		return -EINVAL;

	memset(tmpbuf, 0x0, TMPBUFSIZE);

	if (copy_from_user(tmpbuf, buf, count))
		return -EFAULT;
	
	for (i = 0; i < MAX_OPROF_DOMAINS; i++)
		active_domains[i] = -1;
	adomains = 0;

	while (1) {
		val = simple_strtol(startp, &endp, 0);
		if (endp == startp)
			break;
		while (ispunct(*endp))
			endp++;
		active_domains[adomains++] = val;
		if (adomains >= MAX_OPROF_DOMAINS)
			break;
		startp = endp;
	}
	if (oprofile_set_active())
		return -EINVAL; 
	return count;
}

static ssize_t adomain_read(struct file * file, char __user * buf, 
			    size_t count, loff_t * offset)
{
	char tmpbuf[TMPBUFSIZE];
	size_t len = 0;
	int i;
	/* This is all screwed up if we run out of space */
	for (i = 0; i < adomains; i++) 
		len += snprintf(tmpbuf + len, TMPBUFSIZE - len, 
				"%u ", (unsigned int)active_domains[i]);
	len += snprintf(tmpbuf + len, TMPBUFSIZE - len, "\n");
	return simple_read_from_buffer((void __user *)buf, count, 
				       offset, tmpbuf, len);
}


static struct file_operations active_domain_ops = {
	.read		= adomain_read,
	.write		= adomain_write,
};
#endif /* CONFIG_XEN */

void oprofile_create_files(struct super_block * sb, struct dentry * root)
{
	oprofilefs_create_file(sb, root, "enable", &enable_fops);
	oprofilefs_create_file_perm(sb, root, "dump", &dump_fops, 0666);
#ifdef CONFIG_XEN
	oprofilefs_create_file(sb, root, "active_domains", &active_domain_ops);
#endif
	oprofilefs_create_file(sb, root, "buffer", &event_buffer_fops);
	oprofilefs_create_ulong(sb, root, "buffer_size", &fs_buffer_size);
	oprofilefs_create_ulong(sb, root, "buffer_watershed", &fs_buffer_watershed);
	oprofilefs_create_ulong(sb, root, "cpu_buffer_size", &fs_cpu_buffer_size);
	oprofilefs_create_file(sb, root, "cpu_type", &cpu_type_fops); 
	oprofilefs_create_file(sb, root, "backtrace_depth", &depth_fops);
	oprofilefs_create_file(sb, root, "pointer_size", &pointer_size_fops);
	oprofile_create_stats_files(sb, root);
	if (oprofile_ops.create_files)
		oprofile_ops.create_files(sb, root);
}
