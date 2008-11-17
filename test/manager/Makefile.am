if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-child.la			\
	test-children.la		\
	test-configuration.la		\
	test-leader.la			\
	test-egg.la			\
	test-headers.la			\
	test-control-decoder.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(top_srcdir)/test/lib	\
	-I$(top_srcdir)

AM_CFLAGS = 				\
	$(MILTER_MANAGER_CFLAGS)	\
	$(GCUTTER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
		-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter/manager/libmilter-manager.la	\
	$(top_builddir)/test/lib/libmilter-manager-test.la	\
	$(GCUTTER_LIBS)

test_child_la_SOURCES			= test-child.c
test_children_la_SOURCES		= test-children.c
test_configuration_la_SOURCES		= test-configuration.c
test_leader_la_SOURCES			= test-leader.c
test_egg_la_SOURCES			= test-egg.c
test_headers_la_SOURCES			= test-headers.c
test_control_decoder_la_SOURCES		= test-control-decoder.c
