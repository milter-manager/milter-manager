smf_method_script = milter-manager

if SOLARIS_PLATFORM
smf_methoddir = $(prefix)/lib/svc/method
smf_method_SCRIPTS = $(smf_method_script)
$(smf_method_script): $(srcdir)/$(smf_method_script).in
	@echo creating $@
	@sed -e 's|@''localstatedir''@|${localstatedir}|g' \
	  $? > $@
endif
