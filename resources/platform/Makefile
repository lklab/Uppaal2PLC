PWD=$(shell pwd)

LIB_TARGET=libplatform.a
LIB_PATH=$(PWD)/$(LIB_TARGET)

INCLUDE=-I$(PWD)/include
CFLAGS=-c

export LIB_PATH
export INCLUDE
export CFLAGS

all: $(OS)

LINUX:
	$(MAKE) -C linux

clean:
	$(MAKE) -C linux clean
	rm -rf $(LIB_PATH)
