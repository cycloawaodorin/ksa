CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -Wdeprecated-declarations -Wimplicit-function-declaration -Wimplicit-int -Wpointer-arith -Wwrite-strings -Wmissing-noreturn -Wno-unused-parameter -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wunused-variable -Winit-self -Wshadow -Wlogical-op -Wconversion -c -O3
CXXFLAGS = -Wall -Wextra -Wdeprecated-declarations -Wpointer-arith -Wwrite-strings -Wmissing-noreturn -Wno-unused-parameter -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wunused-variable -Winit-self -Wshadow -Wlogical-op -Wconversion -Wold-style-cast -c -O3
INCLUDES = 
LD = gcc
LDFLAGS = -shared -Wl,--dll,--enable-stdcall-fixup
LIBS = lua/lua51.dll
RM = rm -f

TARGET = ksa_ext.dll
OBJS = ksa_ext.o

.SUFFIXES: .cpp .o
.SUFFIXES: .c .o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS)

release:
	make all
	strip $(TARGET)
	cp -f $(TARGET) ../scripts
	make clean
