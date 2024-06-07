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
#include <linux/miscdevice.h>

/*
 * This Part of Program is Driver Module
 * Beacuse for Platform Device is a module & Drive is also a module 
 * Purpose : We are going to code Platform driver by Device Tree 
 * Trigger : Beep
 */
#define MISCBEEP_NAME   "miscbeep"
#define MISCBEEP_MINOR  144

#define BEEP_OFF        0
#define BEEP_ON         1

/* 5. Device Structure of MiscBeep */
struct miscbeep_dev 
{
	struct device_node *nd;
    int beep_gpio;
};

struct miscbeep_dev miscbeep;


/* 7_2. operaions Function */
static int miscbeep_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &miscbeep;
    return 0;
}

static int miscbeep_release(struct inode *inode, struct file *filp)
{
    struct miscbeep_dev *dev = (struct miscbeep_dev *)filp -> private_data;
    return 0;
}

static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{

    int ret = 0;
    unsigned char databuf[1];

    /* 1. GEt The Private Data */
    struct miscbeep_dev *dev = (struct miscbeep_dev *)filp -> private_data;
    ret = copy_from_user(databuf, buf, count);
    if(ret < 0)
    {
        return -EINVAL;
    }
    
    /* 2. Control Beep : ON/OFF */
    if(databuf[0] == BEEP_ON)
    {
        gpio_set_value(dev->beep_gpio, 0); /* ON  */
    }
    else if(databuf[0] == BEEP_OFF)
    {
        gpio_set_value(dev->beep_gpio, 1); /* OFF */
    }

    return 0;
}

/* 7. Chrdev Operations */
static const struct file_operations miscbeep_fops =
{
    .owner   = THIS_MODULE,             /* The owner of This file */
    .open    = miscbeep_open,                /* Device Open file       */
    .release = miscbeep_release,              /* Device Close file      */
    .write   = miscbeep_write                /* Device Write file      */
};

/* 6. MISC Device structure*/
static struct miscdevice beep_miscdev =
{
    .minor = MISCBEEP_MINOR,
    .name  = MISCBEEP_NAME,
    .fops  = &miscbeep_fops,
};


/* 4. Probe Function */
static int miscbeep_probe(struct platform_device *dev)
{
    int ret = 0;
    /* A_1. Beep IO INitial */
    miscbeep.nd = dev->dev.of_node;     /* Find Device Node Via Platform */
    miscbeep.beep_gpio = of_get_named_gpio(miscbeep.nd, "beep-gpios", 0);
    if(miscbeep.beep_gpio < 0)
    {
        ret = -EINVAL;
        goto fail_findgpio;
    }
    /* A_2. Requset IO port */
    ret = gpio_request(miscbeep.beep_gpio, "beep-gpios");           /* Requset IO port */
    if(ret)
    {
        printk("Can't Request %d gpio~\r\n", miscbeep.beep_gpio);
        ret = -EINVAL;
        goto fail_findgpio;
    }
    /* A_3. Set IO as Input/Output */
    ret = gpio_direction_output(miscbeep.beep_gpio, 1);             /* Set Output as */
    if(ret < 0)
    {
        goto fail_setoutput;
    }

    /* B_1. MISC Registry */
    ret = misc_register(&beep_miscdev);
    if(ret < 0)
    {
        goto fail_setoutput;
    }

    return 0;

fail_setoutput :
    gpio_free(miscbeep.beep_gpio);               /* if Error Free IO */
fail_findgpio :
    return ret;
}
/* 4. Remove Function */
static int miscbeep_remove(struct platform_device *dev)
{
    gpio_set_value(miscbeep.beep_gpio, 1);       /* Beep OFF               */
    gpio_free(miscbeep.beep_gpio);               /* Free IO                */

    misc_deregister(&beep_miscdev);         /* Deregister MISC Device */
    return 0;
}



/* 3. Platform Match Table */
static const struct of_device_id beep_of_match[] =
{
    {.compatible = "JetWen,beep"},
    {/* Sentinel */},
};





/* 2. Platform Structure */
static struct platform_driver miscbeep_driver = 
{
    .driver =
    {
        .name = "JetWen,beep",              /* Must as same as Device Tree */
        .of_match_table = beep_of_match,
    },
    .probe  = miscbeep_probe,               /* Probe Function         */   
    .remove = miscbeep_remove,              /* Remove Driver Function */
};

/* 1. Entry Point of Module */
static int __init miscbeep_init(void)
{
    return platform_driver_register(&miscbeep_driver);
}

/* 1. Exit Point of Module */
static void __exit miscbeep_exit(void)
{
    platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GLP");
MODULE_AUTHOR("JetWen");
