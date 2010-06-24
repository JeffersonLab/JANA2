

DIRS := janadot janactl janaroot janarate jana_iotest TestSpeed

.PHONY: $(DIRS)

all install clean pristine relink: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)



