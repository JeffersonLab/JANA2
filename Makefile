
.PHONY : install all clean depclean

ifndef BMSDIR
BMSDIR := ./BMS
endif
include $(BMSDIR)/Makefile.config

all clean:
	make -C src $(MAKECMDGOALS)

install: all
	mkdir -p $(prefix)/bin
	mkdir -p $(prefix)/lib
	mkdir -p $(prefix)/include
	install -p $(JANA_BUILD_DIR)/bin/* $(prefix)/bin
	install -p $(JANA_BUILD_DIR)/lib/* $(prefix)/lib
	cp -rp $(JANA_BUILD_DIR)/include $(prefix)
	install -p $(filter-out scripts/Makefile,$(wildcard scripts/*)) $(prefix)/bin
	cp -rp BMS/osrelease.pl BMS/get_macos_arch $(prefix)/bin
	cp -rp BMS/jana-config $(prefix)/bin
	chmod +x $(prefix)/bin/jana-config
	

depclean: clean
	rm -rf bin lib
