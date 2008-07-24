
#ROOTCFLAGS   := $(shell root-config --cflags)
#ROOTLIBS     := $(shell root-config --glibs)
#ROOTGLIBS    := $(shell root-config --glibs)
#ROOTINC      := $(shell root-config --incdir)

CFLAGS		+= -D_ROOT_ $(ROOTCFLAGS) -I$(ROOTINC)
CXXFLAGS		+= -D_ROOT_ $(ROOTCFLAGS) -I$(ROOTINC)
MISC_LIBS 	+= -L$(ROOTLIBDIR) $(ROOTGLIBS) -lThread -lMinuit

# Create ROOT dictionaries from C++ files containing ClassDef
HSRC = $(wildcard *.h)
ifneq ($(strip $(HSRC)),)
  DICT_IN = $(shell grep -l "ClassDef" $(HSRC))
  ifneq ($(strip $(DICT_IN)),)
    DICT_SRC = $(addsuffix _Dict.cc,$(basename $(DICT_IN)))
    DICT_FILES = $(DICT_SRC) $(DICT_SRC:.cc=.h)
  endif
endif

# Add the dictionaries to the list of objects to make, but
# that are NOT added t the DEPS list. Otherwise, the dictionary
# files will always be made
NO_DEP_OBJS  += $(sort $(addsuffix .o,$(basename $(DICT_SRC))))


# Rule to make ROOT dictionary
%_Dict.cc: %.h
	rootcint -f $@ -c $(INCS) $< 
