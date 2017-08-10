#ifndef _UPPAAL_TYPES_H
#define _UPPAAL_TYPES_H

#define LOCATION_NORMAL 1
#define LOCATION_URGENT 2
#define LOCATION_COMMITTED 3
#define CHANNEL_NORMAL 4
#define CHANNEL_URGENT 5
#define CHANNEL_BROADCAST 6
#define CHANNEL_DATAEXCHANGED 7

#ifndef NULL
#define NULL 0
#endif

#define true 1
#define false 0

typedef struct Program Program;
typedef struct Template Template;
typedef struct Location Location;
typedef struct Transition Transition;
typedef struct Channel Channel;

typedef int (*finvariant_t)(void*, int);
typedef int (*fguard_t)(void*);
typedef void (*fupdate_t)(void*);

#define bool int
typedef unsigned long long Clock;

struct Program
{
	void* context;
	void* context_backup;
	unsigned int context_size;

	Template** tasks;
	Clock program_clock;
};

struct Template
{
	void* context;
	void* context_backup;
	unsigned int context_size;
	Location* current;

	Transition* ready;
};

struct Location
{
	Transition** transitions;
	int mode;
	finvariant_t invariant;
};

struct Transition
{
	Location* source;
	Location* target;
	Channel* chan_in;
	Channel* chan_out;
	fguard_t guard;
	fupdate_t update;
};

struct Channel
{
	int mode;
};

#endif
