CC=gcc
AR=ar

PWD=$(shell pwd)

OBJS=main.o uppaal.o model.o
SRCS=$(OBJS:.o=.c)

PLATFORM=platform

INCLUDE=-I$(PLATFORM)/include
CFLAGS=-c
LDFLAGS=-L$(PLATFORM) -l$(PLATFORM) -lpthread -lrt

TARGET=PlcApp

export CC
export AR
export PWD

.SUFFIXES:.c.o
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) $<

all: $(OBJS) PLATFORM_LIB
	@if [ "$(OS)" = "LINUX" ]; then \
		if [ "$(PROTO)" = "STDIO" ]; then \
			$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) -lrt ; \
			echo "create $(TARGET) for lunux & stdio platform" ; \
		elif [ "$(PROTO)" = "SOEM" ]; then \
			$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) -lpthread -lrt ; \
			echo "create $(TARGET) for lunux & soem platform" ; \
		fi \
	fi

PLATFORM_LIB:
	$(MAKE) -C $(PLATFORM)

main.o: model.h main.c
	$(CC) $(INCLUDE) $(CFLAGS) main.c

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -rf *.o $(TARGET)
