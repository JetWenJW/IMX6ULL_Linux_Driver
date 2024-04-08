#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

/*
 * The main purpose of this Project is 
 * 1. Create Device node via DeviceTree
 * 2. Code a Driver to get Property of Device Tree
 * 3.Use the Property we get Initialize Led & GPIO 
 */

#define DTSLED_CNT      1           /* Numerous of Device ID  */
#define DTSLED_NAME     "dtsled"    /* Name of Device ID      */

/* Device Structure */
struct dtsled_dev
{
    dev_t devid;        /* Device ID     */
    struct cdev cdev;   /* Char Device   */
    int major;          /* Major Device  */
    int minor;          /* minor         */


};

struct dtsled_dev dtsled;                   /* Led Device */

/* Char Device Operations */
static const struct file_operations dtsled_fops =
{
    .owner = THIS_MODULE,
    .write = 
    .open = 
    .read = 
};



/* Entry Point */
static int __init dtsled_init(void)
{
    int ret = 0;

    /* 1.Register CHar Device by Device Tree */
    dtsled.major = 0;       /* Kernel Auto Assigned Device ID */
    if(dtsled.major)        /* Define Device ID               */
    {
        dtsled.devid = MKDEV(dtsmajor, 0);
        ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&dtsled.devid, DTSLED_CNT, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(desled.devid);
    }

    if(ret < 0)         /* Fail DEVICE ID */
    {
        goto fail_devid;
    }

    /* 2. Registery CHAR Device */
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);
    ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    /* 3.Auto Create Device Node */

    return 0;

fail_cdev :
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

fail_devid :
    return ret;
}
/* Exit Point */
static void __exit dtsled_exit(void)
{
    /* Delete Char_Device */
    cdev_del(&dtsled.cdev);

    /* Unregister Char_Device */
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
}



















/* Register Module Entry Point */
module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");



