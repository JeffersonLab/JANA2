

.PHONY: all install clean pristine

PACKAGES := CERNLIB
SOLIB_NAME = janahbook.so

BMSDIR := ../../BMS
include $(BMSDIR)/Makefile.config

# Only try building this if ROOT was configured by the configure script
ifeq ($(HAVE_CERNLIB),yes)
include $(BMSDIR)/Makefile.bin
else
install: cernlibtest
clean:
pristine:
endif


cernlibtest:
	@echo 
	@echo "CERNLIB not configured. Skipping build of janahbook plugin."
	@echo

