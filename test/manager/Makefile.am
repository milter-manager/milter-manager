if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-manager.la				\
	test-child.la				\
	test-children.la			\
	test-configuration.la			\
	test-leader.la				\
	test-egg.la				\
	test-control-command-decoder.la		\
	test-control-reply-decoder.la		\
	test-control-command-encoder.la		\
	test-control-reply-encoder.la		\
	test-reply-encoder.la			\
	test-reply-decoder.la			\
	test-launch-command-encoder.la		\
	test-launch-command-decoder.la		\
	test-controller-context.la		\
	test-controller.la			\
	test-applicable-condition.la		\
	test-process-launcher.la
endif

AM_CPPFLAGS =				\
	-I../lib			\
	-I$(srcdir)			\
	-I$(top_srcdir)/test/lib	\
	-I$(top_srcdir)

AM_CFLAGS =								\
	-DCUSTOM_CONFIG_FILE_NAME="\"$(CUSTOM_CONFIG_FILE_NAME)\""	\
	-DPREFIX="\"$(prefix)\""					\
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
	$(top_builddir)/test/lib/libmilter-test.la		\
	$(GCUTTER_LIBS)

test_manager_la_SOURCES			= test-manager.c
test_child_la_SOURCES			= test-child.c
test_children_la_SOURCES		= test-children.c
test_configuration_la_SOURCES		= test-configuration.c
test_leader_la_SOURCES			= test-leader.c
test_egg_la_SOURCES			= test-egg.c
test_control_command_decoder_la_SOURCES	= test-control-command-decoder.c
test_control_reply_decoder_la_SOURCES	= test-control-reply-decoder.c
test_control_command_encoder_la_SOURCES	= test-control-command-encoder.c
test_control_reply_encoder_la_SOURCES	= test-control-reply-encoder.c
test_reply_encoder_la_SOURCES		= test-reply-encoder.c
test_reply_decoder_la_SOURCES		= test-reply-decoder.c
test_controller_context_la_SOURCES	= test-controller-context.c
test_controller_la_SOURCES		= test-controller.c
test_applicable_condition_la_SOURCES	= test-applicable-condition.c
test_launch_command_encoder_la_SOURCES	= test-launch-command-encoder.c
test_launch_command_decoder_la_SOURCES	= test-launch-command-decoder.c
test_process_launcher_la_SOURCES	= test-process-launcher.c
