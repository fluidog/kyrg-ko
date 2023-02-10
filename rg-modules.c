/**
 * @file rg-modules.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief
 * @version 0.1
 * @date 2022-12-18
 *
 * @copyright Copyright (c) 2022
 *
 */
// #define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/audit.h>
#include <linux/string.h>


struct rg_module {
    struct list_head list;
    const char *name;
    void *base;
    unsigned long text_size;
    unsigned long ro_size;
    bool modified;
    char base_hash[HASH_SIZE];
};

static LIST_HEAD(rg_modules);

int add_rg_module(const char *name)
{
    struct rg_module *rg_module;
    struct module *module;

    list_for_each_entry(rg_module, &rg_modules, list)
    {
        if (strcmp(rg_module->name, name) == 0) { // already added
            return 0;
        }
    }

    module = ksyms_find_module(name);
    if (IS_ERR_OR_NULL(module)) {
        pr_err("module %s not found\n", name);
        return -ENOENT;
    }

    rg_module = kmalloc(sizeof(struct rg_module), GFP_KERNEL);
    if (IS_ERR(rg_module))
        return PTR_ERR(rg_module);

    rg_module->name = kstrdup(name, GFP_KERNEL);
    rg_module->base = module->core_layout.base;
    rg_module->modified = 0;
    rg_module->text_size = module->core_layout.text_size;
    rg_module->ro_size = module->core_layout.ro_size;

    hash_value(rg_module->base, rg_module->text_size, rg_module->base_hash);

    pr_debug("name:%s, base: %lx, text:%lx", rg_module->name, (unsigned long)rg_module->base, rg_module->text_size);

    list_add(&rg_module->list, &rg_modules);

    return 0;
}

void del_rg_module(const char *name)
{
    struct rg_module *rg_module, *tmp;

    list_for_each_entry_safe(rg_module, tmp, &rg_modules, list)
    {
        if (strcmp(rg_module->name, name) == 0) {
            list_del(&rg_module->list);
            kfree(rg_module);
        }
    }
}

int do_rg_modules(void)
{
    struct rg_module *rg_module, *tmp;
    char hash[HASH_SIZE];
    struct module *module;

    list_for_each_entry_safe(rg_module, tmp, &rg_modules, list)
    {
        pr_debug("guard module: %s\n", rg_module->name);
        rg_module->modified = 0;

        module = ksyms_find_module(rg_module->name);
        if (IS_ERR_OR_NULL(module)) {
            pr_warn("module: %s has been removed\n", rg_module->name);
            del_rg_module(rg_module->name);
            continue;
        }

        hash_value(rg_module->base, rg_module->text_size, hash);

        if (memcmp(hash, rg_module->base_hash, HASH_SIZE) != 0) {
            rg_module->modified = 1;
            audit_log(audit_context(), GFP_ATOMIC, AUDIT_KYRG, 
                "module: %s has been modified\n", rg_module->name);
            // pr_warn("module: %s has been modified\n", rg_module->name);
            continue;
        }
    }

    return 0;
}

ssize_t show_rg_modules(char *buf, size_t size)
{
    struct rg_module *module;
    int len = 0;

    list_for_each_entry(module, &rg_modules, list)
    {
        len += snprintf(buf + len, BUFFER_SIZE(size, len),
                        "%lx-%lx\t%d\t%s\n",
                        (unsigned long)module->base,
                        (unsigned long)module->base+(unsigned long)module->text_size,
                        module->modified,
                        module->name);
    }
    return len;
}

int init_rg_modules(void)
{
    return 0;
}

void exit_rg_modules(void)
{
    struct rg_module *module, *module_tmp;

    list_for_each_entry_safe(module, module_tmp, &rg_modules, list)
    {
        list_del(&module->list);
        kfree(module);
    }
}
