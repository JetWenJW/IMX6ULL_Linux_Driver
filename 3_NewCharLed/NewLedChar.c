#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

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
    return 0;
}

static int newchrled_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t newchrled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int retvalue;
    unsigned char databuf[1];
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
    printk("New ChrLed Initial\r\n");
    
    /* 1.Initial LED */
    unsigned int val = 0;

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

    writel(0x5, SW_MUX_GPIO1_IO03_BASE);        /* Configured GPIO io03 */
    writel(0x10B0, SW_PAD_GPIO1_IO03_BASE);     /* Set Electronical Property */

    val = readl(GPIO1_GDIR);                    
    val |= (1 << 3);                            /* Set bit 3 = 1,Set as GPIO Output */
    writel(val, GPIO1_GDIR);

    val = readl(GPIO1_DR);
    val &= ~(1 << 3);                           /* Open LED by Default */
    writel(val, GPIO1_DR);



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
        return -1;
    }
    printk("newchrled major= %d, minor= %d\r\n", newchrled.major, newchrled.minor);

    /* 3.Registry Char Device */
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);
    ret = cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_COUNT);
    return 0;

    /* 4.Auto Make Device node (Plug And Play) */






}

/* Exit Point */
static void __exit newchrled_exit(void)
{
    printk("New ChrLed Fail~\r\n");

    unsigned int val = 0;
    val = readl(GPIO1_DR);
    val |= (1 << 3);                           /* Open LED by Default */
    writel(val, GPIO1_DR);

    /* Cancel Memory Mapping */
    iounmap(CCM_CCGR1_BASE);
    iounmap(SW_MUX_GPIO1_IO03_BASE);
    iounmap(SW_PAD_GPIO1_IO03_BASE);
    iounmap(GPIO1_GDIR_BASE);
    iounmap(GPIO1_DR_BASE);    

    /* Delete Char Device */
    cdev_del(&newchrled.cdev);

    /* Unregistry Char Device */
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_COUNT);
}

/* Rregister & Unregister */
module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");