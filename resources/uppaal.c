#include "uppaal_types.h"
#include "model.h"

static int check_invariant(Template* task, Transition* transition);

static Transition* find_valid_transition(Template* task);
static int can_take_transition(Template* task, Transition* transition);
static void take_transition(Template* task, Transition* transition);

int uppaal_init(void)
{
	return 0;
}

int uppaal_step(void)
{
	Transition* transition;
	Template* task;

	userPeriodicFunc();

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

static int check_invariant(Template* task, Transition* transition)
{
	;
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
