# -*- rd -*-

= milter configuration written by Ruby --- How to write configuration file

== About this document

Milters written by Ruby support configuration file that has
the same syntax of milter-manager's one. But configuration
items are different. This documents describes about how to
write configuration file and configuration items.

== Summary

You can specify configuration file by -c or --configuration
option. For example, here is a command line that a milter,
milter-regexp.rb, uses /etc/milter-regexp.conf as its
configuration:

  % ruby milter-regexp.rb -c /etc/milter-regexp.conf

Configuration file uses the following syntax:

  GROUP.ITEM = VALUE

For example, set value of "connection_spec" item in "milter"
group to "inet:12345" for set milter's listen socket address
to IPv4 address and 12345 port:

  milter.connection_spec = "inet:12345"

Here are available groups:

  * Log
  * Milter
  * Database

Here are descriptions of configuration items.

== Log

You can use configuration items same as milter manager's one
about logging. Please see milter-manager's document page for
details.

: log.level

   ((<log.level|configuration.rd#log.level>))

: log.use_syslog

   ((<log.use_syslog|configuration.rd#log.use_syslog>))

: log.syslog_facility

   ((<log.syslog_facility|configuration.rd#log.syslog_facility>))

== Milter

...

== Database

...
