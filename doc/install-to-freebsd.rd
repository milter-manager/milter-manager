# -*- rd -*-

= Install to FreeBSD --- How to install milter manager to FreeBSD

== About this document

This document describes how to install milter manager to
FreeBSD. See ((<Install|install.rd>)) for general install
information.

This document assumes that FreeBSD 8.0-RELEASE is used.

== Install packages

To install the following packages, related packages are also
installed:

  % sudo /usr/local/sbin/portupgrade -NRr ruby18-glib2
  % sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_RUBY_MODULE=true' rrdtool

We use Postfix as MTA:

  % sudo /usr/local/sbin/portupgrade -NRr postfix

We use spamass-milter, clamav-milter and milter-greylist as
milters:

  % sudo /usr/local/sbin/portupgrade -NRr spamass-milter milter-greylist
  % sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_MILTER=true' clamav

== Build and Install

We work at ~/src/. We will install milter manager into /usr/local/.

  % mkdir -p ~/src/
  % cd ~/src/
  % fetch http://downloads.sourceforge.net/milter-manager/milter-manager-1.4.1.tar.gz
  % tar xvzf milter-manager-1.4.1.tar.gz
  % cd milter-manager-1.4.1
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

milter works as 'mailnull' user. It's better that each
milter works as other user but we use 'mailnull' user for
all milter because of the following reasons:

  * don't need to add because 'mailnull' user is created by
    default
  * 'mailnull' user isn't used because we use Postfix
    instead of Sendmail

Exception: milter-manager is ran as 'milter-manager' user.

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

spamd should be started:

  % sudo /usr/local/etc/rc.d/sa-spamd start

spamass-milter creates a socket file
as /var/run/spamass-milter.sock by default. But general user
can't create a new file in /var/run/. We create
/var/run/milter/ directory owned by 'mailnull'
user. spamass-milter creates a socket file in the directory:

  % sudo mkdir /var/run/milter/
  % sudo /usr/sbin/chown mailnull:mailnull /var/run/milter

We add the following to /etc/rc.conf:

  spamass_milter_enable="YES"
  spamass_milter_user="mailnull"
  spamass_milter_group="mailnull"
  spamass_milter_socket="/var/run/milter/spamass-milter.sock"
  spamass_milter_socket_owner="mailnull"
  spamass_milter_socket_group="mail"
  spamass_milter_socket_mode="660"

spamass-milter should be started:

  % sudo /usr/local/etc/rc.d/spamass-milter start

=== Configure clamav-milter

At first, we configure ClamAV.

We add the following to /etc/rc.conf to enable clamd and
freshclam:

  clamav_clamd_enable="YES"
  clamav_freshclam_enable="YES"

clamd and freshclam should be started:

  % sudo /usr/local/etc/rc.d/clamav-clamd start
  % sudo /usr/local/etc/rc.d/clamav-freshclam start

clamav-milter is ran as 'clamav' user and 'clamav' group by
default. We use the configuration because 'clamav' user is
general user. We set group read/write permission of socket.

We add the following to /etc/rc.conf:

  clamav_milter_enable="YES"
  clamav_milter_socket_mode="660"
  clamav_milter_socket_group="mail"

clamav-milter should be started:

  % sudo /usr/local/etc/rc.d/clamav-milter start

=== Configure milter-greylist

We copy /usr/local/etc/mail/greylist.conf.sample to
/usr/local/etc/mail/greylist.conf and change it like the
following to use greylist by default:

Before:
  racl whitelist default

After:
  racl greylist default

We add the following to /etc/rc.conf:

  miltergreylist_enable="YES"
  miltergreylist_runas="mailnull:mailnull"

We create /etc/rc.conf.d/miltergreylist to set socket file's
permission. 'sleep 1' is just for waiting milter-greylist is
ran. If 1 second is small, we can improve it like
/usr/local/etc/rc.d/clamav-milter:

/etc/rc.conf.d/miltergreylist

  start_postcmd=start_postcmd

  start_postcmd() {
    sleep 1
    /bin/chmod g+w $miltergreylist_sockfile
    /usr/bin/chgrp mail $miltergreylist_sockfile
  }

milter-greylist should be started:

  % sudo /usr/local/etc/rc.d/milter-greylist start

=== Configure milter-manager

We create 'milter-manager' user because we run
milter-manager as 'milter-manager' user:

  % sudo /usr/sbin/pw groupadd milter-manager
  % sudo /usr/sbin/pw useradd milter-manager -g milter-manager -G mail

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
    milter.connection_spec = "unix:/var/run/milter/spamass-milter.sock"
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
  % sudo /usr/sbin/chown -R milter-manager:mail /var/run/milter-manager

milter-manager's configuration is completed. We start to
setup running milter-manager.

milter-manager has its own run script for FreeBSD. It will
be installed into
/usr/local/etc/milter-manager/rc.d/milter-manager. We need
to create a symbolic link to /usr/local/etc/rc.d/ and mark
it run on start-up:

  % cd /usr/local/etc/rc.d/
  % sudo ln -s /usr/local/etc/milter-manager/rc.d/milter-manager ./

We add the following to /etc/rc.conf to enable milter-manager:

  milter_manager_enable="YES"

milter-manager should be started:

  % sudo /usr/local/etc/rc.d/milter-manager start

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

  % sudo /usr/local/etc/rc.d/milter-manager start

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

  milter_protocol = 6 # if Postfix >= 2.5
  milter_default_action = accept
  milter_mail_macros = {auth_author} {auth_type} {auth_authen}

Here are descriptions of the configuration.

: milter_protocol = 6

   Postfix uses milter protocol version 6. This version is
   default for Sendmail 8.14. But version 6 is supported
   since Postfix 2.5. If you use Postfix <= 2.4, don't use
   the option.

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

  % sudo /usr/local/etc/rc.d/postfix reload

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

  % sudo /usr/local/etc/rc.d/XXX stop
  % sudo vim /etc/rc.conf # XXX_enabled="YES" => XXX_enabled="NO"

We need to reload milter-manager after we disable a milter.

  % sudo /usr/local/etc/rc.d/milter-manager reload

milter-manager detects a milter is disabled and doesn't use
it. We doesn't need to change Postfix's main.cf.

We can reduce maintainance cost by introducing
milter-manager if we use some milters on FreeBSD.
