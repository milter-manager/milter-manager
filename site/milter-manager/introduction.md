---
title: Introduction
---

# Introduction --- An introduction of milter-manager

## About this document

This document describes the following advantages by introducingmilter manager:

* reduce milter administration cost
* combine milters flexibly

This document describes about milter because this documentdoesn't assume that readers knows milter.

## milter

milter is an abbreviation for '<em>m</em>ail f<em>ilter</em>'. Itis an mail filter plugin system developed by Sendmail. Wecan introduce spam mail filter and/or virus check filter toSendmail without modifying Sendmail by milter. We can alsoimplement more effective mail filter by combining multiplefeatures because multiple milters can be appliedconcurrently.

We confirm terms used in milter manager's documents beforewe show a figure that describes relation between Sendmailand milter.

The term "milter" is used as "mail filter plugin system" and"mail filter plugin". In the documents, we use other termsfor them:

* milter: mail filter plugin
* milter system: mail filter plugin system

milter and Sendmail isn't same process. milter works asanother process. milter and Sendmail is communicated withdedicated protocol. The protocol is called as "milterprotocol" in the documents. Communication between Sendmailconnects and takes off a milter is called "milter session".

Here is a figure that describes relation between Sendmailand milter with terms as mentioned above:

Relation between Sendmail and milter

Many milters has been developed since Sendmail providesmilter system. We can search registered milters athttps://www.milter.org/. There are 62 milters inthe site at 2008/12. There are many milters that isn'tregistered at milter.org. It seems that there are more than100 milters. For example:

<dl>
<dt>clamav-milter</dt>
<dd>A milter for using anti-virus free software[ClamAV](http://www.clamav.net/).(including in ClamAV)</dd>
<dt>[amavisd-milter](http://amavisd-milter.sourceforge.net/)</dt>
<dd>A milter for using general content check free software[amavisd-new](http://www.ijs.si/software/amavisd/).</dd>
<dt>[spamass-milter](http://savannah.nongnu.org/projects/spamass-milt/)</dt>
<dd>A milter for using anti-spam free software[SpamAssassin](http://spamassassin.apache.org/).</dd></dl>

Some new anti-spam implementations are implemented asmilter. Implemented milters will be increased in the future.

### Postfix supports milter system

Originally, milter system is dedicated system for Sendmailbut Postfix starts supporting milter system since 2.3. Itmeans that we can use and development mail filter that isused with both Sendmail and Postfix. It makes milter moreportable.

Relation between Sendmail, Postfix and milter

milter can be used with any milter system support MTA likeSendmail and Postfix. But configuration for using milteris different in all MTAs. We can share same milter with bothSendmail and Postfix but cannot share effective miltercombination configuration.

### More effective milter combination

There are many anti-spam techniques. But there is not theperfect technique. We need to combine some techniques. Alltechniques have a weakness. For example, it doesn't havefalse detection but low detection rate, it has highdetection rate but long processing time, it has highdetection rate but high load and so on.

We need to use some techniques with using its advantages andcovering its weaknesses because there are many spamtechniques in the Internet.

There are two big kinds of anti-spam techniques, contentbased technique and connection based technique. Contentbased technique use mail content for detecting spammail. Connection based technique doesn't use mail content,just use connected host, from address, recipient addressesand so on for detecting spam mail.

Connection based technique has good detection rate and morelower load than content based technique. But it will notwork well against spam mails that uses normal mail serverlike Gmail. But content based technique will be able todetect spam mails that can't be detected by connection basedtechnique.

Here is a mainstream anti-spam technique at 2008/12.

1. detects most spam mails by connection based technique.
2. detects spam mails that can't be detected by      connection based technique by content based technique.

There are some connection based techniques, Greylisting thatdenies delivered a mail at first and accepts onlyretransmitted mail, blacklist that uses blacklist databaseto detect spam and so on. Most commertial products use theirown blacklist database. Most free softwares use DNSBL thatuses DNS for accessing blacklistdatabase. [S25R](http://gabacho.reto.jp/en/anti-spam/)(Selective SMTP Rejection) doesn't use database, just usesome regular expressions.

If we use Greylisting and/or DNSBL, retransmission and/ornetwork connection are occurred. They are low load butoccurs deliver delay and/or long processing time. If adelivered mail is not spam clearly, (e.g. submitted fromlocalhost, submitted by authorized user and so on) we canskip those techniques to avoid delay.

Here is an example to use those techniques with using itsadvantages and covering its weaknesses:

1. detects spam by S25R
   1. spam: detects spam by DNSBL
      1. spam: detects spam by Greylisting
         1. retransmitted: not spam
         2. not retransmitted: spam
      2. not spam: skip Greylisting
   2. not spam: skip both DNSBL and Greylisting

In the above example, Greylisting is apllied only if bothS25R and DNSBL detects spam. It reduces false delay byGreylisting. It also uses Greylisting for maintaining S25R'swhitelist. S25R has a weakness that needes to maintainwhitelist by hand but it is covered by combined withGreylisting.

As mentioned above, we can create effective anti-spamtechnique by combine each techniques.

### MTA configuration problems

Many anti-spam techniques are implemented as milter. How toconfigure Sendmail or Postfix to use milters effectively.

Both Sendmail and Postfix apply registered milters to allmails. We cannot apply milters only if some condition istrue as mentioned above. FIXME

Sendmail can specify default action and timeout for eachmilter but Postfix can specify them only for all milters.

milter is useful because there are many implementation anduses with other MTA. But we can't use it effectively becauseMTA doesn't support flexible milter apply configuration.

milter manager is a free software to use milter's advantageseffectively.

### milter manager

milter manager is a milter that manages multiple milters.We can register multiple milters to milter managers anda milter session for milter manager is transferred toregistered milters. Registered milter is called "childmilter".

milter manager works as a proxy. milter manager looks like amilter from MTA side. milter manager looks like an MTA fromchild milter.

MTA, milter manager and child milter

milters can be managed by milter manager layer not MTA layerby the structure. milter manager has the following featuresthat improve milter management:

1. milter detection feature
2. flexible milter apply feature

The former is for "reduce milter administration cost" advantage,the the latter is for "combine milters flexibly" advantage.

We can use milters effectively by milter manager's thosefeatures.

#### Advantage: milter detection feature

milter manager embeds Ruby interpreter. Ruby is a realprogramming language that provides easy to read syntax andflexibility.

milter manager can configure milters more flexible thanexisting MTA because milter manager's configuration file isprocessed as a Ruby script. For example, milter manager candetect milters installed in your system and registerit. It means that you can change each milter's configurationwithout updating milter manager's configuration.

Currently, Ubuntu (Debian) and FreeBSD are supported. If youwant to use milters installed by package system (dpkg orports), you don't need to change milter manager'sconfiguration. If a milter is installed by package systemand enabled, milter manager detects and uses it. SeeInstall for more information.

There is a opinion that milter manager's configuration filehas more difficult syntax than MTA's configuration file. Asmentioned above, Ruby has easy to read syntax, it's notdifficult in normal use. FIXME

For example, here is a configuration for connection socket:

<pre>manager.connection_spec = "inet:10025@localhost"</pre>

It's almost same as Postfix's configuration syntax.("item = value")

milter manager will also provide Web interface forconfiguration since the next release.

#### Advantage: flexible milter apply feature

In MTA configuration, each milter always applies or not. Itcan't be done that a milter applies only when someconditions are true. milter manager has some check points inmilter session. They can be used for it.

We can decide whether apply a milter or not by using S25Rresult by the feature.

#### Speed

We have effect about performance by introducing miltermanager. But the effect is very small because milter managerworks fast enough. So, it seems that the effect isnone. milter manager will not be bottleneck.

Registered child milters to milter manager are applied onlyif some conditions are true. It means that child miltersdoesn't run if they aren't needed. But registered milters toMTA are always ran. Total processing time for milter systemis almost same as milter system without milter manager orless than milter system without milter manager because thenumber of child milters to be ran are less than the numberof whole milters.

## Conclusion

* there is milter that introduces mail filter to MTA
* anti-spam and anti-virus can be implemented as milter
* Sendmail and Postfix support milter
* anti-spam techniques have advantages and weaknesses
* to use anti-spam techniques effectively, we need to    combine some techniques
* milter has advantages and weaknesses
  * advantages:
    * can be used with other MTA
    * there are many implementations
  * weakness:
    * it's hard to maintain configuration of MTA and        milter because configurations for MTA and milter are        separated
    * MTA doesn't provide configuration for combining        milters effectively
* we can use milters effectively and cover milters'    weakness by using milter manager


