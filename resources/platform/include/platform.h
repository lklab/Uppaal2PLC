#ifndef _PLATFORM_H
#define _PLATFORM_H

typedef void (*proc_t)(void);

typedef struct
{
	proc_t proc;
	unsigned long long period;
	int alive;
} task_t;

int task_init(task_t* task, void (*proc)(void), unsigned long long period);
int task_start(task_t* task);
int task_stop(task_t* task);

int io_init(void);
int io_mapping(void* variable, int size, char* address, int direction);
int io_exchange(void);
int io_cleanup(void);

int os_register_interrupt_handler(void (*handler)(void));
void os_exit_process(int value);

#endif
