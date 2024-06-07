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
#include <linux/of_irq.h>
#include <linux/of_address.h>

/* Entry Point of Module */
static int __init dtsof_init(void)
{
    int ret = 0;
    struct device_node  *bl_nd = NULL;      /* Node of Backlight            */
    struct property     *comppro = NULL;    /* For Property of Node         */
    const char *str;                        /* For Read Property of String  */
    u32 def_value = 0;                      /* For Read Property of number  */
    u32 elemsize = 0;                       /* For Read element of property */
    u32 *brival;                            /* For memory allocate          */
    u8 i = 0;                               /* For for loop                 */

    /* 1.Find Backlight node,PATH=/backlight */
    bl_nd = of_find_node_by_path("/backlight");
    if(bl_nd == NULL)                           /* Fail to find Backlight Node */
    {
        ret = -EINVAL;
        goto fail_findnd;
    }
    
    /* 1.1 Get Property of Node */
    comppro = of_find_property(bl_nd, "compatible", NULL);
    if(comppro == NULL)                           /* Fail to find Backlight Node */
    {
        ret = -EINVAL;
        goto fail_findpro;
    }
    else
    {
        printk("Compatible = %s\r\n", (char *)comppro -> value);
    }

    /* 1.2 Read String Property */
    ret = of_property_read_string(bl_nd, "status", &str);
    if(ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    /* 1.3 Read Number Property */
    ret = of_property_read_u32(bl_nd, "default-brightness-level", )
    if(ret < 0)
    {
        goto fail_read32;
    }
    else
    {
        printk("default-brightness-level= %d\r\n", def_value);
    }

    /* 1.4 Read Structure of Property */
    elemsize = of_property_count_elems_of_size(bl_nd, "brightness-levels", sizeof(u32));
    if(elemsize < 0)
    {
        ret = -EINVAL;
        goto fail_readele;
    }
    else
    {
        printk("brightness-levels=%d\r\n", elemsize);
    }

    /*
     * Dynamic allocate Memory space
     * Use kmalloc() function in Linux Kernel
     * Just Like malloc() function in C Language
     */
    brival = kmalloc(elemsize * sizeof(u32), GFP_KERNEL);
    if(!brival)
    {
        ret = -EINVAL;
        goto fail_mem;
    }

    /* 1.5 Get Property array */
    ret = of_property_read_u32_array(bl_nd, "brightness-levels", brival, elemsize);
    if(ret < 0)
    {
        goto fail_read32array;
    }
    else
    {
        for(i = 0; i < elemsize; i++)
        {
            printk("brightness-levels[%d] = %d\r\n", i, *(brival + i));
        }
    }
    kfree(brival);      /* Release Memory Space */

    return 0;

fail_read32array :
    kfree(brival);      /* Release Memory Space */
fail_mem :
fail_readele :
fail_read32 : 
fail_rs :
fail_findpro :
fail_findnd :
    return ret;
}

/* Exit Point of Module */
static void __exit dtsof_exit(void)
{

}

/* Registry Module Entry Point & Exit Point */
module_init(dtsof_init);
module_exit(dtsof_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");

