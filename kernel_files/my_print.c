#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage int sys_my_print(int pid, unsigned long start_s, unsigned long start_ns, unsigned long end_s, unsigned long end_ns){
	printk("[Project 1] %d %ld.%ld %ld.%ld\n", pid, start_s, start_ns, end_s, end_ns);
	return 0;
}
