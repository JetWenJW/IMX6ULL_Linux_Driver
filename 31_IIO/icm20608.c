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
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/unaligned/be_byteshift.h>
#include "icm20608reg.h"


/*  
 * In this Project We are gonna Practice I2C Device
 * 1. Add SPI ICM20608 sub-device node to Device Tree
 * 2. Build SPI Driver Framework
 * 3. Accompilshed the Framework
 */

#define ICM20608_CNT            1
#define ICM20608_NAME           "icm20608"
#define ICM20608_TEMP_OFFSET    0
#define ICM20608_TEMP_SCALE     326800000

static const int gyro_scale_icm20608[]  = {7629, 15258, 30517, 61035};
static const int accel_scale_icm20608[] = {61035, 122070, 244140, 4882};


/* Define ICM20608 Scan Structure */
enum inv_icm20608_scan
{
    INV_ICM20608_SCAN_ACCL_X,
    INV_ICM20608_SCAN_ACCL_Y,
    INV_ICM20608_SCAN_ACCL_Z,
    INV_ICM20608_SCAN_TEMP,
    INV_ICM20608_SCAN_GYRO_X,
    INV_ICM20608_SCAN_GYRO_Y,
    INV_ICM20608_SCAN_GYRO_Z,
    INV_ICM20608_SCAN_TIMESTAMP,
};

#define ICM20608_CHAN(_type, channel2, _index){             \
    .type = _type,                                          \
    .modified = 1,                                          \
    .channel2 = channel2,                                   \
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),   \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),           \
        | BIT(IIO_CHAN_INFO_CALIBBIAS),                     \
    .scan_index = _index,                                   \
    .scan_type = {                                          \
        .sign = 's',                                        \
        .realbits = 16,                                     \
        .storagebits = 16,                                  \
        .shift = 0,                                         \
        .endianness = IIO_BE,                               \
    },                                                      \
}

/* ICM20608 Channel */
static const struct iio_chan_spec icm20608_channels[] =
{
    /* Temperture */
    .type = IIO_TEM,
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        | BIT(IIO_CHAN_INFO_SCALE) | BIT(IIO_CHAN_INFO_OFFSET),
    .scan_index = INV_ICM20608_SCAN_TEMP,
    .scan_type = 
    {
        .sign = 's',
        .realbits = 16,
        .storagebits = 16,
        .shift = 0,
        .endianness = IIO_BE,
    },

    /* Accel X, Y, Z Channel */
    ICM20608_CHAN(IIO_ACCEL, IIO_MOD_X, INV_ICM20608_SCAN_ACCL_X),
    ICM20608_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_ICM20608_SCAN_ACCL_Y),
    ICM20608_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_ICM20608_SCAN_ACCL_Z),

    /* Gyro X, Y, Z */
    ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_ICM20608_SCAN_GYRO_X),
    ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_ICM20608_SCAN_GYRO_Y),
    ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_ICM20608_SCAN_GYRO_Z),
};



/* Step6. SPI Structure */
struct icm20608_dev
{
    struct spi_device *spi;
    struct regmap * regmap;
    struct regmap_config regmap_config;
    struct mutex lock;
};


/* ICM20608 Read Single Register */
static unsigned char icm20608_read_onereg(struct icm20608_dev *dev, u8 reg)
{
    unsigned int data = 0;
    u8 ret = 0;
    /* Use regmap API Function to Read One Data from SPI */
    ret = regmap_read(dev->regmap, reg, &data);
    return (u8)data;
}
/* ICM20608 Write Single Register */
static void icm20608_write_onereg(struct icm20608_dev *dev, u8 reg, u8 value)
{
    u8 ret = 0;
    /* Use regmap API Function to write One Data to SPI */
    ret = regmap_write(dev->regmap, reg, value);
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

    icm20608_write_onereg(dev, ICM20_SMPLRT_DIV,    0x00); 	
	icm20608_write_onereg(dev, ICM20_GYRO_CONFIG,   0x18); 	/* gyro ±2000dps  				           */
	icm20608_write_onereg(dev, ICM20_ACCEL_CONFIG,  0x18); 	/* accelerater ±16G   					   */
	icm20608_write_onereg(dev, ICM20_CONFIG,        0x04); 	/* gyro low-pass filter BW = 20Hz 	       */
	icm20608_write_onereg(dev, ICM20_ACCEL_CONFIG2, 0x04); 	/* accelerater low-pass filter BW = 21.2Hz */
	icm20608_write_onereg(dev, ICM20_PWR_MGMT_2,    0x00); 	/* Open all axis of accelerater & gyro     */
	icm20608_write_onereg(dev, ICM20_LP_MODE_CFG,   0x00); 	/* Close Low Power 						   */
	icm20608_write_onereg(dev, ICM20_FIFO_EN,       0x00);	/* Close FIFO						       */

}

static int icm20608_sensor_set(struct icm20608_dev *dev, int reg, int axis, int val)
{
    int ind, result;
    __be16 d = cpu_to_be16(val);

    ind = (axis - IIO_MOD_X) * 2;
    result = regmap_bulk_write(dev->regmap, reg +ind, (u8 *)&d, 2);
    if(result)  return -EINVAL;
    return 0;
}

static int icm20608_sensor_show(struct icm20608_dev *dev, int reg, int axis, int *val)
{
    int ind, result;
    __be16 d;

    ind = (axis - IIO_MOD_X) * 2;
    result = regmap_bulk_read(dev->regmap, reg + ind, (u8 *)&d, 2);
    if(result)  return -EINVAL;
    *val = (short)be16_to_cpup(&d);

    return IIO_VAL_INT;
}

static int icm20608_read_channel_data(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val)
{
    int ret =0;
    struct icm20608_dev *dev = iio_priv(indio_dev);

    switch(chan->type)
    {
        case IIO_ACCEL:
            ret = icm20608_sensor_show(dev, ICM20_GYRO_XOUT_H, chan->channel2, val);
            break;

        case IIO_ANGL_VEL:
            ret = icm20608_sensor_show(dev, ICM20_ACCEL_XOUT_H, chan->channel2, val);
            break;

        case IIO_TEMP:
            ret = icm20608_sensor_show(dev, ICM20_TEMP_OUT_H, IIO_MOD_X, val);
            break;
        
        default:
            ret = -EINVAL;
    }
    return ret;
}

static int icm20608_write_gyro_scale(struct icm20608_dev *dev, int val)
{
    int result, i;
    u8 d;
    for(i = 0;i < ARRAY_SIZE(gyro_scale_icm20608);i++)
    {
        if(gyro_scale_icm20608[i] == val)
        {
            d = (i << 3);
            result = regmap_write(dev->regmap, ICM20_GYRO_CONFIG, d);
            if(result)  return result;
            return 0;
        }
    }
    return -EINVAL;
}

static int icm20608_write_accel_scale(struct icm20608_dev *dev, int val)
{
    int result, i;
    u8 d;

    for(i = 0;i < ARRAY_SIZE(accel_scale_icm20608);i++)
    {
        if(accel_scale_icm20608[i] == val)
        {
            d = (i << 3);
            result = regmap_write(dev->regmap, ICM20_ACCEL_CONFIG, d);
            if(result)  return result;
            return 0;
        }
    }
    return -EINVAL;
}

static int icm20608_read_raw(struct iio_dev *indio_dev, 
        struct iio_chan_spec const *chan, int * val, int * val2, long mask)
{
    int ret =0;
    unsigned char regdata;
    struct icm20608_dev *dev = iio_priv(indio_dev);

    /* Distinguished Data Types is offset, scale, raw? */
    switch(mask)
    {
        case IIO_CHAN_INFO_RAW:
            mutex_lock(&dev->lock);
            ret = icm20608_read_channel_data(indio_dev, chan, val);
            mutex_unlock(&dev->lock);
            return ret;

        case IIO_CHAN_INFO_SCALE:
            switch(chan->type)
            {
                case IIO_ANGL_VEL:
                    mutex_lock(&dev->lock);
                    regdata = (icm20608_read_onereg(dev, ICM20_GYRO_CONFIG) & 0x18) >> 3;
                    *val = 0;
                    *val2 = gyro_scale_icm20608[regdata];
                    mutex_unlock(&dev->lock);
                    return IIO_VAL_INT_PLUS_MICRO;

                case IIO_ACCEL:
                    mutex_lock(&dev->lock);
                    regdata = (icm20608_read_onereg(dev, ICM20_ACCEL_CONFIG) & 0x18) >> 3;
                    *val = 0;
                    *val2 = accel_scale_icm20608[regdata];
                    mutex_unlock(&dev->lock);
                    return IIO_VAL_INT_PLUS_NANO;

                case IIO_TEMP:
                    *val = ICM20608_TEMP_SCALE / 1000000;
                    *val2 = ICM20608_TEMP_SCALE % 1000000;
                    return IIO_VAL_INT_PLUS_MICRO;
                
                default:
                    return -EINVAL;
            }

            return ret;

        case IIO_CHAN_INFO_OFFSET:
            switch(chan->type)
            {
                case IIO_TEMP:
                    *val = ICM20608_TEMP_OFFSET;
                    return IIO_VAL_INT;
                
                default:
                    return -EINVAL;
            }

            return ret;

        case IIO_CHAN_INFO_CALIBBIAS:
            switch(chan->type)
            {
                case IIO_ANGL_VEL:
                    mutex_lock(&dev->lock);
                    ret = icm20608_sensor_show(dev, ICM20_XG_OFFS_USRH, chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    return ret;

                case IIO_ACCEL:
                    mutex_lock(&dev->lock);
                    ret = icm20608_sensor_show(dev, ICM20_XA_OFFSET_H, chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    return ret;
                
                default:
                    return -EINVAL;
            }

            return ret; 

        default:
            return -EINVAL;
    }
}

static int icm20608_write_raw(struct iio_dev *indio_dev, 
        struct iio_chan_spec const *chan, int val, int val2, long mask)
{
    struct icm20608_dev *dev = iio_priv(indio_dev);
    int ret = 0;

    switch(mask)
    {
        case IIO_CHAN_INFO_SCALE:
            switch(chan->type)
            {
                case IIO_ANGL_VEL:
                    mutex_lock(&dev->lock);
                    ret = icm20608_write_gyro_scale(dev, val2);
                    mutex_unlock(&dev->lock);
                    break;

                case IIO_ACCEL:
                    mutex_lock(&dev->lock);
                    ret = icm20608_write_accel_scale(dev, val2);
                    mutex_unlock(&dev->lock);
                    break;
            }
            break;

        case IIO_CHAN_INFO_CALIBBIAS:
            switch(chan->type)
            {
                case IIO_ANGL_VEL:
                    mutex_lock(&dev->lock);
                    ret = icm20608_sensor_set(dev, ICM20_XG_OFFS_USRH, chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    break;

                case IIO_ACCEL:
                    mutex_lock(&dev->lock);
                    ret = icm20608_sensor_set(dev, ICM20_XA_OFFSET_H, chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    break;

                default:
                    ret = -EINVAL;
                    break;
            }
            break;

        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}

static int icm20608_write_raw_get_fmt(struct iio_dev *indio_dev, 
        struct iio_chan_spec const *chan, long mask)
{
    switch(mask)
    {
        case IIO_CHAN_INFO_SCALE:
            switch(chan->type)
            {
                case IIO_ANGL_VEL:
                    return IIO_VAL_INT_PLUS_MICRO;

                default:
                    return IIO_VAL_INT_PLUS_NANO;
            }

        default:
            return IIO_VAL_INT_PLUS_MICRO;
    }
    return -EINVAL;
}

/* iio_info Structure */
static const struct iio_info icm20608_info = 
{
    .driver_module     = THIS_MODULE,
    .read_raw          = icm20608_read_raw,
    .write_raw         = icm20608_write_raw,
    .write_raw_get_fmt = icm20608_write_raw_get_fmt,
};

/* Step5. Probe Function */
static int icm20608_probe(struct spi_device *spi)
{
    int ret = 0;
    struct icm20608_dev *dev;
    struct iio_dev *indio_dev;

    printk("icm20608_Probe\r\n");

    /* Register IIO dev & ICM20608 dev */
    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
    if(!indio_dev)
    {
        ret = -ENOMEM;
        goto fail_iio_dev;
    }

    dev = iio_priv(indio_dev);          /* Get icm20608_dev head address */
    dev->spi = spi;
    spi_dev_drvdata(spi, indio_dev);

    /* Mutex Innitial */
    mutex_init(&dev->lock);

    /* IIO Initial */
    indio_dev->dev.parent   = &spi->dev;
    indio_dev->channels     = icm20608_channels;
    indio_dev->num_channels = ARRAY_SIZE(icm20608_channels);
    indio_dev->name         = ICM20608_NAME;
    indio_dev->modes        = INDIO_DIRECT_MODE;           /* Direct Mode Provide sysfs Function */
    indio_dev->info         = &icm20608_info;

    /* IIO_dev Register to Kernel */
    ret = iio_device_register(indio_dev);
    if(ret < 0)
    {
        dev_err(&spi->dev, "unable to register iio device\n");
        goto fail_iio_register;
    }

    /* Regmap Initial */ 
    dev->regmap_config.reg_bits = 8;
    dev->regmap_config.val_bits = 8;
    dev->regmap_config.read_flag_mask = 0x80;
    dev->regmap = regmap_init_spi(spi, &dev->regmap_config,);
    if(IS_ERR(dev->regmap))
    {
        ret = PTR_ERR(dev->regmap);
        goto fail_regmap_init;
    }

    /* Initial SPI Device Mode(Ex.CPOL,CPHA)*/
    spi->mode = SPI_MODE_0;
    spi_setup(spi);

    /* ICM20608 Device Initial */
    icm20608_reginit(dev);

    return 0;

fail_regmap_init:
    iio_device_unregister(indio_dev);
fail_iio_register: 
fail_iio_dev:

    return ret;

}

/* Step5. Remove Function */
static int icm20608_remove(struct spi_device *spi)
{
    int ret = 0;
    struct iio_dev *indio_dev = spi_get_drvdata(spi);
    struct icm20608_dev *dev;
    dev = iio_priv(indio_dev);

    /* 1. IIO Device Unregister */
    iio_device_unregister(indio_dev);

    /* 2. regmap delete */
    regmap_exit(dev->regmap);
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
struct spi_driver icm20608_driver =
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
