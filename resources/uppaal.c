#include "uppaal_types.h"
#include "model.h"

#include <stdlib.h> // TODO: memcpy, malloc abstraction

static int check_invariant(Template* task, Transition* transition);

static Transition* find_valid_transition(Template* task);
static int can_take_transition(Template* task, Transition* transition);
static void take_transition(Template* task, Transition* transition);

int uppaal_init(void)
{
	int i;

	/* allocate context backup area */
	for(i = 0; system.tasks[i] != NULL, i++)
		task -> context_backup = malloc(task -> context_size);

	return 0;
}

int uppaal_step(void)
{
	int i, j;
	Template* task;
	Transition* transition;

	userPeriodicFunc();

	/* process dataExchanged channel */
	/* first step : check guard */
	for(i = 0; system.tasks[i] != NULL, i++)
	{
		task = system.tasks[i];
		task -> ready = NULL;
		for(j = 0; task -> current -> transitions[j] != NULL; j++)
		{
			transition = task -> current -> transitions[j];
			if(transition -> chan_in -> mode == CHANNEL_DATAEXCHANGED)
			{
				if(transition -> guard(task -> context))
				{
					task -> ready = transition;
					break;
				}
			}
		}
	}

	/* second step : check invariant and take transition */
	for(i = 0; system.tasks[i] != NULL, i++)
	{
		task = system.tasks[i];
		if(task -> ready)
		{
			if(check_invariant(task, task -> ready))
				take_transition(task, task -> ready);
		}
	}

	task = system.tasks[0];
	do
	{
		transition = find_valid_transition(task);
		if(transition != NULL)
			take_transition(task, transition);
	} while(task -> current -> mode == LOCATION_COMMITTED);

	system.system_clock++;

	return 0;
}

int uppaal_cleanup(void)
{
	int i;

	/* free context backup area */
	for(i = 0; system.tasks[i] != NULL, i++)
		free(task -> context_backup);

	return 0;
}

static int check_invariant(Template* task, Transition* transition)
{
	int i;
	int result = 1;

	if(transition -> update != NULL)
	{
		/* backup context */
		memcpy(system.context_backup, system.context, system.context_size);
		memcpy(task -> context_backup, task -> context, task -> context_size);

		/* take update for test */
		transition -> update(task -> context);

		/* check all invariants */
		for(i = 0; system.tasks[i] != NULL; i++)
		{
			if(system.tasks[i] == task)
			{
				if(!(transition -> target -> invariant(task -> context, 0)))
				{
					result = 0;
					break;
				}
			}
			else
			{
				if(!(system.tasks[i] -> current -> invariant(system.tasks[i] -> context, 0)))
				{
					result = 0;
					break;
				}
			}
		}

		/* restore context */
		memcpy(system.context, system.context_backup, system.context_size);
		memcpy(task -> context, task -> context_backup, task -> context_size);

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

static Transition* find_valid_transition(Template* task)
{
	int i;
	Location* location = task -> current;
	Transition** transitions = location -> transitions;

	if(transitions == NULL)
		return NULL;

	for(i = 0; transitions[i] != NULL; i++)
	{
		if(can_take_transition(task, transitions[i]))
		{
			return transitions[i];
		}
	}

	return NULL;
}

static int can_take_transition(Template* task, Transition* transition)
{
	if(transition -> guard != NULL)
	{
		if (!(transition -> guard(task -> context)))
			return 0;
	}

	return 1;
}

static void take_transition(Template* task, Transition* transition)
{
	if(transition -> update != NULL)
		transition -> update(task -> context);
	task -> current = transition -> target;
}
