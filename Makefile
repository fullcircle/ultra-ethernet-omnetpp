#
# Makefile for Ultra Ethernet OMNeT++ Simulation
#

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -DNDEBUG -MMD -MP -MF $(@:.o=.d)

# OMNeT++ configuration
OMNETPP_INCL_DIR = $(OMNETPP_ROOT)/include
OMNETPP_LIB_DIR = $(OMNETPP_ROOT)/lib

# INET Framework (if used)
INET_PROJ = $(INET_ROOT)
INET_LIB = -L$(INET_PROJ)/src -lINET$(DBG_SUFFIX)

# Include paths
INCLUDE_PATH = -I. -I$(OMNETPP_INCL_DIR) -I$(INET_PROJ)/src

# Libraries
LIBS = $(INET_LIB) -loppenvir$(DBG_SUFFIX) -loppsim$(DBG_SUFFIX)

# Source files
SOURCES = $(wildcard *.cc)
OBJECTS = $(SOURCES:.cc=.o)
HEADERS = $(wildcard *.h)
MSG_FILES = $(wildcard *.msg)
MSG_CC = $(MSG_FILES:.msg=_m.cc)
MSG_H = $(MSG_FILES:.msg=_m.h)

# Target executable
TARGET = ultraethernet_sim

# Default target
all: $(TARGET)

# Message file compilation
%_m.cc %_m.h: %.msg
	opp_msgc -s _m.cc -h _m.h $<

# Object file compilation
%.o: %.cc $(MSG_H)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) $(MSG_CC:.cc=.o)
	$(CXX) -o $@ $^ $(LIBS) -Wl,-rpath,$(OMNETPP_LIB_DIR)

# Clean
clean:
	rm -f $(TARGET) *.o *.d *_m.cc *_m.h

# Install (copy to results directory)
install: $(TARGET)
	mkdir -p results
	cp $(TARGET) omnetpp.ini results/

# Run simulation
run: $(TARGET)
	./$(TARGET) -u Cmdenv -c UltraEthernet_1K

# Run performance comparison
benchmark: $(TARGET)
	./$(TARGET) -u Cmdenv -c Performance_Comparison

# Run parameter sweep
sweep: $(TARGET)
	./$(TARGET) -u Cmdenv -c Parameter_Sweep

# Parallel execution (MPI)
run-parallel: $(TARGET)
	mpirun -np 4 ./$(TARGET) -u Cmdenv -c UltraEthernet_10K \
		--parsim-communications-class=cMPICommunications

.PHONY: all clean install run benchmark sweep run-parallel

# Include dependency files
-include $(OBJECTS:.o=.d)