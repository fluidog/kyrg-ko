/**
 * @file kallsyms_lookup_name.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief kallsyms_lookup_name implement by kprobe.
 * @version 0.1
 * @date 2022-12-14
 *
 *
 * @copyright Copyright (c) 2022
 *
 */
#define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/kprobes.h>

const char *ksyms_stext, *kysms_etext;
struct module *(*ksyms_find_module)(const char *name);

unsigned long ksyms_kallsyms_lookup_name(const char *name)
{
    struct kprobe kp;
    static unsigned long (*_kallsyms_lookup_name)(const char *name) = NULL;
    if (_kallsyms_lookup_name)
        return _kallsyms_lookup_name(name);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    _kallsyms_lookup_name = kallsyms_lookup_name;
#else
    kp = (struct kprobe){
        .symbol_name = "kallsyms_lookup_name",
    };

    if (register_kprobe(&kp) < 0) {
        pr_err("kallsyms_lookup_name not found by kprobe");
        return -ENXIO;
    }

    _kallsyms_lookup_name = (typeof(_kallsyms_lookup_name))kp.addr;
    unregister_kprobe(&kp);
#endif

    return _kallsyms_lookup_name(name);
}

int ksyms_init(void)
{
    ksyms_find_module = (typeof(ksyms_find_module))ksyms_kallsyms_lookup_name("find_module");
    ksyms_stext = (typeof(ksyms_stext))ksyms_kallsyms_lookup_name("_stext");
    kysms_etext = (typeof(kysms_etext))ksyms_kallsyms_lookup_name("_etext");

    if (IS_ERR_OR_NULL(ksyms_find_module) || IS_ERR_OR_NULL(ksyms_stext) || IS_ERR_OR_NULL(kysms_etext)) {
        pr_err("ksyms not found");
        return -ENXIO;
    }

    pr_info("All ksyms init success\n");

    return 0;
}
