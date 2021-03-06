CC=gcc
OBJCOPY=objcopy
NM=nm
AR=ar

ADDR_BITS=64

all: do-it-all

AS=nasm
ASFLAGS=-felf$(ADDR_BITS) -O3
CPPFLAGS=-D__QO3__=1
COMMON_CFLAGS=-Wall -gstabs -Wextra -std=c99 -m$(ADDR_BITS) -Wno-unused-parameter -mssse3 -Os -fno-stack-check -fno-stack-protector -fno-omit-frame-pointer -mmovbe -m64
INCLUDES=-I./
CFLAGS=$(COMMON_CFLAGS) -g $(INCLUDES) -Os -fno-strict-aliasing
CXXFLAGS=$(COMMON_CFLAGS)
LDFLAGS=-nostdlib -m$(ADDR_BITS) -Wl,-Ttext,100000 -lgcc -Wl,-Map,QO3.map -Wl,-z,max-page-size=4096 # -Wl,-T,kernel/QO3.ld -s

export LANG=C
DEPEND_INC=-I$(shell $(CC) --print-search-dirs | awk '/: \// {print $$2}' )include $(INCLUDES)

ifeq ($(wildcard .submake.mk),.submake.mk)
include .submake.mk
do-it-all: all_binaries
else
.PHONY: first
do-it-all: first
endif

-include .depend

first: submake
	+make -C . .depend
	+make -C . all

dummy:

.PHONY: submake
submake:
	sh readmodule.sh > .submake.mk

gen: $(ALL_GEN_SOURCES) 
	echo $(ALL_GEN_SOURCES) 
	touch gen

.depend: gen
	touch .depend
	@makedepend $(DEPEND_INC) $(CPPFLAGS) $(ALL_SOURCES) $(ALL_GEN_SOURCES) -f.depend
#gcc -MM -I. $(DEPEND_INC) $(CPPFLAGS) $(ALL_SOURCES) $(ALL_GEN_SOURCES) $^ > $@

redep: submake
	rm -f .depend
	+make .depend

clean:
	rm -f .depend $(ALL_GEN_FILES) *~ .submake.mk .depend.bak
	find -name '*~' -o -name '*.exe' -o -name '*.so' -o -name '*.map' | xargs rm -f

all_binaries: $(ALL_TARGETS)


dist:
	cd ..; tar -zcf qo3.tar.gz qo3

