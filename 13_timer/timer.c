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
#include <linux/timer.h>
#include <linux/jiffies.h>


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
    struct timer_list tmr;  /* For Software Timer  */
    int led_gpio;           /* For IO Number       */
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
    .owner   = THIS_MODULE,               /* The owner of This file */
    .open    = timer_open,                /* Device Open file       */
    .release = timer_release,             /* Device Close file      */
    .write   = timer_write,               /* Device Write file      */
    .read    = timer_read                 /* Device Read File       */
};

/* 
 * Timer IRQ_Handler Function 
 * (When Timer Expire, it will call this Function)
 */
static void timer_func(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)arg;
    static int stat = 1;

    stat = !stat;
    gpio_set_value(dev -> led_gpio, stat);

    /* Restart Timer, it'll call this function when timer expired */
    mod_timer(&dev -> timer, jiffies + msecs_to_jiffies(500));
}

/* LED IO Initial */
int led_init(struct timer_dev timer)
{
    int ret = 0;

    /* A. Find IO Node Path */
    dev -> nd = of_find_node_by_path("/gpioled");
    if(dev -> nd == NULL)
    {
        ret = -EINVAL;
        goto fail_find_node;
    }
    
    /* B. Get Device Node */
    dev -> led_gpio = of_get_named_gpio(dev -> nd, "led-gpios", 0);
    if(dev -> led_gpio < 0)
    {
        ret = -EINVAL;
        goto fail_gpio;
    }
    
    /* C. IO request to Use */
    ret = gpio_request(dev->led_gpio, "led");
    if(ret)
    {
        ret = -EBUSY;
        printk("IO %d Cannot Request~\r\n", dev -> led_gpio);
        goto fail_request;
    }
    
    /* D. Set IO Direction */
    ret = gpio_direction_output(dev -> led_gpio, 1);    /* Default LED OFF */
    if(ret < 0)
    {
        ret = -EINVAL;
        goto fail_gpio_set;
    }
    
    return 0;

fail_gpio_set :
    gpio_free(dev -> led_gpio);
fail_request :
fail_gpio :
fail_find_node : 
    return ret;
}





/* Entry Point Function */
static int __init timer_init(void)
{
    int ret = 0;        /* For Error Happen */


    /* 1.Chardev Registry */
    timer.major = 0;
    
    if(timer.major)   /* Assign Device ID */
    {
        timer.devid = MKDEV(timer.major, 0);
        ret = register_chrdev_region(timer.devid, TIMER_CNT, TIMER_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&timer.devid, 0, TIMER_CNT, TIMER_NAME);
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
    ret = cdev_add(&timer.cdev, timer.devid, TIMER_CNT);

    /* Fail Cdev Add */
    if(ret < 0)
    {
        goto fail_cdev;
    }
    /* 4.Add Device class */
    timer.class = class_create(THIS_MODULE, TIMER_NAME);
    if(IS_ERR(timer.class))
    {
        ret = PTR_ERR(timer.class);
        goto fail_class;
    }

    /* 5.Create Device */
    timer.device = device_create(timer.class, NULL, timer.devid, NULL, TIMER_NAME);
    if(IS_ERR(timer.device))
    {
        ret = PTR_ERR(timer.device);
        goto fail_device;
    }

    /* 6. S/W Timer Initial */

    ret = led_init(&timer);               /* Must Earlier than Timer Initial */
    if(ret < 0)
    {
        goto fail_led_init;
    }
    init_timer(&timer.tmr);

    timer.tmr.function = timer_func;
    timer.tmr.expires = jiffies + msecs_to_jiffies(500);        /* 500ms Timer Expires */
    timer.tmr.data = (unsigned long)&timer;                     /* Set Parameter       */
    add_timer(&timer.tmr);                                      /* Add TImer to System */


    return 0;

fail_led_init :
fail_device :
    class_destoy(timer.class);
fail_class :
    cdev_del(&timer.cdev);
fail_cdev :
    unregister_chrdev_region(timer.devid, TIMER_CNT);
fail_devid :
    return ret;
}

/* Exit Point Function */
static void __exit timer_exit(void)
{
    /* LEDOFF */
    gpio_set_value(timer.led_gpio, 1);

    /* Delete Timer */
    del_timer(&timer.tmr);

    /* Unregistry Chrdev */
    cdev_del(&timer.cdev);
    unregister_chrdev_region(timer.devid, TIMER_CNT);

    /* Destroy Device => Class */
    device_destroy(timer.class, timer.devid);
    class_destroy(timer.classs);

    /* Reliase IO */
    gpio_free(timer.led_gpio);
}


/* Module Registry */
module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");