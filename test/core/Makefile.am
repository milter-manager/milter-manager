if WITH_CUTTER
noinst_LTLIBRARIES =		\
	test-decoder.la		\
	test-encoder.la		\
	test-option.la		\
	test-reader.la		\
	test-writer.la		\
	test-utils.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/test/lib

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =							\
	$(top_builddir)/milter/core/libmilter-core.la	\
	$(top_builddir)/test/lib/libmilter-test.la	\
	$(GCUTTER_LIBS)

test_decoder_la_SOURCES = test-decoder.c
test_encoder_la_SOURCES = test-encoder.c
test_option_la_SOURCES = test-option.c
test_reader_la_SOURCES = test-reader.c
test_writer_la_SOURCES = test-writer.c
test_utils_la_SOURCES = test-utils.c
