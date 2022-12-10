---
title: Install to FreeBSD (optional)
---

# Install to FreeBSD (optional) --- How to install milter manager related softwares to FreeBSD

## About this document

This document describes how to install milter managerrelated softwares to FreeBSD. See Install toFreeBSD for milter manager installinformation and Install for general installinformation.

## Install milter-manager-log-analyzer {#milter-manager-log-analyzer}

milter-manager-log-analyzer analyzer milter manager's logand generates graphs for statistics information.

There are two ways to view generated graphs; (1) view themvia a Web server at the same host and (2) view them via[Munin](http://munin-monitoring.org/) (and a Webserver) at other host. If we already have Munin orexclusive system monitoring server, Munin is a betterway. Otherwise, a Web server at the same host is a betterway. FIXME

We install milter-manager-log-analyzer and then configurean environment to view generated graphs.

### Install packages

We use RRDtool for generating graphs. We also install Rubybindings to RRDtool.

<em>NOTE: The Ruby bindings in RRDtool 1.3.8 has a bug. Don'tspecify "-M'WITH_RUBY_MODULE=yes'" when you use 1.3.8.</em>

<pre>% sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_RUBY_MODULE=true' databases/rrdtool</pre>

### Configure milter-manager-log-analyzer

milter-manager-log-analyzer analyzes log and generatesgraphs periodically by cron. There is a cron configurationfile at/usr/local/etc/milter-manager/cron.d/freebsd/milter-manager-log.milter-manager-log-analyzer analyzes log each 5 minutes andgenerates graphs into ~milter-manager/public_html/log/ byusing the file.

<pre>% sudo -u milter-manager -H crontab /usr/local/etc/milter-manager/cron.d/freebsd/milter-manager-log</pre>

We can confirm that milter-manager-log-analyzer is ran each5 minutes at /var/log/cron.

We complete milter-manager-log-analyzer installation. Wewill setup an environment to view generated graphs.  First,a way that a Web server in the same host will be explained,then a way that using Munin will be explained.

### Way 1: View via a Web server at the same host

#### Install packages

We use Apache as Web server. We will install Apache 2.2series. (www/apache22)

<pre>% sudo /usr/local/sbin/portupgrade -NRr www/apache22</pre>

#### Configure Apache

Apache publishes users' files. We edit/usr/local/etc/apache22/httpd.conf like the following:

Before:

<pre># User home directories
#Include etc/apache22/extra/httpd-userdir.conf</pre>

After:

<pre># User home directories
Include etc/apache22/extra/httpd-userdir.conf</pre>

Then we reload configuration file:

<pre>% sudo /usr/local/etc/rc.d/apache22 reload</pre>

Now, we can see graphs at http://localhost/milter-manager-log/.

### Way 2: View via Munin at other host {#munin}

Next way is using Munin at other host.

#### Install packages

We install milter-manager-munin-plugins package thatprovides statistics data collected bymilter-manager-log-analyzer to Munin:

<pre>% sudo /usr/local/sbin/portupgrade -NRr munin-node</pre>

#### Configure munin-node

We will install Munin plugins to provide statisticsinformation collected by milter-manager-log-analyzer toMunin. Those plugins are installed into/usr/local/share/munin/plugins/. First, we install them tomunin-node's plugins directory:

<pre>% sudo ln -s /usr/local/share/milter-manager/munin/plugins/* /usr/local/share/munin/plugins</pre>

Next, we put a configuration for those plugins to/usr/local/etc/munin/plugin-conf.d/milter-manager.conf:

/usr/local/etc/munin/plugin-conf.d/milter-manager.conf:

<pre>[milter_manager_*]
  user milter-manager
  env.PATH /bin:/usr/local/bin:/usr/bin
  env.logdir /home/milter-manager/public_html/log
  env.pidfile /var/run/milter-manager/milter-manager.pid</pre>

At the last, we enables only plugins what we need:

<pre>% sudo /usr/local/sbin/munin-node-configure --shell | grep -e '\(milter_manager_\|postfix_processes\|sendmail_processes\)' | sudo sh</pre>

Plugins installation is completed.

<em>NOTE: We need to use databases created bymilter-manager-log-analyzer bundled with milter manager1.5.0 or later to provide statistics data to Munin. If wehave databases that are created by oldermilter-manager-log-analyzer, we need to remove~milter-manager/public_html/log/. If we remove thedirectory, milter-manager-log-analyzer re-creates statisticsdatabases 5 minutes later.</em>

Munin-node should accept accesses from Munin server. IfMunin server is 192.168.1.254, we need to append thefollowing lines to /etc/munin/munin-node.conf:

/etc/munin/munin-node.conf:

<pre>allow ^192\.168\.1\.254$</pre>

We need to restart munin-node to apply our configuration:

<pre>% sudo /usr/sbin/service munin-node restart</pre>

#### Configure Munin server

Works in this section at system monitor server. We assumethat system monitor server works on FreeBSD.

First, we install munin and Apache:

<pre>monitoring-server% sudo /usr/local/sbin/portupgrade -NRr munin-main www/apache22</pre>

We add our mail server that works munin-node to munin'smonitor target. We assume that mail server has the followingconfiguration:

<dl>
<dt>Host name</dt>
<dd>mail.example.com</dd>
<dt>IP address</dt>
<dd>192.168.1.2</dd></dl>

We need to add the following lines to/usr/local/etc/munin/munin.conf to add the mail server:

/usr/local/etc/munin/munin.conf:

<pre>[mail.example.com]
    address 192.168.1.2
    use_node_name yes</pre>

We export graphs generated by munin:

<pre>% sudo ln -s /usr/local/www/munin/ /usr/local/www/apache22/data/</pre>

We will be able to view graphs athttp://monitoring-server/munin/ 5 minutes later.

## Conclusion

We can confirm milter's effect visually bymilter-manager-log-analyzer. If we use Postfix as MTA, wecan compare with[Mailgraph](http://mailgraph.schweikert.ch/)'s graphsto confirm milter's effect. We can use graphs generated bymilter-manager-log-analyzer effectively when we are tryingout a milter.


