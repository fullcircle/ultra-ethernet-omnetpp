#
# OMNeT++/OMNEST Makefile for UEC
#
# This file was generated with the command:
#  opp_makemake -f --deep
#

# Name of target to be created (-o option)
TARGET_DIR = .
TARGET_NAME = UEC$(D)
TARGET = $(TARGET_NAME)$(EXE_SUFFIX)
TARGET_FILES = $(TARGET_DIR)/$(TARGET)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(QTENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(QTENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH =

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS =

# MPI Support
MPI_CFLAGS = $(shell mpic++ --showme:compile)
MPI_LIBS = $(shell mpic++ --showme:link)
CFLAGS_EXTRA += $(MPI_CFLAGS) -DWITH_MPI
LDFLAGS_EXTRA += $(MPI_LIBS)

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = ultra-ethernet-omnetpp
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/AIHPCApplication.o \
    $O/INCProcessor.o \
    $O/PerformanceAnalyzer.o \
    $O/SwitchFabric.o \
    $O/SwitchPort.o \
    $O/TopologyGenerator.o \
    $O/UETTransport.o \
    $O/UltraEthernetIP.o \
    $O/UltraEthernetLink.o \
    $O/UltraEthernetPhy.o \
    $O/UltraEthernetMsg_m.o

# Message files
MSGFILES = \
    UltraEthernetMsg.msg

# SM files
SMFILES =

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
CONFIGFILE = $(shell opp_configfilepath)
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = $(OPPMAIN_LIB) $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS) $(IMPORT_DEFINES)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE (if COPTS has changed since last build)
# and make the object files depend on it
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
  $(shell $(MKPATH) "$O")
  $(file >$(COPTS_FILE),$(COPTS))
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
#------------------------------------------------------------------------------

# Main target
all: $(TARGET_FILES)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clang-msabi)
	-$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib) 2>/dev/null

$O/$(TARGET_NAME).pdb: $O/$(TARGET)
endif

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating executable: $@
	$(Q)$(CXX) $(LDFLAGS) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY: all clean cleanall depend msgheaders smheaders

# disabling all implicit rules
.SUFFIXES :
.PRECIOUS : %_m.h %_m.cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $<)_m.h.d $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_FILES)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(CLEANALL_COMMAND)
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

help:
	@echo "$$HELP_SYNOPSYS"
	@echo "$$HELP_TARGETS"
	@echo "$$HELP_VARIABLES"
	@echo "$$HELP_EXAMPLES"

# Custom simulation targets
run-dragonfly-1000:
	@echo "Running 1000-host Dragonfly topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./$(TARGET) -u Cmdenv -c Dragonfly_1000 -f large_scale_test.ini

run-leafspine-1000:
	@echo "Running 1000-host Leaf-Spine topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./$(TARGET) -u Cmdenv -c LeafSpine_1000 -f large_scale_test.ini

run-mesh-1000:
	@echo "Running 1000-host Mesh topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./$(TARGET) -u Cmdenv -c Mesh_1000 -f large_scale_test.ini

run-torus-1000:
	@echo "Running 1000-host Torus topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./$(TARGET) -u Cmdenv -c Torus_1000 -f large_scale_test.ini

# MPI Parallel simulation targets
run-parallel-dragonfly-1000:
	@echo "Running 1000-host Dragonfly topology with MPI parallelization (8 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 8 ./$(TARGET) -u Cmdenv -c Dragonfly_1000 -f large_scale_test.ini

run-parallel-leafspine-1000:
	@echo "Running 1000-host Leaf-Spine topology with MPI parallelization (8 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 8 ./$(TARGET) -u Cmdenv -c LeafSpine_1000 -f large_scale_test.ini

run-parallel-mesh-1000:
	@echo "Running 1000-host Mesh topology with MPI parallelization (8 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 8 ./$(TARGET) -u Cmdenv -c Mesh_1000 -f large_scale_test.ini

run-parallel-torus-1000:
	@echo "Running 1000-host Torus topology with MPI parallelization (8 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 8 ./$(TARGET) -u Cmdenv -c Torus_1000 -f large_scale_test.ini

# Extreme scale (10K hosts) with MPI
run-parallel-10k-leafspine:
	@echo "Running 10K-host Leaf-Spine simulation with MPI parallelization (16 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 16 ./$(TARGET) -u Cmdenv -c LeafSpine_10K -f large_scale_test.ini

run-parallel-10k-dragonfly:
	@echo "Running 10K-host Dragonfly simulation with MPI parallelization (16 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 16 ./$(TARGET) -u Cmdenv -c Dragonfly_10K -f large_scale_test.ini

# Optimized large-scale traffic test targets
run-largescale-1k:
	@echo "Running optimized 1K-host large-scale traffic simulation..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./$(TARGET) -u Cmdenv -c LargeScaleTest_1K -f large_scale_test.ini

run-largescale-5k:
	@echo "Running optimized 5K-host extreme-scale traffic simulation..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./$(TARGET) -u Cmdenv -c LargeScaleTest_5K -f large_scale_test.ini

# MPI-enabled large-scale tests
run-largescale-1k-mpi:
	@echo "Running 1K-host simulation with distributed MPI (4 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 4 ./$(TARGET) -u Cmdenv -c LargeScaleTest_1K_PARSIM -f large_scale_1k_parsim.ini

run-largescale-5k-mpi:
	@echo "Running 5K-host simulation with distributed MPI (8 processes)..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && mpirun -np 8 ./$(TARGET) -u Cmdenv -c LargeScaleTest_5K_PARSIM -f large_scale_5k_parsim.ini

run-parallel-10k: run-parallel-10k-dragonfly

run-parallel: run-parallel-dragonfly-1000

.PHONY: run-dragonfly-1000 run-leafspine-1000 run-mesh-1000 run-torus-1000 run-parallel-dragonfly-1000 run-parallel-leafspine-1000 run-parallel-mesh-1000 run-parallel-torus-1000 run-parallel-10k-leafspine run-parallel-10k-dragonfly run-parallel-10k run-parallel run-largescale-1k run-largescale-5k run-largescale-1k-mpi run-largescale-5k-mpi

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
