#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static char *name = "default";

module_param(name, charp, 0644);

static int hello_init(void)
{
    printk(KERN_INFO "Hello world: I am %s speaking from the Kernel\n", name);
    return 0;
}

static void hello_exit(void)
{
    printk(KERN_INFO "Goodbye from %s, I am exiting the Kernel\n", name);
}

module_init(hello_init);
module_exit(hello_exit);
