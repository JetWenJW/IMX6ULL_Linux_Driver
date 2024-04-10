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


#define TIMER_CNT     1
#define TIMER_NAME    "timer"

#define timer_0_VALUE     0xF0         
#define INVAtimer         0x00      

struct timer_dev 
{
    dev_t devid;            /* Device ID           */
    int major;              /* Major Device ID     */
    int minor;              /* minot Device ID     */
    struct cdev cdev;       /* For Char Device     */
    struct device *device;  /* For Device          */
    struct class *class;    /* For class Function  */
    struct device_node *nd; /* Device Node         */
    int timer_gpio;         /* IO Number(ID)       */
    atomic_t timervalue;    /* atomic Operation    */
};

struct timer_dev timer; /*  struct Declare */

static int timer_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &timer;
    return 0;
}

static int timer_release(struct inode *inode, struct file *filp)
{
    struct timer_dev *dev = (struct timer_dev *)filp -> private_data;
    return 0;
}

static ssize_t timer_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
   
    return ret;
}

static ssize_t timer_read(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    int value = 0;
    struct timer_dev *dev = (struct timer_dev *)filp -> private_data;

    if(gpio_get_value(dev -> timer_gpio) == 0)        /* Press timer           */
    {
        while(!gpio_get_value(dev -> timer_gpio));    /* Wait to release timer */
        atomic_set(&dev -> timervalue, timer_0_VALUE);
    }
    else
    {
        atomic_set(&dev -> timervalue, INVAtimer);
    }
    
    value = atomic_read(&dev -> timervalue);
    ret = copy_to_user(buf, &value, sizeof(value));


    return ret;
}
/* Chrdev Operations */
static const struct file_operations timer_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = timer_open,                /* Device Open file       */
    .release = timer_release,             /* Device Close file      */
    .write   = timer_write,               /* Device Write file      */
    .read    = timer_read                 /* Device Read File       */
};

/* timer IO initial  */
static int timerio_init(struct timer_dev *dev)
{
    int ret = 0;

    dev -> nd = of_find_node_by_path("/timer");
    if(dev -> nd == NULL)
    {
        ret = -EINVAL;
        goto fail_nd;
    }
    
    dev -> timer_gpio = of_get_named_gpio(dev -> nd, "timer-gpios", 0);
    if(dev -> timer_gpio < 0)
    {
        ret = -EINVAL;
        goto fail_gpio;
    }
    ret = gpio_request(dev -> timer_gpio, "timer0");
    if(ret)
    {
        ret = -EINVAL;
        printk("IO %d cannot request~\r\n", dev -> timer_gpio);
        goto fail_request;
    }
    
    ret = gpio_direction_input(dev -> timer_gpio);
    if(ret)
    {
        ret = -EINVAL;
        goto fail_input;
    }
    
    return 0;


fail_input :
    gpio_free(dev -> timer_gpio);
fail_request :
fail_gpio :
fail_nd :
    return ret;
}

/* Entry Point Function */
static int __init timer_init(void)
{
    int ret = 0;        /* For Error Happen */

    atomic_set(&timer.timervalue.timervalue, INVAtimer);    

    /* 1.Chardev Registry */
    timer.major = 0;
    
    if(timer.major)   /* Assign Device ID */
    {
        timer.devid = MKDEV(timer.major, 0);
        ret = register_chrdev_region(timer.devid, timer_CNT, timer_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&timer.devid, 0, timer_CNT, timer_NAME);
        timer.major = MAJOR(timer.devid);
        timer.minor = MINOR(timer.devid);
    }
    /* Fail Device ID */
    if(ret < 0)
    {
        goto fail_devid;
    }

    printk("timer Major = %d\r\n, minor = %d\r\n", timer.major, timer.minor);

    /* 2.Chrdev Initial  */
    timer.cdev.owner = THIS_MODULE;
    cdev_init(&timer.cdev, &timer_fops);

    /* 3.Add Chardev to Kernel */
    ret = cdev_add(&timer.cdev, timer.devid, timer_CNT);

    /* Fail Cdev Add */
    if(ret < 0)
    {
        goto fail_cdev;
    }
    /* 4.Add Device class */
    timer.class = class_create(THIS_MODULE, timer_NAME);
    if(IS_ERR(timer.class))
    {
        ret = PTR_ERR(timer.class);
        goto fail_class;
    }

    /* 5.Create Device */
    timer.device = device_create(timer.class, NULL, timer.devid, NULL, timer_NAME);
    if(IS_ERR(timer.device))
    {
        ret = PTR_ERR(timer.device);
        goto fail_device;
    }
    
    ret = timerio_init(&timer);
    if(ret < 0)
    {
        goto fail_device;
    }


    return 0;

fail_device :
    class_destoy(timer.class);
fail_class :
    cdev_del(&timer.cdev);
fail_cdev :
    unregister_chrdev_region(timer.devid, timer_CNT);
fail_devid :
    return ret;
}

/* Exit Point Function */
static void __exit timer_exit(void)
{
    /* timer OFF When we exit Module */
    gpio_set_value(timer.timer_gpio, 1);      /* Set timer as High Voltage (OFF) */

    /* Unregistry Chrdev */
    cdev_del(&timer.cdev);
    unregister_chrdev_region(timer.devid, timer_CNT);

    /* Destroy Device => Class */
    device_destroy(timer.class, timer.devid);
    class_destroy(timer.classs);

    /* FREE timer GPIO */
    gpio_free(timer.timer_gpio);
}


/* Module Registry */
module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");