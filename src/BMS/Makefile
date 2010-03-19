

ifndef BMSDIR
BMSDIR = .
endif

# The BMSDIR variable needs to be set prior to including this file
include Makefile.common

install: all
	mkdir -p $(INSTALL_DIR)/BMS
	install -p $(wildcard $(BMSDIR)/*) $(INSTALL_DIR)/BMS


