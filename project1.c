#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>

struct process{
	char Ni[32];
	int Ri;
	int Ti;
	int pid;
	int ran;
	unsigned long start_s, start_ns, end_s, end_ns;
};
typedef struct process Process;

Process *array;
int processing = 0;
int done_process = 0;
int compare(const void *data1, const void *data2){
	Process *ptr1 = (Process *)data1;
	Process *ptr2 = (Process *)data2;
	if (ptr1 -> Ri < ptr2 -> Ri)
		return -1;
	else if (ptr1 -> Ri > ptr2 -> Ri)
		return 1;
	else 
		return 0;
}

void unit_time(){
	volatile unsigned long i;
	for(i = 0; i < 1000000UL; i++);
	return;
}

void set_idle(int pid){
	struct sched_param param;
	param.sched_priority = 0;
	if (sched_setscheduler(pid, SCHED_IDLE, &param) == -1){
		fprintf(stderr, "set idle error\n");
		exit(1);
	}
	return;
}

void set_other(int pid){
	struct sched_param param;
	param.sched_priority = 0;
	if (sched_setscheduler(pid, SCHED_OTHER, &param) == -1){
		fprintf(stderr, "set other error\n");
		exit(1);
	}
	return;
}

int set_cpu(int core){
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(core, &set);
	if (sched_setaffinity(0, sizeof(set), &set) == -1){
		fprintf(stderr, "set core error");
		exit(1);
	}
	return 0;
}
int fork_process(int Ti, int now){
	pid_t pid = fork();
	if (pid < 0){
		fprintf(stderr, "Fork failed.");
		return 1;
	}
	else if (pid == 0){
		set_cpu(2);
		pid_t proc_pid = getpid();
		for(int j = 0; j < Ti; j++){
			unit_time();
		}
		fprintf(stderr, "%s %d\n", array[now].Ni, proc_pid);
		exit(0);
	}
	return pid;
}

int end_process(int now){
	int status;
	waitpid(array[now].pid, &status, 0);
	syscall(335, &array[now].end_s, &array[now].end_ns);
	syscall(336, array[now].pid, array[now].start_s, array[now].start_ns, array[now].end_s, array[now].end_ns);
	processing = 0;
	done_process++;
	return 0;
}

int run_min(int num_of_process, int time_unit, int now){
	int min = 2147483647;
	int found = 0;
	for(int i = 0; i < num_of_process; i++){
		if(array[i].Ti > 0  && array[i].pid != 0){
			if (array[i].Ti < min){
				min = array[i].Ti;
				found = 1;
				now = i;
			}
		}
	}
	if (found){
		set_other(array[now].pid);
		if (!array[now].ran){
			syscall(335, &array[now].start_s, &array[now].start_ns);
			array[now].ran = 1;
		}
		processing = 1;
	}
	return now;
}

int run_pmin(int num_of_process, int time_unit, int now){
	int found = 0;
	int min = array[now].Ti;
	int nnow;
	for(int i = 0; i < num_of_process; i++){
		if (i == now)
			continue;
		if(array[i].Ti > 0  && array[i].pid != 0){
			if (array[i].Ti < min){
				min = array[i].Ti;
				found = 1;
				nnow = i;
			}
		}
	}
	if (found){
		set_idle(array[now].pid);
		set_other(array[nnow].pid);
		if (!array[nnow].ran){
			syscall(335, &array[nnow].start_s, &array[nnow].start_ns);
			array[nnow].ran = 1;
		}
		processing = 1;
		return nnow;
	}
	else
		return now;
}

int main(int argc, char **argv){
	char policy[10];
	int num_of_process;
	scanf("%s", policy);
	scanf("%d", &num_of_process);
	array = (Process *)malloc(num_of_process*sizeof(Process)); 
	for(int i = 0; i < num_of_process; i++){
		scanf("%s" , array[i].Ni);
		scanf("%d %d", &array[i].Ri, &array[i].Ti);
		array[i].pid = 0;
		array[i].ran = 0;
	}
	int time_unit = 0;
	int now = 0;
	int next_round = 0;
	qsort(array, num_of_process, sizeof(Process), compare);
	set_cpu(0);
	set_other(getpid());
	unsigned long start_s, start_ns, end_s, end_ns;
	int *arr_queue = malloc(num_of_process * sizeof(int));
	int front = -1;
	int rear = -1;
	int size = 0;
	
	while(done_process != num_of_process){
		pid_t pid;
		for (int i = 0; i < num_of_process; i++){
			int status;
			if (array[i].Ri == time_unit){
				pid = fork_process(array[i].Ti, i);
				set_idle(pid);
				array[i].pid = pid;
				if (strcmp(policy, "RR") == 0){
					if (front == -1){
						front = rear = 0;
						arr_queue[rear] = i;
					}
					else if (rear == num_of_process - 1 && front != 0){
						rear = 0;
						arr_queue[rear] = i;
					}
					else{
						rear++;
						arr_queue[rear] = i;
					}
				}
			}
		}
		if (strcmp(policy, "FIFO") == 0){
			if(processing && array[now].Ti == 0){
				end_process(now);
				if(done_process == num_of_process)
					break;
			}	
			if (!processing){
				for(int i = 0; i < num_of_process; i++){
					if(array[i].pid != 0 && array[i].Ti > 0){
						set_other(array[i].pid);
						syscall(335, &array[i].start_s, &array[i].start_ns);
						processing = 1;
						now = i;
						break;
					}
				}
			}

		}
		if (strcmp(policy, "RR") == 0){
			if (processing && array[now].Ti == 0){
				end_process(now);
				if (done_process == num_of_process)
					break;
			}
			else if (processing && time_unit == next_round){
				set_idle(array[now].pid);
				processing = 0;
				if (front == -1){
					front = rear = 0;
					arr_queue[rear] = now;
				}
				else if (rear == num_of_process - 1 && front != 0){
					rear = 0;
					arr_queue[rear] = now;
				}
				else{
					rear++;
					arr_queue[rear] = now;
				}
			}
			if (!processing){
				if (front != -1){
					now = arr_queue[front];
					arr_queue[front] = -1;
					if(front == rear){
						front = rear = -1;
					}
					else if (front == num_of_process -1)
						front = 0;
					else
						front++;
					set_other(array[now].pid);
					if (!array[now].ran){
						syscall(335, &array[now].start_s, &array[now].start_ns);
						array[now].ran = 1;
					}	
					processing = 1;
					if (array[now].Ti > 500)
						next_round = time_unit + 500;
					else
						next_round = time_unit + array[now].Ti;
				}
			}
		}
		if (strcmp(policy, "SJF") == 0){
			if (processing && array[now].Ti == 0){	
				end_process(now);
				if(done_process == num_of_process)
					break;
			}
			if (!processing){
				now = run_min(num_of_process, time_unit, now);
			}
		}
		if (strcmp(policy, "PSJF") == 0){
			if (processing && array[now].Ti == 0){
				end_process(now);
				if(done_process == num_of_process)
					break;
			}
			else if (processing){
				now = run_pmin(num_of_process, time_unit, now);

			}
			if (!processing){
				now = run_min(num_of_process, time_unit, now);
			}
		}
		unit_time();
		if (processing){
			array[now].Ti--;
		}	
		time_unit++;
	}
	return 0;
}
