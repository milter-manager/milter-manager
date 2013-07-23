if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-decoder.la			\
	test-command-decoder.la		\
	test-reply-decoder.la		\
	test-encoder.la			\
	test-command-encoder.la		\
	test-reply-encoder.la		\
	test-error-emittable.la		\
	test-finished-emittable.la	\
	test-option.la			\
	test-reader.la			\
	test-macros-requests.la		\
	test-writer.la			\
	test-utils.la			\
	test-logger.la			\
	test-syslog-logger.la		\
	test-connection.la		\
	test-headers.la			\
	test-esmtp.la			\
	test-protocol.la		\
	test-message-result.la		\
	test-session-result.la
endif

AM_CPPFLAGS =				\
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

test_decoder_la_SOURCES			= test-decoder.c
test_command_decoder_la_SOURCES		= test-command-decoder.c
test_reply_decoder_la_SOURCES		= test-reply-decoder.c
test_encoder_la_SOURCES			= test-encoder.c
test_command_encoder_la_SOURCES		= test-command-encoder.c
test_reply_encoder_la_SOURCES		= test-reply-encoder.c
test_error_emittable_la_SOURCES		= test-error-emittable.c
test_finished_emittable_la_SOURCES	= test-finished-emittable.c
test_option_la_SOURCES			= test-option.c
test_reader_la_SOURCES			= test-reader.c
test_macros_requests_la_SOURCES		= test-macros-requests.c
test_writer_la_SOURCES			= test-writer.c
test_utils_la_SOURCES			= test-utils.c
test_logger_la_SOURCES			= test-logger.c
test_connection_la_SOURCES		= test-connection.c
test_headers_la_SOURCES			= test-headers.c
test_syslog_logger_la_SOURCES		= test-syslog-logger.c
test_esmtp_la_SOURCES			= test-esmtp.c
test_protocol_la_SOURCES		= test-protocol.c
test_message_result_la_SOURCES		= test-message-result.c
test_session_result_la_SOURCES		= test-session-result.c
