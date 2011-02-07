
SOLIB_NAME = janapfm.so

ifndef PFM
PFM=/usr/local/pfm-4
endif

CXXFLAGS += -I$(PFM)/include
LD_DIRS += -L$(PFM)/lib
LD_LIBS += -lpfm

ifndef BMSDIR
BMSDIR := ../../../BMS
endif

include $(BMSDIR)/Makefile.shlib

