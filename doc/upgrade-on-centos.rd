# -*- rd -*-

= Upgrade on CentOS --- How to upgrade milter manager on CentOS

== About this document

This document describes how to upgrade milter manager on
CentOS. See ((<Install to CentOS|install-to-centos.rd>)) for
newly install information.

== Upgrade

We just upgrade milter manager package.
  % sudo yum update -y milter-manager

=== Upgrade from before 2.1.0

We have provided packages on
((<packagecloud|URL:https://packagecloud.io/milter-manager/repos>))
since 2.1.0. So we need to update repository information. We can
remove milter-manager-release package.

  % sudo yum remove milter-manager-release

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on CentOS
(optional)|upgrade-options-on-centos.rd>)).
