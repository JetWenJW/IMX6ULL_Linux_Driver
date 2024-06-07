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
#include <linux/of_irq.h>
#include <linux/of_address.h>

/*
 * The main purpose of this Project is 
 * 1. Create Device node via DeviceTree
 * 2. Code a Driver to get Property of Device Tree
 * 3.Use the Property we get Initialize Led & GPIO 
 */

#define DTSLED_CNT      1           /* Numerous of Device ID  */
#define DTSLED_NAME     "dtsled"    /* Name of Device ID      */

/* Define For LED to Use */
#define LEDOFF      0       /* OPEN  */
#define LEDON       1       /* CLOSE */


/* Virtual Address Pointer */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_GDIR;
static void __iomem *GPIO1_DR;

/* Device Structure */
struct dtsled_dev
{
    dev_t devid;                /* Device ID        */
    struct cdev cdev;           /* Char Device      */
    struct class *class;        /* Class            */
    struct device *device;      /* Device           */
    int major;                  /* Major Device     */
    int minor;                  /* minor            */
    struct device_node *nd;     /* Device Tree Node */
};

struct dtsled_dev dtsled;                   /* Led Device */

/* LED Switch Function */
static void led_switch(u8 state)
{
    u32 val = 0;

    if(state == LEDON)
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);             /* LED ON LED */
        writel(val, GPIO1_DR);
    }
    else if(state == LEDOFF)
    {
        val = readl(GPIO1_DR);
        val |= (1 << 3);              /* LED OFF LED */
        writel(val, GPIO1_DR);
    }
}

static int dtsled_open(struct inode *inode, struct file *filp)
{
    filp -> private_data = &dtsled;
    return 0;
}

static int dtsled_release(struct inode *inode, struct file *filp)
{
    struct dtsled_dev *dev = (struct dtsle_dev *)filp -> private_data;
    return 0;
}

static ssize_t dtsled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct dtsled_dev *dev = (struct dtsled_dev *)filp -> private_data;

    int retvalue;
    unsigned char databuf[1];

    retvalue = copy_from_user(databuf, buf, count);
    if(retvalue < 0)
    {
        return -EFAULT;
    }

    /* ON/OFF */
    led_switch(databuf[0]);

    return 0;
}

/* Char Device Operations */
static const struct file_operations dtsled_fops =
{
    .owner   = THIS_MODULE,
    .write   = dtsled_write,
    .open    = dtsled_open,
    .release = dtsled_release 
};



/* Entry Point */
static int __init dtsled_init(void)
{
    int ret = 0;
    const char *str;        /* For Read String Property */
    u32 regdata[10];        /* For Array of Property    */
    u8 i = 0;               /* For for loop to use      */
    unsigned int val;       /* For LED initial Function */

    /* 1.Register CHar Device by Device Tree */
    dtsled.major = 0;       /* Kernel Auto Assigned Device ID */
    if(dtsled.major)        /* Define Device ID               */
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    }
    else                /* Unassigned Device ID */
    {
        ret = alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }

    if(ret < 0)         /* Fail DEVICE ID */
    {
        goto fail_devid;
    }

    /* 2. Registery CHAR Device */
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);
    ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    /* 3.Auto Create Device Node */
    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if(IS_ERR(dtsled.class))
    {
        ret = PTR_ERR(dtsled.class);
        goto fail_class;
    }

    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if(IS_ERR(dtsled.device))
    {
        ret = PTR_ERR(dtsled.device);
        goto fail_device;
    }

    /* A. Get Device Tree Property */
    dtsled.nd = of_find_node_by_path("/alphaled");
    if(dtsled.nd == NULL)
    {
        ret = -EINVAL;
        goto fail_find_node;
    }

    /* A.1 Get String of Property */
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if(ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        printk("status = %s\r\n",str);
    }

    /* A.2 Get Compatible Property */
    ret = of_property_read_string(dtsled.nd, "compatible", &str);
    if(ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        printk("compatible = %s\r\n",str);
    }

    /* A.3 Get Property of Array (The most Important)*/
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if(ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        printk("reg data: \r\n");
        for(i = 0;i < 10; i++)
        {
            printk("%#X ", i, regdata[i]);

        }
        printk("\r\n");    
    }

    /* LED Initial (Memory Mapping )*/
    IMX6U_CCM_CCGR1     = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03   = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03   = ioremap(regdata[4], regdata[5]);
    GPIO1_DR            = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR          = ioremap(regdata[8], regdata[9]);

    /* LED Initial (Set bits) */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);                          /* Clear Pervious config bit[26:27] */
    val |= (3 << 26);                           /* Set bit[26:27] */
    writel(val, IMX6U_CCM_CCGR1);

    writel(0x5, SW_MUX_GPIO1_IO03);        /* Configured GPIO io03 */
    writel(0x10B0, SW_PAD_GPIO1_IO03);     /* Set Electronical Property */

    val = readl(GPIO1_GDIR);                    
    val |= (1 << 3);                            /* Set bit 3 = 1,Set as GPIO Output */
    writel(val, GPIO1_GDIR);

    val = readl(GPIO1_DR);
    val &= ~(1 << 3);                           /* Open LED by Default */
    writel(val, GPIO1_DR);

    return 0;

fail_rs :
fail_find_node :
    device_destroy(dtsled.class, dtsled.devid);
fail_device :
    class_destroy(dtsled.class);
fail_class :
    cdev_del(&dtsled.cdev);
fail_cdev :
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
fail_devid :
    return ret;
}
/* Exit Point */
static void __exit dtsled_exit(void)
{   
    unsigned int val = 0;
    val = readl(GPIO1_DR);
    val |= (1 << 3);                           /* Open LED by Default */
    writel(val, GPIO1_DR);

    /* Cancel Memory Mapping */
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_GDIR);
    iounmap(GPIO1_DR);

    /* Delete Char_Device */
    cdev_del(&dtsled.cdev);

    /* Unregister Char_Device */
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

    /* Drstroy Device */
    device_destroy(dtsled.class, dtsled.devid);

    /* Destroy Class */
    class_destroy(dtsled.class);

}

/* Register Module Entry Point */
module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");



