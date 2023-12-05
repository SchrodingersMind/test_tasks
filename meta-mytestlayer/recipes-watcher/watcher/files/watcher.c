/*
 * data_breakpoint.c - Sample HW Breakpoint file to watch kernel data address
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * usage: insmod data_breakpoint.ko ksym=<ksym_name>
 *
 * This file is a kernel module that places a breakpoint over ksym_name kernel
 * variable using Hardware Breakpoint register. The corresponding handler which
 * prints a backtrace is invoked every time a write operation is performed on
 * that variable.
 *
 * Copyright (C) IBM Corporation, 2009
 *
 * Author: K.Prasad <prasad@linux.vnet.ibm.com>
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/sysfs.h> 


struct perf_event * __percpu *sample_hwbp = NULL;
// struct perf_event * __percpu *sample_hrbp = NULL;
struct perf_event *event;
typedef unsigned long long ull;


static struct kobject *mymodule; 
static int myvariable = 0;
static ulong watch_address = &myvariable;
module_param(watch_address, ulong, 0);

// Breakpoint callback

static void sample_hwbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	pr_info("Hardware breakpoint triggered\n");
	dump_stack();
	pr_info("Dump stack from sample_hwbp_handler\n");
}

static int create_hw_break(ull address) 
{
	struct perf_event_attr attr;
	struct perf_event_attr attr2;
	int ret = 0;

	hw_breakpoint_init(&attr);
	// Also, address could be manually retrieved using: `sudo grep sys_call_table /proc/kallsyms`
	attr.bp_addr = address;//0xfffffaec54078; //kallsyms_lookup_name(ksym_name);
	attr.bp_len = HW_BREAKPOINT_LEN_4;
	attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
	sample_hwbp = register_wide_hw_breakpoint(&attr, sample_hwbp_handler, NULL);
	if (IS_ERR((void __force *)sample_hwbp)) {
		pr_info("create_bp: unable to create write breakpoint\n");
		ret = PTR_ERR((void __force *)sample_hwbp);
		return ret;
	}
	// hw_breakpoint_init(&attr2);
	// attr2.bp_addr = address;
	// attr2.bp_len = HW_BREAKPOINT_LEN_4;
	// attr2.bp_type = HW_BREAKPOINT_R;
	// sample_hrbp = register_wide_hw_breakpoint(&attr2, sample_hwbp_read_handler, NULL);
	// if (IS_ERR((void __force *)sample_hrbp)) {
	// 	pr_info("create_bp: unable to create read breakpoint\n");
	// 	unregister_wide_hw_breakpoint(sample_hwbp);
	// 	ret = PTR_ERR((void __force *)sample_hrbp);
	// 	return ret;
	// }
	return ret;
}
 
static ssize_t myvariable_show(struct kobject *kobj, 
                               struct kobj_attribute *attr, char *buf) 
{ 
    pr_info("Printing myvariable...\n");
    return sprintf(buf, "%d\n", myvariable);
} 
 
static ssize_t myvariable_store(struct kobject *kobj, 
                                struct kobj_attribute *attr, char *buf, 
                                size_t count) 
{ 
	pr_info("New myvariable\n");
    sscanf(buf, "%du", &myvariable);
    return count; 
}

static ssize_t watch_address_show(struct kobject *kobj, 
                               struct kobj_attribute *attr, char *buf) 
{ 
    return sprintf(buf, "%lx\n", watch_address);
}
static ssize_t watch_address_store(struct kobject *kobj, 
                                struct kobj_attribute *attr, char *buf, 
                                size_t count) 
{ 
    sscanf(buf, "%lx", &watch_address);
	pr_info("Receiving new value..\n");
    if (sample_hwbp) {
    	unregister_wide_hw_breakpoint(sample_hwbp);
    	// unregister_wide_hw_breakpoint(sample_hrbp);
    }

    if (!create_hw_break(watch_address)) {
    	pr_info("Breakpoint for %lx installed\n", watch_address);
    } else {
    	sample_hwbp = 0;
	    pr_info("Breakpoint for %lx not installed\n", watch_address);
    }
    return count; 
}
 
static struct kobj_attribute myvariable_attribute = 
    __ATTR(myvariable, 0660, myvariable_show, (void *)myvariable_store);
static struct kobj_attribute watch_address_attribute = 
    __ATTR(watch_address, 0660, watch_address_show, (void *)watch_address_store);





static int __init hw_break_module_init(void)
{
	int ret;
	pr_info("mymodule: initialised\n"); 

	ret = create_hw_break(watch_address);
	if (ret)
		goto fail;


    mymodule = kobject_create_and_add("mymodule", kernel_kobj); 
    if (!mymodule) { 
    	pr_info("Unable to create kobject mymodule\n");
        ret = -ENOMEM;
    	goto fail;
    }

    ret = sysfs_create_file(mymodule, &myvariable_attribute.attr);
    if (ret) { 
        pr_info("failed to create the myvariable file " 
                "in /sys/kernel/mymodule\n");
        goto fail;
    }

    ret = sysfs_create_file(mymodule, &watch_address_attribute.attr);
    if (ret) { 
        pr_info("failed to create the watch_address file " 
                "in /sys/kernel/mymodule\n");
        goto fail;
    }

	printk(KERN_INFO "HW Breakpoint installed\n");
	return 0;
fail:
	printk(KERN_INFO "Breakpoint registration failed\n");
	return ret;
}

static void __exit hw_break_module_exit(void)
{
	if (sample_hwbp) 
		unregister_wide_hw_breakpoint(sample_hwbp);
	kobject_put(mymodule);
	printk(KERN_INFO "HW Breakpoint uninstalled and module removed\n");
}

module_init(hw_break_module_init);
module_exit(hw_break_module_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hardware watch point");