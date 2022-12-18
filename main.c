/**
 * @file main.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief 
 * @version 0.1
 * @date 2022-12-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/string.h>
#include <linux/slab.h>

#include <linux/sched/mm.h>
#include <linux/mm.h>


static int pid_a = 1;
static int pid_b = 1;
module_param(pid_a, int, 0644);
module_param(pid_b, int, 0644);


static int proc_test(void)
{
	int rc;
	char *buffer;
	struct task_struct *task;
	struct mm_struct *mm;

	task = get_pid_task(find_get_pid(pid_a), PIDTYPE_PID);
	if (IS_ERR(task))
		return PTR_ERR(task);

	mm = get_task_mm(task);
	if(IS_ERR(mm))
		return PTR_ERR(mm);

	pr_info("afile: %lx",mm->mmap->vm_file);
	
	task = get_pid_task(find_get_pid(pid_b), PIDTYPE_PID);
	if (IS_ERR(task))
		return PTR_ERR(task);

	mm = get_task_mm(task);
	if(IS_ERR(mm))
		return PTR_ERR(mm);

	pr_info("bfile: %lx",mm->mmap->vm_file);

	return 0;
}

extern void example_hash_256(void);

void (*ksyms_print_modules)(void);


extern int example_access_process_memory(int vpid);
static int __init my_main(void)
{
	unsigned long addr;
	struct module *m;
	// proc_test();
	// return 0;
	example_access_process_memory(pid_a);
	pr_info("-- start access b\n");
	example_access_process_memory(pid_b);

	ksyms_init();

	pr_info("hello world\n");

	ksyms_find_module = (typeof(ksyms_find_module))ksyms_kallsyms_lookup_name("find_module");
	ksyms_stext = (typeof(ksyms_stext))ksyms_kallsyms_lookup_name("_stext");
	kysms_etext = (typeof(kysms_etext))ksyms_kallsyms_lookup_name("_etext");

	// pr_info("ksyms_find_module: %px, ksyms_stext: %px, kysms_etext: %px\n",
	// ksyms_find_module, ksyms_stext, kysms_etext);
	// example_hash_256();

	// proc_test(gpid);
	// vm_test(gpid);
	// ksyms_print_modules = (typeof(ksyms_print_modules))ksyms_kallsyms_lookup_name("print_modules");
	// ksyms_print_modules();

	pr_info("mian end\n");
	return 0;
}

static void __exit my_exit(void)
{
	pr_info("goodbye world\n");
}


module_init(my_main);
module_exit(my_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("liuqi");
MODULE_VERSION("v0.1");
MODULE_DESCRIPTION("kyrg");
