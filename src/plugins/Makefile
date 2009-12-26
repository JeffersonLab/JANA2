

DIRS := janadot janactl janaroot TestSpeed

.PHONY: $(DIRS)

all install clean pristine relink: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)



