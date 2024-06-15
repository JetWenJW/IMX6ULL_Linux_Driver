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
#include <linux/i2c.h>
#include "ap3216creg.h"

/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add I2C AP3216C sub-device node to Device Tree
 * 2. Build I2C Driver Framework
 * 3. Accompilshed the Framework
 */

#define AP3216C_CNT     1
#define AP3216C_NAME    "ap3216c"


/* Step6. Create Structure of AP3216C for registering Funciton */
struct ap3216c_dev
{
    int major;
    dev_t devid;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *private_data;

    unsigned short ir, als, ps;
};

struct ap3216c_dev ap3216cdev;

/* Step8. Read Muilti Bytes AP3216C Register Value */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len)
{
    int ret = 0;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    /* msg[0] : Master Send Data */
    msg[0].addr  = client->addr;    /* Slave Address  */
    msg[0].flags = 0;               /* Tx Data        */
    msg[0].buf   = &reg;            /* Tx Data buffer */
    msg[0].len   = 1;               /* Tx buffer Addr */

    /* msg[1] : Master Read Data */
    msg[1].addr  = client->addr;    /* Slave Address  */
    msg[1].flags = I2C_M_RD;        /* Rx Data        */
    msg[1].buf   = val;             /* Rx Data buffer */
    msg[1].len   = len;             /* Rx buffer Addr */
    
    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret  == 2)
    {
        ret = 0;
    }
    else
    {
        printk("I2C Read Fail = %d, reg = %06x, len = %d \r\n", ret, reg, len);
        ret = -EREMOTEIO;
    }

    return ret;

}

/* Step8_1 Write Muilti Bytes AP3216C Register Value */
static int ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 buffer[8];
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    /* Build Transfer Data Format */
    buffer[0];
    memcpy(&buffer[1], buf, len);

    /* msg[0] : Send Data */
    msg[0].addr  = client->addr;    /* Slave Address  */
    msg[0].flags = 0;               /* Tx Data        */
    msg[0].buf   = &buffer;         /* Tx Data buffer */
    msg[0].len   = len + 1;         /* Tx buffer Addr */


    return i2c_transfer(client->adapter, &msg, 1);
}

/* Step9. Read Register One */
static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
    u8 data = 0;

    ap3216c_read_regs(dev, reg, &data, 1);
    return data;
}

/* Step9. Write Register One */
static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    u8 buffer = 0;
    buffer = data;
    ap3216c_write_regs(dev, reg, &buffer, 1);
}

/* AP3216C Read Data */
void ap3216c_readData(struct ap3216c_dev *dev)
{
    unsigned char buf[6];
    unsigned char i = 0;

    for(i = 0;i < 6; i++)
    {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
    }
    if(buf[0] &0x80)/* Ir & PS data invalid */
    {
        dev->ir = 0;
        dev->ps = 0;
    }
    else            /* Ir & PS data Valid */
    {
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0x03);
        dev->ps = (((unsigned short)buf[5] & 0x3F) << 4) | (buf[4] & 0x0F);
    }
    dev->als = ((unsigned short)buf[3] << 8) | buf[2] ;
}

/* Step7_1 file operation Function */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    /* Private Data */
    unsigned char value = 0;
	filp -> private_data = &ap3216cdev;

    /* 1. INitial Device AP3216C */
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x4);
    mdelay(50);                                     /* Delay 50ms */    
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x3);

    value = ap3216c_read_reg(&ap3216cdev, AP3216C_SYSTEMCONG);
    printk("AP3216C_SYSTENCONG = %#x\r\n", value);

    
    return 0;
}

/* Step7_2 file operation Function */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
    struct ap3216c_dev *dev = (struct ap3216c_dev *)filp -> private_data;
    return 0;
}

/* Step7_3 file operation Function */
ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    struct ap3216c_dev *dev = (struct ap3216c_dev *)filp->private_data;
    short data[3];
    long err = 0;
    /* A. Sent Data From AP3216C to APP */
    ap3216c_readData(dev);
    data[0] = dev->ir;
    data[1] = dev->als;
    data[3] = dev->ps;

    err = copy_to_user(buf, data, sizeof(data));

    return 0;
}

/* Step7. Add file Operations of AP3216C */
static const struct file_operations ap3216c_fops = 
{
    .owner      = THIS_MODULE,
    .open       = ap3216c_open,
    .release    = ap3216c_release,
    .read       = ap3216c_read,
};

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
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{   
    int ret = 0;
    /* 5_1.Registry Char Device */
    ap3216cdev.major = 0;       /* Assigned device ID by Kernel */

    if(ap3216cdev.major)
    {
        /* Assigned Major Device ID */
        ap3216cdev.devid = MKDEV(ap3216cdev.major, 0);
        ret = register_chrdev_region(ap3216cdev.devid, AP3216C_CNT, AP3216C_NAME);
    }
    else
    {
        /* Unassigned Major Device ID */
        ret = alloc_chrdev_region(&ap3216cdev.devid, 0, AP3216C_CNT, AP3216C_NAME);
        ap3216cdev.major = MAJOR(ap3216cdev.devid);
        ap3216cdev.minor = MINOR(ap3216cdev.devid);
    }

    if(ret < 0)
    {
        printk("AP3216C Char Dev Region Error\r\n");
        goto fail_devid;
    }
    printk("ap3216c major= %d, minor= %d\r\n", ap3216cdev.major, ap3216cdev.minor);

    /* 5_2.Registry Char Device */
    ap3216cdev.cdev.owner = THIS_MODULE;
    cdev_init(&ap3216cdev.cdev, &ap3216c_fops);

    ret = cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, AP3216C_CNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    /* 
     * 5_3.Auto Make Device node (Plug And Play) 
     * Note: Class must be created earlier than Device
     */
    ap3216cdev.class = class_create(THIS_MODULE, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.class))
    {
        ret = PTR_ERR(ap3216cdev.device);
        goto fail_class;
    }

    /* 5_4. Auto assigned Device */
    ap3216cdev.device = device_create(ap3216cdev.class, NULL, 
                        ap3216cdev.devid, NULL, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.device))
    {
        ret = PTR_ERR(ap3216cdev.device);
        goto fail_device;
    }

    ap3216cdev.private_data = client;

    return 0;
/* The Order cannot change */
fail_device :
    class_destroy(ap3216cdev.class);

fail_class :
    cdev_del(&ap3216cdev.cdev);

fail_cdev :
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);

fail_devid :
    return ret;
}

/* Step5. Remove Function(this Function will be called when Deleted) */
static int ap3216c_remove(struct i2c_client *client)
{   
    /* 5_5. Delete Char Device */
    cdev_del(&ap3216cdev.cdev);

    /* 5_6. Unregistry Char Device */
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);

    /* 
     * 5_7. Destroy Device & Class 
     * NOTE: Device must be destroied earlier than Clsss
     */
    device_destroy(ap3216cdev.class, ap3216cdev.devid);
    class_destroy(ap3216cdev.class);
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
        .of_match_table = of_match_ptr(&ap3216c_of_match),
    },

    .id_table = ap3216c_id,
};

/* Step1. Entry Point Function */
static int __init ap3216c_init(void)
{
    int ret = 0;        
    ret = i2c_add_driver(&ap3216c_driver);
 
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
