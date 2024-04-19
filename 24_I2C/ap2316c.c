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
#include <linux/i2c.h>
#include "ap3216creg.h"

/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add I2C AP3216C sub-device node to Device Tree
 * 2. Build I2C Driver Framework
 * 3. Accompilshed the Framework
 */


/* Step3. Traditional Matched Table (id_table) */
static struct i2c_device_id ap3216c_id[] =
{
    {"JetWen,ap3216c", 0},
    {}
};
/* Step4. Device Tree Matched Table */
static struct of_device_id ap3216c_of_match[] = 
{
    {.compatible = "JetWen,ap3216c"},
    {}
};

/* Step5. Probe Function(this Function will be called when matched) */
static int ap3216c_probe(struct i2c_client *client, cinst struct i2c_device_id *id)
{
    return 0;
}

/* Step5. Remove Function(this Function will be called when Deleted) */
static int ap3216c_remove(struct i2c_client *client)
{
    return 0;
}


/* Step2. I2C Driver Structure */
static struct i2c_driver ap3216c_driver =
{
    .probe  = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver =
    {
        .name           = "ap3216c",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(&ap3216c_of_match);
    },

    .id_table = ap3216c_id;
};

/* Step1. Entry Point Function */
static int __init ap3216c_init(void)
{
    int ret = 0;        
    ret = i2c_add_driver(ap3216c_driver);
 
    return ret;
}

/* Step1. Exit Point Function */
static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}


/* Module Registry */
module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");