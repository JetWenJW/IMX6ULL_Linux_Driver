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
#include <linux/atomic.h>

#define GPIOLED_CNT     1
#define GPIOLED_NAME    "gpioled"

#define LEDOFF          0
#define LEDON           1

#define oneline_test    0


struct gpioled_dev 
{
    dev_t devid;             /* Device ID           */
    int major;               /* Major Device ID     */
    int minor;               /* minot Device ID     */
    struct cdev cdev;        /* For Char Device     */
    struct device *device;   /* For Device          */
    struct class *class;     /* For class Function  */
    struct device_node *nd;  /* Device Node         */
    int led_gpio;            /* IO Number(ID)       */

    atomic_t lock;          /* atomic operation    */
};

struct gpioled_dev gpioled; /*  struct Declare*/

static int led_open(struct inode *inode, struct file *filp)
{

    filp -> private_data = &gpioled;

    /* Cannot access driver */
    if(!atomic_dec_and_test((&gpioled.lock)))
    {
        atomic_inc(&gpioled.lock);
        return -EBUSY;
    }

#if oneline_test
    /* If lock smaller than 0, refering to BUSY */
    if(atomic_read(&gpioled.lock) < 0)
    {
        return -EBUSY;                  /* Not allow to access resource */
    }
    else
    {
        atomic_dec(&gpioled.lock);      /* lock auto minus one (-1)     */
    }
#endif

    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = (struct gpioled_dev *)filp -> private_data;
    
    /* Release the Lock */
    atomic_inc(&dev -> lock);           /* Auto Plus 1 to release lock   */

    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    unsigned char databuf[1];

    /* 1. GEt The Private Data */
    struct gpioled_dev *dev = (struct gpioled_dev *)filp -> private_data;
    ret = copy_from_user(databuf, buf, count);
    if(ret < 0)
    {
        return -EINVAL;
    }
    
    /* 2. Control LED : ON/OFF */
    if(databuf[0] == LEDON)
    {
        gpio_set_value(dev -> led_gpio, 0);
    }
    else if(databuf[0] == LEDOFF)
    {
        gpio_set_value(dev -> led_gpio, 1);
    }


    return 0;
}
/* Chrdev Operations */
static const struct file_operations led_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = led_open,                /* Device Open file       */
    .release = led_release,             /* Device Close file      */
    .write   = led_write,               /* Device Write file      */
};

/* Entry Point Function */
static int __init gpioled_init(void)
{
    int ret = 0;        /* For Error Happen */
    
    /* atomic Initial */
    atomic_set(&gpioled.lock, 1);

    /* 1.Chardev Registry */
    gpioled.major = 0;
    
    if(gpioled.major)   /* Assign Device ID */
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    /* Fail Device ID */
    if(ret < 0)
    {
        goto fail_devid;
    }

    /* 2.Chrdev Initial  */
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &led_fops);

    /* 3.Add Chardev to Kernel */
    ret = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    /* Fail Cdev Add */
    if(ret < 0)
    {
        goto fail_cdev;
    }
    /* 4.Add Device class */
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.class))
    {
        ret = PTR_ERR(gpioled.class);
        goto fail_class;
    }

    /* 5.Create Device */
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if(IS_ERR(gpioled.device))
    {
        ret = PTR_ERR(gpioled.device);
        goto fail_devices;
    }
    
    /* A. Add Device Node */
    gpioled.nd = of_find_node_by_path("/gpioled");

    if(gpioled.nd == NULL)      /* Fail To find Node */
    {
        ret = -EINVAL;
        goto fail_find_node;
    }

    /* B. Get GPIO Number(ID) to accesss */
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
    if(gpioled.led_gpio < 0)
    {
        printk("Cannot Find Led GPIO\r\n");
        ret = -EINVAL;
        goto fail_find_node;
    }
    printk("led_gpio NUmber = %d\r\n", gpioled.led_gpio);

    /* C. Request IO to use it */
    ret = gpio_request(gpioled.led_gpio, "led-gpio");
    
    if(ret)     /* Fail to Request */
    {
        printk("Failed to request the Led GPIO\r\n");
        ret = -EINVAL;
        goto fail_find_node;
    }

    /* D. Use The IO we Just Request */
    ret = gpio_direction_output(gpioled.led_gpio, 1);               /* Set GPIO as High Voltage(OFF) */
    if(ret)
    {
        goto fail_setoutput;
    }

    /* E. Set GPIO as Low Voltage(ON) */
    gpio_set_value(gpioled.led_gpio, 0);

    return 0;

fail_setoutput :
    gpio_free(gpioled.led_gpio);
fail_rs :
fail_find_node :
    device_destroy(gpioled.class, gpioled.devid);
fail_devices :
    class_destroy(gpioled.class);
fail_class :
    cdev_del(&gpioled.cdev);
fail_cdev :
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
fail_devid :
    return ret;
}

/* Exit Point Function */
static void __exit gpioled_exit(void)
{
    /* LED OFF */
    gpio_set_value(gpioled.led_gpio, 1);

    /* Unregistry Chrdev */
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    /* Destroy Device => Class */
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);

    /* Free The IO we just Request */
    gpio_free(gpioled.led_gpio);

}


/* Module Registry */
module_init(gpioled_init);
module_exit(gpioled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");
