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

/*
 * This Part of Program is Driver Module
 * Beacuse for Platform Device is a module & Drive is also a module 
 * Purpose : We are going to code Platform driver by Device Tree 
 * Trigger : Beep
 */


/* 4. Probe Function */
static int miscbeep_probe(struct platform_device *dev)
{
    return 0;
}
/* 4. Remove Function */
static int miscbeep_remove(struct platform_device *dev)
{
    return 0;
}



/* 3. Platform Match Table */
static const struct of_device_id beep_of_match[] =
{
    {compatible = "JetWen,beep"},
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