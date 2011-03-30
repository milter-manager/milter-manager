initdir = $(sysconfdir)/init.d

init_script = milter-manager

milter-manager: Makefile $(srcdir)/milter-manager.in
	sed \
		-e "s,\$$(prefix),$(prefix),g" \
		$(srcdir)/milter-manager.in > milter-manager

if SOLARIS_PLATFORM
dist_init_SCRIPTS = $(init_script)
else
EXTRA_DIST = $(init_script)
endif
