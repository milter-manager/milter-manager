service = milter-manager.service

if REDHAT_PLATFORM
initscript_DATA = $(service)
initscriptdir = $(pkgdatadir)
else
EXTRA_DIST = $(service)
endif
