SUBDIRS =		\
	reference

EXTRA_DIST =						\
	configuration.rd				\
	configuration.rd.ja				\
	introduction.rd					\
	introduction.rd.ja				\
	install.rd					\
	install.rd.ja					\
	install-to-ubuntu.rd				\
	install-to-ubuntu.rd.ja				\
	install-to-centos.rd				\
	install-to-centos.rd.ja				\
	install-to-freebsd.rd				\
	install-to-freebsd.rd.ja			\
	install-to-others.rd				\
	install-to-others.rd.ja				\
	install-options-to-ubuntu.rd			\
	install-options-to-ubuntu.rd.ja			\
	install-options-to-centos.rd			\
	install-options-to-centos.rd.ja			\
	install-options-to-freebsd.rd			\
	install-options-to-freebsd.rd.ja		\
	admin-applicable-condition-ja.png		\
	admin-applicable-condition.png			\
	admin-milters-ja.png				\
	admin-milters.png				\
	milter-manager-admin-ja.svg			\
	milter-manager-admin.svg			\
	mta-and-milter-manager-and-child-milter-ja.svg	\
	mta-and-milter-manager-and-child-milter.svg	\
	sendmail-and-milter-ja.svg			\
	sendmail-and-milter.svg				\
	sendmail-and-postfix-and-milter-ja.svg		\
	sendmail-and-postfix-and-milter.svg		\
	mail.day.png					\
	session.day.png					\
	stop.day.png					\
	milter-manager-log-analyzer-graphs.svg		\
	poster-for-explanation-ja.svg			\
	milter-test-server.rd				\
	$(man_pages)

man_pages = milter-test-server.1

MAINTAINERCLEANFILES = $(man_pages)

SUFFIXES: .rd .1

.rd.1:
	rd2 -r$(top_srcdir)/misc/rd2man-lib $< > $@
