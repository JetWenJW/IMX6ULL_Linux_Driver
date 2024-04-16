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
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/ide.h>
#include <linux/platform_device.h>

/*
 * This Part of Program is Driver Module
 * Beacuse for Platform Device is a module & Drive is also a module 
 * Purpose: We are going to code Platform driver by Device Tree 
 */

#define GPIOLED_CNT     1
#define GPIOLED_NAME    "dtsplatformled"

#define LEDOFF          0
#define LEDON           1

struct gpioled_dev 
{
    dev_t devid;            /* Device ID           */
    int major;              /* Major Device ID     */
    int minor;              /* minot Device ID     */
    struct cdev cdev;       /* For Char Device     */
    struct device *device;  /* For Device          */
    struct class *class;    /* For class Function  */
    struct device_node *nd  /* Device Node         */
    int led_gpio;           /* IO Number(ID)       */
};

struct gpioled_dev gpioled; /*  struct Declare*/

static int led_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &gpioled;
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = (struct gpioled_dev *)filp -> private_data;
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
    else if(datdbuf[0] == LEDOFF)
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
    .release = led_release              /* Device Close file      */
    .write   = led_write                /* Device Write file      */
};
/* Step4. Code Led Probe Functions (When Device & Driver matched this function will be called) */
static int led_probe(struct platform_device *dev)
{
    int ret = 0;        /* For Error Happen */


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
        ret = PTR_ERR(gpiolde.device);
        goto fail_devices;
    }

#if 0   /* Once We matched via Platform that absolutely contain node data */
    /* A. Add Device Node */
    gpioled.nd = of_find_node_by_path("/gpioled");

    if(gpioled.nd == NULL)      /* Fail To find Node */
    {
        ret = -EINVAL;
        goto fail_find_node;
    }
#endif
    /*
     * Platform also provide lots of API function
     * to get node data from Device Tree
     * purpose: We'd better get used to it.
     */
    gpioled.nd = dev->dev.of_node;     /* oneline to get node data */

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
fail_rs ;
fail_find_node :
    device_destroy(gpioled.class, gpioled.devid);
fail_device :
    class_destoy(gpioled.class);
fail_class :
    cdev_del(&gpioled.cdev);
fail_cdev :
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
fail_devid :
    return ret;
}

/* Step5. Code Led Remove Function (When Device & Driver unmatched this function will be called) */
static int led_remove(struct platform_device *dev)
{
    /* LED OFF */
    gpio_set_value(gpioled.led_gpio, 1);

    /* Unregistry Chrdev */
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    /* Destroy Device => Class */
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.classs);

    /* Free The IO we just Request */
    gpio_free(gpioled.led_gpio);
}

/* Step3. Declare an array using to matched Device & Driver */
struct of_device_id led_of_match[] =
{
    {.compatible = "JetWen,gpioled"},
    {/* Sentinel */},
};

/* Step2. Platform Driver */
struct platform_driver led_driver =
{
    .driver = 
    {
        .name = "imx6ull-led",          /* To Use to matched Device & Driver without Device Tree */
        .of_match_table = led_of_match, /* Devcie Tree matched Table                             */
    
    },
    .probe   = led_probe,
    .remove  = led_remove,
};

/* Step1. Load Driver */
static int __init ledDriver_init(void)
{
    /* Registry Platform Driver */
    return platform_driver_register(&led_driver);
}

/* Step1. Unload Driver */
static void __exit ledDriver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(ledDriver_init);
module_exit(ledDriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");


