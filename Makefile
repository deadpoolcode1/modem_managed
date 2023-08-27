# Output binary
OUTPUT ?= modem_host.bin

# Compiler
CXX ?= /usr/bin/g++

# Flags
CXXFLAGS ?= -Og -g

# Libraries
LIBS = -lpthread

# Source files
SRCS = main.cpp CellularManager.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OUTPUT) $(OBJS)
