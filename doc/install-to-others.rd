# -*- rd -*-

= Install to other platforms --- How to install milter manager to other platforms

== About this document

This document describes some hints to install milter manager
to platforms except ((<Ubuntu Linux|install-to-ubuntu.rd>)),
((<CentOS|install-to-centos.rd>)) and
((<FreeBSD|install-to-freebsd.rd>)). See
((<Install|install.rd>)) for platform independent install
information.

== Package platform detection

milter-manager detects milters installed in your system
automatically. It's decided how to detect installed milters
on 'configure'. 'configure' outputs detected "Package
Platform" information at the last:

  % ./configure
  ...
  Configure Result:

    Package Platform : debian
    Package Options  :
  ...

In the above example, milter-manager uses auto-detect method
for "debian" platform.

"Package Platform" is "unknown" when "configure" fails to
detect your platform or milter-manager doesn't support your
platform.

If "configure" fails to detect your platform, you can
specify platform by "--with-package-platform" option explicitly.

  % ./configure --with-package-platform=freebsd
  ...
  Configure Result:

    Package Platform : freebsd
    Package Options  :
  ...

To specify additional information, you can use
"--with-package-options" option. Additional information is
formated as "NAME1=VALUE1,NAME2=VALUE2,...". Here is an
example for using /etc/rc.d/ instead of /usr/pkg/etc/rc.d/
for run-script on "pkgsrc" platform:

  % ./configure --with-package-platform=pkgsrc --with-package-options=prefix=/etc
  ...
  Configure Result:

    Package Platform : pkgsrc
    Package Options  : prefix=/etc
  ...

For pkgsrc, --with-rcddir option is provided. You can use
the following options to use the same configuration as the
above:

  % ./configure --with-package-platform=pkgsrc --with-rcddir=/etc/rc.d
  ...
  Configure Result:

    Package Platform : pkgsrc
    Package Options  : rcddir=/etc/rc.d
  ...

Currently, "debian", "redhat", "freebsd" (and "pkgsrc") are
supported as platform. You can specify other platform.

For example, you want to support "suse" platform:

  % ./configure --with-package-platform=suse
  ...
  Configure Result:

    Package Platform : suse
    Package Options  :
  ...

Now, we use "suse" platform. We need to write milter detect
script for "suse" platform by Ruby. The script name is
"#{PLATFORM_NAME}.conf" and it is placed in
$prefix/etc/milter-manager/defaults/. In this case,
"#{PLATFORM_NAME}.conf" is "suse.conf"

$prefix/etc/milter-manager/defaults/suse.conf script detects
milters and registers them by
define_milter. ((<Configuration|configuration.rd>))
describes about how to register a milter.

You can also confirm the current package platform after
installing:

  % /usr/local/sbin/milter-manager --show-config
  ...
  package.platform = "debian"
  package.options = nil
  ...

In this case, platform is "debian" and additional
information is none.
