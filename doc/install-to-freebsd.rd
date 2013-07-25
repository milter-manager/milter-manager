# -*- rd -*-

= Install to FreeBSD --- How to install milter manager to FreeBSD

== About this document

This document describes how to install milter manager to
FreeBSD. See ((<Install|install.rd>)) for general install
information.

This document assumes that FreeBSD 9.1-RELEASE is used.

== Install packages

To install the following packages, related packages are also
installed:

  % sudo /usr/local/sbin/portupgrade -NRr lang/ruby18 glib20

We use Postfix as MTA:

  % sudo /usr/local/sbin/portupgrade -NRr postfix

We use spamass-milter, clamav-milter and milter-greylist as
milters:

  % sudo /usr/local/sbin/portupgrade -NRr spamass-milter
  % sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_POSTFIX=true' milter-greylist
  % sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_MILTER=true' clamav

== Build and Install

We work at ~/src/. We will install milter manager into /usr/local/.

  % mkdir -p ~/src/
  % cd ~/src/
  % fetch http://downloads.sourceforge.net/milter-manager/milter-manager-2.0.0.tar.gz
  % tar xvzf milter-manager-2.0.0.tar.gz
  % cd milter-manager-2.0.0
  % ./configure CPPFLAGS="-I/usr/local/include"
  % gmake
  % sudo gmake install

== Configuration

Here is a basic configuration policy.

We use UNIX domain socket for accepting connection from
MTA because security and speed.

We use general user for milter's effective user. This is
also for security. 'mail' group has permission of read/write
UNIX domain socket. 'postfix' user is joined to 'mail' group.

milter-greylist should be applied only if
((<S25R|URL:http://gabacho.reto.jp/en/anti-spam/>))
condition is matched to reduce needless delivery delay.
But the configuration is automatically done by
milter-manager. We need to do nothing for it.

=== Configure spamass-milter

At first, we configure spamd.

We create /usr/local/etc/mail/spamassassin/local.cf with the
following configuration. This configuration is for adding
headers only if spam detected.

  remove_header ham Status
  remove_header ham Level

We need to append the following to /etc/rc.conf to enable
spamd:

  spamd_enable=YES

If our SMTP server has many concurrent connections, we
should increase max concurrent connections. It is 5 by
default. It's a good first value that about 1/3 of the max
SMTP connections. e.g. about 30 for about 100 connections
SMTP server:

  spamd_flags="-c --max-children=30 "

We can adjust apposite value after operation. We can see
milter manager's statistics graphs at the time.

spamd should be started:

  % sudo /usr/sbin/service sa-spamd start

Next, we configure spamass-milter. We run spamass-milter
with 'spamd' user and 'spamd' group.

spamass-milter creates a socket file
as /var/run/spamass-milter.sock by default. But general user
can't create a new file in /var/run/. We create
/var/run/spamass-milter/ directory owned by 'spamd'
user. spamass-milter creates a socket file in the directory:

  % sudo mkdir /var/run/spamass-milter/
  % sudo /usr/sbin/chown spamd:spamd /var/run/spamass-milter

We add the following to /etc/rc.conf:

  spamass_milter_enable="YES"
  spamass_milter_user="spamd"
  spamass_milter_group="spamd"
  spamass_milter_socket="/var/run/spamss-milter/spamass-milter.sock"
  spamass_milter_socket_owner="spamd"
  spamass_milter_socket_group="mail"
  spamass_milter_socket_mode="660"
  spamass_milter_localflags="-u spamd -- -u spamd"

spamass-milter should be started:

  % sudo /usr/sbin/service spamass-milter start

=== Configure clamav-milter

At first, we configure ClamAV.

We add the following to /etc/rc.conf to enable clamd and
freshclam:

  clamav_clamd_enable="YES"
  clamav_freshclam_enable="YES"

Get the latest definition files before run clamd:

  % sudo /usr/local/bin/freshcram

clamd and freshclam should be started:

  % sudo /usr/sbin/service clamav-clamd start
  % sudo /usr/sbin/service clamav-freshclam start

clamav-milter is ran as 'clamav' user and 'clamav' group by
default. We use the configuration because 'clamav' user is
general user. We set group read/write permission of socket.

We add the following to /etc/rc.conf:

  clamav_milter_enable="YES"
  clamav_milter_socket_mode="660"
  clamav_milter_socket_group="mail"

We may need to configure /usr/local/etc/clamav-milter.conf.
e.g.:

/usr/local/etc/clamav-milter.conf

Before:
  #OnInfected Quarantine
  #AddHeader Replace
  #LogSyslog yes
  #LogFacility LOG_MAIL
  #LogInfected Basic

After:
  OnInfected Reject
  AddHeader Replace
  LogSyslog yes
  LogFacility LOG_MAIL
  LogInfected Full

Here are explanations of the above configurations:

: OnInfected Reject
   Rejects infected mails. The default value is
   Quarantine. It puts infected mails into Postfix's hold
   queue. If we don't want to confirm hold queue
   periodically, Reject is a good way for easy maintenance.

: AddHeader Replace
  Replaces X-Virus-Scanned header even if it's existed.

: LogSyslog yes
   Logs to syslog.

: LogFacility LOG_MAIL
   Logs to syslog with LOG_MAIL facility. /var/log/maillog
   is the default LOG_MAIL log file.

: LogInfected Full
   Logs verbosity on finding infected mails.

clamav-milter should be started:

  % sudo /usr/sbin/service clamav-milter start

=== Configure milter-greylist

We run milter-greylist as 'mailnull' user and 'mailnull' group.
'mailnull' user is the default configuration and it is unused
user on Postfix environment.

We copy /usr/local/etc/mail/greylist.conf.sample to
/usr/local/etc/mail/greylist.conf and change it for the
following configurations:

  * change the effective group to "mail" group from "mailnull" group.
  * add write permission for the socket file to "mail" group.
  * use the leading 24bits for IP address match to avoid
    Greylist adverse effect for sender uses some MTA case.
  * decrease retransmit check time to 10 minutes from 30
    minutes (default value) to avoid Greylist adverse effect.
  * increase auto whitelist period to a week from 1 day
    (default value) to avoid Greylist adverse effect.
  * use Greylist by default.

  # note
  The configuration relaxes Greylist check to avoid Greylist
  adverse effect. It increases received spam mails but we
  should give priority to avoid false positive rather than
  false negative. We should not consider that we blocks all
  spam mails by Greylist. We can blocks spam mails that
  isn't blocked by Greylist by other anti-spam technique
  such as SpamAssassin. milter manager helps constructing
  mail system that combines some anti-spam techniques.

Before:
  socket "/var/milter-greylist/milter-greylist.sock"
  user "mailnull:mailnull"
  racl whitelist default

After:
  socket "/var/milter-greylist/milter-greylist.sock" 660
  user "mailnull:mail"
  subnetmatch /24
  greylist 10m
  autowhite 1w
  racl greylist default

We add the following to /etc/rc.conf:

  miltergreylist_enable="YES"
  miltergreylist_runas="mailnull:mail"

milter-greylist should be started:

  % sudo /usr/sbin/service milter-greylist start

=== Configure milter-manager

We create 'milter-manager' user because we run
milter-manager as 'milter-manager' user:

  % sudo /usr/sbin/pw groupadd milter-manager
  % sudo /usr/sbin/pw useradd milter-manager -g milter-manager -G mail -m

milter-manager detects milters that installed in system.
We can confirm spamass-milter, clamav-milter and
milter-greylist are detected:

  % sudo /usr/local/sbin/milter-manager -u milter-manager --show-config

The following output shows milters are detected:

  ...
  define_milter("milter-greylist") do |milter|
    milter.connection_spec = "unix:/var/milter-greylist/milter-greylist.sock"
    ...
    milter.enabled = true
    ...
  end
  ..
  define_milter("clamav-milter") do |milter|
    milter.connection_spec = "unix:/var/run/clamav/clmilter.sock"
    ...
    milter.enabled = true
    ...
  end
  ..
  define_milter("spamass-milter") do |milter|
    milter.connection_spec = "unix:/var/run/spamss-milter/spamass-milter.sock"
    ...
    milter.enabled = true
    ...
  end
  ..

We should confirm that milter's name, socket path and
'enabled = true'. If the values are unexpected,
we need to change
/usr/local/etc/milter-manager/milter-manager.conf.
See ((<Configuration|configuration.rd>)) for details of
milter-manager.conf.

But if we can, we want to use milter manager without editing
miter-manager.conf. If you report your environment to the
milter manager project, the milter manager project may
improve detect method.

milter-manager creates socket file as
/var/run/milter-manager/milter-manager.sock by default on
FreeBSD. We need to create /var/run/milter-manager directory
before running milter-manager:

  % sudo mkdir -p /var/run/milter-manager
  % sudo /usr/sbin/chown -R milter-manager:milter-manager /var/run/milter-manager

milter-manager's configuration is completed. We start to
setup running milter-manager.

We add the following to /etc/rc.conf to enable milter-manager:

  milter_manager_enable="YES"

milter-manager should be started:

  % sudo /usr/sbin/service milter-manager start

/usr/local/bin/milter-test-server is usuful to confirm
milter-manager was ran:

  % sudo -u mailnull milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock

Here is a sample success output:

  status: pass
  elapsed-time: 0.128 seconds

If milter-manager fails to run, the following message will
be shown:

  Failed to connect to unix:/var/run/milter-manager/milter-manager.sock: No such file or directory

In this case, we can use log to solve the
problem. milter-manager is verbosily if --verbose option is
specified. milter-manager outputs logs to standard output if
milter-manager isn't daemon process.

We add the following to /etc/rc.conf to output verbose log
to standard output:

  milter_manager_debug="YES"

milter-manager should be started:

  % sudo /usr/sbin/service milter-manager restart

Some logs are output if there is a problem. Running
milter-manager can be exitted by Ctrl+c.

milter_manager_debug configuration in /etc/rc.conf should be
commented out after the problem is solved to run
milter-manager as daemon process. And milter-manager should
restarted.

=== Configure Postfix

We add 'postfix' user to 'mail' group:

  % sudo /usr/sbin/pw groupmod mail -m postfix

We start milter's configuration.

We add the following milter configuration to
/usr/local/etc/postfix/main.cf:

  milter_protocol = 6
  milter_default_action = accept
  milter_mail_macros = {auth_author} {auth_type} {auth_authen}

Here are descriptions of the configuration.

: milter_protocol = 6

   Postfix uses milter protocol version 6.

: milter_default_action = accept

   Postfix accepts a mail if Postfix can't connect to
   milter. It's useful configuration for not stopping mail
   server function if milter has some problems. But it
   causes some problems that spam mails and virus mails may
   be delivered until milter is recovered.

   If you can recover milter, 'tempfail' will be better
   choice rather than 'accept'. Default is 'tempfail'.

: milter_mail_macros = {auth_author} {auth_type} {auth_authen}

   Postfix passes SMTP Auth related infomation to
   milter. Some milters like milter-greylist use it.

We need to register milter-manager to Postfix. It's
important that spamass-milter, clamav-milter,
milter-greylist aren't needed to be registered because they
are used via milter-manager.

We add the following to /usr/local/etc/postfix/main.cf:

  smtpd_milters = unix:/var/run/milter-manager/milter-manager.sock

Postfix should reload its configuration:

  % sudo /usr/sbin/service postfix reload

Postfix's milter configuration is completed.

milter-manager logs to syslog. If milter-manager works well,
some logs can be showen in /var/log/maillog. We need to sent
a test mail for confirming.

== Conclusion

There are many configurations to work milter and Postfix
together. They can be reduced by introducing milter-manager.

Without milter-manager, we need to specify sockets of
spamass-milter, clamav-milter and milter-greylist to
smtpd_milters. With milter-manager, we doesn't need to
specify sockets of them, just specify a coket of
milter-manager. They are detected automatically. We doesn't
need to take care some small mistakes like typo.

milter-manager also supports xxx_enabled="NO" configuration
used in /etc/rc.conf. If we disable a milter, we use the
following steps:

  % sudo /usr/sbin/service XXX stop
  % sudo vim /etc/rc.conf # XXX_enabled="YES" => XXX_enabled="NO"

We need to reload milter-manager after we disable a milter.

  % sudo /usr/sbin/service milter-manager reload

milter-manager detects a milter is disabled and doesn't use
it. We doesn't need to change Postfix's main.cf.

We can reduce maintainance cost by introducing
milter-manager if we use some milters on FreeBSD.

milter manager also provides tools to help
operation. Installing them is optional but we can reduce
operation cost too. If we also install them, we will go to
((<Install to FreeBSD
(optional)|install-options-to-freebsd.rd>)).
