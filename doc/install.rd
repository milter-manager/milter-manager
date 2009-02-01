# -*- rd -*-

= Install --- How to install milter manager

== About this document

This document describes how to install milter manager.

== Dependencies

This section describes libraries and softwares that are
depended on milter-manager.

=== Required

milter manager depends on the following libraries. They
should be installed before building milter manager.

  * GLib >= 2.12.3
  * Ruby >= 1.8.5 (1.9.x isn't supported)
  * Ruby/GLib2 (Ruby-GNOME2) >= 0.16.0

=== Optional: Testing

To run milter manager's unit tests, the following softwares
are needed. They are needed for running unit tests but not
needed for running milter manager.

  * Cutter >= 1.0.6 (not released yet)
  * LCOV

# === Optional: Graph

# FIXME

#  * RRDtool
#  * RRDtool's Ruby bindings

# === Optional: Web interface for administration

# FIXME

#  * RubyGems
#  * Rake
#  * Ruby on Rails 2.2.2
#  * ...

== Details

  * ((<Ubuntu|install-to-ubuntu.rd>))
#  * ((<CentOS|install-to-centos.rd>))
  * ((<FreeBSD|install-to-freebsd.rd>))
#   * ((<Etc|install-to-others.rd>))
