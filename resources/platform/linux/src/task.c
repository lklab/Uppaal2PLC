#include "platform.h"

#include <unistd.h>
#include <time.h>

#define NSEC_PER_SEC 1000000000ULL

struct timespec read_time;
struct timespec release_time;

static void timespec_chk_overflow(struct timespec *ts)
{
	while (ts -> tv_nsec >= NSEC_PER_SEC)
	{
		ts -> tv_nsec -= NSEC_PER_SEC;
		ts -> tv_sec++;
	}
}

int task_init(task_t* task, void (*proc)(void), unsigned long long period)
{
	task -> proc = proc;
	task -> period = period;
	task -> alive = 1;

	return 0;
}

int task_start(task_t* task)
{
	clock_gettime(CLOCK_REALTIME, &read_time);

	while(task -> alive)
	{
		task -> proc();

		read_time.tv_nsec += task -> period;
		timespec_chk_overflow(&read_time);
		release_time.tv_sec = read_time.tv_sec;
		release_time.tv_nsec = read_time.tv_nsec;
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &release_time, NULL);
	}

	return 0;
}

int task_stop(task_t* task)
{
	task -> alive = 0;
	return 0;
}
