
#define DEBUG

// #define pr_fmt(fmt) "%s: " fmt, __func__
#define pr_fmt(fmt) KBUILD_MODNAME": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>


#define HASH_ALG_NAME "sha256"
#define HASH_SIZE (256 >> 8)

int ksyms_init(void);
unsigned long ksyms_kallsyms_lookup_name(const char *name);
// Call ksym_init() before using these below.
extern const char *ksyms_stext, *kysms_etext;
extern struct module * (*ksyms_find_module)(const char *name);


void init_periodic_timer(void (*cb)(void), unsigned long msecs_period);
void exit_periodic_timer(void);


int walk_process_vm(struct task_struct *task, struct vm_area_struct *mmap,
			void *private, int (*func)(struct task_struct *, char *, unsigned long, void *));
int walk_process_mmaps(int vpid, int (*func)(struct task_struct *, struct vm_area_struct *));


int hash_alg_init(char *hash_alg_name);
void hash_alg_exit(void);
// Call hash_alg_init() before using these below.
int hash_value_init(void);
int hash_value_update(const u8 *data, unsigned int len);
int hash_value_final(u8 *out);
int hash_value(const u8 *data, unsigned int len, u8 *out);

