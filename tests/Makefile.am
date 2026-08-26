EXTRA_DIST =

EXTRA_INCLUDES =
EXTRA_LIBS =
EXTRA_FLAGS =

INCLUDES = \
  -I$(top_srcdir)/include \
  $(EXTRA_INCLUDES)

bin_PROGRAMS = bulktest bulktest1 bulktest2

bulktest_SOURCES = bulktest.c

bulktest1_SOURCES = bulktest1.c

bulktest2_SOURCES = bulktest2.c

bulktest_LDADD = \
  $(top_builddir)/src/libbulkrdp8compress.la $(EXTRA_LIBS) \
  $(top_builddir)/src/libbulkrdp8decompress.la $(EXTRA_LIBS)

bulktest1_LDADD = \
  $(top_builddir)/src/libbulkrdp8compress.la $(EXTRA_LIBS) \
  $(top_builddir)/src/libbulkrdp8decompress.la $(EXTRA_LIBS)

bulktest2_LDADD = \
  $(top_builddir)/src/libbulkrdp8compress.la $(EXTRA_LIBS) \
  $(top_builddir)/src/libbulkrdp8decompress.la $(EXTRA_LIBS)

bulktest_LDFLAGS = $(EXTRA_FLAGS)

bulktest1_LDFLAGS = $(EXTRA_FLAGS)

bulktest2_LDFLAGS = $(EXTRA_FLAGS)
