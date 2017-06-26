# -*- rd -*-

= Upgrade on Debian --- How to upgrade milter manager on Debian GNU/Linux

== About this document

This document describes how to upgrade milter manager on
Debian GNU/Linux. See ((<Install to
Debian|install-to-debian.rd>)) for newly install
information.

== Upgrade

We just upgrade milter manager package.

  % sudo apt -y upgrade

=== Upgrade from before 2.1.0

We have provided packages on
((<packagecloud|URL:https://packagecloud.io/milter-manager/repos>))
since 2.1.0. So we need to update repository information.

We cannot provide packages for sid on packagecloud.

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on Debian
(optional)|upgrade-options-on-debian.rd>)).
