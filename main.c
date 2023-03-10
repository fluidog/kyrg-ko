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
// #define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/module.h>

static int __init kyrg_register(void)
{
    int error;

    error = ksyms_init();
    if (error < 0)
        return error;

    error = hash_alg_init("sha256");
    if (error < 0)
        return error;

    error = init_fs();
    if (error < 0)
        return error;

    error = init_rg();
    if (error < 0)
        return error;

    return 0;
}

static void __exit kyrg_unregister(void)
{
    exit_fs();

    hash_alg_exit();

    exit_rg();
}

#ifdef MODULE
module_init(kyrg_register);
#else
late_initcall_sync(kyrg_register);
#endif
module_exit(kyrg_unregister);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liuqi");
MODULE_VERSION("v0.1");
MODULE_DESCRIPTION("KYRG");
