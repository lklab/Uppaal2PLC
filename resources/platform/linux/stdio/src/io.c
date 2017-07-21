#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct io_value_t io_value_t;

struct io_value_t
{
	void* variable;
	int size;
	char* name;
	io_value_t* next;
};

static int exchange_count = 0;
static int skip_count = 0;

static io_value_t* input_list = NULL;
static io_value_t* output_list = NULL;

static io_value_t* input_tail = NULL;
static io_value_t* output_tail = NULL;

int io_init(void)
{
	return 0;
}

int io_mapping(void* variable, int size, char* address, int direction)
{
	io_value_t* io_value = (io_value_t*)malloc(sizeof(io_value_t));
	io_value -> variable = variable;
	io_value -> size = size;
	io_value -> name = address;
	io_value -> next = NULL;

	switch(size)
	{
	case 1 :
	case 2 :
	case 4 : break;
	default :
		free(io_value);
		return -1;
	}

	if(direction)
	{
		if(input_list == NULL)
			input_list = io_value;
		else
			input_tail -> next = io_value;
		input_tail = io_value;
	}
	else
	{
		if(output_list == NULL)
			output_list = io_value;
		else
			output_tail -> next = io_value;
		output_tail = io_value;
	}

	return 0;
}

int io_exchange(void)
{
	io_value_t* list;

	printf("exchange count = %d\n\n", exchange_count++);

	list = output_list;
	while(list != NULL)
	{
		switch(list -> size)
		{
		case 1 :
			printf("output value of [%s] : %d", list -> name,
				*((char*)(list -> variable)));
			break;
		case 2 :
			printf("output value of [%s] : %d", list -> name,
				*((short*)(list -> variable)));
			break;
		case 4 :
			printf("output value of [%s] : %d", list -> name,
				*((int*)(list -> variable)));
			break;
		}
		list = list -> next;
	}
	printf("\n");

	if(skip_count > 0)
	{
		skip_count--;
		printf("===========================================================\n");
		return 0;
	}

	list = input_list;
	while(list != NULL)
	{
		printf("input value of [%s] : ", list -> name);
		switch(list -> size)
		{
		case 1 :
			scanf("%hhd", (char*)(list -> variable));
			break;
		case 2 :
			scanf("%hd", (short*)(list -> variable));
			break;
		case 4 :
			scanf("%d", (int*)(list -> variable));
			break;
		}
		fflush(stdin);
		list = list -> next;
	}
	printf("\nskip count : ");
	scanf("%d", &skip_count);
	fflush(stdin);

	printf("\n===========================================================\n");

	return 0;
}

int io_cleanup(void)
{
	return 0;
}
