#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init chardevbase_init(void)
{
    printk("Chrdevbase init\r\n");
    return 0;
}
static void __exit chardevbase_exit(void)
{
    printk("Chrdevbase exit\r\n");
}

/* 
 * Module Entry & Module Exit
 */
module_init(chardevbase_init);  /* Module Entry */
module_exit(chardevbase_exit);  /* Module Exit  */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");