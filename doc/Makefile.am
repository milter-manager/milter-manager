SUBDIRS =		\
	reference

EXTRA_DIST =						\
	configuration.rd				\
	configuration.rd.ja				\
	performance.rd					\
	performance.rd.ja				\
	introduction.rd					\
	introduction.rd.ja				\
	install.rd					\
	install.rd.ja					\
	install-to-debian.rd				\
	install-to-debian.rd.ja				\
	install-to-ubuntu.rd				\
	install-to-ubuntu.rd.ja				\
	install-to-centos5.rd				\
	install-to-centos5.rd.ja			\
	install-to-centos.rd				\
	install-to-centos.rd.ja				\
	install-to-freebsd.rd				\
	install-to-freebsd.rd.ja			\
	install-to-others.rd				\
	install-to-others.rd.ja				\
	install-options-to-debian.rd			\
	install-options-to-debian.rd.ja			\
	install-options-to-ubuntu.rd			\
	install-options-to-ubuntu.rd.ja			\
	install-options-to-centos.rd			\
	install-options-to-centos.rd.ja			\
	install-options-to-freebsd.rd			\
	install-options-to-freebsd.rd.ja		\
	upgrade-on-debian.rd				\
	upgrade-on-debian.rd.ja				\
	upgrade-on-ubuntu.rd				\
	upgrade-on-ubuntu.rd.ja				\
	upgrade-on-centos.rd				\
	upgrade-on-centos.rd.ja				\
	upgrade-on-freebsd.rd				\
	upgrade-on-freebsd.rd.ja			\
	upgrade-options-on-debian.rd			\
	upgrade-options-on-debian.rd.ja			\
	upgrade-options-on-ubuntu.rd			\
	upgrade-options-on-ubuntu.rd.ja			\
	upgrade-options-on-centos.rd			\
	upgrade-options-on-centos.rd.ja			\
	upgrade-options-on-freebsd.rd			\
	upgrade-options-on-freebsd.rd.ja		\
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
	milter-test-server.rd.ja			\
	milter-test-client.rd				\
	milter-test-client.rd.ja			\
	milter-performance-check.rd			\
	milter-performance-check.rd.ja			\
	milter-report-statistics.rd			\
	milter-report-statistics.rd.ja			\
	milter-manager-log-analyzer.rd			\
	milter-manager-log-analyzer.rd.ja		\
	milter-manager.rd				\
	milter-manager.rd.ja				\
	reverse-dictionary.rd				\
	reverse-dictionary.rd.ja			\
	users.rd					\
	users.rd.ja					\
	ruby-milter-tutorial.rd				\
	ruby-milter-tutorial.rd.ja			\
	ruby-milter-reference.rd			\
	ruby-milter-reference.rd.ja			\
	ruby-milter-configuration.rd			\
	ruby-milter-configuration.rd.ja			\
	available-milters.rd				\
	available-milters.rd.ja				\
	log-list.rd					\
	log-list.rd.ja

dist_man1_MANS =			\
	milter-manager.man		\
	milter-test-server.man		\
	milter-test-client.man		\
	milter-performance-check.man	\
	milter-report-statistics.man	\
	milter-manager-log-analyzer.man

ja_man1dir = $(mandir)/ja/man1
dist_ja_man1_mans = 				\
	milter-manager.jman			\
	milter-test-server.jman			\
	milter-test-client.jman			\
	milter-performance-check.jman		\
	milter-report-statistics.jman		\
	milter-manager-log-analyzer.jman

EXTRA_DIST += $(dist_ja_man1_mans)

if WITH_RD2
install-data-local: $(dist_ja_man1_mans)
else
install-data-local:
endif
	test -z "$(DESTDIR)$(ja_man1dir)" || $(mkdir_p) "$(DESTDIR)$(ja_man1dir)"
	@for jman in $(dist_ja_man1_mans); do			\
	  test -f "$${jman}" || break;				\
	  jman_path=$(top_builddir)/doc/$${jman};		\
	  jman_file=`echo $${jman} | sed -e s/jman$$/1/`;	\
	  echo $(INSTALL_DATA) $${jman_path}			\
	    "$(DESTDIR)$(ja_man1dir)"/$${jman_file};		\
	  $(INSTALL_DATA) $${jman_path}				\
	    "$(DESTDIR)$(ja_man1dir)"/$${jman_file};		\
	done

uninstall-local:
	@for jman in $(dist_ja_man1_mans); do			\
	  jman_file=`echo $${jman} | sed -e s/jman$$/1/`;	\
	  rm -f "$(DESTDIR)$(ja_man1dir)"/$${jman_file};	\
	done

# dirty...
milter-manager.jman: milter-manager.rd.ja
milter-test-server.jman: milter-test-server.rd.ja
milter-test-client.jman: milter-test-client.rd.ja
milter-performance-check.jman: milter-performance-check.rd.ja
milter-report-statistics.jman: milter-report-statistics.rd.ja
milter-manager-log-analyzer.jman: milter-manager-log-analyzer.rd.ja

MAINTAINERCLEANFILES = $(dist_man1_MANS) $(dist_ja_man1_mans)

SUFFIXES = .rd .man .rd .jman .svg .png

RD2RUBY = @RD2RUBY@

.rd.man:
	$(RD2RUBY) -I $(CUTTER_SOURCE_PATH)/misc $(RD2) -r rd/rd2man-lib $< > $@

.rd.jman: # dirty...
	$(RD2RUBY) -I $(CUTTER_SOURCE_PATH)/misc $(RD2) -r rd/rd2man-lib $<.ja > $@

.svg.png:
	inkscape --export-png $@ $<
