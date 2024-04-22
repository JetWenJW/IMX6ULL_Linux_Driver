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
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include "ap3216creg.h"

/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add SPI ICM20608 sub-device node to Device Tree
 * 2. Build SPI Driver Framework
 * 3. Accompilshed the Framework
 */

#define ICM20608_CNT        1
#define ICM20608_NAME       "icm20608"




/* Step6. SPI Structure */
struct icm20608_dev
{
    int major;
    int minor;
    dev_t devid;
    struct cdev *cdev;
    struct class *class;
    struct device *device;
    void *private_data;
};

struct icm20608_dev icm20608;       /* Decalre Structure */

static struct icm20608_open(struct inode *inode, struct file *filp)
{

}

ssize_t icm20608_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{

}

static int icm20608_release(struct inode *inode, struct file *filp)
{

}

static const struct file_operaions icm20608_fops = 
{
    .owner   = THIS_MODULE,
    .open    = icm20608_open,
    .read    = icm20608_read,
    .release = icm20608_release,
};





/* Step5. Probe Function */
static int icm20608_probe(struct spi_device *spi)
{
    int ret = 0;
    printk("icm20608_Probe\r\n");

    /* 1. Register Char Device */
    icm20608dev.major = 0;
    if(icm20608dev.major)
    {
        icm20608.devid = MKDEV(icm20608.major, 0);
        ret = register_chrdev_region(icm20608.devid, ICM20608_CNT, ICM20608_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&icm20608.devid, 0, ICM20608_CNT, ICM20608_NAME);
        icm20608.major = MAJOR(icm20608.devid);
        icm20608.monir = MINOR(icm20608.devid);
    }
    if(ret < 0)
    {
        printk("icm20608_chrdev_region_Err\r\n");
        goto fail_devid;
    }
    printk("icm20608 major = %d, monir = %d\r\n", icm20608.major, icm20608.minor);

    icm20608dev.cdev.owner = THIS_MODULE;
    cdev_init(&icm20608.cdev, &icm20608_fops);
    ret = cdev_add(&icm20608.cdev, icm20608dev.devid, ICM20608_CNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    icm20608dev.class = class_create(THIS_MODULE, ICM20608_NAME);
    if(IS_ERR(icm20608dev.class))
    {
        ret = PTR_ERR(icm20608dev.class);
        goto fail_class;
    }

    icm20608dev.device = device_create(icm20608dev.class, NULL, icm20608dev.device, NULL, ICM20608_NAME);
    if(IS_ERR(icm20608dev.device))
    {
        ret = PTR_ERR(icm20608dev.device);
        goto fail_device;
    }

    icm20608dev.private_data = spi;

    return 0;

fail_device :
    class_destroy(icm20608dev.class);

fail_class :
    cdev_del(&icm20608dev.cdev);

fail_cdev : 
    unregister_chrdev_region(icm20608dev.devid, ICM20608_CNT);
    
fail_devid :

    return ret;

}

/* Step5. Remove Function */
static int icm20608_remove(struct spi_device *spi)
{
    int ret = 0;

    cdev_del(&icm206058.cdev);

    unregister_chrdev_region(icm20608dev.devid, ICM20608_CNT);

    device_destroy(icm20608.class, icm20608dev.devid);

    class_destroy(icm20608.class);


    return ret;
}



/* Step4. Traditional match */
struct spi_device_id icm20608_id[] =
{
    {"JetWen,icm20608", 0},
    {},
};

/* Step3. Matched with Device Tree */
static const struct of_device_id icm20608_of_match[] =
{
    { .compatible = "JetWen,icm20608"},
    {},
};

/* Step2. SPI Driver Structure */
struct spi_driver icm20608_driver
{
    .probe  = icm20608_probe,
    .remove = icm20608_remove,

    .driver = 
    {
        .name = "icm20608",
        .owner = THIS_MODULE,
        .of_match_table = icm20608_of_match,
    },
    .id_table = icm20608_id,
};




/* Step1. Entry Point Function */
static int __init icm20608_init(void)
{
    int ret = 0;        
    
    /* A. SPI Initial */
    ret = spi_register_driver(&icm20608_driver);

    return ret;
}

/* Step1. Exit Point Function */
static void __exit icm20608_exit(void)
{
    spi_unregister_driver(&icm20608_driver);
}


/* Step1. Module Registry */
module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");