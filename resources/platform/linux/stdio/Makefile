PWD=$(shell pwd)
SRC_PATH=$(PWD)/src

OBJS=$(SRC_PATH)/io.o
SRCS=$(OBJS:.o=.c)

.SUFFIXES:.c.o
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<

all: $(OBJS)
	$(AR) rc $(LIB_PATH) $(OBJS)

clean:
	rm -rf $(SRC_PATH)/*.o
