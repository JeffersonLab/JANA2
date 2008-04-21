
# Make sure JANA_HOME is defined
ifdef JANA_HOME


LD_LIBS += -lJANA -ldl
LD_DIRS += -L$(JANA_HOME)/lib/$(OSNAME)

include $(JANA_HOME)/src/BMS/Makefile.bin

else
errmess:
	@echo "You do not seem to have your JANA_HOME environment variable set"
	@echo "Please set it to the directory that contains src/BMS."
endif

