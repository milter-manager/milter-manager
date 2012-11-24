# -*- rd -*-

= Install to CentOS 5 --- How to install milter manager to CentOS 5

== About this document

This document describes how to install milter manager to CentOS 5. See
((<Install to CentOS 6|install-to-centos.rd>)) for CentOS 6 specific
install information. See ((<Install|install.rd>)) for general install
information.

We assume that CentOS version is 5.8. We also assume that we
use sudo to run a command with root authority. If you don't
use sudo, use su instead.

== Install packages

To install the following packages, related packages are also
installed:

  % sudo yum install -y glib2 ruby

We use Sendmail as MTA because it's installed by default.

  % sudo yum install -y sendmail-cf

We use spamass-milter, clamav-milter and milter-greylist as
milters. We use milter packages registered in RPMforge.

We register RPMforge like the following.

On 32bit environment:
  % sudo rpm -Uhv http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.i386.rpm

On 64bit environment:
  % sudo rpm -Uhv http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.x86_64.rpm

Now, we can install milters:

  % sudo yum install -y spamass-milter clamav-milter milter-greylist

And we install RRDtool for generating graphs:

  % sudo yum install -y ruby-rrdtool

== Build and Install

milter manager can be installed by yum.

We register milter manager yum repository like the following:

  % sudo rpm -Uvh http://downloads.sourceforge.net/milter-manager/centos/milter-manager-release-1.1.0-0.noarch.rpm

Now, we can install milter manager:

  % sudo yum install -y milter-manager

== Configuration

Here is a basic configuration policy.

We use IPv4 socket and accepts connections only from localhost.

milter-greylist should be applied only if
((<S25R|URL:http://gabacho.reto.jp/en/anti-spam/>))
condition is matched to reduce needless delivery delay.
But the configuration is automatically done by
milter-manager. We need to do nothing for it.

=== Configure spamass-milter

At first, we configure spamd.

spamd adds "[SPAM]" to spam mail's subject by default. If
you don't like the behavior, edit
/etc/mail/spamassassin/local.cf.

Before:
  rewrite_header Subject [SPAM]

After:
  # rewrite_header Subject [SPAM]

We add the following configuration to
/etc/spamassassin/local.cf. This configuration is for adding
headers only if spam detected.

  remove_header ham Status
  remove_header ham Level

We start spamd on startup:

  % sudo /sbin/chkconfig spamassassin on

We start spamd:

  % sudo /sbin/service spamassassin start

Here are spamass-milter's configuration items:

  * Change socket address.
  * Disable needless body change feature.
  * Reject if score is larger than or equal to 15.

We change /etc/sysconfig/spamass-milter:

Before:
  #SOCKET=/var/run/spamass.sock
  #EXTRA_FLAGS="-m -r 15"

After:
  SOCKET="inet:11120@[127.0.0.1]"
  EXTRA_FLAGS="-m -r 15"

We start spamass-milter on startup:

  % sudo /sbin/chkconfig spamass-milter on

We start spamass-milter:

  % sudo /sbin/service spamass-milter start

=== Configure clamav-milter

We start clamd.

  % sudo /sbin/service clamd start

We edit /etc/clamav-milter.conf to change clamav-milter's
socket address.

Before:
  #MilterSocketMode 0660

After:
  MilterSocketMode 0660

We start clamav-milter:

  % sudo /sbin/service clamav-milter start

=== Configure milter-greylist

We change /etc/mail/greylist.conf for the following
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

# For newer milter-greylist:
#  socket "/var/milter-greylist/milter-greylist.sock"
#  # ...
#
#  socket "/var/milter-greylist/milter-greylist.sock" 666
#  # ...

Before:
  racl whitelist default

After:
  subnetmatch /24
  greylist 10m
  autowhite 1w
  racl greylist default

We create /etc/sysconfig/milter-greylist with the following
content:

  OPTIONS="$OPTIONS -p inet:11122@[127.0.0.1]"

We start milter-greylist on startup:

  % sudo /sbin/chkconfig milter-greylist on

We start milter-greylist:

  % sudo /sbin/service milter-greylist start

=== Configure milter-manager

We add 'milter-manager' user to 'clamav' group to access
clamav-milter's socket:

  % sudo usermod -G clamav -a milter-manager

milter-manager detects milters that installed in system.
We can confirm spamass-milter, clamav-milter and
milter-greylist are detected:

  % sudo /usr/sbin/milter-manager -u milter-manager --show-config

The following output shows milters are detected:

  ...
  define_milter("milter-greylist") do |milter|
    milter.connection_spec = "inet:11122@[127.0.0.1]"
    ...
    milter.enabled = true
    ...
  end
  ...
  define_milter("clamav-milter") do |milter|
    milter.connection_spec = "unix:/var/clamav/clmilter.socket"
    ...
    milter.enabled = true
    ...
  end
  ...
  define_milter("spamass-milter") do |milter|
    milter.connection_spec = "inet:11120@[127.0.0.1]"
    ...
    milter.enabled = true
    ...
  end
  ...

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

milter-manager's configuration is finished. We start to
milter-manager:

  % sudo /sbin/service milter-manager start

milter-test-server is usuful to confirm milter-manager was
ran:

  % sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock

Here is a sample success output:

  status: pass
  elapsed-time: 0.128 seconds

If milter-manager fails to run, the following message will
be shown:

  Failed to connect to unix:/var/run/milter-manager/milter-manager.sock

In this case, we can use log to solve the
problem. milter-manager is verbosily if --verbose option is
specified. milter-manager outputs logs to standard output if
milter-manager isn't daemon process.

We can add the following configuration to
/etc/sysconfig/milter-manager to output verbose log to
standard output:

  OPTION_ARGS="--verbose --no-daemon"

We start milter-manager again:

  % sudo /sbin/service milter-manager restart

Some logs are output if there is a problem. Running
milter-manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/sysconfig/milter-manager
should be commented out after the problem is solved to run
milter-manager as daemon process. And we should restart
milter-manager.

=== Configure Sendmail

First, we enable Sendmail:

  % sudo /sbin/chkconfig --add sendmail
  % sudo /sbin/service sendmail start

We append the following content to /etc/mail/sendmail.mc to
register milter-manager to Sendmail.

  INPUT_MAIL_FILTER(`milter-manager', `S=local:/var/run/milter-manager/milter-manager.sock')

It's important that spamass-milter, clamav-milter,
milter-greylist aren't needed to be registered because they
are used via milter-manager.

We update Sendmail configuration and reload it.

  % sudo make -C /etc/mail
  % sudo /sbin/service sendmail reload

Sendmail's milter configuration is completed.

milter-manager logs to syslog. If milter-manager works well,
some logs can be shown in /var/log/maillog. We need to sent
a test mail for confirming.

== Conclusion

There are many configurations to work milter and Sendmail
together. They can be reduced by introducing milter-manager.

Without milter-manager, we need to specify sockets of
spamass-milter, clamav-milter and milter-greylist to
sendmail.mc. With milter-manager, we doesn't need to
specify sockets of them, just specify a socket of
milter-manager. They are detected automatically. We doesn't
need to take care some small mistakes like typo.

milter-manager also detects which '/sbin/chkconfig -add' is
done or not. If we disable a milter, we use the following
steps:

  % sudo /sbin/service milter-greylist stop
  % sudo /sbin/chkconfig --del milter-greylist

We need to reload milter-manager after we disable a milter.

  % sudo /sbin/service milter-manager reload

milter-manager detects a milter is disabled and doesn't use
it. We doesn't need to change Sendmail's sendmail.mc.

We can reduce maintainance cost by introducing
milter-manager if we use some milters on CentOS.

milter manager also provides tools to help
operation. Installing them is optional but we can reduce
operation cost too. If we also install them, we will go to
((<Install to CentOS
(optional)|install-options-to-centos.rd>)).
