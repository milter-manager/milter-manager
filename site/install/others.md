---
title: Install to other platforms
---

# Install to other platforms --- How to install milter manager to other platforms

## About this document

This document describes some hints to install milter managerto platforms except Ubuntu Linux,CentOS andFreeBSD. SeeInstall for platform independent installinformation.

## Package platform detection

milter-manager detects milters installed in your systemautomatically. It's decided how to detect installed milterson 'configure'. 'configure' outputs detected "PackagePlatform" information at the last:

<pre>% ./configure
...
Configure Result:

  Package Platform : debian
  Package Options  :
...</pre>

In the above example, milter-manager uses auto-detect methodfor "debian" platform.

"Package Platform" is "unknown" when "configure" fails todetect your platform or milter-manager doesn't support yourplatform.

If "configure" fails to detect your platform, you canspecify platform by "--with-package-platform" option explicitly.

<pre>% ./configure --with-package-platform=freebsd
...
Configure Result:

  Package Platform : freebsd
  Package Options  :
...</pre>

To specify additional information, you can use"--with-package-options" option. Additional information isformated as "NAME1=VALUE1,NAME2=VALUE2,...". Here is anexample for using /etc/rc.d/ instead of /usr/pkg/etc/rc.d/for run-script on "pkgsrc" platform:

<pre>% ./configure --with-package-platform=pkgsrc --with-package-options=prefix=/etc
...
Configure Result:

  Package Platform : pkgsrc
  Package Options  : prefix=/etc
...</pre>

For pkgsrc, --with-rcddir option is provided. You can usethe following options to use the same configuration as theabove:

<pre>% ./configure --with-package-platform=pkgsrc --with-rcddir=/etc/rc.d
...
Configure Result:

  Package Platform : pkgsrc
  Package Options  : rcddir=/etc/rc.d
...</pre>

Currently, "debian", "redhat", "freebsd" (and "pkgsrc") aresupported as platform. You can specify other platform.

For example, you want to support "suse" platform:

<pre>% ./configure --with-package-platform=suse
...
Configure Result:

  Package Platform : suse
  Package Options  :
...</pre>

Now, we use "suse" platform. We need to write milter detectscript for "suse" platform by Ruby. The script name is"#{PLATFORM_NAME}.conf" and it is placed in$prefix/etc/milter-manager/defaults/. In this case,"#{PLATFORM_NAME}.conf" is "suse.conf"

$prefix/etc/milter-manager/defaults/suse.conf script detectsmilters and registers them bydefine_milter. Configurationdescribes about how to register a milter.

You can also confirm the current package platform afterinstalling:

<pre>% /usr/local/sbin/milter-manager --show-config
...
package.platform = "debian"
package.options = nil
...</pre>

In this case, platform is "debian" and additionalinformation is none.


