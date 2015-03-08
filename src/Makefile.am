EXTRA_DIST =

EXTRA_BULK_MPPC_DEFINES =
EXTRA_BULK_MPPC_INCLUDES =
EXTRA_BULK_MPPC_LIBS =
EXTRA_BULK_MPPC_FLAGS =
EXTRA_BULK_MPPC_COMPRESS_SRC =

EXTRA_BULK_RDP8_DEFINES =
EXTRA_BULK_RDP8_INCLUDES =
EXTRA_BULK_RDP8_LIBS =
EXTRA_BULK_RDP8_FLAGS =
EXTRA_BULK_RDP8_COMPRESS_SRC =

AM_CFLAGS = \
  -I../include \
  $(EXTRA_BULK_MPPC_FLAGS)

lib_LTLIBRARIES = libbulkmppccompress.la libbulkrdp8decompress.la

# MPPC
libbulkmppccompress_la_LDFLAGS =

libbulkmppccompress_ladir = $(moduledir)

libbulkmppccompress_la_SOURCES = bulk_mppc_compress.c \
$(EXTRA_BULK_MPPC_SRC)

libbulkmppccompress_la_LIBADD =

# RDP8
libbulkrdp8decompress_la_LDFLAGS =

libbulkrdp8decompress_ladir = $(moduledir)

libbulkrdp8decompress_la_SOURCES = bulk_rdp8_decompress.c \
$(EXTRA_BULK_RDP8_SRC)

libbulkrdp8decompress_la_LIBADD =

