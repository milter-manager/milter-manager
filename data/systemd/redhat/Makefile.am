service = milter-manager.service

if REDHAT_SYSTEMD_PLATFORM
initscript_DATA = $(service)
initscriptdir = $(pkgdatadir)
else
EXTRA_DIST = $(service)
endif
