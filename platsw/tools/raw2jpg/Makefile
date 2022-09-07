# 
# Copyright (C) 2019 AutoX, Inc.
#

PSW_ROOT=$(PWD)
BLD_PATH=$(PSW_ROOT)/build

CXX=g++
CXXFLAGS=-std=c++11 -mavx2
LDFLAGS=-no-pie  -lrt   ./libturbojpeg.a

INC=-I$(PSW_ROOT)/include

SRC=$(wildcard *.cpp)
HDR=$(wildcard *.h)
OBJ=$(SRC:.cpp=.o)
TARGET=raw2jpg


.PHONY:all
all: $(TARGET)

$(TARGET): $(OBJ) $(LIB)
	$(CXX) -o $@ $^ $(LDFLAGS)
	@rm -f $(OBJ)

%.o: %.cpp $(HDR)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

build:
	mkdir -p $(BLD_PATH)/bin

.PHONY: install
install: all build
	cp $(TARGET) $(BLD_PATH)/bin/

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
