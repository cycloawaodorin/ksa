CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -Wdeprecated-declarations -Wimplicit-function-declaration -Wimplicit-int -Wpointer-arith -Wwrite-strings -Wmissing-noreturn -Wno-unused-parameter -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wunused-variable -Winit-self -Wshadow -Wlogical-op -Wconversion -c -O3
CXXFLAGS = -Wall -Wextra -Wdeprecated-declarations -Wpointer-arith -Wwrite-strings -Wmissing-noreturn -Wno-unused-parameter -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wunused-variable -Winit-self -Wshadow -Wlogical-op -Wconversion -Wold-style-cast -c -O3
INCLUDES = 
LD = gcc
LDFLAGS = -shared -Wl,--dll,--enable-stdcall-fixup
LIBS = lua/lua51.dll
RM = rm -f
SH = bash

TARGET = ksa_ext.dll
OBJ = ksa_ext.o
DEF = ksa_ext.def

all: $(TARGET)

$(TARGET): ksa_ext.o
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJ) $(DEF) $(LIBS)

$(OBJ): ksa_ext_main.cpp ksa_ext.cpp functions.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OBJ) ksa_ext_main.cpp

functions.c: ksa_ext.cpp extract_functions.sh
	$(SH) extract_functions.sh

clean:
	$(RM) $(TARGET) $(OBJ) functions.c

release:
	make all
	strip $(TARGET)
	cp -f $(TARGET) ../scripts
	make clean
