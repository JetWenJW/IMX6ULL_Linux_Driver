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

/*  
 * In this Project We are gonna Practice INPUT Device
 * The INPUT Struct contain file_operations
 * So We don't need file_operation Structure any more
 */
#define KEYINPUT_CNT     1
#define KEYINPUT_NAME    "keyinput"

#define KEY_NUM          1
#define KEY0VALUE        0x01
#define INVAKEY          0xFF

/* IRQ of Key Structure */
struct irq_keydesc
{
    int gpio;                               /* IO NUmber        */
    int irqnum;                             /* IRQ Number       */
    unsigned char value;                    /* Key Value        */
    char name[10];                          /* Interrupt name   */                                
    irqreturn_t (*handler)(int, void *)     /* IRQ Handler      */
};


struct keyinput_dev 
{
    struct device_node *nd;                 /* Device Node         */
    struct irq_keydesc irqkey[KEY_NUM];     /* For Interrput       */
    struct timer_list timer;                /* For Key filter      */

    struct input_dev *inputdev;             /* For INPUT Device    */
};

struct keyinput_dev keyinputdev;            /* struct Declare */



/* KEY_IRQ Handler */
static irqreturn_t key0_handler(int irq, void *dev_id)
{
    int value = 0;
    struct keyinput_dev *dev = dev_id;
    /*
     * This Part Only need to Enable Timer
     * The Key Value for IRQ Handler to handle
     */
    dev->timer.data = (unsigned long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(20)); /* 20 ms delay as filter */

    return IRQ_HANDLED;
}

/* Timer Handler  */
static void timer_func(unsigned long arg)
{
    int value = 0;
    struct keyinput_dev *dev = (struct keyinput_dev *)arg;

    value = gpio_get_value(dev->irqkey[0].gpio);
    if(value == 0)      /* Press KEY */
    {
        /* Report Key Value to Kernel */
        input_event(dev->inputdev, EV_KEY, KEY_0, 1);
        input_sync(dev->inputdev);
    }
    else if(value == 1) /* Release KEY */
    {
        /* Report Key Value to Kernel */
        input_event(dev->inputdev, EV_KEY, KEY_0, 0);
        input_sync(dev->inputdev);
    }
}

/* key IO initial  */
static int keyio_init(struct keyinput_dev *dev)
{
    int ret = 0;
    int i = 0;

    /* A. Key Initial */
    /* A_1. Find Node from Device Tree */
    dev -> nd = of_find_node_by_path("/imx6uirq");
    if(dev -> nd == NULL)
    {
        ret = -EINVAL;
        goto fail_nd;
    }
    
    /* A_2. Get IO Number of Key */
    for(i = 0; i < KEY_NUM; i++)
    {
        dev -> irqkey[1].gpio = of_get_named_gpio(dev -> nd, "key-gpios", i);
        if(dev -> irqkey[1].gpio < 0)
        {
            ret = -EINVAL;
            goto fail_gpio;
        }
    }

    /* A_3 Initial IO */
    for(i = 0;i < KEY_NUM;i++)/* Uss for loop Because we might have muiltiple Device */
    {
        memset(dev -> irqkey[1].name, 0, sizeof(dev->irqkey[1].name));
        sprintf(dev->irqkey[i].name, "KEY%d", i);

        ret = gpio_request(dev -> irqkey[1].gpio, dev->irqkey[i].name);
        if(ret)
        {
            ret = -EINVAL;
            printk("IO %d cannot request~\r\n", dev -> irqkey[1].gpio);
            goto fail_request;
        }
        gpio_direction_input(dev->irqkey[i].gpio); /* Set Pin as Input */
        ret = gpio_direction_input(dev->irqkey[i].gpio);
        if(ret)
        {
            ret = -EINVAL;
            goto fail_input;
        }

        /* 1. Get IRQ Number */
        dev->irqkey[1].irqnum = gpio_to_irq(dev->irqkey[i].gpio); /* Get IRQ Number */
#if 0
        dev->irqkey[i].irqnum = irq_of_parse_and_map(dev->nd, i);
#endif
    }

    dev->irqkey[0].handler  = key0_handler;
    dev->irqkey[0].value    = KEY_0;

    /* B. Key Interrupt Innitial  */
    for(i = 0;i < KEY_NUM;i++)
    {
        ret = request_irq(dev->irqkey[i].irqnum, dev->irqkey[i].handler, 
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                        dev->irqkey[i].name, &keyinputdev);
        if(ret)
        {
            printk("irq %d request failed\r\n", dev->irqkey[i].irqnum);
            goto fail_irq;
        }
    }

    /* C. Initial Timer in order to Key filter */
    init_timer(&keyinputdev.timer);
    keyinputdev.timer.function = timer_func;    

    return 0;

fail_irq : 
fail_input :
    for(i = 0;i < KEY_NUM;i++)
    {
        gpio_free(dev->irqkey[i].gpio);    
    }
fail_request :
fail_gpio :
fail_nd :
    return ret;
}

/* Entry Point Function */
static int __init keyinput_init(void)
{
    int ret = 0;        /* For Error Happen */


 
    /* 1. IO Initial */
    ret = keyio_init(&keyinputdev);
    if(ret < 0)
    {
        goto fail_keyinit;
    }
    
    /* 2. INPUT Device Register */
    keyinputdev.inputdev = input_allocate_device();
    if(keyinputdev.inputdev == NULL)
    {
        ret = -EINVAL;
        goto fail_keyinit;
    }

    /* 4. Initial Device Name */
    keyinputdev.inputdev->name = KEYINPUT_NAME;
    __set_bit(EV_KEY, keyinputdev.inputdev->evbit);         /* Set Event (Press Key)       */
    __set_bit(EV_REP, keyinputdev.inputdev->evbit);         /* Set Event (Repeat PressKey) */
    __set_bit(KEY_0 , keyinputdev.inputdev->keybit);        /* Set Event (Key Value)       */

    /* 5. Register INPUT Device */
    ret = input_register_device(keyinputdev.inputdev);
    if(ret)
    {
        goto fail_input_register;
    }

    return 0;

fail_input_register : 
    input_free_device(keyinputdev.inputdev);
fail_keyinit :

    return ret;
}

/* Exit Point Function */
static void __exit keyinput_exit(void)
{
    int i = 0;
    /* 1. Relese Interrupt */
    for(i = 0;i < KEY_NUM;i++)
    {
        free_irq(keyinputdev.irqkey[i].irqnum, &keyinputdev);
    }

    /* 2. FREE IO */
    for(i = 0;i < KEY_NUM;i++)
    {
        gpio_free(keyinputdev.irqkey[i].gpio);    
    }

    /* 3. Delete Timer */
    del_timer_sync(&keyinputdev.timer);

    /* 4. unregistry INPUT Device */
    input_unregister_device(keyinputdev.inputdev);
    input_free_device(keyinputdev.inputdev);


}


/* Module Registry */
module_init(keyinput_init);
module_exit(keyinput_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JetWen");
