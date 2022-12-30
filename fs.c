/**
 * @file fs.c
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
#include <linux/security.h>

static ssize_t read_period(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
#define BUF_SIZE 32
    char kbuf[BUF_SIZE];
    int length;

    length = snprintf(kbuf, BUF_SIZE, "%llu", get_rg_periodic());

    return simple_read_from_buffer(buf, size, ppos, kbuf, length);
}

static ssize_t write_period(struct file *file, const char __user *buf,
                            size_t size, loff_t *ppos)
{
    int error;
    unsigned long long period_sec;

    error = kstrtou64_from_user(buf, size, 10, &period_sec);
    if (error)
        return error;

    error = set_rg_periodic(period_sec);
    if (error)
        return error;

    return size;
}

/**
 * @brief  [add|del] <pid> <kill> eg. add 12364 1
 *
 * @param file
 * @param buf
 * @param size
 * @param ppos
 * @return ssize_t
 */
static ssize_t write_policy_proc(struct file *file, const char __user *buf,
                                 size_t size, loff_t *ppos)
{
    int error;
    ssize_t length = 0;
    char *kbuf, *tmp, *opt;
    int pid, kill;

    kbuf = kmalloc(size, GFP_KERNEL);
    if (IS_ERR_OR_NULL(kbuf))
        return PTR_ERR(kbuf);

    if (copy_from_user(kbuf, buf, size))
        goto err;

    error = -EINVAL;
    opt = strsep(&kbuf, " ");
    if (unlikely(!opt || !kbuf))
        goto err;
    length += kbuf - opt;

    if (!strcmp(opt, "add")) {
        tmp = strsep(&kbuf, " ");
        if (unlikely(!tmp || !kbuf || kstrtos32(tmp, 10, &pid)))
            goto err;
        length += kbuf - tmp;

        tmp = strsep(&kbuf, "\n");
        if (unlikely(!tmp || !kbuf || kstrtos32(tmp, 10, &kill)))
            goto err;
        length += kbuf - tmp;

        error = add_rg_process(pid, kill);
        if (!error)
            error = length; // return length

        goto err;
    }

    if (!strcmp(opt, "del")) {
        tmp = strsep(&kbuf, "\n");
        if (unlikely(!tmp || !kbuf || kstrtos32(tmp, 10, &pid)))
            goto err;
        length += kbuf - tmp;

        del_rg_process(pid);
        error = length; // return length
        goto err;
    }

err:
    pr_debug("%s %d %d, return:%d\n", opt, pid, kill, error);
    kfree(opt); // kbuf pointer have changed, opt points the original kbuf.
    return error;
}

static ssize_t read_policy_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    size_t length = 0;
    char *kbuf;

    kbuf = (char *)kmalloc(PAGE_SIZE, GFP_KERNEL);

    length = show_rg_processes(kbuf, PAGE_SIZE);
    if (unlikely(length >= PAGE_SIZE)) {
        length++; // +1 Used to add a terminator "\0"
        kfree(kbuf);
        kbuf = kmalloc(length, GFP_KERNEL);
        if (IS_ERR_OR_NULL(kbuf))
            return PTR_ERR(kbuf);
        length = show_rg_processes(kbuf, length);
        pr_debug("relloc kbuf length: %lu", length);
    }

    length = simple_read_from_buffer(buf, size, ppos, kbuf, length);

    kfree(kbuf);
    return length;
}

static ssize_t read_policy_module(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
#define BUFFER_SIZE(total_buf_size, data_length) \
    total_buf_size > data_length ? total_buf_size - data_length : 0
    size_t length = 0;
    char *kbuf;

    kbuf = (char *)kmalloc(PAGE_SIZE, GFP_KERNEL);

    length = show_rg_modules(kbuf, PAGE_SIZE);
    length += show_rg_kernel(kbuf + length, BUFFER_SIZE(PAGE_SIZE, length));

    if (unlikely(length >= PAGE_SIZE)) {
        size_t tmp;
        length++; // +1 Used to add a terminator "\0"
        kfree(kbuf);
        kbuf = kmalloc(length, GFP_KERNEL);
        if (IS_ERR_OR_NULL(kbuf))
            return PTR_ERR(kbuf);
        tmp = show_rg_modules(kbuf, length);
        tmp += show_rg_kernel(kbuf + tmp, length - tmp);
        pr_debug("relloc kbuf length: %lu", tmp);
    }

    length = simple_read_from_buffer(buf, size, ppos, kbuf, length);

    kfree(kbuf);
    return length;
}

static ssize_t write_policy_module(struct file *file, const char __user *buf,
                                   size_t size, loff_t *ppos)
{
    int error;
    ssize_t length = 0;
    char *kbuf, *opt, *name;

    kbuf = kmalloc(size, GFP_KERNEL);
    if (IS_ERR(kbuf))
        return PTR_ERR(kbuf);

    if (copy_from_user(kbuf, buf, size))
        goto err;

    error = -EINVAL;
    opt = strsep(&kbuf, " ");
    if (unlikely(!opt || !kbuf))
        goto err;
    length += kbuf - opt;

    name = strsep(&kbuf, "\n");
    if (unlikely(!name || !kbuf))
        goto err;
    length += kbuf - name;

    if (!strcmp(opt, "add")) {
        if (!strcmp(name, "kernel"))
            error = add_rg_kernel();
        else
            error = add_rg_module(name);

        if (!error)
            error = length; // return length

        goto err;
    }

    if (!strcmp(opt, "del")) {
        if (!strcmp(name, "kernel"))
            del_rg_kernel();
        else
            del_rg_module(name);

        error = length; // return length
    }

    pr_debug("%s %s\n", opt, name);

err:
    kfree(opt); // kbuf pointer have changed, opt points the original kbuf.
    return error;
}

static ssize_t write_immediate(struct file *file, const char __user *buf,
                               size_t size, loff_t *ppos)
{
    // any write to this file will trigger an immediate check
    do_rg();
    return size;
}

static ssize_t read_status(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
#define BUF_SIZE 32
    char kbuf[BUF_SIZE];
    int length;

    length = snprintf(kbuf, BUF_SIZE, "%d", get_rg_status());

    return simple_read_from_buffer(buf, size, ppos, kbuf, length);
}

static ssize_t write_status(struct file *file, const char __user *buf,
                            size_t size, loff_t *ppos)
{
    int error;
    int status;

    error = kstrtos32_from_user(buf, size, 10, &status);
    if (error)
        return error;

    error = set_rg_status(status);
    if (error)
        return error;

    return size;
}

static const struct file_operations period_ops = {
    .read = read_period,
    .write = write_period,
};
static const struct file_operations policy_proc_ops = {
    .read = read_policy_proc,
    .write = write_policy_proc,
};
static const struct file_operations immediate_ops = {
    .write = write_immediate,
};
static const struct file_operations policy_module_ops = {
    .read = read_policy_module,
    .write = write_policy_module,
};
static const struct file_operations status_ops = {
    .read = read_status,
    .write = write_status,
};

static const struct tree_descr kyrg_files[] = {
    [3] = {"status", &status_ops, S_IRUGO | S_IWUSR},
    [4] = {"period", &period_ops, S_IRUGO | S_IWUSR},
    [5] = {"immediate", &immediate_ops, S_IRUGO | S_IWUSR},
    [6] = {"policy_module", &policy_module_ops, S_IRUGO | S_IWUSR},
    [7] = {"policy_process", &policy_proc_ops, S_IRUGO | S_IWUSR},
    /* last one */
    {NULL}};

static struct dentry *kyrg_root, *files_dentry[10];

int init_fs(void)
{
    int i;

    kyrg_root = securityfs_create_dir("kyrg", NULL);

    for (i = 3; kyrg_files[i].name; i++) {
        files_dentry[i] = securityfs_create_file(kyrg_files[i].name,
                                                 kyrg_files[i].mode, kyrg_root, NULL, kyrg_files[i].ops);
    }

    pr_info("Fs init success\n");

    return 0;
}

void exit_fs(void)
{
    int i;

    for (i = 3; kyrg_files[i].name; i++) {
        securityfs_remove(files_dentry[i]);
    }

    securityfs_remove(kyrg_root);

    pr_info("Fs exit\n");
}