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
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/blkdev.h>



/* Define A Disk Size */
#define RAMDISK_SIZE        (2 * 1024 * 1024)       /* Size We Define as 2MB */
#define RAMDISK_NAME        "ramdisk"               /* Name of disk          */ 
#define RAMDISK_MINOR       3                       /* 3 sector              */

/* ramdisk Device Structure */
struct ramdisk_dev
{
    int major;                                      /* majot Device Number */
    unsigned char *ramdisk;                         /* Simulate Disk Size  */
    struct  gendisk *gendisk;
    struct request_queue *queue;
    spinlock_t lock;


};

struct ramdisk_dev ramdisk;

/* Request Function */
static void ramdisk_request_fn(struct request_queue *q)
{

}

/* Open Function of Block Device Operations */
static int ramdisk_open(struct block_device *bdev, fmode_t mode)
{
    printk("ramdisk Open\r\n");
    return 0;
}

/* Release Function of Block Device Operations */
static void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
    printk("ramdisk Release\r\n");
    return 0;
}

/* Getgeo Function of Block Device Operations */
struct int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    printk("ramdisk getgeo\r\n");
    return 0;
}

/* Block Device Operations */
static const struct block_device_operations ramdisk_fops =
{
    .owner      = THIS_MODULE,
    .open       = ramdisk_open,
    .release    = ramdisk_release,
    .getgeo     = ramdisk_getgeo,
}

/* Step1. Entry Point Function */
static int __init ramdisk_init(void)
{
    int ret = 0;        
    printk("ramdisk Initial\r\n");

    /* 1. Request Memory Space */
    ramdisk.ramdiskbuf = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
    if(ramdisk.ramdiskbuf == NULL)
    {
        ret = -EINVAL;
        goto ramalloc_fail;
    }

    /* 2. Register Block Device */
    ramdisk.major = register_blkdev(0, RAMDISK_NAME);
    if(ramdisk.major < 0)
    {
        ret = -EINVAL;
        goto ramdisk_register_blkdev_fail;
    }
    printk("ramdisk major = %d\r\n", ramdisk.major);

    /* 3. gendisk Request */
    ramdisk.gendisk = alloc_disk(RAMDISK_MINOR);
    if(ramdisk.gendisk == NULL)
    {
        ret = -EINVAL;
        goto gendisk_alloc_fail;
    }

    /* 4. Initial Spinlock */
    spin_lock_init(&ramdisk.lock);

    /* 5. Request & Initial Queue */
    ramdisk.queue = blk_init_queue(ramdisk_request_fn, &ramdisk.lock);
    if(!ramdisk.queue)
    {
        ret = -EINVAL;
        goto blk_queue_fail;
    }

    /* 6. Initial gendisk */
    ramdisk.gendisk->major        = ramdisk.major; /* major Device Number */
    ramdisk.gendisk->first_minor  = 0;
    ramdisk.gendisk->fops         = &ramdisk_fops;
    ramdisk.gendisk->private_data = &ramdisk;
    ramdisk.gendisk->queue        = ramdisk.queue;
    sprintf(ramdisk.gendisk->disk_name, RAMDISK_NAME);
    set_capacity(ramdisk.gendisk, RAMDISK_SIZE/512);        /* Set capacity as Sector */
    add_disk(ramdisk.gendisk);

    return 0;

blk_queue_fail : 
    put_disk(ramdisk.gendisk);
gendisk_alloc_fail :
    unregister_blkdev(ramdisk.major, RAMDISK_NAME);
ramdisk_register_blkdev_fail : 
    kfree(ramdisk.ramdiskbuf);
ramalloc_fail :

    return ret;
}

/* Step1. Exit Point Function */
static void __exit ramdisk_exit(void)
{
    printk("ramdisl exit\r\n");
    del_gendisk(ramdisk.gendisk);
    put_disk(ramdisk.gendisk);
    blk_cleanup_queue(ramdisk.queue);

    unregister_blkdev(ramdisk.major, RAMDISK_NAME);
    kfree(ramdisk.ramdiskbuf);

}


/* Module Registry */
module_init(ramdisk_init);
module_exit(ramdisk_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");