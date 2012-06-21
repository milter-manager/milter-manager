smf_method_script = milter-manager

EXTRA_DIST =			\
	$(smf_method_script).in

if SOLARIS_PLATFORM
smf_methoddir = $(prefix)/lib/svc/method
smf_method_SCRIPTS = $(smf_method_script)
$(smf_method_script): Makefile $(srcdir)/$(smf_method_script).in
	@echo creating $@
	@sed -e 's|@''localstatedir''@|${localstatedir}|g' \
	     -e 's|@''prefix''@|${prefix}|g' \
	  $(srcdir)/$(smf_method_script).in > $@
endif
