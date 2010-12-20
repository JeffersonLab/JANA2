

# Location of makefiles is set by BMSDIR. If this is not set, try
# setting it based on JANA_HOME. If that's not set, use relative path
ifndef BMSDIR
ifndef JANA_HOME
BMSDIR := ../../BMS
else
BMSDIR := $(JANA_HOME)/BMS
endif
endif

# Include generic makefile for building executables. It will compile
# all source in the directory and link it to executables for all .cc
# files defining "main" with the executable name being the basename
# of the .cc file.
include $(BMSDIR)/Makefile.bin

