
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/topology.h>


MODULE_AUTHOR("Baptiste Pires, LIP6");
MODULE_LICENSE("GPL");


static int __init test_lock_init(void)
{
    
    
    sched_get_rd();
}
module_init(test_lock_init);

static void __exit test_lock_exit(void)
{
   
}
module_exit(test_lock_exit);