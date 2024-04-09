#include <linux/module.h>
#include <linux/of_irq.h>
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


#define BEEP_CNT     1
#define BEEP_NAME    "beep"

#define BEEP_OFF          0
#define BEEP_ON           1

struct beep_dev 
{
    dev_t devid;            /* Device ID           */
    int major;              /* Major Device ID     */
    int minor;              /* minot Device ID     */
    struct cdev cdev;       /* For Char Device     */
    struct device *device;  /* For Device          */
    struct class *class;    /* For class Function  */
    struct device_node *nd  /* Device Node         */
    int beep_gpio;           /* IO Number(ID)       */
};

struct beep_dev beep; /*  struct Declare */

static int beep_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &beep;
    return 0;
}

static int beep_release(struct inode *inode, struct file *filp)
{
    struct beep_dev *dev = (struct beep_dev *)filp -> private_data;
    return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    unsigned char databuffer[1];        

    struct beep_dev *dev = (struct beep_dev *)filp -> private_data; /* Get File Permission */
    ret = copy_from_user(databuffer, buf, count);
    if(ret < 0)     /* Fail Write file */
    {
        return -EINVAL;
    }

    /* Succeed Write File */
    if(databuffer[0] == BEEP_ON)
    {
        gpio_set_value(dev -> beep_gpio, 0);      /* Set Beep as High Voltage (ON) */
    }
    else if (databuffer[0] == BEEP_OFF)
    {
        gpio_set_value(dev -> beep_gpio, 1);      /* Set Beep as High Voltage (OFF) */
    }

    return 0;
}
/* Chrdev Operations */
static const struct file_operations beep_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = beep_open,                /* Device Open file       */
    .release = beep_release              /* Device Close file      */
    .write   = beep_write                /* Device Write file      */
};

/* Entry Point Function */
static int __init beep_init(void)
{
    int ret = 0;        /* For Error Happen */


    /* 1.Chardev Registry */
    beep.major = 0;
    
    if(beep.major)   /* Assign Device ID */
    {
        beep.devid = MKDEV(beep.major, 0);
        ret = register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
        beep.major = MAJOR(beep.devid);
        beep.minor = MINOR(beep.devid);
    }
    /* Fail Device ID */
    if(ret < 0)
    {
        goto fail_devid;
    }

    /* 2.Chrdev Initial  */
    beep.cdev.owner = THIS_MODULE;
    cdev_init(&beep.cdev, &beep_fops);

    /* 3.Add Chardev to Kernel */
    ret = cdev_add(&beep.cdev, beep.devid, BEEP_CNT);

    /* Fail Cdev Add */
    if(ret < 0)
    {
        goto fail_cdev;
    }
    /* 4.Add Device class */
    beep.class = class_create(THIS_MODULE, BEEP_NAME);
    if(IS_ERR(beep.class))
    {
        ret = PTR_ERR(beep.class);
        goto fail_class;
    }

    /* 5.Create Device */
    beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
    if(IS_ERR(beep.device))
    {
        ret = PTR_ERR(beep.device);
        goto fail_device;
    }
    
    /* A. Get Beep Node From Device Tree(Beep Initialize) */
    beep.nd = of_find_node_by_path("/beep");
    if(beep.nd == NULL)
    {
        ret = -EINVAL;
        goto fail_find_node;
    }

    /* B. Beep GPIO Number(ID) */
    beep.beep_gpio = of_get_named_gpio(beep.nd, "beep-gpios", 0);
    if(beep.beep_gpio < 0)
    {
        ret = -EINVAL;
        goto fail_find_node;
    }

    /* C. Request IO In order to Accesss */
    ret = gpio_request(beep.beep_gpio, "beep-gpios");
    if(ret)
    {
        printk("Cannot Request BEEP GPIO\r\n");
        goto fail_find_node;
    }

    /* D. Set BEEP GPIO output direction */
    ret = gpio_direction_output(beep.beep_gpio, 0);
    if(ret < 0)
    {
        goto fail_setoutput;
    }

    /* E. Set BEEP GPIO Value */
    gpio_set_value(beep.beep_gpio, 0);  /* Set Beep as Low Voltage (ON) */

    return 0;

fail_setoutput :
    gpio_free(beep.beep_gpio);
fail_rs ;
fail_find_node :
    device_destroy(beep.class, beep.devid);
fail_device :
    class_destoy(beep.class);
fail_class :
    cdev_del(&beep.cdev);
fail_cdev :
    unregister_chrdev_region(beep.devid, BEEP_CNT);
fail_devid :
    return ret;
}

/* Exit Point Function */
static void __exit beep_exit(void)
{
    /* Beep OFF When we exit Module */
    gpio_set_value(beep.beep_gpio, 1);      /* Set Beep as High Voltage (OFF) */

    /* Unregistry Chrdev */
    cdev_del(&beep.cdev);
    unregister_chrdev_region(beep.devid, BEEP_CNT);

    /* Destroy Device => Class */
    device_destroy(beep.class, beep.devid);
    class_destroy(beep.classs);

    /* FREE BEEP GPIO */
    gpio_free(beep.beep_gpio);
}


/* Module Registry */
module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");