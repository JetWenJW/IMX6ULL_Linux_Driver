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


#define GPIOLED_CNT     1
#define GPIOLED_NAME    "gpioled"

struct gpioled_dev 
{
    dev_t devid;            /* Device ID           */
    int major;              /* Major Device ID     */
    int minor;              /* minot Device ID     */
    struct cdev cdev;       /* For Char Device     */
    struct device *device;  /* For Device          */
    struct class *class;    /* For class Function  */
};

struct gpioled_dev gpioled; /*  struct Declare*/

static int led_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &gpioled;
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    struct dtsled_dev *dev = (struct dtsle_dev *)filp -> private_data;
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{

    return 0;
}
/* Chrdev Operations */
static const struct file_operations led_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = led_open,                /* Device Open file       */
    .release = led_release              /* Device Close file      */
    .write   = led_write                /* Device Write file      */
};

/* Entry Point Function */
static int __init gpioled_init(void)
{
    /* 1.Chardev Registry */
    gpioled.major = 0;
    
    if(gpioled.major)   /* Assign Device ID */
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }
    else                /* Unassigned Device ID */
    {
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }

    /* 2.Chrdev Initial  */
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &led_fops);

    /* 3.Add Chardev to Kernel */
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    /* 4.Add Device class */
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.class))
    {
        return PTR_ERR(gpioled.class);
    }

    /* 5.Create Device */
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if(IS_ERR(gpioled.device))
    {
        return PTR_ERR(gpiolde.device);
    }
    return 0;
}

/* Exit Point Function */
static void __exit gpioled_exit(void)
{
    /* Unregistry Chrdev */
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    /* Destroy Device => Class */
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.classs);

}


/* Module Registry */
module_init(gpioled_init);
module_exit(gpioled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");