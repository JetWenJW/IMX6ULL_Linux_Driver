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


/* Define Register Physical Address */
#define CCM_CCGR1_BASE               (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE       (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE       (0x020E02F4)
#define GPIO1_DR_BASE                (0x0209C000)
#define GPIO1_GDIR_BASE              (0x0209C004)

#define REGISTER_LENGTH                 4

/* B .For Platform Device */
void leddevice_release(struct device *dev)
{
    printk("leddevice release!\r\n");
}

/* C .For Resources Parameter (Memory Space)*/
static struct resource led_resource[] =
{
    [0] =
        {
            .start = CCM_CCGR1_BASE,
            .end   = CCM_CCGR1_BASE + REGISTER_LENGTH -1,
            .flags = IORESOURCE_MEN,
        },
    [1] =
        {
            .start = SW_MUX_GPIO1_IO03_BASE,
            .end   = SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH -1,
            .flags =IORESOURCE_MEN,
        },    
    [2] =
        {
            .start = SW_PAD_GPIO1_IO03_BASE,
            .end   = SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH -1,
            .flags = IORESOURCE_MEN,
        },    
    [3] =
        {
            .start = GPIO1_DR_BASE,
            .end   = GPIO1_DR_BASE + REGISTER_LENGTH -1,
            .flags = IORESOURCE_MEN,
        },     
    [4] =
        {
            .start = GPIO1_GDIR_BASE,
            .end   = GPIO1_GDIR_BASE + REGISTER_LENGTH -1,
            .flags = IORESOURCE_MEN,
        },
};

/* A .Registry Structure Device of latform Bus */
static struct platform_device leddevice =
{
    .name   = "imx6ull-led",                    /* Match Platform by device name (the most important) */
    .id     = -1,                               /* -1 means that ths device has no ID                 */
    .dev    = {
                .release = leddevice_release,
              },
    .num_resources =  ARRAY_SIZE(led_resource),   /* Numbers of Resources */
    .resource = led_resource,
};


/* 1. Load Device */
static int __init leddevice_init(void)
{
    /* Registry Platform Device */
    return platform_device_register(&ledddevice);
}

/* 1. Unload Device */
static void __exit leddevice_exit(void)
{
    platform_device_unregister(&ledddevice);
}

module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");


