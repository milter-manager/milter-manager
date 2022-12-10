---
title: milter configuration written by Ruby
---

# milter configuration written by Ruby --- How to write configuration file

## About this document

Milters written by Ruby support configuration file that hasthe same syntax of milter-manager's one. But configurationitems are different. This documents describes about how towrite configuration file and configuration items.

## Summary

You can specify configuration file by -c or --configurationoption. For example, here is a command line that a milter,milter-regexp.rb, uses /etc/milter-regexp.conf as itsconfiguration:

<pre>% ruby milter-regexp.rb -c /etc/milter-regexp.conf</pre>

Configuration file uses the following syntax:

<pre>GROUP.ITEM = VALUE</pre>

For example, set value of "connection_spec" item in "milter"group to "inet:12345" for set milter's listen socket addressto IPv4 address and 12345 port:

<pre>milter.connection_spec = "inet:12345"</pre>

Here are available groups:

* Security
* Log
* Milter
* Database

Here are descriptions of configuration items.

## Security {#security}

You can use configuration items same as milter manager's"log" group. Please see milter-manager's document page fordetails.

<dl>
<dt>security.effective_user</dt>
<dd>See security.effective_user.</dd>
<dt>security.effective_group</dt>
<dd>See security.effective_group.</dd></dl>

## Log {#log}

You can use configuration items same as milter manager's"security" group. Please see milter-manager's document pagefor details.

<dl>
<dt>log.level</dt>
<dd>See log.level.</dd>
<dt>log.path</dt>
<dd>See log.path.</dd>
<dt>log.use_syslog</dt>
<dd>See log.use_syslog.</dd>
<dt>log.syslog_facility</dt>
<dd>See log.syslog_facility.</dd></dl>

## Milter {#milter}

You can use configuration items same as milter manager's"manager" group. Please see milter-manager's document page fordetails.

<dl>
<dt>milter.connection_spec</dt>
<dd>See manager.connection_spec.</dd>
<dt>milter.unix_socket_mode</dt>
<dd>See manager.unix_socket_mode.</dd>
<dt>milter.unix_socket_group</dt>
<dd>See manager.unix_socket_group.</dd>
<dt>milter.remove_unix_socket_on_create</dt>
<dd>See manager.remove_unix_socket_on_create.</dd>
<dt>milter.remove_unix_socket_on_close</dt>
<dd>See manager.remove_unix_socket_on_close.</dd>
<dt>milter.daemon</dt>
<dd>See manager.daemon.</dd>
<dt>milter.pid_file</dt>
<dd>See manager.pid_file.</dd>
<dt>milter.maintenance_interval</dt>
<dd>See manager.maintenance_interval.</dd>
<dt>milter.suspend_time_on_unacceptable</dt>
<dd>See manager.suspend_time_on_unacceptable.</dd>
<dt>milter.max_connections</dt>
<dd>See manager.max_connections.</dd>
<dt>milter.max_file_descriptors</dt>
<dd>See manager.max_file_descriptors.</dd>
<dt>milter.event_loop_backend</dt>
<dd>See manager.event_loop_backend.</dd>
<dt>milter.event_loop_backend</dt>
<dd>See manager.event_loop_backend.</dd>
<dt>milter.n_workers</dt>
<dd>See manager.n_workers.</dd>
<dt>milter.packet_buffer_size</dt>
<dd>See manager.packet_buffer_size.</dd>
<dt>milter.max_pending_finished_sessions</dt>
<dd>See manager.max_pending_finished_sessions.</dd>
<dt>milter.maintained</dt>
<dd>See manager.maintained.</dd>
<dt>milter.event_loop_created</dt>
<dd>See manager.event_loop_created.</dd>
<dt>milter.name</dt>
<dd>Returns child milter's name. Since 1.8.1.</dd></dl>

## Database {#database}

You can use configuration items same as '"database"group'.  Please seemilter-manager's document page for details. It includessetup document and tutorial.

<dl>
<dt>database.type</dt>
<dd>See database.type.</dd>
<dt>database.name</dt>
<dd>See database.name.</dd>
<dt>database.host</dt>
<dd>See database.host.</dd>
<dt>database.port</dt>
<dd>See database.port.</dd>
<dt>database.path</dt>
<dd>See database.path.</dd>
<dt>database.user</dt>
<dd>See database.user.</dd>
<dt>database.password</dt>
<dd>See database.password.</dd>
<dt>database.setup</dt>
<dd>See database.setup.</dd>
<dt>database.load_models(path)</dt>
<dd>See database.load_models.</dd></dl>


