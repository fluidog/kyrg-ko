
// #define pr_fmt(fmt) "%s: " fmt, __func__
#define pr_fmt(fmt) KBUILD_MODNAME": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>

#include <crypto/hash.h>

#define AUDIT_KYRG 1467

#define HASH_ALG_NAME "sha256"
#define HASH_SIZE (256 >> 3)

int ksyms_init(void);
unsigned long ksyms_kallsyms_lookup_name(const char *name);
// Call ksym_init() before using these below.
extern const char *ksyms_stext, *kysms_etext;
extern struct module *(*ksyms_find_module)(const char *name);

void init_periodic_timer(int (*cb)(void), unsigned long long msecs_period);
void exit_periodic_timer(void);

int init_fs(void);
void exit_fs(void);

int walk_process_vm(struct task_struct *task, struct vm_area_struct *mmap,
                    int (*func)(struct task_struct *, char *, unsigned long, void *), void *private);
int walk_process_mmaps(int vpid, int (*func)(struct task_struct *, struct vm_area_struct *, void *), void *private);
int example_access_process_memory(int vpid);

int hash_alg_init(char *hash_alg_name);
void hash_alg_exit(void);
// Call hash_alg_init() before using these below.
int hash_value_init(void);
int hash_value_update(const u8 *data, unsigned int len);
int hash_value_final(u8 *out);
int hash_value(const u8 *data, unsigned int len, u8 *out);

int init_rg(void);
void exit_rg(void);
int do_rg(void);
int set_rg_status(int status);
int get_rg_status(void);
int set_rg_periodic(unsigned long long period_sec);
unsigned long long get_rg_periodic(void);

int init_rg_processes(void);
void exit_rg_processes(void);
int add_rg_process(int pid, int kill);
void del_rg_process(int pid);
int do_rg_processes(void);
ssize_t show_rg_processes(char *buf, size_t size);

int init_rg_modules(void);
void exit_rg_modules(void);
int add_rg_module(const char *name);
void del_rg_module(const char *name);
int do_rg_modules(void);
ssize_t show_rg_modules(char *buf, size_t size);

int init_rg_kernel(void);
void exit_rg_kernel(void);
int add_rg_kernel(void);
void del_rg_kernel(void);
int do_rg_kernel(void);
ssize_t show_rg_kernel(char *buf, size_t size);
