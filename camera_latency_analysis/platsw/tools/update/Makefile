# 
# Copyright (C) 2019 AutoX, Inc.
#

SRCDIR = .
CC = gcc
KERINC = $(PWD)/../../kernel/drivers/acash3/include/
CFLAGS = -I$(KERINC)
SRCS = $(wildcard ./*.c)
OBJS = $(SRCS:.c=.o)
HDR = $(wildcard ./*.h)

all: x_update

x_update: $(OBJS)
	$(CC) -o $@ $^
	@rm -f $(OBJS)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) x_update
