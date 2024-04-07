#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

#define CHARDEVBASE_MAJOR    200            /* Major Chaedevbase number */
#define CHARDEVBASE_NAME     "chrdevbse"    /* Name of chaedevbase      */

static int chardevbase_open(struct inode *inode, struct file *filp)
{
    printk("chardevbase Open\r\n");
    return 0;
}

static int chardevbase_release(struct inode *inode, struct file *filp)
{
    printk("chardevbase Release\r\n");
    return 0;
}

static ssize_t chardevbase_read(struct file *filp, __user char *buf, size_t count, loff_t *ppos)
{
    printk("chardevbase read\r\n");
    return 0;
}

static ssize_t chardevbase_write(struct file *filp, __user char *buf, size_t count, loff_t *ppos)
{
    printk("chardevbase Write\r\n");
    return 0;
}

/* 
 * This is Linux Driver Framework 
 * All we need to do is complete it.
 */
static struct file_operations chrdevbase_fops =
{
    .owner      = THIS_MODULE, 
    .open       = chardevbase_open,
    .release    = chardevbase_release,
    .read       = chardevbase_read,
    .write      = chardevbase_write
};



static int __init chardevbase_init(void)
{
    int ret = 0;

    /* Register chardevbase */
    ret = register_chrdev(CHARDEVBASE_MAJOR, CHARDEVBASE_NAME, &chrdevbase_fops);

    if(ret < 0)
    {
        printk("CharDEVBASE initial Fail~\r\n");
    }
    printk("Chrdevbase init\r\n");

    return 0;
}
static void __exit chardevbase_exit(void)
{
    /* unregister chardevbase */
    unregister_chrdev(CHARDEVBASE_MAJOR, CHARDEVBASE_NAME);

    printk("Chrdevbase exit\r\n");

}

/* 
 * Module Entry & Module Exit
 */
module_init(chardevbase_init);  /* Module Entry */
module_exit(chardevbase_exit);  /* Module Exit  */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");