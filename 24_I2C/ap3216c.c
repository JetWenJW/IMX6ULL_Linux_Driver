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
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include "ap3216creg.h"

/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add I2C AP3216C sub-device node to Device Tree
 * 2. Build I2C Driver Framework
 * 3. Accompilshed the Framework
 */

#define AP3216C_CNT     1
#define AP3216C_NAME    "ap3216c"


/* Step6. Create Structure of AP3216C for registering Funciton */
struct ap3216c_dev
{
    int major;
    dev_t devid;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

struct ap3216c_dev ap3216cdev;


/* Step7_1 file operation Function */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    /* Private Data */
    filp -> private_data = &ap3216cdev;
    return 0;
}

/* Step7_2 file operation Function */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
    struct ap3216c_dev *dev = (struct ap3216c_dev *)filp -> private_data;
    return 0;
}

/* Step7_3 file operation Function */
ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    return 0;
}

/* Step7. Add file Operations of AP3216C */
static const struct file_operations ap3216c_fops = 
{
    .owner      = THIS_MODULE,
    .open       = ap3216c_open,
    .release    = ap3216c_release,
    .read       = ap3216c_read,
};

/* Step3. Traditional Matched Table (id_table) */
static struct i2c_device_id ap3216c_id[] =
{
    {"JetWen,ap3216c", 0},
    {}
};
/* Step4. Device Tree Matched Table */
static struct of_device_id ap3216c_of_match[] = 
{
    {.compatible = "JetWen,ap3216c"},
    {}
};

/* Step5. Probe Function(this Function will be called when matched) */
static int ap3216c_probe(struct i2c_client *client, cinst struct i2c_device_id *id)
{   
    int ret = 0;
    /* 5_1.Registry Char Device */
    ap3216cdev.major = 0;       /* Assigned device ID by Kernel */

    if(ap3216cdev.major)
    {
        /* Assigned Major Device ID */
        ap3216cdev.devid = MKDEV(ap3216cdev.major, 0);
        ret = register_chrdev_region(ap3216cdev.devid, AP3216C_CNT, AP3216C_NAME);
    }
    else
    {
        /* Unassigned Major Device ID */
        ret = alloc_chrdev_region(&ap3216cdev.devid, 0, AP3216C_CNT, AP3216C_NAME);
        ap3216cdev.major = MAJOR(ap3216cdev.devid);
        ap3216cdev.minor = MINOR(ap3216cdev.devid);
    }

    if(ret < 0)
    {
        printk("AP3216C Char Dev Region Error\r\n");
        goto fail_devid;
    }
    printk("ap3216c major= %d, minor= %d\r\n", ap3216cdev.major, ap3216cdev.minor);

    /* 5_2.Registry Char Device */
    ap3216cdev.cdev.owner = THIS_MODULE;
    cdev_init(&ap3216cdev.cdev, &ap3216c_fops);

    ret = cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, AP3216C_CNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    /* 
     * 5_3.Auto Make Device node (Plug And Play) 
     * Note: Class must be created earlier than Device
     */
    ap3216cdev.class = class_create(THIS_MODULE, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.class))
    {
        ret = PTR_ERR(ap3216cdev.device);
        goto fail_class;
    }

    /* 5_4. Auto assigned Device */
    ap3216cdev.device = device_create(ap3216cdev.class, NULL, 
                        ap3216cdev.device, NULL, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.device))
    {
        ret = PTR_ERR(ap3216cdev.device);
        goto fail_device;
    }

    return 0;
/* The Order cannot change */
fail_device :
    class_destroy(ap3216cdev.class);

fail_class :
    cdev_del(&ap3216cdev.cdev);

fail_cdev :
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_NAME);

fail_devid :
    return ret;
    return 0;
}

/* Step5. Remove Function(this Function will be called when Deleted) */
static int ap3216c_remove(struct i2c_client *client)
{   
    /* 5_5. Delete Char Device */
    cdev_del(&ap3216cdev.cdev);

    /* 5_6. Unregistry Char Device */
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);

    /* 
     * 5_7. Destroy Device & Class 
     * NOTE: Device must be destroied earlier than Clsss
     */
    device_destroy(ap3216cdev.class, ap3216cdev.device);
    class_destroy(ap3216cdev.class);
    return 0;
}


/* Step2. I2C Driver Structure */
static struct i2c_driver ap3216c_driver =
{
    .probe  = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver =
    {
        .name           = "ap3216c",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(&ap3216c_of_match);
    },

    .id_table = ap3216c_id;
};

/* Step1. Entry Point Function */
static int __init ap3216c_init(void)
{
    int ret = 0;        
    ret = i2c_add_driver(ap3216c_driver);
 
    return ret;
}

/* Step1. Exit Point Function */
static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}


/* Module Registry */
module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");