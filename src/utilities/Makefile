
DIRS := jana janadump jcalibread jgeomread jcalibcopy janactl

.PHONY: $(DIRS)

all install clean pristine relink: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)


