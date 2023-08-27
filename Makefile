# Output binary
OUTPUT ?= modem_host.bin

# Compiler
CXX ?= /usr/bin/g++

# Flags
CXXFLAGS ?= -Og -g $(shell pkg-config --cflags dbus-1) -I/usr/include/dbus-c++-1

# Libraries
LIBS = -lpthread -ldbus-c++-1 `pkg-config --libs dbus-1`

# Source files
SRCS = main.cpp CellularManager.cpp Logic.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OUTPUT) $(OBJS)
