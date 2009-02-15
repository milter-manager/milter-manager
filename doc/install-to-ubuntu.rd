# -*- rd -*-

= Install to Ubuntu Linux --- How to install milter manager to Ubuntu Linux

== About this document

This document describes how to install milter manager to
Ubuntu Linux. See ((<Install|install.rd>)) for general
install information.

== Install packages

To install the following packages, related packages are also
installed:

  % sudo aptitude -V -D install libtool intltool libglib2.0-dev ruby ruby1.8-dev libglib2-ruby1.8

We use Postfix as MTA:

  % sudo aptitude -V -D install postfix

We use spamass-milter, clamav-milter and milter-greylist as
milters:

  % sudo aptitude -V -D install spamass-milter clamav-milter
  % sudo aptitude -V -D --without-recommends install milter-greylist

It's the reason why --without-recommends is specified that
Sendmail is recommended package. If --without-recommends
option isn't specified, Sendmail is installed and Postfix is
removed.

== Build and Install

We work at ~/src/. We will install milter manager into /usr/local/.

  % mkdir -p ~/src/
  % cd ~/src/
  % wget http://downloads.sourceforge.net/milter-manager/milter-manager-0.8.1.tar.gz
  % tar xvzf milter-manager-0.8.1.tar.gz
  % cd milter-manager-0.8.1
  % ./configure
  % make
  % sudo make install
  % sudo /sbin/ldconfig

We create a user for milter-manager:

  % sudo /usr/sbin/adduser --system milter-manager --group

== Configuration

Here is a basic configuration policy.

We use UNIX domain socket for accepting connection from
MTA because security and speed.

We set read/write permission for 'postfix' group to UNIX
domain socket because existing milter packages'
configuration can be used.

milter-greylist should be applied only if
((<S25R|URL:http://gabacho.reto.jp/en/anti-spam/>))
condition is matched to reduce needless delivery delay.
But the configuration is automatically done by
milter-manager. We need to do nothing for it.

=== Configure spamass-milter

At first, we configure spamd.

We add the following configuration to
/etc/spamassassin/local.cf. This configuration is for adding
headers only if spam detected.

  remove_header ham Status
  remove_header ham Level

We change /etc/default/spamassassin like the following to
enable spamd:

Before:
  ENABLED=0

After:
  ENABLED=1

spamd should be started:

  % sudo /etc/init.d/spamassassin start

There are no changes for spamass-milter's configuration.

=== Configure clamav-milter

We change /etc/default/clamav-milter to permit 'postfix'
group to read/write socket:

Before:
  #USE_POSTFIX='yes'
  #SOCKET=local:/var/spool/postfix/clamav/clamav-milter.ctl

After:
  USE_POSTFIX='yes'
  SOCKET=local:/var/spool/postfix/clamav/clamav-milter.ctl

clamav-milter should be restarted:

  % sudo /etc/init.d/clamav-milter restart

=== Configure milter-greylist

We change /etc/milter-greylist/greylist.conf to use greylist
by default:

Before:
  racl whitelist default

After:
  racl greylist default

We change /etc/default/milter-greylist to enable
milter-greylist. milter-greylist uses IPv4 socket because
milter-gresylist's run script doesn't support changing
socket's group permission:

Before:
  ENABLED=0

After:
  ENABLED=1
  SOCKET="inet:11125@[127.0.0.1]"
  DOPTIONS="-P $PIDFILE -u $USER -p $SOCKET"

We need to specify not only SOCKET but also DOPTIONS because
/etc/init.d/milter-greylist has a problem in 8.04 LTS Hardy
Heron. The problem was fixed in 8.10 Intrepid Ibex. We
doesn't need to specify DOPTIONS if we use 8.10.

milter-greylist should be started:

  % sudo /etc/init.d/milter-greylist start

=== Configure milter-manager

milter-manager detects milters that installed in system.
We can confirm spamass-milter, clamav-milter and
milter-greylist are detected:

  % /usr/local/sbin/milter-manager --show-config

The following output shows milters are detected:

  ...
  define_milter("milter-greylist") do |milter|
    milter.connection_spec = "inet:11125@[127.0.0.1]"
    ...
    milter.enabled = true
    ...
  end
  ..
  define_milter("clamav-milter") do |milter|
    milter.connection_spec = "local:/var/spool/postfix/clamav/clamav-milter.ctl"
    ...
    milter.enabled = true
    ...
  end
  ..
  define_milter("spamass-milter") do |milter|
    milter.connection_spec = "unix:/var/spool/postfix/spamass/spamass.sock"
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

milter-manager saves its process ID to
/var/run/milter-manager/milter-manager.pid by default on
Ubuntu. We need to create /var/run/milter-manager/ directory
before running milter-manager:

  % sudo mkdir -p /var/run/milter-manager
  % sudo chown -R milter-manager:milter-manager /var/run/milter-manager

Postfix runs /var/spool/postfix/ chrooted environment,
milter-manager's socket file should be placed into under
/var/spool/postfix/. We use
/var/spool/postfix/milter-manager/milter-manager.sock for
the place. We need to create
/var/spool/postfix/milter-manager/ before running
milter-manager:

  % sudo mkdir -p /var/spool/postfix/milter-manager
  % sudo chown -R milter-manager:milter-manager /var/spool/postfix/milter-manager

We need to configure effective group and socket. Effective
group is 'postfix' group and socket is
/var/spool/postfix/milter-manager/milter-manager.sock.
We create /etc/default/milter-manager with the following
content:

  GROUP=postfix
  SOCKET_GROUP=postfix
  CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock

milter-manager's configuration is completed. We start to
configure about milter-manager's start-up.

milter-manager has its own run script for Ubuntu. It will be
installed into
/usr/local/etc/milter-manager/init.d/debian/milter-manager.
We need to create a symbolic link to /etc/init.d/ and
mark it run on start-up:

  % cd /etc/init.d
  % sudo ln -s /usr/local/etc/milter-manager/init.d/debian/milter-manager ./
  % sudo /usr/sbin/update-rc.d milter-manager defaults

The run script assumes that milter-manager command is
installed into /usr/sbin/milter-manager. We need to create a
symbolik link:

  % cd /usr/sbin
  % sudo ln -s /usr/local/sbin/milter-manager ./

We start milter-manager:

  % sudo /etc/init.d/milter-manager start

/usr/local/bin/milter-test-server is usuful to confirm
milter-manager was ran:

  % sudo -u postfix milter-test-server -s unix:/var/spool/postfix/milter-manager/milter-manager.sock

Here is a sample success output:

  status: pass
  elapsed-time: 0.128 seconds

If milter-manager fails to run, the following message will
be shown:

  Failed to connect to unix:/var/spool/postfix/milter-manager/milter-manager.sock: No such file or directory

In this case, we can use log to solve the
problem. milter-manager is verbosily if --verbose option is
specified. milter-manager outputs logs to standard output if
milter-manager isn't daemon process.

We can add the following configuration to
/etc/default/milter-manager to output verbose log to
standard output:

  OPTION_ARGS="--verbose --no-daemon"

We start milter-manager again:

  % sudo /etc/init.d/milter-manager start

Some logs are output if there is a problem. Running
milter-manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/default/milter-manager
should be commented out after the problem is solved to run
milter-manager as daemon process. And we should restart
milter-manager.

=== Configure Postfix

We add the following milter configuration to
/etc/postfix/main.cf.

  milter_default_action = accept
  milter_mail_macros = {auth_author} {auth_type} {auth_authen}

Here are descriptions of the configuration.

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

We need to add the following configuration to
/etc/postfix/main.cf. Note that Postfix chrooted to
/var/spool/postfix/.

  smtpd_milters = unix:/milter-manager/milter-manager.sock

We reload Postfix configuration:

  % sudo /etc/init.d/postfix reload

Postfix's milter configuration is completed.

milter-manager logs to syslog. If milter-manager works well,
some logs can be showen in /var/log/mail.info. We need to
sent a test mail for confirming.

== Conclusion

There are many configurations to work milter and Postfix
together. They can be reduced by introducing milter-manager.

Without milter-manager, we need to specify sockets of
spamass-milter, clamav-milter and milter-greylist to
smtpd_milters. With milter-manager, we doesn't need to
specify sockets of them, just specify a socket of
milter-manager. They are detected automatically. We doesn't
need to take care some small mistakes like typo.

milter-manager also supports ENABELD configuration used in
/etc/default/milter-greylist. If we disable a milter, we
use the following steps:

  % sudo /etc/init.d/milter-greylist stop
  % sudo vim /etc/default/milter-greylist # ENABLED=1 => ENABLED=0

We need to reload milter-manager after we disable a milter.

  % sudo /etc/init.d/milter-manager reload

milter-manager detects a milter is disabled and doesn't use
it. We doesn't need to change Postfix's main.cf.

We can reduce maintainance cost by introducing
milter-manager if we use some milters on Ubuntu.
