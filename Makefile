# Makefile
#
# Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

SHELL			= /bin/bash
TARGET			= libsc
DESC			= C terminal display tool
SOV			?= 1
O 			?= .
# Auto load all the .c files dependencies, and object files
LIBSRC			:= ${notdir ${wildcard src/*.c}}

# Tell make/gcc to find the files in VPATH
VPATH 			= src
IPATH 			= src
ifneq ($(CC), emcc)
CFLAGS			= -Wall -Og -gdwarf-2 -g3
endif

-include ../Makefile.common
-include Makefile.common

all			: sc.h
all			: shared

.PHONY: static shared tools tests
static			: $(LIB)/$(TARGET).a
shared			: ${LIB}/$(TARGET).so.$(SOV)

LIBOBJ			:= ${patsubst %, ${OBJ}/%, ${notdir ${LIBSRC:.c=.o}}}

$(LIB)/$(TARGET).a 	: $(LIBOBJ) | $(OBJ)
$(LIB)/$(TARGET).so.$(SOV) : $(LIBOBJ) | $(LIB)/$(TARGET).a

static: $(LIB)/$(TARGET).a
shared: ${LIB}/$(TARGET).so.$(SOV)

sc.h 			: ${LIBSRC} ${wildcard src/*.h}
	@if [ -x ${shell which tcc} ]; then \
		echo Generating $@ from src; \
		tcc -run pph.c >sc.h; \
	else \
		echo TCC not present, skipping; \
	fi ; \
	if [ -d ../include ]; then cp sc.h ../include/ ; fi

clean			::
	rm -f sc_test* sc_cpu*
	rm -f $(LIB)/$(TARGET).*

TESTS_SRC	= ${wildcard tests/*.c}
TESTS_DST	:= ${patsubst %, ${OBJ}/%, ${notdir ${TESTS_SRC:.c=}}}
TESTS_DST_DBG	:= ${patsubst %, ${OBJ}/%, ${notdir ${TESTS_SRC:.c=_d}}}

tests			: | sc.h
tests			: $(TESTS_DST)
debug			: $(TESTS_DST_DBG)

${OBJ}/%	: tests/%.c sc.h | all
	built=$$(date '+%Y-%m-%d');
	gcc -I../include -I. -Wall -static -DBUILT=\"$$built\" -Os -g $< -o $@
${OBJ}/%_d	: tests/%.c sc.h | all
	gcc -I../include -I. -DSC_DEBUG -DBUILT=\"debug\" $(CFLAGS) $< -o $@ \
		-L$(LIB) \
		-Wl,-rpath,${shell readlink -f ${LIB}} \
		-lsc

DESTDIR		?= /usr/local

install		:	sc.h
	install sc.h $(DESTDIR)/include/ && \
	install $(LIB)/$(TARGET){.a,.so.$(SOV)} $(DESTDIR)/lib/

deb:
	echo Nothing to be done
