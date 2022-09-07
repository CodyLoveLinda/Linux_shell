# 
# Copyright (C) 2019 AutoX, Inc.
#

PLATSW=$(PWD)/../..
LIB_PATH=$(PLATSW)/lib/axcan
INC=-I$(LIB_PATH)/include
BLD_PATH=$(PWD)/build/bin

CXX=gcc
CXXFLAGS=-Wall -march=native -O3 -g -pipe 
LDFLAGS=-L$(LIB_PATH)/build/bin -laxcan -lpthread

#SRC=$(wildcard *.c)
SRC=canapp.c
HDR=$(wildcard *.h)
OBJ=$(SRC:.c=.o)
TARGET=canapp

.PHONY:all
all: $(TARGET)

$(TARGET): $(OBJ) $(LIB)
	$(CXX)  $(INC) -o $@ $^ $(LDFLAGS)
	@rm -f $(OBJ)

%.o: %.c $(HDR)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

build:
	mkdir -p $(BLD_PATH)/bin

.PHONY: install
install: all build
	cp $(TARGET) $(BLD_PATH)/bin/

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
