

.PHONY: all install clean pristine

PACKAGES := ROOT
SOLIB_NAME = janaroot.so

ifndef BMSDIR
BMSDIR := ../../../BMS
endif
include $(BMSDIR)/Makefile.config

# Only try building this if ROOT was configured by the configure script
ifeq ($(HAVE_ROOT),1)
include $(BMSDIR)/Makefile.shlib
else
install: roottest
clean: roottest
depclean: roottest
endif


roottest:
	@echo 
	@echo "ROOT not configured. Skipping build of janaroot plugin."
	@echo

