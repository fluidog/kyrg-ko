/**
 * @file hash.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief
 * @version 0.1
 * @date 2022-12-17
 *
 * @copyright Copyright (c) 2022
 *
 */

// #define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>
#include <crypto/hash.h>

static struct shash_desc *desc;

int hash_alg_init(char *hash_alg_name)
{
    struct crypto_shash *alg;

    alg = crypto_alloc_shash(hash_alg_name, 0, 0);
    if (IS_ERR(alg)) {
        pr_err("can't alloc alg %s\n", hash_alg_name);
        return PTR_ERR(alg);
    }

    desc = kmalloc(sizeof(struct shash_desc) + crypto_shash_descsize(alg), GFP_KERNEL);
    if (IS_ERR(desc)) {
        crypto_free_shash(desc->tfm);
        return PTR_ERR(desc);
    }

    desc->tfm = alg;
    pr_info("Hash alg %s init success\n", hash_alg_name);
    return 0;
}

void hash_alg_exit(void)
{
    crypto_free_shash(desc->tfm);
    kfree(desc);
}

int hash_value_init(void)
{
    return crypto_shash_init(desc);
}

int hash_value_update(const u8 *data, unsigned int len)
{
    return crypto_shash_update(desc, data, len);
}

int hash_value_final(u8 *out)
{
    return crypto_shash_final(desc, out);
}

int hash_value(const u8 *data, unsigned int len, u8 *out)
{
    return crypto_shash_digest(desc, data, len, out);
}

// 256 bits = 32 bytes
void example_hash_256(void)
{
#define BUF_SIZE 32
    hash_alg_init("sha256");

    {
        const u8 *data = "abc";
        unsigned int len = strlen(data);
        u8 *out = kzalloc(BUF_SIZE, GFP_KERNEL);

        hash_value_init();
        hash_value_update(data, len);
        hash_value_final(out);

        print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16, 1, out, BUF_SIZE, false);
        kfree(out);
    }

    hash_alg_exit();
}