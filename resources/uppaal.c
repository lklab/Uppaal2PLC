#include "platform.h"

#include "uppaal_types.h"
#include "model.h"

static int committed_location_processing();
static int normal_location_processing();

static int take_vaild_transition(Template* task);
static int check_and_take_broadcast_channel(Template* send_task, Transition* send_transition);
static int check_and_take_normal_channel(Template* send_task, Transition* send_transition); // TODO

static int check_guard(Template* task, Transition* transition);
static int check_invariant_and_take_if_true(Template* task, Transition* transition); // TODO

static int urgent_check(); // TODO

int uppaal_init(void)
{
	int i;

	/* allocate context backup memory */
	program.context_backup = os_malloc(program.context_size);
	for(i = 0; program.tasks[i] != NULL; i++)
		program.tasks[i] -> context_backup = os_malloc(program.tasks[i] -> context_size);

	return 0;
}

int uppaal_step(void)
{
	userPeriodicFunc();

	/* dataExchanged channel processing */
	if(!check_and_take_broadcast_channel(NULL, NULL))
		return -1; /* deadlock condition */

	do
	{
		if(!committed_location_processing())
			return -1; /* deadlock condition */
	}
	while(normal_location_processing());

	if(!urgent_check())
		return -1; /* deadlock condition */

	program.program_clock++;

	return 0;
}

int uppaal_cleanup(void)
{
	int i;

	/* free context backup memory */
	os_free(program.context_backup);
	for(i = 0; program.tasks[i] != NULL; i++)
		os_free(program.tasks[i] -> context_backup);

	return 0;
}

static int committed_location_processing()
{
	int i;
	int state_changed = 0;
	int all_committed_proceeded = 1;

	Template* task;

	do
	{
		state_changed = 0;
		all_committed_proceeded = 1;

		for(i = 0; program.tasks[i] != NULL; i++)
		{
			task = program.tasks[i];

			if(task -> current -> mode == LOCATION_COMMITTED)
			{
				state_changed |= take_vaild_transition(task);
				all_committed_proceeded = 0;
			}
		}

		if(!state_changed && !all_committed_proceeded)
			return 0; /* deadlock condition */
	}
	while(!all_committed_proceeded);

	return 1;
}

static int normal_location_processing()
{
	int i;

	for(i = 0; program.tasks[i] != NULL; i++)
	{
		if(take_vaild_transition(program.tasks[i]))
			return 1;
	}

	return 0;
}

static int take_vaild_transition(Template* task)
{
	int i;

	Transition** transitions = task -> current -> transitions;
	Transition* transition;

	for(i = 0; transitions[i] != NULL; i++)
	{
		transition = transitions[i];

		if(!check_guard(task, transition))
			continue;

		if(transition -> chan_in)
			continue;

		if(transition -> chan_out)
		{
			switch(transition -> chan_out -> mode)
			{
				case CHANNEL_NORMAL :
				case CHANNEL_URGENT :
					if(check_and_take_normal_channel(task, transition))
						return 1;
					break;
				case CHANNEL_BROADCAST :
					if(check_and_take_broadcast_channel(task, transition))
						return 1;
					break;
				default :
					break;
			}
			continue;
		}

		if(check_invariant_and_take_if_true(task, transition))
			return 1;
		else
			continue;
	}

	return 0;
}

static int check_and_take_broadcast_channel(Template* send_task, Transition* send_transition)
{
	/* 
	 * WARINING : Current code can behave differently from UPPAAL, when there
	 * are two or more transitions that receive broadcast channels in the
	 * current location of a task and the invariant results can vary depending
	 * on which transition is selected.
	 *
	 * It is recommended to avoid receive broadcast channels for two or more
	 * transitions that are not determined by guard at any location.
	 */

	int i, j;
	int result = 1;
	int is_data_exchanged;

	Template* task;
	Transition* transition;
	Channel* send_channel;

	fupdate_t update;
	finvariant_t invariant;

	if(send_task == NULL || send_transition == NULL)
	{
		send_channel = data_exchanged;
		is_data_exchanged = 1;
	}
	else
	{
		send_channel = send_transition -> chan_out;
		is_data_exchanged = 0;
	}

	if(send_channel == NULL)
		return 0;

	/* 1st step : check guard */
	if(!is_data_exchanged)
	{
		if(!check_guard(send_task, send_transition))
			return 0;
	}
	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		task -> ready = NULL;
		if(task == send_task)
			continue;

		for(j = 0; task -> current -> transitions[j] != NULL; j++)
		{
			transition = task -> current -> transitions[j];
			if(transition -> chan_in == send_channel)
			{
				if(check_guard(task, transition))
				{
					task -> ready = transition;
					break;
				}
			}
		}
	}

	/* 2nd step : backup context and take update for invariant check */
	os_memcpy(program.context_backup, program.context, program.context_size);
	if(!is_data_exchanged)
	{
		os_memcpy(send_task -> context_backup, send_task -> context, send_task -> context_size);
		send_transition -> update(send_task -> context);
	}
	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		if(task -> ready == NULL)
			continue;

		update = task -> ready -> update;
		if(update != NULL)
		{
			os_memcpy(task -> context_backup, task -> context, task -> context_size);
			task -> ready -> update(task -> context);
		}
	}

	/* 3rd step : check invariant */
	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];

		if(task -> ready != NULL)
			invariant = task -> ready -> target -> invariant;
		else
		{
			if(!is_data_exchanged && task == send_task)
				invariant = send_transition -> target -> invariant;
			else
				invariant = task -> current -> invariant;
		}

		if(invariant != NULL && !invariant(task -> context, 0))
		{
			result = 0;
			break;
		}
	}

	/* 4th step */
	if(result) /* take transtion if invariant is true */
	{
		if(!is_data_exchanged)
			send_task -> current = send_transition -> target;

		for(i = 0; program.tasks[i] != NULL; i++)
		{
			task = program.tasks[i];
			if(task -> ready != NULL)
				task -> current = task -> ready -> target;
		}
	}
	else /* restore context if invariant is false */
	{
		os_memcpy(program.context, program.context_backup, program.context_size);
		if(!is_data_exchanged)
			os_memcpy(send_task -> context, send_task -> context_backup, send_task -> context_size);
		for(i = 0; program.tasks[i] != NULL; i++)
		{
			task = program.tasks[i];
			if(task -> ready == NULL)
				continue;

			update = task -> ready -> update;
			if(update != NULL)
				os_memcpy(task -> context, task -> context_backup, task -> context_size);
		}
	}

	return result;

	/* dead code (2nd(final) step) : This works differently from UPPAAL */
/*	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		if(task -> ready)
		{
			if(check_invariant(task, task -> ready))
				take_transition(task, task -> ready);
		}
	}*/
}

static int check_and_take_normal_channel(Template* send_task, Transition* send_transition)
{
	return 0;
}

static int check_guard(Template* task, Transition* transition)
{
	fguard_t guard = transition -> guard;

	if(guard == NULL || guard(task -> context))
		return 1;
	else
		return 0;
}

static int check_invariant_and_take_if_true(Template* task, Transition* transition)
{
	int i;
	int result = 1;

	if(transition -> update != NULL)
	{
		/* backup context */
		os_memcpy(program.context_backup, program.context, program.context_size);
		os_memcpy(task -> context_backup, task -> context, task -> context_size);

		/* take update for invariant check */
		transition -> update(task -> context);

		/* check all invariants */
		for(i = 0; program.tasks[i] != NULL; i++)
		{
			if(program.tasks[i] == task)
			{
				if(!(transition -> target -> invariant(task -> context, 0)))
				{
					result = 0;
					break;
				}
			}
			else
			{
				if(!(program.tasks[i] -> current -> invariant(program.tasks[i] -> context, 0)))
				{
					result = 0;
					break;
				}
			}
		}

		/* restore context */
		os_memcpy(program.context, program.context_backup, program.context_size);
		os_memcpy(task -> context, task -> context_backup, task -> context_size);

		return result;
	}
	else
	{
		/* if there is no update, check only invariant in the next location */
		if(!(transition -> target -> invariant(task -> context, 0)))
			return 0;
		else
			return 1;
	}
}

static int urgent_check()
{
	return 0;
}
