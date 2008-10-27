if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-child.la			\
	test-children.la		\
	test-configuration.la		\
	test-controller.la		\
	test-spawn.la
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

test_child_la_SOURCES		= test-child.c
test_children_la_SOURCES	= test-children.c
test_configuration_la_SOURCES	= test-configuration.c
test_controller_la_SOURCES	= test-controller.c
test_spawn_la_SOURCES		= test-spawn.c
