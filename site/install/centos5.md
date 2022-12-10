---
title: Install to CentOS 5
---

# Install to CentOS 5 --- How to install milter manager to CentOS 5

## About this document

This document describes how to install milter manager to CentOS 7. SeeInstall to CentOS 7 for CentOS 7 specificinstall information. SeeInstall to CentOS 6 for CentOS 6 specificinstall information. See Install for general installinformation.

We assume that CentOS version is 5.8. We also assume that weuse sudo to run a command with root authority. If you don'tuse sudo, use su instead.

## Install packages

To install the following packages, related packages are alsoinstalled:

<pre>% sudo yum install -y glib2 ruby</pre>

We use Sendmail as MTA because it's installed by default.

<pre>% sudo yum install -y sendmail-cf</pre>

We use spamass-milter, clamav-milter and milter-greylist asmilters. We use milter packages registered in RPMforge.

We register RPMforge like the following.

On 32bit environment:

<pre>% sudo rpm -Uhv http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.i386.rpm</pre>

On 64bit environment:

<pre>% sudo rpm -Uhv http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.x86_64.rpm</pre>

Now, we can install milters:

<pre>% sudo yum install -y spamass-milter clamav-milter milter-greylist</pre>

And we install RRDtool for generating graphs:

<pre>% sudo yum install -y ruby-rrdtool</pre>

## Build and Install

milter manager can be installed by yum.

We register milter manager yum repository like the following:

<pre>% curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | sudo bash</pre>

See also: https://packagecloud.io/milter-manager/repos/install

Now, we can install milter manager:

<pre>% sudo yum install -y milter-manager</pre>

## Configuration

Here is a basic configuration policy.

We use IPv4 socket and accepts connections only from localhost.

milter-greylist should be applied only if[S25R](http://gabacho.reto.jp/en/anti-spam/)condition is matched to reduce needless delivery delay.But the configuration is automatically done bymilter-manager. We need to do nothing for it.

### Configure spamass-milter

At first, we configure spamd.

spamd adds "[SPAM]" to spam mail's subject by default. Ifyou don't like the behavior, edit/etc/mail/spamassassin/local.cf.

Before:

<pre>rewrite_header Subject [SPAM]</pre>

After:

<pre># rewrite_header Subject [SPAM]</pre>

We add the following configuration to/etc/spamassassin/local.cf. This configuration is for addingheaders only if spam detected.

<pre>remove_header ham Status
remove_header ham Level</pre>

We start spamd on startup:

<pre>% sudo /sbin/chkconfig spamassassin on</pre>

We start spamd:

<pre>% sudo /sbin/service spamassassin start</pre>

Here are spamass-milter's configuration items:

* Change socket address.
* Disable needless body change feature.
* Reject if score is larger than or equal to 15.

We change /etc/sysconfig/spamass-milter:

Before:

<pre>#SOCKET=/var/run/spamass.sock
#EXTRA_FLAGS="-m -r 15"</pre>

After:

<pre>SOCKET="inet:11120@[127.0.0.1]"
EXTRA_FLAGS="-m -r 15"</pre>

We start spamass-milter on startup:

<pre>% sudo /sbin/chkconfig spamass-milter on</pre>

We start spamass-milter:

<pre>% sudo /sbin/service spamass-milter start</pre>

### Configure clamav-milter

We start clamd.

<pre>% sudo /sbin/service clamd start</pre>

We edit /etc/clamav-milter.conf to change clamav-milter'ssocket address.

Before:

<pre>#MilterSocketMode 0660</pre>

After:

<pre>MilterSocketMode 0660</pre>

We start clamav-milter:

<pre>% sudo /sbin/service clamav-milter start</pre>

### Configure milter-greylist

We change /etc/mail/greylist.conf for the followingconfigurations:

* use the leading 24bits for IP address match to avoid    Greylist adverse effect for sender uses some MTA case.
* decrease retransmit check time to 10 minutes from 30    minutes (default value) to avoid Greylist adverse effect.
* increase auto whitelist period to a week from 1 day    (default value) to avoid Greylist adverse effect.
* use Greylist by default.

<pre># note
The configuration relaxes Greylist check to avoid Greylist
adverse effect. It increases received spam mails but we
should give priority to avoid false positive rather than
false negative. We should not consider that we blocks all
spam mails by Greylist. We can blocks spam mails that
isn't blocked by Greylist by other anti-spam technique
such as SpamAssassin. milter manager helps constructing
mail system that combines some anti-spam techniques.</pre>

Before:

<pre>racl whitelist default</pre>

After:

<pre>subnetmatch /24
greylist 10m
autowhite 1w
racl greylist default</pre>

We create /etc/sysconfig/milter-greylist with the followingcontent:

<pre>OPTIONS="$OPTIONS -p inet:11122@[127.0.0.1]"</pre>

We start milter-greylist on startup:

<pre>% sudo /sbin/chkconfig milter-greylist on</pre>

We start milter-greylist:

<pre>% sudo /sbin/service milter-greylist start</pre>

### Configure milter-manager

We add 'milter-manager' user to 'clamav' group to accessclamav-milter's socket:

<pre>% sudo usermod -G clamav -a milter-manager</pre>

milter-manager detects milters that installed in system.We can confirm spamass-milter, clamav-milter andmilter-greylist are detected:

<pre>% sudo /usr/sbin/milter-manager -u milter-manager --show-config</pre>

The following output shows milters are detected:

<pre>...
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
...</pre>

We should confirm that milter's name, socket path and'enabled = true'. If the values are unexpected,we need to change/etc/milter-manager/milter-manager.conf.See Configuration for details ofmilter-manager.conf.

But if we can, we want to use milter manager without editingmiter-manager.conf. If you report your environment to themilter manager project, the milter manager project mayimprove detect method.

milter-manager's configuration is finished. We start tomilter-manager:

<pre>% sudo /sbin/service milter-manager start</pre>

milter-test-server is usuful to confirm milter-manager wasran:

<pre>% sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

Here is a sample success output:

<pre>status: pass
elapsed-time: 0.128 seconds</pre>

If milter-manager fails to run, the following message willbe shown:

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock</pre>

In this case, we can use log to solve theproblem. milter-manager is verbosily if --verbose option isspecified. milter-manager outputs logs to standard output ifmilter-manager isn't daemon process.

We can add the following configuration to/etc/sysconfig/milter-manager to output verbose log tostandard output:

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

We start milter-manager again:

<pre>% sudo /sbin/service milter-manager restart</pre>

Some logs are output if there is a problem. Runningmilter-manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/sysconfig/milter-managershould be commented out after the problem is solved to runmilter-manager as daemon process. And we should restartmilter-manager.

### Configure Sendmail

First, we enable Sendmail:

<pre>% sudo /sbin/chkconfig --add sendmail
% sudo /sbin/service sendmail start</pre>

We append the following content to /etc/mail/sendmail.mc toregister milter-manager to Sendmail.

<pre>INPUT_MAIL_FILTER(`milter-manager', `S=local:/var/run/milter-manager/milter-manager.sock')</pre>

It's important that spamass-milter, clamav-milter,milter-greylist aren't needed to be registered because theyare used via milter-manager.

We update Sendmail configuration and reload it.

<pre>% sudo make -C /etc/mail
% sudo /sbin/service sendmail reload</pre>

Sendmail's milter configuration is completed.

milter-manager logs to syslog. If milter-manager works well,some logs can be shown in /var/log/maillog. We need to senta test mail for confirming.

## Conclusion

There are many configurations to work milter and Sendmailtogether. They can be reduced by introducing milter-manager.

Without milter-manager, we need to specify sockets ofspamass-milter, clamav-milter and milter-greylist tosendmail.mc. With milter-manager, we doesn't need tospecify sockets of them, just specify a socket ofmilter-manager. They are detected automatically. We doesn'tneed to take care some small mistakes like typo.

milter-manager also detects which '/sbin/chkconfig -add' isdone or not. If we disable a milter, we use the followingsteps:

<pre>% sudo /sbin/service milter-greylist stop
% sudo /sbin/chkconfig --del milter-greylist</pre>

We need to reload milter-manager after we disable a milter.

<pre>% sudo /sbin/service milter-manager reload</pre>

milter-manager detects a milter is disabled and doesn't useit. We doesn't need to change Sendmail's sendmail.mc.

We can reduce maintainance cost by introducingmilter-manager if we use some milters on CentOS.

milter manager also provides tools to helpoperation. Installing them is optional but we can reduceoperation cost too. If we also install them, we will go toInstall to CentOS(optional).


