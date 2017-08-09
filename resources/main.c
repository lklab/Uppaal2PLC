#include "platform.h"

#include "uppaal.h"
#include "model.h"

#ifndef PERIOD
#define PERIOD 10000000LL // 10ms
#endif

static void task_proc(void);
static void interrupt_handler(void);
static void cleanup_and_exit(int value);

static task_t task;

int main(void)
{
	int i, ret;

	ret = io_init();
	if(ret < 0)
	{
		io_cleanup();
		os_exit_process(-1);
	}

	for(i = 0; mapping_list[i].variable != NULL; i++)
	{
		ret = io_mapping(mapping_list[i].variable, mapping_list[i].size,
			mapping_list[i].address, mapping_list[i].direction);
		if(ret < 0)
		{
			io_cleanup();
			os_exit_process(-1);
		}
	}

	uppaal_init();
	os_register_interrupt_handler(interrupt_handler);

	task_init(&task, task_proc, PERIOD);
	task_start(&task);

	return 0;
}

static void task_proc(void)
{
	if(io_exchange() < 0)
		cleanup_and_exit(-1);
	if(uppaal_step() < 0)
		cleanup_and_exit(-1);
}

static void interrupt_handler(void)
{
	cleanup_and_exit(0);
}

static void cleanup_and_exit(int value)
{
	task_stop(&task);
	io_cleanup();
	uppaal_cleanup();
	os_exit_process(value);
}
