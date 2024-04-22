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
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include "icm20608reg.h"

/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add SPI ICM20608 sub-device node to Device Tree
 * 2. Build SPI Driver Framework
 * 3. Accompilshed the Framework
 */

#define ICM20608_CNT        1
#define ICM20608_NAME       "icm20608"




/* Step6. SPI Structure */
struct icm20608_dev
{
    int major;
    int minor;
    dev_t devid;
    struct cdev *cdev;
    struct class *class;
    struct device *device;
    void *private_data;
    int cs_gpio;                /* Get Chip Select signal */
    struct device_node *nd;

    /* ICM20608 Parameter */
	signed int gyro_x_act;		
	signed int gyro_y_act;		
	signed int gyro_z_act;		
	signed int accel_x_act;		
	signed int accel_y_act;		
	signed int accel_z_act;		
	signed int temp_act;
};

struct icm20608_dev icm20608;       /* Decalre Structure */

#if 0
/* ICM20608 Read Register Function */
static int icm20608_read_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len)
{
    int ret = 0;
    unsigned char txdata[len];
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    gpio_set_value(dev-> cs_gpio, 0);       /* Chip Select Pull Low */
    /* Create SPI Transfer */
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

    /* 1. Send Address */
    txdata[0] = reg | 0x80;
    t->tx_buf = txdata;
    t->len    = 1;

    /* 2. SPI message */
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);
    
    /* 3. Read Data */
    txdata[0] = 0xFF;
    t->rx_buf = buf;
    t->len = len;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);

    kfree(t);

    gpio_set_value(dev-> cs_gpio, 0);       /* Chip Select Pull High */
}

/* ICM20608 Write Register Function */
static int icm20608_write_regs(struct icm20608_dev *dev, u8 reg, u8 *buf, int len)
{
    int ret = 0;
    unsigned char txdata[len];
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    gpio_set_value(dev-> cs_gpio, 0);       /* Chip Select Pull Low */
    /* Create SPI Transfer */
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

    /* 1. Send Address */
    txdata[0] = reg & ~(0x80);
    t->tx_buf = txdata;
    t->len    = 1;

    /* 2. SPI message */
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);
    
    /* 3. Read Data */
    t->tx_buf = buf;
    t->len = len;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);

    kfree(t);

    gpio_set_value(dev-> cs_gpio, 0);       /* Chip Select Pull High */

    return ret;
}
#endif


/* SPI Read Function(This Function Provide by Kernel) */
static int icm20608_read_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len)
{
    u8 data = 0;
    struct spi_device *spi = (struct spi_device *)dev->peivate_data;

    gpio_set_value(dev->cs_gpio, 0);    /* Pull Low Chip Select */

    data = reg | 0x80;
    spi_write(spi, &data, 1);
    spi_read(spi, buf, len);

    gpio_set_value(dev->cs_gpio, 1);    /* Pull High Chip Select */
}

/* SPI Write Function(This Function Provide by Kernel) */
static int icm20608_write_regs(icm20608_dev *dev, u8 reg, u8 *buf, int len)
{
    u8 data = 0;
    struct spi_device *spi = (struct spi_device *)dev->peivate_data;

    gpio_set_value(dev->cs_gpio, 0);    /* Pull Low Chip Select  */

    data = reg & ~(0x80);
    spi_write(spi, &data, 1);           /* Write Dest. Address   */
    spi_write(spi, buf, len);           /* Write Data & Length   */

    gpio_set_value(dev->cs_gpio, 1);    /* Pull High Chip Select */

}

/* ICM20608 Read Single Register */
static unsigned char icm20608_read_onereg(struct icm20608_dev *dev, u8 reg)
{
    u8 data = 0;
    icm20608_read_regs(dev, reg, *data, 1);
    return data;
}
/* ICM20608 Write Single Register */
static void icm20608_write_onereg(struct icm20608_dev *dev, u8 reg, u8 value)
{
    u8 buffer = value;
    icm20608_write_regs(dev, reg, &buffer, 1);
}

/* ICM20608 Read Data */
void icm20608_readdata(struct icm20608_dev *dev)
{
    unsigned char data[14];
    icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);

    dev->accel_x_act = (signed short)((data[0] << 8) | data[1]);
    dev->accel_y_act = (signed short)((data[2] << 8) | data[3]);
    dev->accel_z_act = (signed short)((data[4] << 8) | data[5]);
    dev->temp_adc    = (signed short)((data[6] << 8) | data[7]);
    dev->gyro_x_act  = (signed short)((data[8] << 8) | data[9]);
    dev->gyro_y_act  = (signed short)((data[10] << 8) | data[11]);
    dev->gyro_z_act  = (signed short)((data[12] << 8) | data[13]);
}

/* ICM20608 Initial Function */
void icm20608_reginit(struct icm20608_dev *dev)
{
    /* ICM20608 Initial */
    u8 value = 0;
    icm20608_write_onereg(dev, ICM20_PWR_MGMT_1, 0x80);     /* Reset get into sleep Mode			*/
	mdelay(50);
	icm20608_write_onereg(dev, ICM20_PWR_MGMT_1, 0x01); 	/* Disable sleep Mode(Auto Select CLK)	*/
	mdelay(50);


    value = icm20608_read_onereg(dev, ICM20_WHO_AM_I);
    printk("ICM20608 ID = %#X\r\n", value);

    icm20608_write_onereg(&icm20608dev, ICM20_SMPLRT_DIV,    0x00); 	
	icm20608_write_onereg(&icm20608dev, ICM20_GYRO_CONFIG,   0x18); 	/* gyro ±2000dps  				           */
	icm20608_write_onereg(&icm20608dev, ICM20_ACCEL_CONFIG,  0x18); 	/* accelerater ±16G   					   */
	icm20608_write_onereg(&icm20608dev, ICM20_CONFIG,        0x04); 	/* gyro low-pass filter BW = 20Hz 	       */
	icm20608_write_onereg(&icm20608dev, ICM20_ACCEL_CONFIG2, 0x04); 	/* accelerater low-pass filter BW = 21.2Hz */
	icm20608_write_onereg(&icm20608dev, ICM20_PWR_MGMT_2,    0x00); 	/* Open all axis of accelerater & gyro     */
	icm20608_write_onereg(&icm20608dev, ICM20_LP_MODE_CFG,   0x00); 	/* Close Low Power 						   */
	icm20608_write_onereg(&icm20608dev, ICM20_FIFO_EN,       0x00);		/* Close FIFO						       */

}

static struct icm20608_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &icm20608dev;
    return 0;
}

ssize_t icm20608_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    signed int data[7];
    long err = 0;
    struct icm20608_dev *dev = (struct icm20608_dev *)filp->private_data;

    icm20608_readdata(dev);
    data[0] = dev->gyro_x_adc;
    data[1] = dev->gyro_y_adc;
    data[2] = dev->gyro_z_adc;
    data[3] = dev->accel_x_adc;
    data[4] = dev->accel_y_adc;
    data[5] = dev->accel_z_adc;
    data[6] = dev->temp_adc;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static int icm20608_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operaions icm20608_fops = 
{
    .owner   = THIS_MODULE,
    .open    = icm20608_open,
    .read    = icm20608_read,
    .release = icm20608_release,
};






/* Step5. Probe Function */
static int icm20608_probe(struct spi_device *spi)
{
    int ret = 0;
    printk("icm20608_Probe\r\n");

    /* 1. Register Char Device */
    icm20608dev.major = 0;
    if(icm20608dev.major)
    {
        icm20608.devid = MKDEV(icm20608.major, 0);
        ret = register_chrdev_region(icm20608.devid, ICM20608_CNT, ICM20608_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&icm20608.devid, 0, ICM20608_CNT, ICM20608_NAME);
        icm20608.major = MAJOR(icm20608.devid);
        icm20608.monir = MINOR(icm20608.devid);
    }
    if(ret < 0)
    {
        printk("icm20608_chrdev_region_Err\r\n");
        goto fail_devid;
    }
    printk("icm20608 major = %d, monir = %d\r\n", icm20608.major, icm20608.minor);

    icm20608dev.cdev.owner = THIS_MODULE;
    cdev_init(&icm20608.cdev, &icm20608_fops);
    ret = cdev_add(&icm20608.cdev, icm20608dev.devid, ICM20608_CNT);
    if(ret < 0)
    {
        goto fail_cdev;
    }

    icm20608dev.class = class_create(THIS_MODULE, ICM20608_NAME);
    if(IS_ERR(icm20608dev.class))
    {
        ret = PTR_ERR(icm20608dev.class);
        goto fail_class;
    }

    icm20608dev.device = device_create(icm20608dev.class, NULL, icm20608dev.device, NULL, ICM20608_NAME);
    if(IS_ERR(icm20608dev.device))
    {
        ret = PTR_ERR(icm20608dev.device);
        goto fail_device;
    }

    /* Get Chip Select Signal*/
    icm20608dev.nd = of_get_parent(spi->dev.of_node);
    icm20608dev.cd_gpio = of_get_named_gpio(icm20608dev.nd, "cs-gpio", 0);
    if(icm20608dev.cs_gpio < 0)
    {
        printk("Cannot get cs-gpio\r\n");
        goto fail_gpio;
    }
    ret = gpio_request(icm20608dev.cs_gpio, "cs");
    if(ret < 0)
    {
        printk("cs_gpio request failed ~\r\n");
    }
    ret = gpio_direction_output(icm20608dev.cs_gpio, 1);    /* Default Value High Voltage */



    /* Initial SPI Device Mode(Ex.CPOL,CPHA)*/
    spi->mode = SPI_MODE_0;
    spi_setup(spi);

    /* Get SPI Private Data */
    icm20608dev.private_data = spi;


    /* ICM20608 Device Initial */
    icm20608_reginit(&icm20608dev);

    return 0;


fail_gpio :
fail_device :
    class_destroy(icm20608dev.class);

fail_class :
    cdev_del(&icm20608dev.cdev);

fail_cdev : 
    unregister_chrdev_region(icm20608dev.devid, ICM20608_CNT);

fail_devid :

    return ret;

}

/* Step5. Remove Function */
static int icm20608_remove(struct spi_device *spi)
{
    int ret = 0;

    cdev_del(&icm206058.cdev);

    unregister_chrdev_region(icm20608dev.devid, ICM20608_CNT);

    device_destroy(icm20608.class, icm20608dev.devid);

    class_destroy(icm20608.class);

    gpio_free(icm20608dev.cs_gpio);

    return ret;
}



/* Step4. Traditional match */
struct spi_device_id icm20608_id[] =
{
    {"JetWen,icm20608", 0},
    {},
};

/* Step3. Matched with Device Tree */
static const struct of_device_id icm20608_of_match[] =
{
    { .compatible = "JetWen,icm20608"},
    {},
};

/* Step2. SPI Driver Structure */
struct spi_driver icm20608_driver
{
    .probe  = icm20608_probe,
    .remove = icm20608_remove,

    .driver = 
    {
        .name = "icm20608",
        .owner = THIS_MODULE,
        .of_match_table = icm20608_of_match,
    },
    .id_table = icm20608_id,
};




/* Step1. Entry Point Function */
static int __init icm20608_init(void)
{
    int ret = 0;        
    
    /* A. SPI Initial */
    ret = spi_register_driver(&icm20608_driver);

    return ret;
}

/* Step1. Exit Point Function */
static void __exit icm20608_exit(void)
{
    spi_unregister_driver(&icm20608_driver);
}


/* Step1. Module Registry */
module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");