#include "platform.h"

#include <stdlib.h>
#include <signal.h>

typedef void (*func)(void);
static func registered_handler = NULL;

static void sigint_handler(int sig)
{
	if(registered_handler != NULL)
		registered_handler();
}

int os_register_interrupt_handler(void (*handler)(void))
{
	registered_handler = handler;
	signal(SIGINT, sigint_handler);
	return 0;
}

void os_exit_process(int value)
{
	exit(value);
}
