
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
	install -p BMS/osrelease.pl BMS/get_macos_arch $(prefix)/bin
	install -p BMS/jana-config $(prefix)/bin
	install -p BMS/setenv.csh $(prefix)
	install -p BMS/env.sh $(prefix)
	

depclean: clean
	rm -rf bin lib
