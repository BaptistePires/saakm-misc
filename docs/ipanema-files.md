# branch : 5.4.1
# ./include/generated/autoconf.h
`+ #define CONFIG_CGROUP_IPANEMA 1`
Pour rajouter les options de compilation

+ ./include/linux/cgroup_subsys.h
```
/* And yet... */
#if IS_ENABLED(CONFIG_CGROUP_IPANEMA)
SUBSYS(ipanema)
#endif

```

./include/linux/interrupt.h
./include/linux/ipanema.h
./include/linux/sched.h
./include/uapi/linux/sched.h
./include/uapi/linux/sched/types.h
./kernel/sched/core.c
./kernel/sched/fair.c
./kernel/sched/ipanema.c
./kernel/sched/ipanema/cfs.c
./kernel/sched/ipanema/cfs_wwc.c
./kernel/sched/ipanema/cfs_wwc_local_new_unblock.c
./kernel/sched/ipanema.h
./kernel/sched/sched.h
./tools/include/uapi/linux/sched.h
./tools/ipanema/ipasetpolicy/ipasetpolicy.c
./tools/ipanema/ipastart/ipastart.c
