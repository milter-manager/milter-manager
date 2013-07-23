AM_CPPFLAGS =						\
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/binding/ruby/ext/core		\
	-I$(top_srcdir)/binding/ruby/ext/client		\
	-I$(top_srcdir)/binding/ruby/ext/server		\
	-I$(top_srcdir)/binding/ruby/ext/manager

AM_CFLAGS = 								\
	-DMILTER_LOG_DOMAIN=\""milter-manager-ruby-configuration"\"	\
	$(MILTER_MANAGER_CFLAGS)					\
	$(MILTER_RUBY_CFLAGS)
#	$(COVERAGE_CFLAGS)

LIBS =					\
	$(RUBY_LIBS)

AM_LDFLAGS =				\
	-module				\
	-avoid-version

CLEANFILES = *.gcno *.gcda

module_configuration_LTLIBRARIES =		\
	milter-manager-ruby-configuration.la

milter_manager_ruby_configuration_la_SOURCES =	\
	milter-manager-ruby-configuration.c

milter_manager_ruby_configuration_la_LIBADD =			\
	$(top_builddir)/milter/manager/libmilter-manager.la
