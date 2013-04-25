# -*- rd -*-

= Reverse Dictionary --- Find how to do by want to do

== About this document

This is a document to find how to do by want to do as key.

== Install

=== Install to Debian GNU/Linux

((<"Install to Debian GNU/Linux"|install-to-debian.rd>))
describes it. You can maintain milter manager package easily
because milter manager can be installed with aptitude.

=== Install to Ubuntu Linux

((<Install to Ubuntu Linux|install-to-ubuntu.rd>)) describes
it. You can maintain milter manager package easily because
milter manager can be installed with aptitude.

=== Install to CentOS

((<Install to CentOS|install-to-centos.rd>)) describes it.
You can maintain milter manager package easily because
milter manager can be installed as RPM package.

=== Install to FreeBSD

((<Install to FreeBSD|install-to-freebsd.rd>)) describes it.

=== Generate graphs for statistics data

There are documents for each platform:

  * ((<For Debian|install-options-to-debian.rd#milter-manager-log-analyzer>))
  * ((<For Ubuntu|install-options-to-ubuntu.rd#milter-manager-log-analyzer>))
  * ((<For CentOS|install-options-to-centos.rd#milter-manager-log-analyzer>))
  * ((<For FreeBSD|install-options-to-freebsd.rd#milter-manager-log-analyzer>))

== Configuration: Basic

=== [configuration-basic-find-configuration-file] Find a configuration file

Here are configuration file locations when you install
milter manager followed by install manual for your platform:

  * Ubuntu: /etc/milter-manager/milter-manager.local.conf
  * CentOS: /etc/milter-manager/milter-manager.local.conf
  * Ubuntu: /usr/local/etc/milter-manager/milter-manager.local.conf

milter-manager.local.conf is a file what you create
newly. milter-manager.conf loads milter-manager.local.conf
in the same directory by default.

=== Connect milter-manager via TCP/IP

"inet:" is used for ((<manager.connection_spec|configuration.rd#manager.connection-spec>)).

  # Listen on 10025 port. milter-manager accepts a connection from localhost
  manager.connection_spec = "inet:10025@localhost"

=== Connect milter-manager via UNIX domain socket

"unix:" is used for ((<manager.connection_spec|configuration.rd.ja#manager.connection-spec>)).

  # Listen on /var/run/milter/milter-manager.sock.
  manager.connection_spec = "unix://var/run/milter/milter-manager.sock"

A socket file permission can be specified by
((<manager.unix_socket_mode|configuration.rd.ja#manager.unix-socket-mode>)).

  # Users who belongs to the same group that owns the socket
  # can connect to milter-manager.
  manager.unix_socket_mode = 0660

A group for socket file can be specified by
((<manager.unix_socket_group|configuration.rd.ja#manager.unix-socket-group>)).

  # Socket file is belongs to "milter" group.
  manager.unix_socket_group = "milter"

=== Cleanup UNIX domain socket

Creating socket is failed when the same name of newly UNIX
domain socket. To avoid the situation, milter-manager
provides features that remove socket file on the following
points:

  (1) before creating a UNIX domain socket
  (2) after finishing a UNIX domain socket use

Normally, milter-manager doesn't fail to create a socket by
'socket file already exists' reason because milter-manager
enables both of them by default.

If you want to disable the features, change the following
configuration respectively:

  (1) ((<manager.remove_unix_socket_on_create|configuration.rd#manager.remove-unix-socket-on-create>))
  (2) ((<manager.remove_unix_socket_on_close|configuration.rd#manager.remove-unix-socket-on-close>))

Here is an configuration to disable both of them:

  # Don't remove an existing socket file before creating a socket file
  manager.remove_unix_socket_on_create = false
  # Don't remove a socket file after its use
  manager.remove_unix_socket_on_close = false

== Configuration: Application

=== Apply milters to messages only for specified accounts

milter-manager provides a sample configuration to restrict
milter application to specified account. This section shows
an example that all registered milters only applied to the
following accounts:

  (1) test-user@example.com
  (2) all accounts in test.example.com domain

Here is a configuration to be appended to
((<milter-manager.local.conf|.#configuration-basic-find-configuration-file>)):

  restrict_accounts_by_list("test-user@example.com",
                            /@test\.example\.com\z/)

This configuration syntax may be changed because this is
still sample. But a feature provided by this configuration
will be still provided. The feature will be more powerful by
supporting database and LDAP as an account source in the
future.
