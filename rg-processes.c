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
struct rg_area {
    struct list_head list;
    struct vm_area_struct *mmap; // The area we map to
    char base_hash[HASH_SIZE];
    bool modified; // 1: modified, 0:...
};
struct rg_process {
    struct list_head list;
    int pid;
    int enforce;               // enforce kill process while process modified
    bool modified;              // 1: modified, 0:...
    struct list_head rg_areas; // all vm areas of this process
};
static LIST_HEAD(rg_processes);

static int access_vm_cb(struct task_struct *task, char *buffer, unsigned long size, void *private)
{
    // struct rg_area *area = (struct rg_area *)private;
    return hash_value_update(buffer, size);
}

static int access_mmap_cb(struct task_struct *task, struct vm_area_struct *mmap, void *private)
{
    int error;
    struct rg_process *process = (struct rg_process *)private;
    struct rg_area *area;

    // only code segment
    if (!(mmap->vm_flags & VM_EXEC))
        return 0;

    // must have backing file (in case [vdso] segment)
    if (mmap->vm_file == NULL || mmap->vm_file->f_path.dentry == NULL)
        return 0;

    area = kzalloc(sizeof(struct rg_area), GFP_KERNEL);
    if (IS_ERR(area))
        return PTR_ERR(area);

    area->mmap = mmap;
    area->modified = 0;

    error = hash_value_init();
    if (error < 0)
        goto err;

    error = walk_process_vm(task, mmap, access_vm_cb, area);
    if (error < 0)
        goto err;

    error = hash_value_final(area->base_hash);
    if (error < 0)
        goto err;

    list_add(&area->list, &process->rg_areas);

    return 0;
err:
    kfree(area);
    return error;
}

int add_rg_process(int pid, int enforce)
{
    int error;
    struct rg_process *process;

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
    INIT_LIST_HEAD(&process->rg_areas);

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
    struct rg_process *process;
    struct rg_area *area, *tmp;

    list_for_each_entry(process, &rg_processes, list)
    {
        if (process->pid == pid)
            break;
    }

    if (&process->list == &rg_processes) {
        pr_warn("delete a not existed process");
        return;
    }

    list_for_each_entry_safe(area, tmp, &process->rg_areas, list)
    {
        list_del(&area->list);
        kfree(area);
    }

    list_del(&process->list);
    kfree(process);
}

int do_rg_processes(void)
{
    int error;
    struct rg_process *process, *tmp;
    struct rg_area *area;
    struct task_struct *task;
    char hash[HASH_SIZE];

    list_for_each_entry_safe(process, tmp, &rg_processes, list)
    {
        pr_debug("guard process: %d\n", process->pid);
        process->modified = 0;

        task = get_pid_task(find_get_pid(process->pid), PIDTYPE_PID);
        if (IS_ERR_OR_NULL(task)) {
            if (task == NULL) {
                pr_warn("process: %d has been killed by someone\n", process->pid);
                del_rg_process(process->pid);
                continue;
            }
            return PTR_ERR(task);
        }

        list_for_each_entry(area, &process->rg_areas, list)
        {
            pr_debug("guard area: %lx-%lx\n", area->mmap->vm_start, area->mmap->vm_end);
            area->modified = 0;

            error = hash_value_init();
            if (error < 0)
                return error;

            error = walk_process_vm(task, area->mmap, access_vm_cb, area);
            if (error < 0)
                return error;

            error = hash_value_final(hash);
            if (error < 0)
                return error;

            if (memcmp(hash, area->base_hash, HASH_SIZE) != 0) { // modified
                area->modified = 1;
                process->modified = 1;
                break;
            }
        }

        if (process->modified) {
            // pr_warn("process: %d has been modified\n", process->pid);
            audit_log(audit_context(), GFP_ATOMIC, AUDIT_KYRG,
                      "process: %d has been modified\n", process->pid);

            if (process->enforce) {
                kill_pid(find_get_pid(process->pid), SIGTERM, 1);
                del_rg_process(process->pid);
            }
        }
    }

    return 0;
}

ssize_t show_rg_processes(char *buf, size_t size)
{
    #define MAX_PATH_LEN 128
    struct rg_process *process;
    struct rg_area *area;
    int len = 0;
    char *path = kmalloc(MAX_PATH_LEN, GFP_KERNEL);

    list_for_each_entry(process, &rg_processes, list)
    {
        len += snprintf(buf + len, BUFFER_SIZE(size, len),
                        "process:%d\tenforce:%d\tmodified:%d\n",
                        process->pid,
                        process->enforce,
                        process->modified);

        list_for_each_entry(area, &process->rg_areas, list)
        {
            len += snprintf(buf + len, BUFFER_SIZE(size, len),
                            "%lx-%lx\t%d\t%s\n",
                            area->mmap->vm_start,
                            area->mmap->vm_end,
                            area->modified,
                            d_path(&area->mmap->vm_file->f_path, path, MAX_PATH_LEN));
        }

        len += snprintf(buf + len, BUFFER_SIZE(size, len), "\n");
    }

    kfree(path);
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

    list_for_each_entry_safe(process, process_tmp, &rg_processes, list)
    {
        list_for_each_entry_safe(area, area_tmp, &process->rg_areas, list)
        {
            list_del(&area->list);
            kfree(area);
        }
        list_del(&process->list);
        kfree(process);
    }
}