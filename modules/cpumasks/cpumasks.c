#include "generated/asm-offsets.h"
#include "linux/slab.h"
#include <linux/module.h>
#include <linux/cpumask.h>

MODULE_AUTHOR("Baptiste Pires, LIP6");
MODULE_LICENSE("GPL");


static int __init cpumasks_mod_init(void)
{
    int ret = 0;
    int core;
    int cpu_sibling;

    pr_info(" ----------- Test for printing, setting, clearing bits from a cpumask ----------- \n");
    struct cpumask *mask = kmalloc(cpumask_size(), GFP_KERNEL);
    if (!mask) {
        pr_err("Failed to allocate memory for cpumask\n");
        return -ENOMEM;
    }

    pr_info("cpumask initial value : %*pb\n", cpumask_pr_args(mask));
    cpumask_clear(mask);
    pr_info("cpumask should be empty : %*pb\n",  cpumask_pr_args(mask));

    pr_info("Setting to 1 the 4 first bits\n");
    for (int i = 0; i < 4; i++) {
        cpumask_set_cpu(i, mask);
    }
    pr_info("cpumask should have 4 firsts bits set to 1 : %*pb\n", cpumask_pr_args(mask));

    pr_info("Difference between %%*pb: '%*pb' and %%*pbl : '%*pbl'\n", cpumask_pr_args(mask), cpumask_pr_args(mask));

    pr_info(" ----------- Test for smt cpumasks ----------- \n");
    for_each_possible_cpu(core) {
        printk(KERN_INFO "core %d\nSiblings :", core);
        const struct cpumask *smt_mask = cpu_smt_mask(core);
        for_each_cpu(cpu_sibling, smt_mask) {
            printk(KERN_INFO "\t\t - %d\n", cpu_sibling);
        }
    }

    return ret;
}

module_init(cpumasks_mod_init);

static void __exit cpumasks_mod_exit(void)
{
}
module_exit(cpumasks_mod_exit);