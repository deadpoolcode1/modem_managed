# Compiler
CXX = g++

# Compilation flags
CXXFLAGS = -Wall -std=c++14 $(shell pkg-config --cflags dbus-1) -I/usr/include/dbus-c++-1

# Libraries
LIBS = -ldbus-c++-1 `pkg-config --libs dbus-1`

# Source and object files
SRCS = main.cpp CellularManager.cpp
OBJS = $(SRCS:.cpp=.o)

# Output binary
TARGET = modem_managed

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) *.o *~ $(TARGET)