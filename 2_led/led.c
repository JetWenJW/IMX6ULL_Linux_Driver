#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#define LED_MAJOR       200
#define LED_NAME        "LED"

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



static int led_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}


/* Char Device Operations */
static const struct file_operations led_fops =
{
    .owner  = THIS_MODULE,
    .write  = led_write,
    .open   = led_open,
    .close  = led_release 
};

/* Entry */
static int  __init led_init(void)
{
    int ret = 0;
    int val = 0
    /* LED Initial (Memory Mapping )*/
    IMX6U_CCM_CCGR1     = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03   = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03   = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_GDIR          = ioremap(GPIO1_GDIR_BASE, 4);
    GPIO1_DR            = ioremap(GPIO1_DR_BASE, 4);

    /* LED Initial (Set bits) */
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


    /* Register Char Device */
    ret = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    
    if(ret < 0)
    {
        printk("Register chardev Fail!\r\n");
        return -EIO;
    }
    
    printk("Led Init_OK\r\n");
    return 0;
}

/* Exit */
static void __exit led_exit(void)
{
    int val = 0;
    val = readl(GPIO1_DR);
    val |= (1 << 3);                           /* Open LED by Default */
    writel(val, GPIO1_DR);



    /* Cancel Memory Mapping */
    iounmap(CCM_CCGR1_BASE);
    iounmap(SW_MUX_GPIO1_IO03_BASE);
    iounmap(SW_PAD_GPIO1_IO03_BASE);
    iounmap(GPIO1_GDIR_BASE);
    iounmap(GPIO1_DR_BASE);

    /* Unregister Char Device */
    unregister_chrdev(LED_MAJOR, LED_NAME);

    printk("Led Exit_OK\r\n");
}








/* Register Load & Unload */
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");


