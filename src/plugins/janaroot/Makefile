

.PHONY: all install clean pristine

PACKAGES := ROOT
SOLIB_NAME = janaroot.so

BMSDIR := ../../BMS
include $(BMSDIR)/Makefile.config

# Only try building this if ROOT was configured by the configure script
ifeq ($(HAVE_ROOT),yes)
include $(BMSDIR)/Makefile.shlib
else
install: roottest
clean:
pristine:
endif


roottest:
	@echo 
	@echo "ROOT not configured. Skipping build of janaroot plugin."
	@echo

