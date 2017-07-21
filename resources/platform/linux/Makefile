PWD=$(shell pwd)
SRC_PATH=$(PWD)/src

OBJS=$(SRC_PATH)/os.o $(SRC_PATH)/task.o
SRCS=$(OBJS:.o=.c)

.SUFFIXES:.c.o
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<
	$(AR) rc $(LIB_PATH) $@

all: $(OBJS) $(PROTO)

SOEM:
	$(MAKE) -C soem

STDIO:
	$(MAKE) -C stdio

clean:
	$(MAKE) -C soem clean
	$(MAKE) -C stdio clean
	rm -rf $(SRC_PATH)/*.o
