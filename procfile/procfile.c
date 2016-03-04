#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/tty.h>
#include <linux/sched.h>

#define PROC_ENTRY "get_proc_info"

MODULE_LICENSE("Dual BSD/GPL");


static void print_tty(const char *s)
{
    struct tty_struct *tty;
    tty = get_current_tty();
    if (tty != NULL) {
        tty->driver->ops->write(tty, s, strlen(s));
    }
}

static int count_list(struct list_head *list)
{
    int count;
    struct list_head *p;
    count = 0;
    p = list;
    while (!list_is_last(p, list)) {
        count += 1;
        p = p->next;
    }
    return count;
}

ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
    char kbuf[64];
    char msg[1024];
    struct task_struct *task;
    pid_t pid;
    char *endptr;

    if (copy_from_user(kbuf, buf, count)){
        return -EACCES;
    }
    pid = (pid_t) simple_strtol(kbuf, &endptr, 10);
    if (endptr == NULL){
        return -EINVAL;
    }
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
    sprintf(msg,"pid: %d\r\nppid: %d\r\nstart_time (monotonic): %ld.%ld\r\nnum_sib: %d\r\n",
            task->pid,
            task->parent->pid,
            task->start_time.tv_sec,
            task->start_time.tv_nsec,
            count_list(&task->sibling));
    print_tty(msg);
    return count;
}

static struct file_operations proc_fops = {
        .owner = THIS_MODULE,
        .write = write_proc
};

int proc_init(void)
{
    proc_create(PROC_ENTRY, 0666, NULL, &proc_fops);
    return 0;
}

void proc_cleanup(void)
{
    remove_proc_entry(PROC_ENTRY, NULL);
}

module_init(proc_init);
module_exit(proc_cleanup);
