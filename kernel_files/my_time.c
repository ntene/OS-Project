#include <linux/ktime.h>
#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage int sys_my_time(unsigned long *time_s, unsigned long *time_ns){
	struct timespec ts;
	getnstimeofday(&ts);
	*time_s = ts.tv_sec;
	*time_ns = ts.tv_nsec;
	printk("time invokeddd\n");
	return 0;
}
