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
#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/module.h>

struct rg_module{
    struct list_head list;
    const char *name;
    char base_hash[HASH_SIZE];
};

static LIST_HEAD(rg_modules);

int add_rg_module(const char *name)
{
    struct rg_module *rg_module;
    struct module *module;
    void *base;
    unsigned int text_size, ro_size;
    
    module = ksyms_find_module(name);
    if (IS_ERR_OR_NULL(module)){
        pr_err("module %s not found", name);
        return -ENOENT;
    }

    __module_get(module);

    rg_module = kmalloc(sizeof(struct rg_module), GFP_KERNEL);
    if (IS_ERR(rg_module))
        return PTR_ERR(rg_module);

    rg_module->name = name;
    
    base = module->__module_layout_align.base;
    text_size = module->__module_layout_align.text_size;
    ro_size = module->__module_layout_align.ro_size;

    hash_value(base, text_size, rg_module->base_hash)

    list_add(&rg_module->list, &rg_modules);

    module_put(module);

    return 0;

}

void del_rg_module(const char *name)
{
    struct rg_module *rg_module, *tmp;

    list_for_each_entry_safe(rg_module, tmp, &rg_modules, list)
    {
        if (strcmp(rg_module->name, name) == 0)
        {
            list_del(&rg_module->list);
            kfree(rg_module);
        }
    }
}

int do_rg_modules(void)
{
    struct rg_module *rg_module, *tmp;
    char hash[HASH_SIZE];
    void *base;
    unsigned int text_size, ro_size;
    struct module *module;

    list_for_each_entry_safe(rg_module, tmp, &rg_modules, list)
    {
        module = ksyms_find_module(rg_module->name);
        if (IS_ERR_OR_NULL(module)){
            pr_warn("module %s is removed", rg_module->name);
            continue;
        }

        __module_get(module);

        base = module->__module_layout_align.base;
        text_size = module->__module_layout_align.text_size;
        ro_size = module->__module_layout_align.ro_size;
        hash_value(base, text_size, hash);

        if (memcmp(hash, rg_module->base_hash, HASH_SIZE) != 0)
        {
            pr_warn("module %s has been modified", rg_module->name);
            continue;
        }

        module_put(module);
    }

    return 0;

}



