#ifndef _UPPAAL_TYPES_H
#define _UPPAAL_TYPES_H

#define LOCATION_NORMAL 1
#define LOCATION_URGENT 2
#define LOCATION_COMMITTED 4
#define CHANNEL_NORMAL 1
#define CHANNEL_URGENT 2
#define CHANNEL_BROADCAST 4
#define CHANNEL_DATAEXCHANGED 8

#ifndef NULL
#define NULL 0
#endif

#define true 1
#define false 0

typedef struct System System;
typedef struct Template Template;
typedef struct Location Location;
typedef struct Transition Transition;
typedef struct Channel Channel;

typedef int (*finvariant_t)(void*, int);
typedef int (*fguard_t)(void*);
typedef void (*fupdate_t)(void*);

#define bool int
typedef unsigned long long Clock;

struct System
{
	void* context;
	void* context_backup;
	int context_size;

	Template** tasks;
	Clock system_clock;
};

struct Template
{
	void* context;
	void* context_backup;
	int context_size;
	Location* current;

	void* ready;
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
	int send_ready;
	int recv_ready;
};

#endif
