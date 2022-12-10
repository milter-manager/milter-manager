---
title: Install to Ubuntu
---

# Install to Ubuntu --- How to install milter manager to Ubuntu Linux

## About this document

This document describes how to install milter manager toUbuntu Linux. See Install for generalinstall information.

## Install packages

We provide milter manager deb packages for Ubuntu on[Launchpad](https://launchpad.net).

You also enable the official backports repository to detect the latestviruses by the latest ClamAV.

<pre>% sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu $(lsb_release -cs)-backports main universe"</pre>

### PPA (Personal Package Archive)

The milter manager APT repository for Ubuntu uses PPA (PersonalPackage Archive) on [Launchpad](https://launchpad.net). You caninstall milter manager by APT from the PPA.

Here are supported Ubuntu versions:

* 16.04 LTS Xenial Xerus
* 18.04 LTS Bionic Beaver
* 19.04 Disco Dingo

Add the <code>ppa:milter-manager/ppa</code> PPA to your system:

<pre>% sudo apt -y install software-properties-common
% sudo add-apt-repository -y ppa:milter-manager/ppa
% sudo apt update</pre>

### Install

Install milter manager:

<pre>% sudo apt -y install milter-manager</pre>

We use Postfix as MTA:

<pre>% sudo apt -V -y install postfix</pre>

We use spamass-milter, clamav-milter and milter-greylist asmilters:

<pre>% sudo apt -V -y install spamass-milter clamav-milter milter-greylist</pre>

## Configuration

Here is a basic configuration policy.

We use UNIX domain socket for accepting connection fromMTA because security and speed.

We set read/write permission for 'postfix' group to UNIXdomain socket because existing milter packages'configuration can be used.

milter-greylist should be applied only if[S25R](http://gabacho.reto.jp/en/anti-spam/)condition is matched to reduce needless delivery delay.But the configuration is automatically done bymilter-manager. We need to do nothing for it.

### Configure spamass-milter

At first, we configure spamd.

We add the following configuration to/etc/spamassassin/local.cf. This configuration is for addingheaders only if spam detected.

<pre>report_safe 0

remove_header ham Status
remove_header ham Level</pre>

We change /etc/default/spamassassin like the following toenable spamd:

Before:

<pre>ENABLED=0</pre>

After:

<pre>ENABLED=1</pre>

spamd should be started:

<pre>% sudo /etc/init.d/spamassassin start</pre>

There are no changes for spamass-milter's configuration.

### Configure clamav-milter

We don't need to change the default clamav-milter's configuration.

### Configure milter-greylist

We change /etc/milter-greylist/greylist.conf for the followingconfigurations:

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

We change /etc/default/milter-greylist to enablemilter-greylist. milter-greylist uses IPv4 socket becausemilter-gresylist's run script doesn't support changingsocket's group permission:

Before:

<pre>ENABLED=0</pre>

After:

<pre>ENABLED=1
SOCKET="inet:11125@[127.0.0.1]"</pre>

milter-greylist should be started:

<pre>% sudo /etc/init.d/milter-greylist start</pre>

### Configure milter-manager

milter-manager detects milters that installed in system.We can confirm spamass-milter, clamav-milter andmilter-greylist are detected:

<pre>% sudo /usr/sbin/milter-manager -u milter-manager --show-config</pre>

The following output shows milters are detected:

<pre>...
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
..</pre>

We should confirm that milter's name, socket path and'enabled = true'. If the values are unexpected,we need to change/etc/milter-manager/milter-manager.conf.See Configuration for details ofmilter-manager.conf.

But if we can, we want to use milter manager without editingmiter-manager.conf. If you report your environment to themilter manager project, the milter manager project mayimprove detect method.

We change /etc/default/milter-manager to work with Postfix:

Before:

<pre># For postfix, you might want these settings:
# SOCKET_GROUP=postfix
# CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock</pre>

After:

<pre># For postfix, you might want these settings:
SOCKET_GROUP=postfix
CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock</pre>

We create a directory for milter-manager's socket:

<pre>% sudo mkdir -p /var/spool/postfix/milter-manager/</pre>

We add milter-manager user to postfix group:

<pre>% sudo adduser milter-manager postfix</pre>

milter-manager's configuration is completed. We startmilter-manager:

<pre>% sudo /etc/init.d/milter-manager restart</pre>

/usr/bin/milter-test-server is usuful to confirmmilter-manager was ran:

<pre>% sudo -u postfix milter-test-server -s unix:/var/spool/postfix/milter-manager/milter-manager.sock</pre>

Here is a sample success output:

<pre>status: accept
elapsed-time: 0.128 seconds</pre>

If milter-manager fails to run, the following message willbe shown:

<pre>Failed to connect to unix:/var/spool/postfix/milter-manager/milter-manager.sock: No such file or directory</pre>

In this case, we can use log to solve theproblem. milter-manager is verbosily if --verbose option isspecified. milter-manager outputs logs to standard output ifmilter-manager isn't daemon process.

We can add the following configuration to/etc/default/milter-manager to output verbose log tostandard output:

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

We start milter-manager again:

<pre>% sudo /etc/init.d/milter-manager restart</pre>

Some logs are output if there is a problem. Runningmilter-manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/default/milter-managershould be commented out after the problem is solved to runmilter-manager as daemon process. And we should restartmilter-manager.

### Configure Postfix

We add the following milter configuration to/etc/postfix/main.cf.

<pre>milter_default_action = accept
milter_protocol = 6
milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

Here are descriptions of the configuration.

<dl>
<dt>milter_protocol = 6</dt>
<dd>Postfix uses milter protocol version 6.</dd>
<dt>milter_default_action = accept</dt>
<dd>Postfix accepts a mail if Postfix can't connect tomilter. It's useful configuration for not stopping mailserver function if milter has some problems. But itcauses some problems that spam mails and virus mails maybe delivered until milter is recovered.

If you can recover milter, 'tempfail' will be betterchoice rather than 'accept'. Default is 'tempfail'.</dd>
<dt>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</dt>
<dd>Postfix passes SMTP Auth related infomation tomilter. Some milters like milter-greylist use it.</dd></dl>

We need to register milter-manager to Postfix. It'simportant that spamass-milter, clamav-milter,milter-greylist aren't needed to be registered because theyare used via milter-manager.

We need to add the following configuration to/etc/postfix/main.cf. Note that Postfix chrooted to/var/spool/postfix/.

<pre>smtpd_milters = unix:/milter-manager/milter-manager.sock</pre>

We reload Postfix configuration:

<pre>% sudo /etc/init.d/postfix reload</pre>

Postfix's milter configuration is completed.

milter-manager logs to syslog. If milter-manager works well,some logs can be showen in /var/log/mail.info. We need tosent a test mail for confirming.

## Conclusion

There are many configurations to work milter and Postfixtogether. They can be reduced by introducing milter-manager.

Without milter-manager, we need to specify sockets ofspamass-milter, clamav-milter and milter-greylist tosmtpd_milters. With milter-manager, we doesn't need tospecify sockets of them, just specify a socket ofmilter-manager. They are detected automatically. We doesn'tneed to take care some small mistakes like typo.

milter-manager also supports ENABELD configuration used in/etc/default/milter-greylist. If we disable a milter, weuse the following steps:

<pre>% sudo /etc/init.d/milter-greylist stop
% sudo vim /etc/default/milter-greylist # ENABLED=1 => ENABLED=0</pre>

We need to reload milter-manager after we disable a milter.

<pre>% sudo /etc/init.d/milter-manager reload</pre>

milter-manager detects a milter is disabled and doesn't useit. We doesn't need to change Postfix's main.cf.

We can reduce maintainance cost by introducingmilter-manager if we use some milters on Ubuntu.

milter manager also provides tools to helpoperation. Installing them is optional but we can reduceoperation cost too. If we also install them, we will go toInstall to Ubuntu(optional).


