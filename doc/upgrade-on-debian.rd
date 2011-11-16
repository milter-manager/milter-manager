# -*- rd -*-

= Upgrade on Debian --- How to upgrade milter manager on Debian GNU/Linux

== About this document

This document describes how to upgrade milter manager on
Debian GNU/Linux. See ((<Install to
Debian|install-to-debian.rd>)) for newly install
information.

== Upgrade

We just upgrade milter manager package.

  % sudo aptitude -V -D -y safe-upgrade

=== Upgrade from before 1.8.0

We have changed the GPG key to sign packages on 2011-11-15.
Please import new GPG key.

  % sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1BD22CD1

We have changed source.list URI. We are going to delete old URI
support, when we release milter manager 1.9.0.

Please update your source.list as soon as possible.

==== For squeeze

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/debian/stable/ squeeze main
  deb-src http://downloads.sourceforge.net/project/milter-manager/debian/stable/ squeeze main
  # deb http://downloads.sourceforge.net/project/milter-manager/debian/development/ squeeze main
  # deb-src http://downloads.sourceforge.net/project/milter-manager/debian/development/ squeeze main

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

==== For wheezy

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/milter-manager/debian/stable/ wheezy main
  deb-src http://downloads.sourceforge.net/project/milter-manager/debian/stable/ wheezy main
  # deb http://downloads.sourceforge.net/project/milter-manager/debian/development/ wheezy main
  # deb-src http://downloads.sourceforge.net/project/milter-manager/debian/development/ wheezy main

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

==== For sid

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/debian/stable/ unstable main
  deb-src http://downloads.sourceforge.net/project/milter-manager/debian/stable/ unstable main
  # deb http://downloads.sourceforge.net/project/milter-manager/debian/development/ unstable main
  # deb-src http://downloads.sourceforge.net/project/milter-manager/debian/development/ unstable main

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on Debian
(optional)|upgrade-options-on-debian.rd>)).
