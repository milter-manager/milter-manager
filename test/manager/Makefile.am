if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-child.la			\
	test-children.la		\
	test-configuration.la		\
	test-leader.la			\
	test-egg.la			\
	test-control-command-decoder.la	\
	test-control-reply-decoder.la	\
	test-control-command-encoder.la	\
	test-control-reply-encoder.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(top_srcdir)/test/lib	\
	-I$(top_srcdir)

AM_CFLAGS =								\
	-DCUSTOM_CONFIG_FILE_NAME="\"$(CUSTOM_CONFIG_FILE_NAME)\""	\
	$(MILTER_MANAGER_CFLAGS)					\
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
test_control_command_decoder_la_SOURCES	= test-control-command-decoder.c
test_control_reply_decoder_la_SOURCES	= test-control-reply-decoder.c
test_control_command_encoder_la_SOURCES	= test-control-command-encoder.c
test_control_reply_encoder_la_SOURCES	= test-control-reply-encoder.c
