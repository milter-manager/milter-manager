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

  * ((<Security|.#security>))
  * ((<Log|.#log>))
  * ((<Milter|.#milter>))
  * ((<Database|.#database>))

Here are descriptions of configuration items.

== [security] Security

You can use configuration items same as milter manager's
"log" group. Please see milter-manager's document page for
details.

: security.effective_user
   See ((<security.effective_user|configuration.rd#security.effective_user>)).

: security.effective_group
   See ((<security.effective_group|configuration.rd#security.effective_group>)).

== [log] Log

You can use configuration items same as milter manager's
"security" group. Please see milter-manager's document page
for details.

: log.level

   See ((<log.level|configuration.rd#log.level>)).

: log.use_syslog

   See ((<log.use_syslog|configuration.rd#log.use_syslog>)).

: log.syslog_facility

   See ((<log.syslog_facility|configuration.rd#log.syslog_facility>)).

== [milter] Milter

You can use configuration items same as milter manager's
"manager" group. Please see milter-manager's document page for
details.

: milter.connection_spec
   See ((<manager.connection_spec|configuration.rd#manager.connection_spec>)).

: milter.unix_socket_mode
   See ((<manager.unix_socket_mode|configuration.rd#manager.unix_socket_mode>)).

: milter.unix_socket_group
   See ((<manager.unix_socket_group|configuration.rd#manager.unix_socket_group>)).

: milter.remove_unix_socket_on_create
   See ((<manager.remove_unix_socket_on_create|configuration.rd#manager.remove_unix_socket_on_create>)).

: milter.remove_unix_socket_on_close
   See ((<manager.remove_unix_socket_on_close|configuration.rd#manager.remove_unix_socket_on_close>)).

: milter.daemon
   See ((<manager.daemon|configuration.rd#manager.daemon>)).

: milter.pid_file
   See ((<manager.pid_file|configuration.rd#manager.pid_file>)).

: milter.maintenance_interval
   See ((<manager.maintenance_interval|configuration.rd#manager.maintenance_interval>)).

: milter.suspend_time_on_unacceptable
   See ((<manager.suspend_time_on_unacceptable|configuration.rd#manager.suspend_time_on_unacceptable>)).

: milter.max_connections
   See ((<manager.max_connections|configuration.rd#manager.max_connections>)).

: milter.max_file_descriptors
   See ((<manager.max_file_descriptors|configuration.rd#manager.max_file_descriptors>)).

: milter.event_loop_backend
   See ((<manager.event_loop_backend|configuration.rd#manager.event_loop_backend>)).

: milter.event_loop_backend
   See ((<manager.event_loop_backend|configuration.rd#manager.event_loop_backend>)).

: milter.n_workers
   See ((<manager.n_workers|configuration.rd#manager.n_workers>)).

: milter.packet_buffer_size
   See ((<manager.packet_buffer_size|configuration.rd#manager.packet_buffer_size>)).

: milter.max_pending_finished_sessions
   See ((<manager.max_pending_finished_sessions|configuration.rd#manager.max_pending_finished_sessions>)).

: milter.maintained
   See ((<manager.maintained|configuration.rd#manager.maintained>)).

: milter.event_loop_created
   See ((<manager.event_loop_created|configuration.rd#manager.event_loop_created>)).

: milter.name
   Returns child milter's name. Since 1.8.1.

== [database] Database

You can use configuration items same as ((<'"database"
group'|configuration.rd#database>)).  Please see
milter-manager's document page for details. It includes
setup document and tutorial.

: database.type
   See ((<database.type|configuration.rd#database.type>)).

: database.name
   See ((<database.name|configuration.rd#database.name>)).

: database.host
   See ((<database.host|configuration.rd#database.host>)).

: database.port
   See ((<database.port|configuration.rd#database.port>)).

: database.path
   See ((<database.path|configuration.rd#database.path>)).

: database.user
   See ((<database.user|configuration.rd#database.user>)).

: database.password
   See ((<database.password|configuration.rd#database.password>)).

: database.setup
   See ((<database.setup|configuration.rd#database.setup>)).

: database.load_models(path)
   See ((<database.load_models|configuration.rd#database.load_models>)).
