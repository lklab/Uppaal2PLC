PWD=$(shell pwd)
SRC_PATH=$(PWD)/src

OBJS=$(SRC_PATH)/io.o
SRCS=$(OBJS:.o=.c)

.SUFFIXES:.c.o
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<
	$(AR) rc $(LIB_PATH) $@

all: $(OBJS)

clean:
	rm -rf $(SRC_PATH)/*.o
