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

/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add I2C FT5426 sub-device node to Device Tree
 * 2. Build I2C Driver Framework
 * 3. Accompilshed the Framework
 */

#define MAX_SUPPORT_POINTS              5       /* 5 Point Touch */
#define FT5426_TOUCH_EVENT_DOWN			0x00	/* Press 	 	 */
#define FT5426_TOUCH_EVENT_UP			0x01	/* Release	 	 */
#define FT5426_TOUCH_EVENT_ON			0x02	/* Touch 	  	 */
#define FT5426_TOUCH_EVENT_RESERVED		0x03	/* No Event      */

#define FT5426_TD_STATUS_REG            0x02    /* The Address of Status        */
#define FT5426_DEVICE_MODE_REG		    0X00 	/* Mode Register 			    */
#define FT5426_IDG_MODE_REG			    0XA4	/* Interrupt Mode				*/
#define FT5426_READLEN                  29      /* The Number Of Reg Will read  */

/* Step6. Create Structure of AP3216C for registering Funciton */
struct ft5426_dev
{
    struct device_node *nd;
    int irq_pin, reset_pin;
    int irq_num;
    void *private_data;
    struct i2c_client *client;
};

struct ft5426_dev ft5426;

/* Step8. Read Muilti Bytes FT5426 Register Value */
static int ft5426_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len)
{
    int ret = 0;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->client;

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

/* Step8_1 Write Muilti Bytes FT5426 Register Value */
static int ft5426_write_regs(struct inode *inode, struct file *filp, u8 reg, u8 *buf, u8 len)
{
    u8 buffer[256];
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->client;

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


/* Step9. Write Register One */
static void ft5426_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    u8 buffer = 0;
    buffer = data;
    ft5426_write_regs(dev, reg, &buffer, 1);
}

/* Step 6 Read Register One */
static u8 ft5426_read_reg(struct ft5426_dev *dev, u8 reg)
{
    u8 data = 0;
    ft5426_read_regs(dev, reg, &data, 1);
    return data;
}




/* Step3. Traditional Matched Table (id_table) */
static struct i2c_device_id ft5426_id[] =
{
    {"edt-ft5426", 0},
    {}
};
/* Step4. Device Tree Matched Table */
static struct of_device_id ft5426_of_match[] = 
{
    {.compatible = "edt,edt-ft5426"},
    {}
};

/* Step5.3 IRQ Handler of FT5426 */
static irqreturn_t ft5426_hanlder(int irq, void *dev_id)
{
    printk("FT5426 Handler\r\n");
    return IRQ_HANDLED;
}


/* Step5.1 Reset Initial FT5426 Function */
static int ft5426_ts_reset(struct i2c_client *client, struct ft5426_dev *dev)
{
    int ret = 0;
    if(gpio_is_valid(dev->reset_pin))
    {
        ret = devm_gpio_request_one(&client->dev, dev->reset_pin, GPIOF_OUT_INIT_LOW, "edt-ft5426 reset");
        if(ret) return ret;
        msleep(5);                          /* 5ms */
        gpio_set_value(dev->reset_pin, 1);
        msleep(300);
    }
    return 0;
}

/* Step5.2 IRQ Initial FT5426 Function */
static int ft5426_ts_irq(struct i2c_client *client, struct ft5426_dev *dev)
{
    int ret = 0;
    /* 1. Request GPIO Interrupt */
    if(gpio_is_valid(dev->irq_pin))
    {
        ret = devm_gpio_request_one(&client->dev, dev->irq_pin, GPIOF_IN, "edt-ft5426 irq");
        if(ret)
        {
            dev_err(&client->dev, "Failed to GPIO %d, error %d\r\n", dev->irq_pin, ret);
            return 0;
        }
    }

    /* 2. Threaded IRQ IO Interrupt */
    ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, ft5426_hanlder, 
                                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, &ft5426);
    if(ret)
    {
        dev_err(&client->dev, "Unable to Request TouchScreen IRQ. \r\n");
        return ret ;
    }
    return 0;
}

/* Step5. Probe Function(this Function will be called when matched) */
static int ft5426_probe(struct i2c_client *client, cinst struct i2c_device_id *id)
{   
    int ret = 0;
    int val = 0
    ft5426.client = client;

    /* Get IRQ & Reset Pin */
    ft5426.irq_pin = of_get_name_gpio(client->dev.of_node, "interrupt-gpios", 0);
    ft5426.reset_pin = of_get_name_gpio(client->dev.of_node, "reset-gpios", 0);

    ft5426_ts_reset(client, &ft5426);   /* Initial FT5426 Reset Pin */
    ft5426_ts_irq(client, &ft5426);     /* Initial FT5426 IRQ Pin   */

    /* Initial FT5426 */
    ft5426_write_reg(&ft5426, FT5426_DEVICE_MODE_REG, 0);   /* Set Ft5426 as Normal Mode */
    ft5426_write_reg(&ft5426, FT5426_IDG_MODE_REG, 1);      /* Set FT5426 as IRQ Mode    */

    val = ft5426_read_reg(&ft5426, FT5426_IDG_MODE_REG);
    printk("FT5426_IDG_MODE_REG %#x\r\n", val);
    return 0;
}

/* Step5. Remove Function(this Function will be called when Deleted) */
static int ft5426_remove(struct i2c_client *client)
{   

    return 0;
}


/* Step2. I2C Driver Structure */
static struct i2c_driver ft5426_driver =
{
    .probe  = ft5426_probe,
    .remove = ft5426_remove,
    .driver =
    {
        .name           = "edt_ft5426",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(ft5426_of_match);
    },

    .id_table = ft5426_id;
};

/* Step1. Entry Point Function */
static int __init ft5426_init(void)
{
    int ret = 0;        
    ret = i2c_add_driver(&ft5426_driver);
 
    return ret;
}

/* Step1. Exit Point Function */
static void __exit ft5426_exit(void)
{
    i2c_del_driver(&ft5426_driver);
}


/* Module Registry */
module_init(ft5426_init);
module_exit(ft5426_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");