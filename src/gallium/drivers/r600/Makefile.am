include Makefile.sources

noinst_LIBRARIES = libr600.a

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	$(RADEON_CFLAGS) \
	$(DEFINES) \
	$(ARCH_FLAGS) \
	$(ASM_FLAGS) \
	$(OPT_FLAGS) \
	$(PIC_FLAGS) \
	$(VISIBILITY_CFLAGS)

libr600_a_SOURCES = \
	$(C_SOURCES)

if USE_R600_LLVM_COMPILER

# This is a hack until we can move the backend into the LLVM project.
# We need to use mklib, because it splits up libradeon.a into object files
# so that we can link it with the r600 objects.
libr600_a_AR = $(top_srcdir)/bin/mklib -o r600 -static

libr600_a_SOURCES += \
	$(LLVM_C_SOURCES)

libr600_a_LIBADD = \
	$(top_srcdir)/src/gallium/drivers/radeon/libradeon.a

AM_CFLAGS += \
	$(LLVM_CFLAGS) \
	-I$(top_srcdir)/src/gallium/drivers/radeon/ \
	-DR600_USE_LLVM

AM_CXXFLAGS= \
	$(LLVM_CXXFLAGS)
else
libr600_a_AR = $(AR) $(ARFLAGS)
endif
