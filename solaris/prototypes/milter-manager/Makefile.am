if SOLARIS_PLATFORM
pkginfodir =
pkginfo_DATA = pkginfo
postinstalldir =
postinstall_DATA = postinstall
postinstall: $(srcdir)/postinstall.in
	@echo creating $@
	@sed -e 's|@''localstatedir''@|${localstatedir}|g' \
	  $? > $@
endif
