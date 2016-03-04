#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "procinfo.h"

MODULE_LICENSE("Dual BSD/GPL");

#define FIRST_MINOR 0
#define MINOR_CNT 1
#define DEVICE_NAME "procinfo"

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static int procinfo_open(struct inode *i, struct file *f)
{
    return 0;
}

static int procinfo_close(struct inode *i, struct file *f)
{
    return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
static int procinfo_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
#else
static long procinfo_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
    procinfo_t info;
    procinfo_arg_t *info_arg;
    struct task_struct *task;
    pid_t pid;

    switch (cmd) {
        case GET_PROCINFO:
            info_arg = (procinfo_arg_t *) arg;
            pid = info_arg->pid;
            if (pid > 0) {
                task = pid_task(find_vpid(pid), PIDTYPE_PID);
            } else if (pid == 0) {
                task = current;
            } else if (pid < 0) {
                task = current->parent;
            }
            if (task == NULL) {
                return -EINVAL;
            }
            info.pid = task->pid;
            info.ppid = task->parent->pid;
            info.start_time = task->start_time;
            info.num_sib = 0;
            if (copy_to_user(info_arg->info, &info, sizeof(procinfo_t))) {
                return -EACCES;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations procinfo_fops = {
        .owner = THIS_MODULE,
        .open = procinfo_open,
        .release = procinfo_close,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
        .ioctl = procinfo_ioctl
#else
        .unlocked_ioctl = procinfo_ioctl
#endif
};

static int procinfo_init(void)
{
    int ret;
    struct device *dev_ret;

    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, DEVICE_NAME)) < 0) {
        return ret;
    }
    cdev_init(&c_dev, &procinfo_fops);
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0) {
        return ret;
    }
    if (IS_ERR(cl = class_create(THIS_MODULE, DEVICE_NAME))) {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, DEVICE_NAME))) {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(dev_ret);
    }
    return 0;
}

static void procinfo_exit(void)
{
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(procinfo_init);
module_exit(procinfo_exit);
