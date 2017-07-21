PWD=$(shell pwd)
SRC_PATH=$(PWD)/src
INCLUDE=-I$(PWD)/include

OBJS=$(SRC_PATH)/ethercatbase.o
OBJS+=$(SRC_PATH)/ethercatcoe.o
OBJS+=$(SRC_PATH)/ethercatconfig.o
OBJS+=$(SRC_PATH)/ethercatdc.o
OBJS+=$(SRC_PATH)/ethercatfoe.o
OBJS+=$(SRC_PATH)/ethercatmain.o
OBJS+=$(SRC_PATH)/ethercatprint.o
OBJS+=$(SRC_PATH)/ethercatsoe.o
OBJS+=$(SRC_PATH)/nicdrv.o
OBJS+=$(SRC_PATH)/osal.o
OBJS+=$(SRC_PATH)/oshw.o

SRCS=$(OBJS:.o=.c)

.SUFFIXES:.c.o
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<
	$(AR) rc $(LIB_PATH) $@

all: $(OBJS)

clean:
	rm -rf $(SRC_PATH)/*.o
