#include <linux/module.h>
#include <linux/spinlock.h>
/*
    Small module to test the overhead of taking a lock without any contention.
    The results are printed in 
*/
#pragma GCC optimize ("O1")

MODULE_LICENSE("GPL");
static int __init test_lock_init(void)
{
    DEFINE_RAW_SPINLOCK(lock);
    ktime_t start, without_lock, with_lock;
    unsigned int i, tmp, rd;
    volatile unsigned int nr_loops = 100000000;

    /* Test without lock */
    tmp = 0;
    start = ktime_get_ns();

    for(i = 0; i < nr_loops; ++i) {
        rd = get_random_u32() % 1000;
        tmp += rd;
    }

    without_lock = ktime_get_ns() - start;
    
    tmp = 0;
    start = ktime_get_ns();

    for(i = 0; i < nr_loops; ++i) {
        raw_spin_lock(&lock);
        rd = get_random_u32() % 1000;
        tmp += rd;
        raw_spin_unlock(&lock);
    }

    with_lock = ktime_get_ns() - start;

    pr_info("nr_loops : %d\n", nr_loops);
    pr_info("Time without lock: %lld ms\n", ktime_to_ms(without_lock));
    pr_info("Time with lock: %lld ms\n", ktime_to_ms(with_lock));

    return 0;
}
module_init(test_lock_init);

static void __exit test_lock_exit(void)
{
   
}
module_exit(test_lock_exit);