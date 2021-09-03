# milter manager: a milter to use milters effectively.

## Site

https://milter-manager.osdn.jp/

[![](https://img.shields.io/badge/deb-packagecloud.io-844fec.svg)](https://6cbde467-a8b8-40b8-bdfd-10f10edab167.pipedrive.email/c/xyol6og7ym/my797q63d4/6pk6g6ex40/0?redirectUrl=http%3A%2F%2Fpackagecloud.io%2F)
[![](https://img.shields.io/badge/rpm-packagecloud.io-844fec.svg)](https://6cbde467-a8b8-40b8-bdfd-10f10edab167.pipedrive.email/c/xyol6og7ym/my797q63d4/6pk6g6ex40/0?redirectUrl=http%3A%2F%2Fpackagecloud.io%2F)

## License

Use the following rules:

  * Files that describe their license: their license
  * Commands: GPL3(license/gpl.txt)
  * Documents: GFDL(license/fdl.txt)
  * Images: Public Domain
  * Libraries: LGPL3(license/lgpl.txt)

Here is a concrete list:

  * Files that describe their license:
    * milter/core/milter-memory-profile.c: LGPL2+
  * Commands: GPL3
    * src/*
    * tool/*
  * Documents: GFDL
    * README, README.ja
    * doc/*
  * Images: Public Domain
    * doc/*.svg, doc/*.png, html/*.svg, html/*.png
  * Libraries: LGPL3
    * others than those above

## milter manager

milter manager is a milter to use multiple milters
effectively.

If milter manager is introduced, milter manager
administrates milters instead of MTA. The has some
advantages:

  * reduce milter administration cost
  * combine milters flexibly

See [Introduction](doc/introduction.rd) for more details.

### Dependencies

  * MTA that supports milter
    * Sendmail >= 8.13.8
    * Postfix >= 2.3.3
  * GLib >= 2.12.3
  * Ruby >= 1.8.5
  * UN*X OS
    * Linux（Debian GNU/Linux, Ubuntu, CentOS）
    * FreeBSD, NetBSD
    * Solaris

### Optional dependencies

  * [Cutter: unit testing framework for C](http://cutter.sourceforge.net/)
    >= 1.0.6

    It is needed for 'make check' and 'make coverage'.

  * LCOV: graphical front-end for GCC's coverage testing tool gcov

    It is needed for 'make coverage'.

    [LCOV - the LTP GCOV extension](http://ltp.sourceforge.net/coverage/lcov.php)

  * RRDtool (It's better that bundled Ruby bindings are also installed)

    It is needed for milter-manager-log-analyzer.

    [RRDtool](http://oss.oetiker.ch/rrdtool/)

### Install

See [Install](doc/install.rd)

### Configuration

See [Configuration](doc/configuration.rd)

## Usage

milter-manager command is installed into sbin/ not
bin/. In most cases, normal user doesn't include sbin/ in
PATH. You will need to use absolute path.

If you don't specify --prefix option for configure script,
milter-manager is installed into /usr/local/sbin/. You can
run milter-manager like the following:

  % /usr/local/sbin/milter-manager --help

Available options are shown if installation is succeeded.
See [milter-manager](doc/milter-manager.rd) for more details.

## Tools

milter manager includes some useful tools. They are
installed into bin/.

  * [milter-test-server](doc/milter-test-server.rd): It
    talks MTA side milter protocol. It can be used for
    testing a milter without MTA.
  * [milter-test-client](doc/milter-test-client.rd): It
    is a milter that just shows received data from MTA. It
    can be used for confirming what data is sent from MTA.
  * [milter-performance-check](doc/milter-performance-check.rd):
    It is a SMTP client that measures MTA performance.
  * [milter-manager-log-analyzer](doc/milter-manager-log-analyzer.rd):
    It analyzes log of milter-manager and visualizes
    behavior of milters registered to milter-manager.

## Mailing list

There is
[milter-manager-users-en](http://lists.osdn.me/mailman/listinfo/milter-manager-users-en)
mailing list. Questions and bug reports are accepted on
it. New release announce is also done on the mailing
list. If you are using milter manager, it's a good idea that
you subscribe the mailling list.

Old: [milter-manager-users-en](https://lists.sourceforge.net/lists/listinfo/milter-manager-users-en)

## Source code

The latest source is available from the Git repository:

  % git clone https://github.com/milter-manager/milter-manager.git

## Thanks

  * OBATA Akio: reported a bug.
  * Павел Гришин: reported bugs.
  * Fumihisa Tonaka:
    * reported bugs.
    * suggested new features.
  * sgyk: reported bugs.
  * Tsuchiya: reported bugs.
  * Syunsuke Komma: reported a bug.
  * Yuto Hayamizu:
    * cleaned test.
    * added useful features for creating milter by Ruby.
  * gorimaru: suggested usage improvements.
  * ZnZ:
    * fixed typos.
    * reported useful advises.
  * Antuan Avdioukhine: suggestions.
  * SATOH Fumiyasu:
    * reported bugs.
    * improved Solaris support.
  * ROSSO: reported a bug.
  * akira yamada:
    * reported a bug.
    * suggested about Debian package.
  * Kenji Shiono:
    * reported bugs.
    * suggested new features.
  * Jordao:
    * reported a bug.
  * Mitsuru Ogino:
    * reported bugs.
  * moto kawasaki:
    * added /etc/rc.conf.local loading support on FreeBSD.

milter-manager packages are distributed via packagecloud.io

[![Private NPM registry and Maven, RPM, DEB, PyPi and RubyGem Repository · packagecloud](https://packagecloud.io/images/packagecloud-badge.png)](https://packagecloud.io/)
