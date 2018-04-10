rtems_tests_PROGRAMS = smppspm01
smppspm01_SOURCES = init.c pspm.h pspmimpl.c main.c app.h app.c system.h ../../support/src/locked_print.c

dist_rtems_tests_DATA = smppspm01.scn
dist_rtems_tests_DATA += smppspm01.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am

AM_CPPFLAGS += -I$(top_srcdir)/../support/include
AM_CPPFLAGS += -DSMPTEST 

LINK_OBJS = $(smppspm01_OBJECTS)
LINK_LIBS = $(smppspm01_LDLIBS)

smppspm01$(EXEEXT): $(smppspm01_OBJECTS) $(smppspm01_DEPENDENCIES)
	@rm -f smppspm01$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
