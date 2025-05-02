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
PROGS			= $(SRC:.c=)
DEPDIR 			:= .deps
DEPFLAGS 		= -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
DEPFILES 		:= $(SRC:%.c=$(DEPDIR)/%.d)

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) *~ $(OBJS) $(DEPS)

%: %.c
%: %.c $(DEPDIR)/%.d | $(DEPDIR)
	@echo " CC     $@"
	${Q}$(CC) $(DEPFLAGS) $(CFLAGS) $(INCS) -o $@ $<

hdReadoutTest: hdReadoutTest.c $(DEPDIR)/hdReadoutTest.d | $(DEPDIR)
	@echo " CC     $@"
	${Q}$(CC) -MT $@ -MMD -MP -MF $(DEPDIR)/hdReadoutTest.d $(CFLAGS)  -lti -lsd -lts -lstdc++ $(INCS) -o $@ $<

hdTestVXS: hdTestVXS.c $(DEPDIR)/hdTestVXS.d | $(DEPDIR)
	@echo " CC     $@"
	${Q}$(CC) -MT $@ -MMD -MP -MF $(DEPDIR)/hdTestVXS.d $(CFLAGS)  -lti -lsd -lts -lstdc++ -lreadline $(INCS) -o $@ $<

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):
include $(wildcard $(DEPFILES))

.PHONY: all clean distclean
