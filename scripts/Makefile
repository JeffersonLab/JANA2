

ifndef BMSDIR
BMSDIR := ../BMS
endif

include $(BMSDIR)/Makefile.common

SCRIPTS := $(filter-out Makefile, $(wildcard *))

install:
	@echo install -p $(SCRIPTS) $(INSTALL_DIR)/bin

