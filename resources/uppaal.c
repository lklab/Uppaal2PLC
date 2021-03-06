#include "platform.h"

#include "uppaal_types.h"
#include "model.h"

static int committed_location_processing();
static int normal_location_processing();

static int take_valid_transition(Template* task);

static int check_and_take_broadcast_channel(Template* send_task, Transition* send_transition);
static int find_sending_broadcast_and_take(Template* receive_task, Transition* receive_transition);
static int check_and_take_normal_channel(Template* send_task, Transition* send_transition);
static int check_invariant_and_take_if_true(Template* task, Transition* transition);

static int backup_context_and_take_update(Template* task, Transition* transition, int backup_program_context);
static int restore_context(Template* task, Transition* transition, int restore_program_context);

static int check_guard(Template* task, Transition* transition);
static int check_validation_for_next_period();

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

	if(!check_validation_for_next_period())
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
				state_changed |= take_valid_transition(task);
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
		if(take_valid_transition(program.tasks[i]))
			return 1;
	}

	return 0;
}

static int take_valid_transition(Template* task)
{
	int i;

	Transition** transitions = task -> current -> transitions;
	Transition* transition;

	for(i = 0; transitions[i] != NULL; i++)
	{
		transition = transitions[i];

		if(!check_guard(task, transition))
			continue;

		if(transition -> chan_in != NULL)
		{
			switch(transition -> chan_in -> mode)
			{
				case CHANNEL_NORMAL :
				case CHANNEL_URGENT :
					if(check_and_take_normal_channel(task, transition))
						return 1;
					break;
				case CHANNEL_BROADCAST :
					if(find_sending_broadcast_and_take(task, transition))
						return 1;
					break;
				default :
					break;
			}
			continue;
		}

		if(transition -> chan_out != NULL)
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
	 * current location of any task and the invariant results can vary
	 * depending on which transition is selected.
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
		backup_context_and_take_update(send_task, send_transition, 0);
	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		if(task -> ready == NULL)
			continue;
		backup_context_and_take_update(task, task -> ready, 0);
	}

	/* 3rd step : check invariant */
	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];

		if(task -> ready != NULL)
			invariant = task -> ready -> target -> invariant;
		else
		{
			if(!is_data_exchanged && (task == send_task))
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
			restore_context(send_task, send_transition, 0);
		for(i = 0; program.tasks[i] != NULL; i++)
		{
			task = program.tasks[i];
			if(task -> ready != NULL)
				restore_context(task, task -> ready, 0);
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

static int find_sending_broadcast_and_take(Template* receive_task, Transition* receive_transition)
{
	int i, j;

	Template* task;
	Transition* transition;
	Channel* receive_channel;

	if(!check_guard(receive_task, receive_transition))
		return 0;
	receive_channel = receive_transition -> chan_in;
	if(receive_channel == NULL)
		return 0;

	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		if(task == receive_task)
			continue;

		for(j = 0; task -> current -> transitions[j] != NULL; j++)
		{
			transition = task -> current -> transitions[j];
			if(transition -> chan_out == receive_channel)
			{
				if(check_and_take_broadcast_channel(task, transition))
					return 1;
				else
					continue;
			}
		}
	}

	return 0;
}

static int check_and_take_normal_channel(Template* ref_task, Transition* ref_transition)
{
	int i, j, k;
	int invariant_result;
	int ref_invariant_result = 1;
	int find_send;

	Template* task;
	Transition* transition;
	Template** send_task;
	Transition** send_transition;
	Template** receive_task;
	Transition** receive_transition;
	Channel* ref_channel;

	finvariant_t invariant;

	if(!check_guard(ref_task, ref_transition))
		return 0;

	if(ref_transition -> chan_in != NULL)
	{
		ref_channel = ref_transition -> chan_in;
		send_task = &task;
		send_transition = &transition;
		receive_task = &ref_task;
		receive_transition = &ref_transition;
		find_send = 1;
	}
	else if(ref_transition -> chan_out != NULL)
	{
		ref_channel = ref_transition -> chan_out;
		send_task = &ref_task;
		send_transition = &ref_transition;
		receive_task = &task;
		receive_transition = &transition;
		find_send = 0;
	}
	else
		return 0;

	if(ref_transition -> update == NULL)
	{
		invariant = ref_transition -> target -> invariant;
		if(invariant != NULL && !invariant(ref_task -> context, 0))
			ref_invariant_result = 0;
	}

	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		if(task == ref_task)
			continue;

		for(j = 0; task -> current -> transitions[j] != NULL; j++)
		{
			transition = task -> current -> transitions[j];

			if((find_send && (transition -> chan_out == ref_channel)) ||
				(!find_send && (transition -> chan_in == ref_channel)))
			{
				/* guard check */
				if(!check_guard(task, transition))
					continue;

				/* if there is no update, check only invariant in the next location */
				if(!ref_invariant_result && transition -> update == NULL)
				{
					invariant = transition -> target -> invariant;
					if(invariant != NULL && !invariant(task -> context, 0))
						continue;
				}

				/* backup context and take update for invariant check*/
				os_memcpy(program.context_backup, program.context, program.context_size);
				backup_context_and_take_update(*send_task, *send_transition, 0);
				backup_context_and_take_update(*receive_task, *receive_transition, 0);

				/* invariant check */
				invariant_result = 1;
				for(k = 0; program.tasks[k] != NULL; k++)
				{
					if(program.tasks[k] == ref_task)
						invariant = ref_transition -> target -> invariant;
					else if(program.tasks[k] == task)
						invariant = transition -> target -> invariant;
					else
						invariant = program.tasks[k] -> current -> invariant;

					if(invariant != NULL && !invariant(program.tasks[k], 0))
					{
						invariant_result = 0;
						break;
					}
				}

				if(invariant_result) /* take transtion if invariant is true */
				{
					ref_task -> current = ref_transition -> target;
					task -> current = transition -> target;
					return 1;
				}
				else /* restore context if invariant is false */
				{
					os_memcpy(program.context, program.context_backup, program.context_size);
					restore_context(*send_task, *send_transition, 0);
					restore_context(*receive_task, *receive_transition, 0);
					continue;
				}
			}
		}
	}

	return 0;
}

static int check_invariant_and_take_if_true(Template* task, Transition* transition)
{
	int i;

	finvariant_t invariant;

	if(backup_context_and_take_update(task, transition, 1))
	{
		/* check all invariants */
		for(i = 0; program.tasks[i] != NULL; i++)
		{
			if(program.tasks[i] == task)
				invariant = transition -> target -> invariant;
			else
				invariant = program.tasks[i] -> current -> invariant;

			if(invariant != NULL && !invariant(program.tasks[i] -> context, 0))
			{
				restore_context(task, transition, 1);
				return 0;
			}
		}

		task -> current = transition -> target;
		return 1;
	}
	else
	{
		/* if there is no update, check only invariant in the next location */
		invariant = transition -> target -> invariant;
		if(invariant == NULL || invariant(task -> context, 0))
		{
			task -> current = transition -> target;
			return 1;
		}
		else
			return 0;
	}
}

static int backup_context_and_take_update(Template* task, Transition* transition, int backup_program_context)
{
	if(transition -> update != NULL)
	{
		if(backup_program_context)
			os_memcpy(program.context_backup, program.context, program.context_size);
		os_memcpy(task -> context_backup, task -> context, task -> context_size);
		transition -> update(task -> context);
		return 1;
	}
	return 0;
}

static int restore_context(Template* task, Transition* transition, int restore_program_context)
{
	if(transition -> update != NULL)
	{
		if(restore_program_context)
			os_memcpy(program.context, program.context_backup, program.context_size);
		os_memcpy(task -> context, task -> context_backup, task -> context_size);
		return 1;
	}
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

static int check_validation_for_next_period()
{
	int i, j;
	Template* task;
	Channel* channel;

	for(i = 0; program.tasks[i] != NULL; i++)
	{
		task = program.tasks[i];
		if(task -> current -> mode == LOCATION_COMMITTED ||
			task -> current -> mode == LOCATION_URGENT)
			return 0;

		if(task -> current -> invariant != NULL)
		{
			if(!(task -> current -> invariant(task -> context, 1)))
				return 0;
		}
	}

	return 1;
}
