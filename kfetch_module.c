#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/utsname.h>
#include <linux/sysinfo.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <asm/cpufeature.h>
#include <linux/sched/signal.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <mutex.h>

#define BUF_SIZE 1024
static char kfetch_buf[BUF_SIZE];
static DEFINE_MUTEX(kfetch_mutex);
static struct class *cls;
static int major;
static int flag[6] = {1, 1, 1, 1, 1, 1};


static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset);
static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset);
static int kfetch_open(struct inode *inode, struct file *filp);
static int kfetch_release(struct inode *inode, struct file *filp);         

const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};


static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    int len, online_cpu, total_cpu, proc, i, t;
    struct new_utsname *uts;
    struct cpuinfo_x86 *cpu = &cpu_data(0);
    struct sysinfo si;
    unsigned long total_memory, free_memory, uptime_j, uptime;
    struct task_struct *task;
    char *line;

    char logo[] =
        "        .-.        \n"
        "       (.. |       \n"
        "       <>  |       \n"
        "      / --- \\      \n"
        "     ( |   | |     \n"
        "   |\\\\_)___/\\)/\\   \n"
        "  <__)------(__/   \n";
    char *logo_copy = kstrdup(logo, GFP_KERNEL);

    /*fetching the information*/
    uts = utsname();

    // print the hostname
    t = snprintf(kfetch_buf, BUF_SIZE, "                   %s\n", uts->nodename);

    // print the separator line
    line = strsep(&logo_copy, "\n");
    strcat(kfetch_buf, line);
    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
       "--------------------\n");

    // check the mask
    for(i = 0; i < 6; ++i){
        if(flag[i]){
            // print one line of logo first
            line = strsep(&logo_copy, "\n");
            strcat(kfetch_buf, line);
            
            switch(i){
                // kernel release
                case 0:
                    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
                        "Kernel:    %s\n", uts->release);
                        break;
                // CPU model
                case 1:
                    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
                        "CPU:       %s\n", cpu->x86_model_id);
                    break;
                // number of CPUs
                case 2:
                    online_cpu = num_online_cpus();
                    total_cpu = num_possible_cpus();
                    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
                        "CPUs:      %d / %d\n", online_cpu, total_cpu);
                    break;
                // memory information
                case 3:
                    si_meminfo(&si);
                    total_memory = si.totalram *PAGE_SIZE;
                    free_memory = (si.totalram - si.freeram) *PAGE_SIZE;
                    total_memory >>= 20;
                    free_memory >>= 20;

                    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
                        "Mem:       %lu MB / %lu MB\n", free_memory, total_memory);
                    break;
                // number of processes
                case 4:
                    for_each_process(task) proc++;
                    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
                        "Procs:     %d\n", proc);
                    break;
                // uptime
                case 5:
                    uptime_j = jiffies - INITIAL_JIFFIES;
                    uptime = jiffies_to_msecs(uptime_j) / (60 * 1000);
                    snprintf(kfetch_buf + strlen(kfetch_buf), BUF_SIZE - strlen(kfetch_buf),
                        "Uptime:    %lu mins\n", uptime);
                    break;
            }
        }
    }

    // print the remaining logo
    if(logo_copy != NULL) strcat(kfetch_buf, logo_copy);

    len = strlen(kfetch_buf);

    /* Copy information to user space */
    if (copy_to_user(buffer, kfetch_buf, len)) {
        pr_alert("Failed to copy data to user");
        return -1; 
    }

    return len;
}


static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    int mask_info, i;
    int mask[6];

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return -1;
    }

    /* setting the information mask */
    for(i = 0; i < 6; ++i){
        if(mask_info % 2) mask[i] = 1;
        else mask[i] = 0;
        mask_info /= 2;
    }

    return length;
}


static int kfetch_open(struct inode *inode, struct file *filp)
{
    mutex_lock(&kfetch_mutex);
    pr_info("kfetch device opened\n");
    return 0;
}


static int kfetch_release(struct inode *inode, struct file *filp)
{
    mutex_unlock(&kfetch_mutex);
    pr_info("kfetch device closed\n");
    return 0;
}


static int __init kfetch_init(void)
{
    pr_info("kfetch new module loaded\n");
    mutex_init(&kfetch_mutex);

    major = register_chrdev(0, "kfetch", &kfetch_ops);
    if(major < 0){
        pr_alert("Failed to register device\n");
        unregister_chrdev(major, "kfetch");
        return -1;
    }

    cls = class_create(THIS_MODULE, "kfetch");
    if(IS_ERR(cls)) {
        pr_alert("Failed to create class\n");
        unregister_chrdev(major, "kfetch");
        return PTR_ERR(cls);
    }

    if(device_create(cls, NULL, MKDEV(major, 0), NULL, "kfetch") == NULL){
        pr_alert("Failed to create device\n");
        class_destroy(cls);
        unregister_chrdev(major, "kfetch");
        return -1;
    }

    return 0;
}


static void __exit kfetch_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    
    unregister_chrdev(major, "kfetch");

    mutex_destroy(&kfetch_mutex);
    pr_info("kfetch module unloaded\n");
}

module_init(kfetch_init);
module_exit(kfetch_exit);

MODULE_LICENSE("GPL");