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
#include <linux/hdreg.h>



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

/* Data transfer proccess */
static void ramdisk_transfer(struct request *req)
{
    /* Data Transfer 3 Factor: 
     * Source(Memory), Destnation(Block Device), Length 
     */

    /* blk_rq_pos Get The Sector Address of Block Device 
     * However we need to get Byte  of Address
     * So we need to << 9(512 byte)
     */
    unsigned long start = blk_rq_pos(req) << 9;
    
    /* Data Length of Block Devie Data */
    unsigned long len = blk_rqcur_bytes(req);

    /* Get the "bio" Data
     * Read : bio store Data that we wanna read from Disk
     * Write : bio store Data that we wanna Write to Disk
     */
    void * buffer = bio_data(req->bio);

    if(rq_data_dir(req) == READ)        /* Read */
    {
        memcpy(buffer, ramdisk.buffer + start, len);
    }
    else                                /* Write */
    {
        memcpy(ramdisk.buffer + start, buffer, len);
    }
}

/* Make Request Functions */
static void ramdisk_make_request(struct request_queue *queue, struct bio *bio)
{
    int offset;
    struct bio_vec bvec;
    struct bvec_iter iter;
    unsigned long len;

    offset = bio->bi_iter.bi_sector << 9;    /* Sector of Disk offset (bytes) */

    /* all sections of bio */
    bio_for_each_segment(bvec, bio, iter)
    {
        char *ptr = page_address(bvec.bv_page) + bvec.bv_offset;
        len = bvec.bv_len;
        if(bio_data_dir(bio) == READ)        /* Read */
        {
            memcpy(ptr, ramdisk.buffer + offset, len);
        }
        else                                /* Write */
        {
            memcpy(ramdisk.buffer + offset, ptr, len);
        }
        offset += len;
    }

    set_bit(BIO_UPTODATE, &bio->bi_flags);
    bio_endio(bio, 0);
}

#if 0
/* Request Function */
static void ramdisk_request_fn(struct request_queue *q)
{
    struct request *req;
    int err = 0;

    req = blk_fetch_request(q);
    while(req)
    {
        ramdisk_transfer(req);

        /* Request Handler (Read/Write Operations) */
        if(! __blk_end_request_cur(req, err))
        {
            req = blk_fetch_request(q);
        }
        
    }
}
#endif

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

/* Getgeo Function of Block Device Operations
 * This Function is used to get the informations of Disk
 * Such as heads, cylinders, sectors
 * The Archtechture
 */
struct int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    /* Get The Disk Message */
    printk("ramdisk getgeo\r\n");

    geo->heads      = 2;
    geo->cylinders  = 32;
    geo->sectors    = RAMDISK_SIZE/(2 * 32 * 512);

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
    ramdisk.queue = blk_alloc_queue(GFP_KERNEL);
    if(!ramdisk.queue)
    {
        ret = -EINVAL;
        goto blk_queue_fail;
    }

    /* Binding Make request Function */
    blk_queue_make_request(ramdisk.queue, ramdisk_make_request);

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