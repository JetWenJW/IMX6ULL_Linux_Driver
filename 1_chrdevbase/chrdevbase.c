#include <linux/module.h>


static int __init chardevbase_init(void)
{
    return 0;
}
static void __exit chardevbase_exit(void)
{

}




/* 
 * Module Entry & Module Exit
 */
module_init(chardevbase_init);  /* Module Entry */
module_exit(chardevbase_exit);  /* Module Exit  */