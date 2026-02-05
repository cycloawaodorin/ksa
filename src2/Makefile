CC = gcc
CXX = g++
CXXWFLAGS = -Wall -Wextra -Wdeprecated-declarations -Wpointer-arith -Wwrite-strings -Wmissing-noreturn -Wno-unused-parameter -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wunused-variable -Winit-self -Wshadow -Wlogical-op -Wconversion -Wold-style-cast
CXXFLAGS = -c -O3 -std=c++23
INCLUDES = 
LD = g++
LDFLAGS = -shared -Wl,--dll,--enable-stdcall-fixup
LIBS = -static -lstdc++ -lwinpthread
RM = rm -f
SH = bash

TARGET = ksa_ext.mod2
OBJ = ksa_ext.o
DEF = ksa_ext.def

all: $(TARGET)

$(TARGET): ksa_ext.o
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJ) $(DEF) $(LIBS)

$(OBJ): ksa_ext_main.cpp ksa_ext.cpp functions.cpp version.hpp
	$(CXX) $(CXXWFLAGS) $(CXXFLAGS) $(INCLUDES) -o $(OBJ) ksa_ext_main.cpp

functions.cpp: ksa_ext.cpp extract_functions.sh
	$(SH) extract_functions.sh

clean:
	$(RM) $(TARGET) $(OBJ) functions.cpp

release:
	make all
	strip $(TARGET)
	cp -f $(TARGET) ../ksa2
	$(SH) makezip.sh ksa
	make clean
