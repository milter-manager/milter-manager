# -*- rd -*-

= Upgrade on Ubuntu --- How to upgrade milter manager on Ubuntu Linux

== About this document

This document describes how to upgrade milter manager on
Ubuntu Linux. See ((<Install to
Ubuntu|install-to-ubuntu.rd>)) for newly install
information.

== Upgrade

We just upgrade milter manager package.

  % sudo apt-get -V -y upgrade

=== Upgrade from before 1.8.0

We have changed the GPG key to sign packages on 2011-11-15.
Please import new GPG key.

  % sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1BD22CD1

We have changed sources.list URI. We are going to delete old URI
support, when we release milter manager 1.9.0.

Please update your source.list as soon as possible.

=== For Lucid Lynx

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ lucid universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ lucid universe
  # deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ lucid universe
  # deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ lucid universe

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

=== For Natty Narwhal

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ natty universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ natty universe
  # deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ natty universe
  # deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ natty universe

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

=== For Oneiric Ocelot

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ oneiric universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ oneiric universe
  # deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ oneiric universe
  # deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ oneiric universe

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

=== For Precise Pangolin

/etc/apt/sources.list.d/milter-manager.list:
  deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ precise universe
  deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/stable/ precise universe
  # deb http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ precise universe
  # deb-src http://downloads.sourceforge.net/project/milter-manager/ubuntu/development/ precise universe

If we use development series, we need to comment the first 2
lines out and enable comment outed the 2 lines.

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on Ubuntu
(optional)|upgrade-options-on-ubuntu.rd>)).
