CC=gcc
all: do-it-all

AS=nasm
ASFLAGS=-felf32 
CPPFLAGS=-D__QO3__=1
COMMON_CFLAGS=-Wall -g -Wextra -std=c99 -Werror -m32 -Wno-unused-parameter -mssse3
INCLUDES=-I./
CFLAGS=$(COMMON_CFLAGS) -g $(INCLUDES) -Os -fno-strict-aliasing
CXXFLAGS=$(COMMON_CFLAGS)
LDFLAGS=-nostdlib -m32 -Wl,-Ttext,100000 -lgcc -Wl,-Map,QO3.map -s

export LANG=C
DEPEND_INC=-I$(shell gcc --print-search-dirs | awk '/: \// {print $$2}' )include $(INCLUDES)

ifeq ($(wildcard .submake.mk),.submake.mk)
include .submake.mk
do-it-all: all_binaries
else
.PHONY: first
do-it-all: first
endif

-include .depend

first: submake
	make -C . .depend
	make -C . all

dummy:

.PHONY: submake
submake:
	sh readmodule.sh > .submake.mk

.depend: # $(ALL_GEN_SOURCES) 
	touch .depend
	makedepend $(DEPEND_INC) $(CPPFLAGS) $(ALL_SOURCES) $(ALL_GEN_SOURCES) -f.depend
#gcc -MM -I. $(DEPEND_INC) $(CPPFLAGS) $(ALL_SOURCES) $(ALL_GEN_SOURCES) $^ > $@

redep: submake
	rm -f .depend
	make .depend

clean:
	rm -f .depend $(ALL_GEN_FILES) *~ .submake.mk .depend.bak
	find -name '*~' -o -name '*.exe' -o -name '*.so' -o -name '*.map' | xargs rm -f

all_binaries: $(ALL_TARGETS)


dist:
	cd ..; tar -zcf qo3.tar.gz qo3

