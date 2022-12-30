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

static char base_hash[HASH_SIZE];
extern const char *ksyms_stext, *kysms_etext;

static bool enable = false;

int add_rg_kernel(void)
{
    int error;
    error = hash_value(ksyms_stext, (kysms_etext - ksyms_stext), base_hash);
    if (!error)
        enable = true;
    return error;
}

void del_rg_kernel(void)
{
    // memset(base_hash, 0, HASH_SIZE);
    enable = false;
}

int init_rg_kernel(void)
{
    return 0;
}

void exit_rg_kernel(void)
{
    return;
}

int do_rg_kernel(void)
{
    int error;
    char hash[HASH_SIZE];

    if (!enable)
        return 0;

    pr_debug("guard kernel\n");

    error = hash_value(ksyms_stext, (kysms_etext - ksyms_stext), hash);
    if (error < 0)
        return error;

    if (memcmp(hash, base_hash, HASH_SIZE) != 0) {
        audit_log(audit_context(), GFP_ATOMIC, AUDIT_KYRG, "kernel has been modified");
        // pr_warn("kernel has been modified\n");
    }

    return 0;
}

ssize_t show_rg_kernel(char *buf, size_t size)
{
#define BUFFER_SIZE(total_buf_size, data_length) \
    total_buf_size > data_length ? total_buf_size - data_length : 0
    int len = 0;

    if (!enable)
        return 0;

    len += snprintf(buf + len, BUFFER_SIZE(size, len),
                    "%s\t%lx\t%lx\n",
                    "kernel",
                    (unsigned long)ksyms_stext,
                    (unsigned long)(kysms_etext - ksyms_stext));

    return len;
}