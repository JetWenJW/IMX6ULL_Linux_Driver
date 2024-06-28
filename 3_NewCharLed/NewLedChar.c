#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define NEWCHRLED_NAME      "newchrled"
#define NEWCHRLED_COUNT     1         

/* Define Register Physical Address */
#define CCM_CCGR1_BASE               (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE       (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE       (0x020E02F4)
#define GPIO1_DR_BASE                (0x0209C000)
#define GPIO1_GDIR_BASE              (0x0209C004)

/* Virtual Address Pointer */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_GDIR;
static void __iomem *GPIO1_DR;

#define LEDOFF      0       /* OPEN  */
#define LEDON       1       /* CLOSE */

/* LED Structure of Device */
struct newchrled_dev
{
    struct cdev cdev;       /* Chr Device          */
    dev_t devid;            /* Device ID           */
    struct class *class;    /* class               */
    struct device *device;   /* Device              */
    int major;              /* Major Device Number */
    int minor;              /* Sub-Device Number   */
};

/* Declare Device LED */
struct newchrled_dev newchrled;

/* LED Switch OFF/ON */
static void led_switch(u8 state)
{
    u32 val = 0;

    if(state == LEDON)
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }
    else if(state == LEDOFF)
    {
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);        
    }
}

static int newchrled_open(struct inode *inode, struct file *filp)
{
    /* Private Data */
    filp -> private_data = &newchrled;
    return 0;
}

static int newchrled_release(struct inode *inode, struct file *filp)
{
   // struct newchrled_dev *dev = (struct newchrled_dev *)filp -> private_data;
    return 0;
}

static ssize_t newchrled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int retvalue;
    unsigned char databuf[1];

	retvalue = copy_from_user(databuf, buf, count);
    if(retvalue < 0)
    {
        printk("Kernel Write Failed !\r\n");
        return -EFAULT;
    }

    /* Condition of LED ON/OFF */
    led_switch(databuf[0]);

    return 0;
}

static const struct file_operations newchrled_fops = 
{
    .owner      = THIS_MODULE,
    .write      = newchrled_write,
    .open       = newchrled_open,
    .release    = newchrled_release
};

/* Entry Point */
static int __init newchrled_init(void)
{
    int ret = 0;
    
    /* 1.Initial LED */
    unsigned int val = 0;
	printk("New ChrLed Initial\r\n");
    /* 1.1 LED Initial (Memory Mapping )*/
    IMX6U_CCM_CCGR1     = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03   = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03   = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_GDIR          = ioremap(GPIO1_GDIR_BASE, 4);
    GPIO1_DR            = ioremap(GPIO1_DR_BASE, 4);

    /* 1.2 LED Initial (Set bits) */
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

    newchrled.major = 0;                        /* Set as 0,registry  Device ID from system */

    /* 2.Registry Char Device */
    if(newchrled.major)
    {
        /* Assigned Major Device ID */
        newchrled.devid = MKDEV(newchrled.major, 0);
        ret = register_chrdev_region(newchrled.devid, NEWCHRLED_COUNT, NEWCHRLED_NAME);
    }
    else
    {
        /* Unassigned Major Device ID */
        ret = alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_COUNT, NEWCHRLED_NAME);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }

    if(ret < 0)
    {
        printk("Char Dev Region Error\r\n");
        goto fail_devid;
    }
    printk("newchrled major= %d, minor= %d\r\n", newchrled.major, newchrled.minor);

    /* 3.Registry Char Device */
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);

    ret = cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_COUNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    /* 
     * 4.Auto Make Device node (Plug And Play) 
     * Note: Class must be created earlier than Device
     */
    newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if(IS_ERR(newchrled.class))
    {
        ret = PTR_ERR(newchrled.device);
        goto fail_class;
    }

    /* 4.1Auto assigned Device */
    newchrled.device = device_create(newchrled.class, NULL, 
                        newchrled.devid, NULL, NEWCHRLED_NAME);
    if(IS_ERR(newchrled.device))
    {
        ret = PTR_ERR(newchrled.device);
        goto fail_device;
    }

    return 0;
/* The Order cannot change */
fail_device :
    class_destroy(newchrled.class);

fail_class :
    cdev_del(&newchrled.cdev);

fail_cdev :
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_COUNT);

fail_devid :
    return ret;
}

/* Exit Point */
static void __exit newchrled_exit(void)
{
    //printk("New ChrLed Fail~\r\n");

    unsigned int val = 0;
    val = readl(GPIO1_DR);
    val |= (1 << 3);                           /* Open LED by Default */
    writel(val, GPIO1_DR);

    /* 1.Cancel Memory Mapping */
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_GDIR);
    iounmap(GPIO1_DR);    

    /* 2.Delete Char Device */
    cdev_del(&newchrled.cdev);

    /* 3.Unregistry Char Device */
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_COUNT);

    /* 
     * 4.Destroy Device & Class 
     * NOTE: Device must be destroied earlier than Clsss
     */
    device_destroy(newchrled.class, newchrled.devid);
    class_destroy(newchrled.class);

}

/* Rregister & Unregister */
module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");
