# -*- rd -*-

= Install to Ubuntu --- How to install milter manager to Ubuntu Linux

== About this document

This document describes how to install milter manager to
Ubuntu Linux. See ((<Install|install.rd>)) for general
install information.

== Install packages

We provide milter manager deb packages for the following distributions.
You put the following content to /etc/apt/sources.list.d/milter-manager.list.

You also enable the official backports repository to detect the latest
viruses by the latest ClamAV.

=== For Lucid Lynx

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ lucid universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ lucid universe

If you don't enable the official backport repository, you need to
create /etc/apt/sources.list.d/backports.list with the following
content:

  deb http://archive.ubuntu.com/ubuntu lucid-backports main universe

=== For Precise Pangolin

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ precise universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ precise universe

If you don't enable the official backport repository, you need to
create /etc/apt/sources.list.d/backports.list with the following
content:

  deb http://archive.ubuntu.com/ubuntu precise-backports main universe

=== For Quantal Quetzal

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ quantal universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ quantal universe

If you don't enable the official backport repository, you need to
create /etc/apt/sources.list.d/backports.list with the following
content:

  deb http://archive.ubuntu.com/ubuntu quantal-backports main universe

=== For Raring Ringtail

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ raring universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ raring universe

If you don't enable the official backport repository, you need to
create /etc/apt/sources.list.d/backports.list with the following
content:

  deb http://archive.ubuntu.com/ubuntu raring-backports main universe

=== Install

We register the key of the package repository:

  % sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1BD22CD1

We install milter manager package:

  % sudo apt-get update
  % sudo apt-get -V -y install milter-manager

We use Postfix as MTA:

  % sudo apt-get -V -y install postfix

We use spamass-milter, clamav-milter and milter-greylist as
milters:

  % sudo apt-get -V -y install spamass-milter clamav-milter milter-greylist

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

  report_safe 0

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

We change /etc/default/clamav-milter to do the followings:

  * Virus check is done by clamd not clamav-milter.
  * 'postfix' group can communicate to clamav-milter.

Before:
  #SOCKET_RWGROUP=postfix

After:
  SOCKET_RWGROUP=postfix

clamav-milter should be restarted:

  % sudo /etc/init.d/clamav-milter restart

=== Configure milter-greylist

We change /etc/milter-greylist/greylist.conf for the following
configurations:

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
  racl whitelist default

After:
  subnetmatch /24
  greylist 10m
  autowhite 1w
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

milter-greylist should be started:

  % sudo /etc/init.d/milter-greylist start

=== Configure milter-manager

milter-manager detects milters that installed in system.
We can confirm spamass-milter, clamav-milter and
milter-greylist are detected:

  % sudo /usr/sbin/milter-manager -u milter-manager --show-config

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
    milter.connection_spec = "unix:/var/run/clamav/clamav-milter.ctl"
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
/etc/milter-manager/milter-manager.conf.
See ((<Configuration|configuration.rd>)) for details of
milter-manager.conf.

But if we can, we want to use milter manager without editing
miter-manager.conf. If you report your environment to the
milter manager project, the milter manager project may
improve detect method.

We change /etc/default/milter-manager to work with Postfix:

Before:
  # For postfix, you might want these settings:
  # SOCKET_GROUP=postfix
  # CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock

After:
  # For postfix, you might want these settings:
  SOCKET_GROUP=postfix
  CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock

We add milter-manager user to postfix group:

  % sudo adduser milter-manager postfix

milter-manager's configuration is completed. We start
milter-manager:

  % sudo /etc/init.d/milter-manager restart

/usr/bin/milter-test-server is usuful to confirm
milter-manager was ran:

  % sudo -u postfix milter-test-server -s unix:/var/spool/postfix/milter-manager/milter-manager.sock

Here is a sample success output:

  status: accept
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

  % sudo /etc/init.d/milter-manager restart

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
  milter_protocol = 6
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

milter manager also provides tools to help
operation. Installing them is optional but we can reduce
operation cost too. If we also install them, we will go to
((<Install to Ubuntu
(optional)|install-options-to-ubuntu.rd>)).
