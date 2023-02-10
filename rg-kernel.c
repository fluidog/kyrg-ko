/**
 * @file rg-kernel.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief
 * @version 0.1
 * @date 2022-12-22
 *
 * @copyright Copyright (c) 2022
 *
 */
// #define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/audit.h>

static struct rg_kernel{
    bool enable;
    char base_hash[HASH_SIZE];
    const char *stext;
    const char *etext;
    bool modified;
} *rg_kernel;


int add_rg_kernel(void)
{
    int error;
    error = hash_value(rg_kernel->stext, (rg_kernel->etext - rg_kernel->stext), rg_kernel->base_hash);
    if (!error){
        rg_kernel->enable = true;
        rg_kernel->modified = false;
    }
    return error;
}

void del_rg_kernel(void)
{
    // memset(base_hash, 0, HASH_SIZE);
    rg_kernel->enable = false;
}

int init_rg_kernel(void)
{
    rg_kernel = kmalloc(sizeof(struct rg_kernel), GFP_KERNEL);
    if (IS_ERR(rg_kernel))
        return PTR_ERR(rg_kernel);

    rg_kernel->enable = false;
    rg_kernel->stext = ksyms_stext;
    rg_kernel->etext = ksyms_etext;
    rg_kernel->modified = false;
    return 0;
}

void exit_rg_kernel(void)
{
    kfree(rg_kernel);
}

int do_rg_kernel(void)
{
    int error;
    char hash[HASH_SIZE];

    if (!rg_kernel->enable)
        return 0;

    pr_debug("guard kernel\n");
    rg_kernel->modified = false;

    error = hash_value(rg_kernel->stext, (rg_kernel->etext - rg_kernel->stext), hash);;
    if (error < 0)
        return error;

    if (memcmp(hash, rg_kernel->base_hash, HASH_SIZE) != 0) {
        rg_kernel->modified = true;
        audit_log(audit_context(), GFP_ATOMIC, AUDIT_KYRG, "kernel has been modified");
        // pr_warn("kernel has been modified\n");
    }

    return 0;
}

ssize_t show_rg_kernel(char *buf, size_t size)
{
    int len = 0;

    if (!rg_kernel->enable)
        return 0;

    len += snprintf(buf + len, BUFFER_SIZE(size, len),
                    "%lx-%lx\t%d\t%s\n",
                    (unsigned long)rg_kernel->stext,
                    (unsigned long)rg_kernel->etext,
                    rg_kernel->modified,
                    "kernel");

    return len;
}