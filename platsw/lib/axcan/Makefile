CC = gcc
PLATSW = ../..
VERSION = 0.1
INC=$(PLATSW)/kernel/drivers/acash3/include/linux/ -I$(PWD)/include
CFLAGS = -fPIC -Wall -Wextra -I$(INC) -DLIB_VER='"$(VERSION)"'
LDFLAGS = -shared
LIB = libaxcan.a
SOLIB = libaxcan.so

SRCS = axcan.c axcan_util.c
OBJS = $(SRCS:.c=.o)
OBJDIR = build/bin

# Debug flags.
ifeq ($(DEBUG),1)
  CFLAGS += -g -O0 -DDEBUG
else
  CFLAGS += -O3
endif

# Default PERF mode=on
PERF=1
ifeq ($(PERF),1)
  CFLAGS += -DPERF
endif

ifeq ($(DEBUG_CANID),1)
  CFLAGS += -DDEBUG_CANID
endif

.PHONY: all
all: ${SOLIB} ${LIB} install

$(SOLIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(LIB): $(OBJS)
	ar rcs $(LIB) -o $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

#include $(SRCS:.c=.d)

.PHONY: install
install: $(OBJS)
	mkdir -p $(PWD)/build/bin
	mkdir -p $(PWD)/build/obj
	mv *.o $(PWD)/build/obj
	mv *.a *.so $(PWD)/build/bin

.PHONY: clean
clean:
	rm -rf ${LIB} ${SOLIB} ${OBJS} $(SRCS:.c=.d) $(PWD)/build
