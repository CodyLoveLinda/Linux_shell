# 
# Copyright (C) 2019 AutoX, Inc.
#

SRCDIR = .
CC = gcc
KERINC = $(PWD)/../../kernel/drivers/acash/include/
CFLAGS = -I$(KERINC)
LIBS = -lpci

all: x_update

x_update: $(SRCDIR)/fw_update.c
	$(CC) $(CFLAGS) -o x_update $(SRCDIR)/fw_update.c $(LIBS)

clean:
	rm x_update
