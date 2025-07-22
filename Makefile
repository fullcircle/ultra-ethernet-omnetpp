#
# OMNeT++/OMNEST Makefile for ultraethernet_sim
#
# This file was generated with the command:
#  opp_makemake -f --deep -o ultraethernet_sim -I/mnt/d/omnetpp-6.2.0/inet4.5/src -L/mnt/d/omnetpp-6.2.0/inet4.5/src -lINET_dbg
#

# Name of target to be created (-o option)
TARGET_DIR = .
TARGET_NAME = ultraethernet_sim$(D)
TARGET = $(TARGET_NAME)$(EXE_SUFFIX)
TARGET_FILES = $(TARGET_DIR)/$(TARGET)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(QTENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(QTENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = -I/mnt/d/omnetpp-6.2.0/inet4.5/src

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)/mnt/d/omnetpp-6.2.0/inet4.5/src  -lINET_dbg

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
ifneq ($(PLATFORM),win32)
LIBS += -Wl,-rpath,$(abspath /mnt/d/omnetpp-6.2.0/inet4.5/src)
endif

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

# Additional targets for scalable testing
run-multi: 
	@echo "Running working multi-host tests..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c TwoHost_Direct -f working_multi_test.ini
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c MultiHost_4 -f working_multi_test.ini
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c MultiHost_16 -f working_multi_test.ini

run-working:
	@echo "Running verified working test..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c General -f test_full.ini --sim-time-limit=10s

run-stress:
	@echo "Running stress test..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c MultiHost_Stress -f working_multi_test.ini

run-64:
	@echo "Running 64-host large scale test..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c MultiHost_64 -f large_scale_test.ini

run-100:
	@echo "Running 100-host extreme scale test..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c MultiHost_100 -f large_scale_test.ini

run-massive:
	@echo "Running 256-host massive scale test..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c MultiHost_256 -f large_scale_test.ini

# Topology-specific 1000-host tests
run-leafspine-1000:
	@echo "Running 1000-host Leaf-Spine topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c LeafSpine_1000 -f large_scale_test.ini

run-dragonfly-1000:
	@echo "Running 1000-host Dragonfly topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c Dragonfly_1000 -f large_scale_test.ini

run-mesh-1000:
	@echo "Running 1000-host Mesh topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c Mesh_1000 -f large_scale_test.ini

run-torus-1000:
	@echo "Running 1000-host Torus topology..."
	export PATH=$$PATH:/mnt/d/omnetpp-6.2.0/bin && ./ultraethernet_sim_dbg -u Cmdenv -c Torus_1000 -f large_scale_test.ini

run-all-topologies-1000:
	@echo "Running all 1000-host topology tests..."
	$(MAKE) run-leafspine-1000
	$(MAKE) run-dragonfly-1000
	$(MAKE) run-mesh-1000
	$(MAKE) run-torus-1000

help:
	@echo "$$HELP_SYNOPSYS"
	@echo "$$HELP_TARGETS"
	@echo "Additional targets:"
	@echo "  run-multi         Run working multi-host tests (2, 4, 16 hosts)"
	@echo "  run-working       Run verified single host test"
	@echo "  run-stress        Run stress test with high traffic"
	@echo "  run-64            Run 64-host large scale test"
	@echo "  run-100           Run 100-host extreme scale test"
	@echo "  run-massive       Run 256-host massive scale test"
	@echo ""
	@echo "Topology-specific 1000-host tests:"
	@echo "  run-leafspine-1000    Run 1000-host Leaf-Spine topology"
	@echo "  run-dragonfly-1000    Run 1000-host Dragonfly topology"
	@echo "  run-mesh-1000         Run 1000-host Mesh topology"
	@echo "  run-torus-1000        Run 1000-host Torus topology"
	@echo "  run-all-topologies-1000  Run all 1000-host topology tests"
	@echo "$$HELP_VARIABLES"
	@echo "$$HELP_EXAMPLES"

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
