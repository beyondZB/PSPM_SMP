##
##

ACLOCAL_AMFLAGS = -I ../../../../aclocal
EXTRA_DIST =

include $(top_srcdir)/../../../../automake/compile.am
include $(top_srcdir)/../../bsp.am

dist_project_lib_DATA = startup/bsp_specs

#isr
#pci
#irq
DISTCLEANFILES = include/bspopts.h


EXTRA_DIST += start/start.S ../shared/liblnk/regs.h
start.$(OBJEXT): start/start.S
	$(CPPASCOMPILE) -o $@ -c $<
project_lib_DATA = start.$(OBJEXT)

project_lib_DATA += linkcmds

noinst_LIBRARIES = libbsp.a
libbsp_a_SOURCES =

# startup
libbsp_a_SOURCES += startup/simple_access.c
libbsp_a_SOURCES += ../../shared/bspclean.c
libbsp_a_SOURCES += ../../shared/bsppredriverhook.c
libbsp_a_SOURCES += ../../shared/bspgetworkarea.c
libbsp_a_SOURCES += startup/bspstart.c
libbsp_a_SOURCES += startup/bspreset.c
libbsp_a_SOURCES += ../../shared/bootcard.c
libbsp_a_SOURCES += ../../shared/getentropy-cpucounter.c
libbsp_a_SOURCES += ../../shared/sbrk.c
libbsp_a_SOURCES += startup/inittlb.c
libbsp_a_SOURCES += ../shared/startup/idttlb.S

# clock
libbsp_a_SOURCES += ../shared/clock/clockdrv.c
libbsp_a_SOURCES += ../shared/clock/mips_timer.S
libbsp_a_SOURCES += ../../shared/clockdrv_shell.h

# console
libbsp_a_SOURCES += console/conscfg.c
libbsp_a_SOURCES += console/printk_support.c
libbsp_a_SOURCES += ../../shared/console.c
libbsp_a_SOURCES += ../../shared/console_select.c
libbsp_a_SOURCES += ../../shared/console_read.c
libbsp_a_SOURCES += ../../shared/console_write.c
libbsp_a_SOURCES += ../../shared/console_control.c
# timer
libbsp_a_SOURCES += ../../shared/timerstub.c

libbsp_a_SOURCES += ../../../../../../bsps/shared/cache/nocache.c
libbsp_a_SOURCES += ../../../../../../bsps/mips/shared/irq/installisrentries.c
libbsp_a_SOURCES += ../../../../../../bsps/mips/shared/irq/isr_entries.S

# pci
libbsp_a_SOURCES += pci/pci.c
libbsp_a_SOURCES += ../../shared/pci/pci_find_device.c

#isr
libbsp_a_SOURCES += ../../shared/src/irq-generic.c
libbsp_a_SOURCES += ../../shared/src/irq-legacy.c
libbsp_a_SOURCES += ../../shared/src/irq-info.c
libbsp_a_SOURCES += ../../shared/src/irq-shell.c
libbsp_a_SOURCES += ../../shared/src/irq-server.c
libbsp_a_SOURCES += ../shared/irq/vectorexceptions.c
libbsp_a_SOURCES += ../shared/irq/irq.c
libbsp_a_SOURCES += irq/vectorisrs.c
libbsp_a_SOURCES += irq/interruptmask.c
libbsp_a_SOURCES += ../shared/irq/i8259.c

include $(top_srcdir)/../../../../automake/local.am
include $(srcdir)/../../../../../../bsps/mips/malta/headers.am
