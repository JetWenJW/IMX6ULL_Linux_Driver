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


#define IMX6UIRQ_CNT     1
#define IMX6UIRQ_NAME    "imx6uirq"

#define KEY_NUM          1
#define KEY0VALUE        0x01
#define INVAKEY          0xFF

/* IRQ of Key Structure */
struct irq_keydesc
{
    int gpio;                               /* IO NUmber        */
    int irqnum;                             /* IRQ Number       */
    unsigned char value;                    /* Key Value        */
    char name[10];                          /* Interrupt name   */                                
    irqreturn_t (*handler)(int, void *)     /* IRQ Handler      */
};


struct imx6uirq_dev 
{
    dev_t devid;                            /* Device ID           */
    int major;                              /* Major Device ID     */
    int minor;                              /* minot Device ID     */
    struct cdev cdev;                       /* For Char Device     */
    struct device *device;                  /* For Device          */
    struct class *class;                    /* For class Function  */
    struct device_node *nd;                 /* Device Node         */
    struct irq_keydesc irqkey[KEY_NUM];     /* For Interrput       */
    struct timer_list timer;                /* For Key filter      */

    atomic_t keyvalue;                      /* For Get Key Value   */
    atomic_t releasekey;                    /* For Detect Release  */

    struct fasync_struct *fasync_queue;     /* For Async Notice    */
};

struct imx6uirq_dev imx6uirq; /*  struct Declare */

static int imx6uirq_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &imx6uirq;
    return 0;
}

static int imx6uirq_fasync(int fd, struct file *filp, int on)
{
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp -> private_data; 
    return fasync_helper(fd, filp, on, &dev->fasync_queue);
}

static int imx6uirq_release(struct inode *inode, struct file *filp)
{

    //struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp -> private_data;
    return imx6uirq_fasync(-1, filp, 0);
}

static ssize_t imx6uirq_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
   
    return ret;
}

static ssize_t imx6uirq_read(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    
    unsigned char keyvalue;
    unsigned char releasekey;
    struct imx6uirq_dev *dev = filp->private_data;
    
    keyvalue    = atomic_read(&dev->keyvalue);
    releasekey  = atomic_read(&dev->releasekey);

    if(releasekey)/* Valid KEY */
    {
        if(keyvalue & 0x80)
        {
            keyvalue &= ~(0x80);
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }
        else
        {
            goto data_error;
        }
        atomic_set(&dev->releasekey, 0);/* Clear Flags */
    }
    else
    {
        goto data_error;
    }   

    return ret;

data_error : 
    return -EINVAL;
}



/* Chrdev Operations */
static const struct file_operations imx6uirq_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = imx6uirq_open,           /* Device Open file       */
    .read    = imx6uirq_read,           /* Device Read File       */
    .fasync  = imx6uirq_fasync,         /* Asynchronize           */ 
    .release = imx6uirq_release         /* For File Release       */
};


/* KEY_IRQ Handler */
static irqreturn_t key0_handler(int irq, void *dev_id)
{
    int value = 0;
    struct imx6uirq_dev *dev = dev_id;
    /*
     * This Part Only need to Enable Timer
     * The Key Value for IRQ Handler to handle
     */
    dev->timer.data = (unsigned long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(20)); /* 20 ms delay as filter */

    return IRQ_HANDLED;
}

/* Timer Handler  */
static void timer_func(unsigned long arg)
{
    int value = 0;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg;

    value = gpio_get_value(dev->irqkey[0].gpio);
    if(value == 0)      /* Press KEY */
    {
        atomic_set(&dev->keyvalue, dev->irqkey[0].value);
    }
    else if(value == 1) /* Release KEY */
    {
        atomic_set(&dev->keyvalue, 0x80 | (dev->irqkey[0].value));
        atomic_set(&dev->releasekey, 1);
    }


    /* For Asynchronize Notice */
    if(atomic_read(&dev->releasekey))          /* Valid Key  */
    {
        if(dev->fasync_queue)
        {
            kill_fasync(&dev->fasync_queue, SIGIO, POLLIN);
        }
    }

}

/* key IO initial  */
static int keyio_init(struct imx6uirq_dev *dev)
{
    int ret = 0;
    int i = 0;

    /* A. Key Initial */
    /* A_1. Find Node from Device Tree */
    dev -> nd = of_find_node_by_path("/imx6uirq");
    if(dev -> nd == NULL)
    {
        ret = -EINVAL;
        goto fail_nd;
    }
    
    /* A_2. Get IO Number of Key */
    for(i = 0; i < KEY_NUM; i++)
    {
        dev -> irqkey[1].gpio = of_get_named_gpio(dev -> nd, "key-gpios", i);
        if(dev -> irqkey[1].gpio < 0)
        {
            ret = -EINVAL;
            goto fail_gpio;
        }
    }

    /* A_3 Initial IO */
    for(i = 0;i < KEY_NUM;i++)/* Uss for loop Because we might have muiltiple Device */
    {
        memset(dev -> irqkey[1].name, 0, sizeof(dev->irqkey[1].name));
        sprintf(dev->irqkey[i].name, "KEY%d", i);

        ret = gpio_request(dev -> irqkey[1].gpio, dev->irqkey[i].name);
        if(ret)
        {
            ret = -EINVAL;
            printk("IO %d cannot request~\r\n", dev -> irqkey[1].gpio);
            goto fail_request;
        }
        gpio_direction_input(dev->irqkey[i].gpio); /* Set Pin as Input */
        ret = gpio_direction_input(dev->irqkey[i].gpio);
        if(ret)
        {
            ret = -EINVAL;
            goto fail_input;
        }

        /* 1. Get IRQ Number */
        dev->irqkey[1].irqnum = gpio_to_irq(dev->irqkey[i].gpio); /* Get IRQ Number */
#if 0
        dev->irqkey[i].irqnum = irq_of_parse_and_map(dev->nd, i);
#endif
    }

    dev->irqkey[0].handler  = key0_handler;
    dev->irqkey[0].value    = KEY0VALUE;

    /* B. Key Interrupt Innitial  */
    for(i = 0;i < KEY_NUM;i++)
    {
        ret = request_irq(dev->irqkey[i].irqnum, dev->irqkey[i].handler, 
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                        dev->irqkey[i].name, &imx6uirq);
        if(ret)
        {
            printk("irq %d request failed\r\n", dev->irqkey[i].irqnum);
            goto fail_irq;
        }
    }

    /* C. Initial Timer in order to Key filter */
    init_timer(&imx6uirq.timer);
    imx6uirq.timer.function = timer_func;

    return 0;

fail_irq : 
fail_input :
    for(i = 0;i < KEY_NUM;i++)
    {
        gpio_free(dev->irqkey[i].gpio);    
    }
fail_request :
fail_gpio :
fail_nd :
    return ret;
}

/* Entry Point Function */
static int __init imx6uirq_init(void)
{
    int ret = 0;        /* For Error Happen */

    atomic_set(&imx6uirq.imx6uirqvalue.imx6uirqvalue, INVAimx6uirq);    

    /* 1.Chardev Registry */
    imx6uirq.major = 0;
    
    if(imx6uirq.major)   /* Assign Device ID */
    {
        imx6uirq.devid = MKDEV(imx6uirq.major, 0);
        ret = register_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&imx6uirq.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
        imx6uirq.major = MAJOR(imx6uirq.devid);
        imx6uirq.minor = MINOR(imx6uirq.devid);
    }
    /* Fail Device ID */
    if(ret < 0)
    {
        goto fail_devid;
    }

    printk("imx6uirq Major = %d\r\n, minor = %d\r\n", imx6uirq.major, imx6uirq.minor);

    /* 2.Chrdev Initial  */
    imx6uirq.cdev.owner = THIS_MODULE;
    cdev_init(&imx6uirq.cdev, &imx6uirq_fops);

    /* 3.Add Chardev to Kernel */
    ret = cdev_add(&imx6uirq.cdev, imx6uirq.devid, IMX6UIRQ_CNT);

    /* Fail Cdev Add */
    if(ret < 0)
    {
        goto fail_cdev;
    }
    /* 4.Add Device class */
    imx6uirq.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
    if(IS_ERR(imx6uirq.class))
    {
        ret = PTR_ERR(imx6uirq.class);
        goto fail_class;
    }

    /* 5.Create Device */
    imx6uirq.device = device_create(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
    if(IS_ERR(imx6uirq.device))
    {
        ret = PTR_ERR(imx6uirq.device);
        goto fail_device;
    }
    
    ret = imx6uirqio_init(&imx6uirq);
    if(ret < 0)
    {
        goto fail_device;
    }


    /* IO Initial */
    ret = keyio_init(&imx6uirq);
    if(ret < 0)
    {
        goto fail_keyinit;
    }
    
    /* Initial atomic Operations' Value */
    atomic_set(&imx6uirq.keyvalue, INVAKEY);
    atomic_set(&imx6uirq.releasekey, 0);

    return 0;
fail_keyinit :
fail_device :
    class_destroy(imx6uirq.class);
fail_class :
    cdev_del(&imx6uirq.cdev);
fail_cdev :
    unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);
fail_devid :
    return ret;
}

/* Exit Point Function */
static void __exit imx6uirq_exit(void)
{
    int i = 0;
    /* Relese Interrupt */
    for(i = 0;i < KEY_NUM;i++)
    {
        free_irq(imx6uirq.irqkey[i].irqnum, &imx6uirq);
    }

    /* FREE IO */
    for(i = 0;i < KEY_NUM;i++)
    {
        gpio_free(imx6uirq.irqkey[i].gpio);    
    }

    /* Delete Timer */
    del_timer_sync(&imx6uirq.timer);

    /* imx6uirq OFF When we exit Module */
    gpio_set_value(imx6uirq.imx6uirq_gpio, 1);      /* Set imx6uirq as High Voltage (OFF) */

    /* Unregistry Chrdev */
    cdev_del(&imx6uirq.cdev);
    unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);

    /* Destroy Device => Class */
    device_destroy(imx6uirq.class, imx6uirq.devid);
    class_destroy(imx6uirq.class);
}


/* Module Registry */
module_init(imx6uirq_init);
module_exit(imx6uirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");
