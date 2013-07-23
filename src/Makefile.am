AM_CPPFLAGS = 					\
	 -I$(top_builddir)			\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(MILTER_MANAGER_CFLAGS)	\
	$(RUBY_CFLAGS)

LIBS =			\
	$(RUBY_LIBS)

CLEANFILES = *.gcno *.gcda

sbin_PROGRAMS =		\
	milter-manager

milter_manager_SOURCES =		\
	milter-manager.c

milter_manager_LDADD =						\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(top_builddir)/milter/manager/libmilter-manager.la
