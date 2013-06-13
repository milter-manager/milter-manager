# -*- rd -*-

= Install to CentOS 6 --- How to install milter manager to CentOS 6

== About this document

This document describes how to install milter manager to CentOS 6. See
((<Install to CentOS 5|install-to-centos5.rd>)) for CentOS 5 specific
install information. See ((<Install|install.rd>)) for general install
information.

In this document, CentOS 6.4 is used. Sudo is used to run a command
with root privilege. If you don't use sudo, use su instead.

== Install packages

Postfix is used as MTA because it's installed by default.

Spamass-milter, clamav-milter and milter-greylist are used as
milters. Milter packages registered in Repoforge are used.

Register Repoforge like the following.

On 32bit environment:

  % sudo rpm -Uhv http://ftp.jaist.ac.jp/pub/Linux/Fedora/epel/6/i386/epel-release-6-8.noarch.rpm

On 64bit environment:

  % sudo rpm -Uhv http://ftp.jaist.ac.jp/pub/Linux/Fedora/epel/6/x86_64/epel-release-6-8.noarch.rpm

Now, you can install milters:

  % sudo yum install -y spamass-milter clamav-milter milter-greylist

And you can install RRDtool for generating graphs:

  % sudo yum install -y rrdtool

== Build and Install

milter manager can be installed by yum.

Register milter manager yum repository like the following:

  % sudo rpm -Uvh http://downloads.sourceforge.net/milter-manager/centos/milter-manager-release-1.1.0-0.noarch.rpm

Now, you can install milter manager:

  % sudo yum install -y milter-manager

== Configuration

Here is a basic configuration policy.

IPv4 socket is used and connections only from localhost are accepted.

milter-greylist should be applied only if
((<S25R|URL:http://gabacho.reto.jp/en/anti-spam/>))
condition is matched to reduce needless delivery delay.
But the configuration is automatically done by
milter manager. You need to do nothing for it.

It's difficult that milter manager runs on SELinux. Disable SELinux
policy module for Postfix and Milter.

  % sudo semodule -d postfix
  % sudo semodule -d milter

# TODO: When milter policy version upgrade to 1.3.1 or later, research
# about semanager and milter_port_t and update this section.
# semanage port -a -t milter_port_t -p tcp <port>

=== Configure spamass-milter

At first, you configure spamd.

spamd adds "[SPAM]" to spam mail's subject by default. If
you don't like the behavior, edit
/etc/mail/spamassassin/local.cf.

Before:
  rewrite_header Subject [SPAM]

After:
  # rewrite_header Subject [SPAM]

Add the following configuration to
/etc/spamassassin/local.cf. This configuration is for adding
headers only if spam detected.

  remove_header ham Status
  remove_header ham Level

Start spamd on startup:

  % sudo /sbin/chkconfig spamassassin on

Start spamd:

  % sudo run_init /sbin/service spamassassin start

Here are spamass-milter's configuration items:

  * Change socket address.
  * Disable needless body change feature.
  * Reject if score is larger than or equal to 15.

Change /etc/sysconfig/spamass-milter:

Before:
  #SOCKET=/var/run/spamass.sock
  #EXTRA_FLAGS="-m -r 15"

After:
  SOCKET="inet:11120@[127.0.0.1]"
  EXTRA_FLAGS="-m -r 15"

Start spamass-milter on startup:

  % sudo /sbin/chkconfig spamass-milter on

Start spamass-milter:

  % sudo run_init /sbin/service spamass-milter start

=== Configure clamav-milter

Update ClamAV virus database and start clamd.

  % sudo freshclam
  % sudo run_init /sbin/service clamd start

Edit /etc/clamav-milter.conf to change clamav-milter's
socket address.

Before:
  #MilterSocketMode 0660

After:
  MilterSocketMode 0660

Start clamav-milter:

  % sudo run_init /sbin/service clamav-milter start

=== Configure milter-greylist

Change /etc/mail/greylist.conf for the following
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
  adverse effect. It increases received spam mails but you
  should give priority to avoid false positive rather than
  false negative. You should not consider that you blocks all
  spam mails by Greylist. You can blocks spam mails that
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

Start milter-greylist on startup:

  % sudo /sbin/chkconfig milter-greylist on

Start milter-greylist:

  % sudo run_init /sbin/service milter-greylist start

=== Configure milter manager

Add 'milter-manager' user to 'clamav' group to access
clamav-milter's socket:

  % sudo usermod -G clamav -a milter-manager

Add 'milter-manager' user to 'smmsp' group to access
milter-greylist's socket:

  % sudo usermod -G smmsp -a milter-manager

milter manager detects milters that installed in system.
You can confirm spamass-milter, clamav-milter and
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

You should confirm that milter's name, socket path and
'enabled = true'. If the values are unexpected,
you need to change
/etc/milter-manager/milter-manager.conf.
See ((<Configuration|configuration.rd>)) for details of
milter-manager.conf.

But if we can, we want to use milter manager without editing
miter-manager.conf. If you report your environment to the
milter manager project, the milter manager project may
improve detect method.

milter manager's configuration is finished. Start to
milter manager:

  % sudo /sbin/service milter-manager restart

milter-test-server is usuful to confirm milter manager was
ran:

  % sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock

Here is a sample success output:

  status: pass
  elapsed-time: 0.128 seconds

If milter manager fails to run, the following message will
be shown:

  Failed to connect to unix:/var/run/milter-manager/milter-manager.sock

In this case, you can use log to solve the
problem. milter manager is verbosily if --verbose option is
specified. milter manager outputs logs to standard output if
milter manager isn't daemon process.

You can add the following configuration to
/etc/sysconfig/milter-manager to output verbose log to
standard output:

  OPTION_ARGS="--verbose --no-daemon"

Restart milter manager:

  % sudo run_init /sbin/service milter-manager restart

Some logs are output if there is a problem. Running
milter manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/sysconfig/milter-manager
should be commented out after the problem is solved to run
milter manager as daemon process. And you should restart
milter manager.

=== Configure Postfix

First, add 'postfix' user to 'milter-manager' group to access
milter manager's socket:

  % sudo usermod -G milter-manager -a postfix

Enables Postfix:

  % sudo /sbin/chkconfig --add postfix
  % sudo run_init /sbin/service postfix start

Configure Postfix for milters. Append following lines to
/etc/postfix/main.cf:
  milter_protocol = 6
  milter_default_action = accept
  milter_mail_macros = {auth_author} {auth_type} {auth_authen}

For details for each lines.

: milter_protocol = 6

   Use milter protocol version 6.

: milter_default_action = accept

   MTA receives mails when MTA cannot access milter. Although there
   are problems between MTA and milter, MTA can deliver mails to clients.
   But until you recover milter, perhaps MTA receives spam mails and
   virus mails.

   If you can recover the system quickly, you can specify 'tempfail'
   instead of 'accept'. Default value is 'tempfail'.

: milter_mail_macros = {auth_author} {auth_type} {auth_authen}

   MTA gives information related SMTP Auth to milters. milter-greylist
   etc. uses it.

Register milter manager to Postfix.  It's important that
spamass-milter, clamav-milter and milter-greylist aren't needed to be
registered because they are used via milter manager.

Append following lines to /etc/postfix/main.cf:
  smtpd_milters = unix:/var/run/milter-manager/milter-manager.sock

Reload Postfix's configuration.

  % sudo /etc/init.d/postfix reload

milter manager logs to syslog. If milter manager works well,
some logs can be shown in /var/log/maillog. You need to send
a test mail for confirming.

== Conclusion

There are many configurations to work milter and Postfix
together. They can be reduced by introducing milter manager.

Without milter manager, you need to specify sockets of
spamass-milter, clamav-milter and milter-greylist to
/etc/postfix/main.cf. With milter manager, you don't need to
specify sockets of them, just specify a socket of
milter manager. They are detected automatically. You don't
need to take care some small mistakes like typo.

milter manager also detects which '/sbin/chkconfig -add' is
done or not. If you disable a milter, you use the following
steps:

  % sudo /sbin/service milter-greylist stop
  % sudo /sbin/chkconfig --del milter-greylist

You need to reload milter manager after you disable a milter.

  % sudo /sbin/service milter-manager reload

milter manager detects a milter is disabled and doesn't use
it. You don't need to change /etc/postfix/main.cf.

You can reduce maintainance cost by introducing
milter manager if you use some milters on CentOS.

milter manager also provides tools to help
operation. Installing them is optional but you can reduce
operation cost too. If you also install them, you will go to
((<Install to CentOS
(optional)|install-options-to-centos.rd>)).
