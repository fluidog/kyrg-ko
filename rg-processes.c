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
// #define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched/signal.h>
#include <linux/audit.h>

struct rg_process {
    struct list_head list;
    int pid;
    int enforce; // enforce kill process while process modified
    int modified;  // 1: modified, 0:...
              // struct rg_area* areas[];  // all vm areas of this process
};
static LIST_HEAD(rg_processes);

struct rg_area {
    struct list_head list;
    struct path *path; // file path we map to
    char base_hash[HASH_SIZE];
    int used;    // linked times by process
    int guarded; // false: is not guarded, true:...
    int modified;  // 1: modified, 0:...
};
static LIST_HEAD(rg_areas);

static enum {
    ADD_PROC,
    DEL_PROC,
    GUARD_PROC,
} current_ops; // ugly, but simple

static int access_vm_cb(struct task_struct *task, char *buffer, unsigned long size, void *private)
{
    // struct rg_area *area = (struct rg_area *)private;
    return hash_value_update(buffer, size);
}

static int access_mmap_cb(struct task_struct *task, struct vm_area_struct *mmap, void *private)
{
    int error;
    // struct rg_process *rg_proc = (struct rg_process *)private;
    struct rg_area *area;

    // only code segment
    if (!(mmap->vm_flags & VM_EXEC))
        return 0;

    // must have backing file (in case [vdso] segment)
    if (mmap->vm_file == NULL || mmap->vm_file->f_path.dentry == NULL)
        return 0;

    list_for_each_entry(area, &rg_areas, list)
    {
        if (area->path->dentry == mmap->vm_file->f_path.dentry) {
            break; // found
        }
    }

    if (current_ops == ADD_PROC) {
        // area have existed
        if (&area->list != &rg_areas) {
            area->used++;
            return 0;
        }

        area = kzalloc(sizeof(struct rg_area), GFP_KERNEL);
        if (IS_ERR(area))
            return PTR_ERR(area);

        area->path = &mmap->vm_file->f_path;
        area->used = 1;

        error = hash_value_init();
        if (error < 0)
            goto err;

        error = walk_process_vm(task, mmap, access_vm_cb, area);
        if (error < 0)
            goto err;

        error = hash_value_final(area->base_hash);
        if (error < 0)
            goto err;

        list_add(&area->list, &rg_areas);
        return 0;
    }

    if (current_ops == DEL_PROC) {
        // area have not existed
        if (&area->list == &rg_areas) {
            pr_warn("delet a not existed area");
            return 0;
        }
        area->used--;
        if (area->used == 0) {
            list_del(&area->list);
            kfree(area);
        }
        return 0;
    }

    if (current_ops == GUARD_PROC) {
        char hash[HASH_SIZE];
        // guard area have not existed
        if (&area->list == &rg_areas) {
            pr_warn("guard a not existed area");
            return 0;
        }
        if (area->guarded)
            return area->modified;

        error = hash_value_init();
        if (error < 0)
            goto err;

        error = walk_process_vm(task, mmap, access_vm_cb, area);
        if (error < 0)
            goto err;

        error = hash_value_final(hash);
        if (error < 0)
            goto err;

        memcmp(hash, area->base_hash, HASH_SIZE) == 0 ?
            (area->modified = 0) :
            (area->modified = 1);
        area->guarded = true;
        return area->modified;
    }

    pr_warn("unknown ops");
    return -EPERM;

err:
    kfree(area);
    return error;
}

int add_rg_process(int pid, int enforce)
{
    int error;
    struct rg_process *process;

    current_ops = ADD_PROC;

    list_for_each_entry(process, &rg_processes, list)
    {
        if (process->pid == pid) { // found
            process->enforce = enforce;
            return 0;
        }
    }

    process = kmalloc(sizeof(struct rg_process), GFP_KERNEL);
    if (IS_ERR(process))
        return PTR_ERR(process);

    process->pid = pid;
    process->enforce = enforce;
    process->modified = 0;

    error = walk_process_mmaps(pid, access_mmap_cb, process);
    if (error < 0) {
        kfree(process);
        return error;
    }

    list_add(&process->list, &rg_processes);
    return 0;
}

void del_rg_process(int pid)
{
    int error;
    struct rg_process *process, *tmp;

    current_ops = DEL_PROC;

    list_for_each_entry_safe(process, tmp, &rg_processes, list)
    {
        if (process->pid == pid) {
            error = walk_process_mmaps(pid, access_mmap_cb, process);
            list_del(&process->list);
            kfree(process);
            return;
        }
    }
}

int do_rg_processes(void)
{
    int error;
    struct rg_process *process, *tmp;
    struct rg_area *area;

    current_ops = GUARD_PROC;

    list_for_each_entry(area, &rg_areas, list)
    {
        area->guarded = false;
    }

    list_for_each_entry_safe(process, tmp, &rg_processes, list)
    {
        pr_debug("guard process: %d\n", process->pid);
        error = walk_process_mmaps(process->pid, access_mmap_cb, process);
        if (error == 0) // all ok
            continue;

        if (error == 1) { // process modified
            process->modified = 1;
            if (process->enforce) {
                kill_pid(find_get_pid(process->pid), SIGTERM, 1);
                del_rg_process(process->pid);
            }
            // pr_warn("process: %d has been modified\n", process->pid);
            audit_log(audit_context(), GFP_ATOMIC, AUDIT_KYRG, 
                "process: %d has been modified\n", process->pid);
        }

        if (error < 0) {           // guard with error
            if (error == -ESRCH) { // process killed by someone
                pr_warn("process: %d has been killed\n", process->pid);
                del_rg_process(process->pid);
                continue;
            }
            return error;
        }
    }

    return 0;
}

ssize_t show_rg_processes(char *buf, size_t size)
{
#define MAX_PATH_LEN 128
#define BUFFER_SIZE(total_buf_size, data_length) \
    total_buf_size > data_length ? total_buf_size - data_length : 0
    struct rg_process *process;
    struct rg_area *area;
    int len = 0;
    char *path = kmalloc(MAX_PATH_LEN, GFP_KERNEL);

    len += snprintf(buf + len, BUFFER_SIZE(size, len), "pid\tenforce\tmodified\n");

    list_for_each_entry(process, &rg_processes, list)
    {
        len += snprintf(buf + len, BUFFER_SIZE(size, len),
                        "%d\t%d\t%d\n",
                        process->pid,
                        process->enforce,
                        process->modified);
    }

    len += snprintf(buf + len, BUFFER_SIZE(size, len), \
        "\nThe guarded areas by all processes:\n" \
        "used\tguarded\tmodified\tpath\n");

    list_for_each_entry(area, &rg_areas, list)
    {
        len += snprintf(buf + len, BUFFER_SIZE(size, len),
                        "%d\t%d\t%d\t%s\n",
                        area->used,
                        area->guarded,
                        area->modified,
                        d_path(area->path, path, MAX_PATH_LEN));
    }

    return len;
}

int init_rg_processes(void)
{
    return 0;
}
void exit_rg_processes(void)
{
    struct rg_process *process, *process_tmp;
    struct rg_area *area, *area_tmp;

    list_for_each_entry_safe(area, area_tmp, &rg_areas, list)
    {
        list_del(&area->list);
        kfree(area);
    }

    list_for_each_entry_safe(process, process_tmp, &rg_processes, list)
    {
        list_del(&process->list);
        kfree(process);
    }
}