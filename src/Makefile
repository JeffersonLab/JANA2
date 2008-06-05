
.PHONY : all clean pristine install uninstall relink


all:
	make -C JANA install
	make -C utilities
	make -C plugins

clean:
	make -C JANA clean
	make -C utilities clean
	make -C plugins clean

pristine:
	make -C JANA pristine
	make -C utilities pristine
	make -C plugins pristine

install:
	make -C JANA install
	make -C utilities install
	make -C plugins install

uninstall:
	make -C JANA uninstall
	make -C utilities uninstall
	make -C plugins uninstall
