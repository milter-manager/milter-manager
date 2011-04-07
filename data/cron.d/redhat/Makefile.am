cron_script = milter-manager-log

if REDHAT_PLATFORM
crondir = $(sysconfdir)/cron.d
dist_cron_DATA = $(cron_script)
else
EXTRA_DIST = $(cron_script)
endif
