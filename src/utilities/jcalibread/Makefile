
CXXFLAGS += -I$(JANA_HOME)/include
LD_LIBS += -lJANA -ldl
LD_DIRS += -L$(JANA_HOME)/lib/$(OSNAME)

ifdef XERCESCROOT
PACKAGES += XERCES
endif

include $(JANA_HOME)/src/BMS/Makefile.bin

