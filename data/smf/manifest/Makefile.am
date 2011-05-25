smf_manifest = milter-manager.xml

if SOLARIS_PLATFORM
smf_manifestdir = $(localstatedir)/svc/manifest/network
smf_manifest_DATA = $(smf_manifest)
$(smf_manifest): $(srcdir)/$(smf_manifest).in
	@echo creating $@
	@sed -e 's|@''prefix''@|${prefix}|g' \
	  $? > $@
endif
