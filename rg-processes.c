/**
 * @file rg-processes.c
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

struct rg_process{
    struct list_head list;
    const char *name;
    
};

struct rg_vm_area{
    char path[256];
    unsigned long pgoff;
    char base_hash[HASH_SIZE];
}


