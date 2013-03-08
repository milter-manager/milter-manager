# -*- rd -*-

= Install to FreeBSD (optional) --- How to install milter manager related softwares to FreeBSD

== About this document

This document describes how to install milter manager
related softwares to FreeBSD. See ((<Install to
FreeBSD|install-to-freebsd.rd>)) for milter manager install
information and ((<Install|install.rd>)) for general install
information.

== [milter-manager-log-analyzer] Install milter-manager-log-analyzer

milter-manager-log-analyzer analyzer milter manager's log
and generates graphs for statistics information.

There are two ways to view generated graphs; (1) view them
via a Web server at the same host and (2) view them via
((<Munin|URL:http://munin-monitoring.org/>)) (and a Web
server) at other host. If we already have Munin or
exclusive system monitoring server, Munin is a better
way. Otherwise, a Web server at the same host is a better
way. ((-If we want to run Munin at the same host, we need
a Web server.-))

We install milter-manager-log-analyzer and then configure
an environment to view generated graphs.

=== Install packages

We use RRDtool for generating graphs. We also install Ruby
bindings to RRDtool.

((*NOTE: The Ruby bindings in RRDtool 1.3.8 has a bug. Don't
specify "-M'WITH_RUBY_MODULE=yes'" when you use 1.3.8.*))

  % sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_RUBY_MODULE=true' databases/rrdtool

=== Configure milter-manager-log-analyzer

milter-manager-log-analyzer analyzes log and generates
graphs periodically by cron. There is a cron configuration
file at
/usr/local/etc/milter-manager/cron.d/freebsd/milter-manager-log.
milter-manager-log-analyzer analyzes log each 5 minutes and
generates graphs into ~milter-manager/public_html/log/ by
using the file.

  % sudo -u milter-manager -H crontab /usr/local/etc/milter-manager/cron.d/freebsd/milter-manager-log

We can confirm that milter-manager-log-analyzer is ran each
5 minutes at /var/log/cron.

We complete milter-manager-log-analyzer installation. We
will setup an environment to view generated graphs.  First,
a way that a Web server in the same host will be explained,
then a way that using Munin will be explained.

=== Way 1: View via a Web server at the same host

==== Install packages

We use Apache as Web server. We will install Apache 2.2
series. (www/apache22)

  % sudo /usr/local/sbin/portupgrade -NRr www/apache22

==== Configure Apache

Apache publishes users' files. We edit
/usr/local/etc/apache22/httpd.conf like the following:

Before:
  # User home directories
  #Include etc/apache22/extra/httpd-userdir.conf

After:
  # User home directories
  Include etc/apache22/extra/httpd-userdir.conf

Then we reload configuration file:

  % sudo /usr/local/etc/rc.d/apache22 reload

Now, we can see graphs at http://localhost/milter-manager-log/.

=== [munin] Way 2: View via Munin at other host

Next way is using Munin at other host.

==== Install packages

We install milter-manager-munin-plugins package that
provides statistics data collected by
milter-manager-log-analyzer to Munin:

  % sudo /usr/local/sbin/portupgrade -NRr munin-node

==== Configure munin-node

We will install Munin plugins to provide statistics
information collected by milter-manager-log-analyzer to
Munin. Those plugins are installed into
/usr/local/share/munin/plugins/. First, we install them to
munin-node's plugins directory:

  % sudo ln -s /usr/local/share/milter-manager/munin/plugins/* /usr/local/share/munin/plugins

Next, we put a configuration for those plugins to
/usr/local/etc/munin/plugin-conf.d/milter-manager.conf:

/usr/local/etc/munin/plugin-conf.d/milter-manager.conf:
  [milter_manager_*]
    user milter-manager
    env.PATH /bin:/usr/local/bin:/usr/bin
    env.logdir /home/milter-manager/public_html/log
    env.pidfile /var/run/milter-manager/milter-manager.pid

At the last, we enables only plugins what we need:

  % sudo /usr/local/sbin/munin-node-configure --shell | grep -e '\(milter_manager_\|postfix_processes\|sendmail_processes\)' | sudo sh

Plugins installation is completed.

((*NOTE: We need to use databases created by
milter-manager-log-analyzer bundled with milter manager
1.5.0 or later to provide statistics data to Munin. If we
have databases that are created by older
milter-manager-log-analyzer, we need to remove
~milter-manager/public_html/log/. If we remove the
directory, milter-manager-log-analyzer re-creates statistics
databases 5 minutes later.*))

Munin-node should accept accesses from Munin server. If
Munin server is 192.168.1.254, we need to append the
following lines to /etc/munin/munin-node.conf:

/etc/munin/munin-node.conf:
  allow ^192\.168\.1\.254$

We need to restart munin-node to apply our configuration:

  % sudo /usr/sbin/service munin-node restart

==== Configure Munin server

Works in this section at system monitor server. We assume
that system monitor server works on FreeBSD.

First, we install munin and Apache:

  monitoring-server% sudo /usr/local/sbin/portupgrade -NRr munin-main www/apache22

We add our mail server that works munin-node to munin's
monitor target. We assume that mail server has the following
configuration:

: Host name
    mail.example.com
: IP address
    192.168.1.2

We need to add the following lines to
/usr/local/etc/munin/munin.conf to add the mail server:

/usr/local/etc/munin/munin.conf:
  [mail.example.com]
      address 192.168.1.2
      use_node_name yes

We export graphs generated by munin:

  % sudo ln -s /usr/local/www/munin/ /usr/local/www/apache22/data/

We will be able to view graphs at
http://monitoring-server/munin/ 5 minutes later.

== Conclusion

We can confirm milter's effect visually by
milter-manager-log-analyzer. If we use Postfix as MTA, we
can compare with
((<Mailgraph|URL:http://mailgraph.schweikert.ch/>))'s graphs
to confirm milter's effect. We can use graphs generated by
milter-manager-log-analyzer effectively when we are trying
out a milter.
