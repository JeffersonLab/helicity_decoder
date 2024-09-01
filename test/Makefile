#
# File:
#    Makefile
#
# Description:
#    Makefile for helicity decoder library test programs
#
#
DEBUG	?= 1
QUIET	?= 1
#
ifeq ($(QUIET),1)
        Q = @
else
        Q =
endif

ifdef CODA_VME
CODA_VME_INC = -I${CODA_VME}/include
endif
ifdef CODA_VME_LIB
CODA_LIB = -L${CODA_VME_LIB}
endif

# linuxvme defaults, if they're not already defined
LINUXVME_INC	?= .
LINUXVME_LIB	?= .

CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc
AR                      = ar
RANLIB                  = ranlib
INCS			= -I. -I../ -I${LINUXVME_INC} ${CODA_VME_INC}
CFLAGS			= -L. -L../ -L${LINUXVME_LIB} ${CODA_LIB} -lrt -ljvme -lhd
ifeq ($(DEBUG),1)
	CFLAGS		+= -Wall -Wno-unused -g
endif

SRC			= $(wildcard *.c)
DEPS			= $(SRC:.c=.d)
PROGS			= $(SRC:.c=)

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) *~ $(OBJS) $(DEPS)

%: %.c
	@echo " CC     $@"
	${Q}$(CC) $(CFLAGS) $(INCS) -o $@ $<

hdTestVXS: hdTestVXS.c
	@echo " CC     $@"
	${Q}$(CC) $(CFLAGS)  -lti -lsd -lts -lstdc++ -lreadline $(INCS) -o $@ $<

hdReadoutTest: hdReadoutTest.c
	@echo " CC     $@"
	${Q}$(CC) $(CFLAGS)  -lti -lsd -lts -lstdc++ -lgsl -lgslTimer $(INCS) -o $@ $<

%.d: %.c
	@echo " DEP    $@"
	@set -e; rm -f $@; \
	$(CC) -MM -shared $(INCS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS)

.PHONY: all clean distclean
