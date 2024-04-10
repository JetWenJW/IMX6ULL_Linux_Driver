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


#define KEY_CNT     1
#define KEY_NAME    "key"

#define KEY_0_VALUE     0xF0         
#define INVAKEY         0x00      

struct key_dev 
{
    dev_t devid;            /* Device ID           */
    int major;              /* Major Device ID     */
    int minor;              /* minot Device ID     */
    struct cdev cdev;       /* For Char Device     */
    struct device *device;  /* For Device          */
    struct class *class;    /* For class Function  */
    struct device_node *nd; /* Device Node         */
    int key_gpio;           /* IO Number(ID)       */
    atomic_t keyvalue;      /* atomic Operation    */
};

struct key_dev key; /*  struct Declare */

static int key_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &key;
    return 0;
}

static int key_release(struct inode *inode, struct file *filp)
{
    struct key_dev *dev = (struct key_dev *)filp -> private_data;
    return 0;
}

static ssize_t key_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
   
    return ret;
}

static ssize_t key_read(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    int value = 0;
    struct key_dev *dev = (struct key_dev *)filp -> private_data;

    if(gpio_get_value(dev -> key_gpio) == 0)        /* Press KEY           */
    {
        while(!gpio_get_value(dev -> key_gpio));    /* Wait to release KEY */
        atomic_set(&dev -> keyvalue, KEY_0_VALUE);
    }
    else
    {
        atomic_set(&dev -> keyvalue, INVAKEY);
    }
    
    value = atomic_read(&dev -> keyvalue);
    ret = copy_to_user(buf, &value, sizeof(value));


    return ret;
}
/* Chrdev Operations */
static const struct file_operations key_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = key_open,                /* Device Open file       */
    .release = key_release,             /* Device Close file      */
    .write   = key_write,               /* Device Write file      */
    .read    = key_read                 /* Device Read File       */
};

/* Key IO initial  */
static int keyio_init(struct key_dev *dev)
{
    int ret = 0;

    dev -> nd = of_find_node_by_path("/key");
    if(dev -> nd == NULL)
    {
        ret = -EINVAL;
        goto fail_nd;
    }
    
    dev -> key_gpio = of_get_named_gpio(dev -> nd, "key-gpios", 0);
    if(dev -> key_gpio < 0)
    {
        ret = -EINVAL;
        goto fail_gpio;
    }
    ret = gpio_request(dev -> key_gpio, "key0");
    if(ret)
    {
        ret = -EINVAL;
        printk("IO %d cannot request~\r\n", dev -> key_gpio);
        goto fail_request;
    }
    
    ret = gpio_direction_input(dev -> key_gpio);
    if(ret)
    {
        ret = -EINVAL;
        goto fail_input;
    }
    
    return 0;


fail_input :
    gpio_free(dev -> key_gpio);
fail_request :
fail_gpio :
fail_nd :
    return ret;
}

/* Entry Point Function */
static int __init key_init(void)
{
    int ret = 0;        /* For Error Happen */

    atomic_set(&key.keyvalue.keyvalue, INVAKEY);    

    /* 1.Chardev Registry */
    key.major = 0;
    
    if(key.major)   /* Assign Device ID */
    {
        key.devid = MKDEV(key.major, 0);
        ret = register_chrdev_region(key.devid, KEY_CNT, KEY_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
        key.major = MAJOR(key.devid);
        key.minor = MINOR(key.devid);
    }
    /* Fail Device ID */
    if(ret < 0)
    {
        goto fail_devid;
    }

    printk("Key Major = %d\r\n, minor = %d\r\n", key.major, key.minor);

    /* 2.Chrdev Initial  */
    key.cdev.owner = THIS_MODULE;
    cdev_init(&key.cdev, &key_fops);

    /* 3.Add Chardev to Kernel */
    ret = cdev_add(&key.cdev, key.devid, KEY_CNT);

    /* Fail Cdev Add */
    if(ret < 0)
    {
        goto fail_cdev;
    }
    /* 4.Add Device class */
    key.class = class_create(THIS_MODULE, KEY_NAME);
    if(IS_ERR(key.class))
    {
        ret = PTR_ERR(key.class);
        goto fail_class;
    }

    /* 5.Create Device */
    key.device = device_create(key.class, NULL, key.devid, NULL, KEY_NAME);
    if(IS_ERR(key.device))
    {
        ret = PTR_ERR(key.device);
        goto fail_device;
    }
    
    ret = keyio_init(&key);
    if(ret < 0)
    {
        goto fail_device;
    }


    return 0;

fail_device :
    class_destoy(key.class);
fail_class :
    cdev_del(&key.cdev);
fail_cdev :
    unregister_chrdev_region(key.devid, KEY_CNT);
fail_devid :
    return ret;
}

/* Exit Point Function */
static void __exit key_exit(void)
{
    /* key OFF When we exit Module */
    gpio_set_value(key.key_gpio, 1);      /* Set key as High Voltage (OFF) */

    /* Unregistry Chrdev */
    cdev_del(&key.cdev);
    unregister_chrdev_region(key.devid, KEY_CNT);

    /* Destroy Device => Class */
    device_destroy(key.class, key.devid);
    class_destroy(key.classs);

    /* FREE key GPIO */
    gpio_free(key.key_gpio);
}


/* Module Registry */
module_init(key_init);
module_exit(key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");