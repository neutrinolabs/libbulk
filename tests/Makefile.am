EXTRA_DIST =

EXTRA_INCLUDES =
EXTRA_LIBS =
EXTRA_FLAGS =

if GOT_PREFIX
EXTRA_INCLUDES += -I$(prefix)/include
EXTRA_FLAGS += -L$(prefix)/lib -Wl,-rpath -Wl,$(prefix)/lib
endif

INCLUDES = \
  -I$(top_srcdir)/include \
  $(EXTRA_INCLUDES)

bin_PROGRAMS = bulktest

bulktest_SOURCES = bulktest.c

bulktest_LDADD = \
  $(top_builddir)/src/libbulkrdp8compress.la $(EXTRA_LIBS) \
  $(top_builddir)/src/libbulkrdp8decompress.la $(EXTRA_LIBS)

bulktest_LDFLAGS = $(EXTRA_FLAGS)

