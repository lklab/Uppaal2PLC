PWD=$(shell pwd)
SRC_PATH=$(PWD)/src

OBJS=$(SRC_PATH)/io.o
SRCS=$(OBJS:.o=.c)

INCLUDE+=-Iiolib/include

.SUFFIXES:.c.o
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<

all: $(OBJS) IOLIB
	$(AR) rc $(LIB_PATH) $(OBJS)

IOLIB:
	$(MAKE) -C iolib

clean:
	$(MAKE) -C iolib clean
	rm -rf $(SRC_PATH)/*.o
