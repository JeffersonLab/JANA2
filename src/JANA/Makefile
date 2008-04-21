
# Make sure JANA_HOME is defined
ifdef JANA_HOME


# Option for building shared libraries
ifdef JANA_SHARED
include $(JANA_HOME)/src/BMS/Makefile.shlib
else
include $(JANA_HOME)/src/BMS/Makefile.lib
endif

else
errmess:
	@echo "You do not seem to have your JANA_HOME environment variable set"
	@echo "Please set it to the directory that contains src/BMS."
endif

