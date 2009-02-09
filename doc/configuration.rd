# -*- rd -*-

= Configuration --- How to write milter-manager.conf

== About this document

This document describes how to write milter-manager.conf
that is milter-manager's configuration file.

== Place

It assumes that milter-manager is installed under
/usr/local/. If you specify --prefix=/usr/local option to
configure script or omit the option, milter-manager is
installed under /usr/local/.

In this case, milter-manager's configuration file is placed
at /usr/local/etc/milter-manager/milter-manager.conf.
If you have installed successfully, the file should exist.

== Summary

The beginning milter-manager.conf is the following:

  # -*- ruby -*-

  load("applicable-conditions/*.conf")
  load_default

Normally, the part should not be changed. If you need to
change your configuration, you should append your
configuration below the part.

Configuration items are categorized as the followings:

  * package
  * security
  * milter-manager
  * controller
  * applicable condition
  * child-milter

There is a convenient feature of milter-manager. It's
introduced before each items are described the below.

If you run milter-manager with --show-config option, the
current configuration is shown.

  % /usr/local/sbin/milter-manager --show-config
  package.platform = "debian"
  package.options = nil

  security.privilege_mode = false
  security.effective_user = nil
  security.effective_group = nil

  manager.connection_spec = nil
  manager.unix_socket_mode = 0660
  manager.unix_socket_group = nil
  manager.remove_unix_socket_on_create = true
  manager.remove_unix_socket_on_close = true
  manager.daemon = false
  manager.pid_file = nil

  controller.connection_spec = nil
  controller.unix_socket_mode = 0660
  controller.remove_unix_socket_on_create = true
  controller.remove_unix_socket_on_close = true

  define_applicable_condition("S25R") do |condition|
    condition.description = "Selective SMTP Rejection"
  end

  define_applicable_condition("Remote Network") do |condition|
    condition.description = "Check only remote network"
  end

You can confirm the current configuration.

The output format is the same as milter-manager.conf's
syntax. You can refer the output for writing
milter-manager.conf.

Here are descriptions of configuration items.

== Package

: package.platform

   ((*Normally, this item doesn't need to be changed.*))

   milter auto-detect method is different on each
   platform. The auto-detect method assumes that milters are
   installed with package system on its platform. If
   real platform and platform detected by milter-manager are
   different, the auto-detect method doesn't work well.

   Your platform is detected on building milter-manager. You
   can specify correct platform if the detected platform is
   wrong. You will use this item only when the detected
   platform is wrong but you can't build again.

   Here are supported platforms:

     * debian: for Debian series Linux like Debian
       GNU/Linux and Ubuntu Linux.
     * redhat: for RedHat series Linux like CentOS.
     * freebsd: for FreeBSD.
     * pkgsrc: for *BSD that use pkgsrc like NetBSD and
       DragonFly BSD.

   Platform name should be surround with '"' (double quote)
   like "debian".

   NOTE: This item should be change ((*before*)) load_default.

   Example:
     package.platform = "pkgsrc"

   Default:
     package.platform = "debian" # depend on your environment.

: package.options

   ((*Normally, this item doesn't need to be changed.*))

   This item is determined on building like package.platform.

   You can pass additional information to milter auto-detect
   method. The format of this item is
   "NAME1=VALUE1,NAME2=VALUE2,...". You can pass zero or
   more information.

   Currently, additional information is only used on
   "pkgsrc" platform. "pkgsrc" platform only uses
   "prefix=/PATH/TO/DIRECTORY/HAS/rc.d". For example, you
   need to specify "prefix=/etc" if you install start-script
   under /etc/rc.d/ directory.

   NOTE: This item should be change ((*before*)) load_default.

   Example:
     package.options = "prefix=/etc,name=value"

   Default:
     package.options = nil # depend on your environment.

== Security

: security.privilege_mode

   Specifies whether run as privilege mode or not. If you
   want to use child milter auto run feature, you need to
   enable the mode.

   It's set true for enabling the mode, false otherwise.

   Example:
     security.privilege_mode = true

   Default:
     security.privilege_mode = false

: security.effective_user

   Specifies effective user of milter-manager process. To switch
   effective user, you need to run milter-manager command as
   root user.

   Effective user is specified like "nobody". User name should
   be surround with '"' (double quote). If you don't want to
   specify user, use nil.

   Example:
     security.effective_user = "nobody"

   Default:
     security.effective_user = nil

: security.effective_group

   Specifies effective group of milter-manager process. To switch
   effective group, you need to run milter-manager command as
   root group.

   Effective group is specified like "nogroup". Group name should
   be surround with '"' (double quote). If you don't want to
   specify group, use nil.

   Example:
     security.effective_group = "nobody"

   Default:
     security.effective_group = nil

== milter-manager

: manager.connection_spec

   Specifies a socket that milter-manager accepts connection
   from MTA.

   Socket is specified like "inet:10025". Socket should be
   surround with '"' (double quote). Available socket
   formats are the following:

     * UNIX domain socket: unix:PATH
       * Example: unix:/var/run/milter/milter-manager.sock

     * IPv4 socket: inet:PORT
       * Example: inet:10025

     * IPv4 socket: inet:PORT@HOST
       * Example: inet:10025@localhost

     * IPv4 socket: inet:PORT@[ADDRESS]
       * Example: inet:10025@[127.0.0.1]

     * IPv6 socket: inet6:PORT
       * Example: inet6:10025

     * IPv6 socket: inet6:PORT@HOST
       * Example: inet6:10025@localhost

     * IPv6 socket: inet6:PORT@[ADDRESS]
       * Example: inet6:10025@[::1]

   If security.effective_user and/or
   security.effective_group are specified, UNIX domain
   socket is made with the authority.

   If HOST of IPv4 socket and IPv6 socket is omitted,
   milter-manager accepts connection from all network
   interface. If HOST is specified, milter-manager accepts
   connection from the address.

   Example:
     manager.connection_spec = "unix:/var/run/milter/milter-manager.sock"

   Default:
     manager.connection_spec = "inet:10025@[127.0.0.1]"

: manager.unix_socket_mode

   Specifies permission of UNIX domain socket that
   milter-manager uses for accepting
   connection. This is used only when UNIX domain socket is
   specified for manager.connection_spec.

   Don't forget to prepend '0' to specify permission as
   octal notation.

   Example:
    manager.unix_socket_mode = 0600

   Default:
    manager.unix_socket_mode = 0660

: manager.unix_socket_group

   Specifies group of UNIX domain socket that
   milter-manager uses for accepting
   connection. This is used only when UNIX domain socket is
   specified for manager.connection_spec.

   Socket's group is changed by chown(2) after creating a
   socket with
   security.effective_user/security.effective_group
   authority. Specified group should be one of the
   security.effective_user's supplementary groups.

   Group is specified like "nogroup". Group name should
   be surround with '"' (double quote). If you don't want to
   specify group, use nil.

   Example:
     manager.unix_socket_group = "nobody"

   Default:
     manager.unix_socket_group = nil

: manager.remove_unix_socket_on_create

   Specifies whether remove existing UNIX domain socket
   before creating new UNIX domain socket or not. This is
   used only when UNIX domain socket is specified for
   manager.connection_spec.

   True if remove, false otherwise.

   Example:
     manager.remove_unix_socket_on_create = false

   Default:
     manager.remove_unix_socket_on_create = true

: manager.remove_unix_socket_on_close

   Specifies whether remove used UNIX domain socket after
   shutting down. This is used only when UNIX domain socket
   is specified for manager.connection_spec.

   True if remove, false otherwise.

   Example:
     manager.remove_unix_socket_on_close = false

   Default:
     manager.remove_unix_socket_on_close = true

: manager.daemon

   Specifies whether run as daemon process or not. This item
   doesn't need to set in configuration file because this
   item can be overridden by --daemon command line option.

   True if run as daemon process, false otherwise.

   Example:
     manager.daemon = true

   Default:
     manager.daemon = false

: manager.pid_file

   Specifies file name where milter-manager saves its
   process ID.

   If security.effective_user and/or
   security.effective_group are specified, the file is wrote
   with the authority. You should take care permission of
   the file.

   File name is specified like
   "/var/run/milter/milter-manager.pid". File name should be
   surround with '"' (double quote). If you don't want to
   specify file name, use nil.

   Example:
     manager.pid_file = "/var/run/milter/milter-manager.pid"

   Default:
     manager.pid_file = nil

== Controller

FIXME

== Applicable condition

FIXME

== Child milter

This section describes about configuration items related
with child milter.

=== Define child milter

Child milter is registered as the following syntax:

  define_milter("NAME") do |milter|
    milter.XXX = ...
    milter.YYY = ...
    milter.ZZZ = ...
  end

For example, to register a milter that accepts connection at
'inet:10026@localhost' as 'test-milter':

  define_milter("test-milter") do |milter|
    milter.connection_spec = "inet:10026@localhost"
  end

The following items can be used in 'define_milter do ... end'.

Required item is just only milter.connection_spec.


: milter.connection_spec

   Specifies socket that the child milter accepts.
   This is ((*required item*)).

   Format is same as manager.connection_spec.

   Example:
     milter.connection_spec = "inet:10026@localhost"

   Default:
     milter.connection_spec = nil

: milter.description

   Specifies description of the child milter.

   Description is specified like "test milter". Description is
   surrounded with '"' (double quote).

   Example:
     milter.description = "test milter"

   Default:
     milter.description = nil

: milter.enabled

   Whether use the child milter or not.

   True if use, false otherwise.

   Example:
     milter.enabled = false

   Default:
     milter.enabled = true

: milter.applicable_conditions

   Specifies applicable conditions for the child milter. The
   child milter is stopped if any condition isn't satisfied.

   The following command shows available applicable
   conditions:

     % /usr/local/sbin/milter-manager --show-config | grep define_applicable_condition
     define_applicable_condition("S25R") do |condition|
     define_applicable_condition("Remote Network") do |condition|

   In the above case, "S25R" and "Remote Network" are
   available.

   Some applicable conditions are available in default
   configuration. You can also define your original
   applicable condition. See ((<Applicable condition>))
   about how to define applicable condition. But defining
   applicable condition requires Ruby's knowledge.

   To specify multiple applicable conditions, you can use
   ",":

     milter.applicable_conditions = ["S25R", "Remote Network"]

   Example:
     milter.applicable_conditions = ["S25R"]

   Default:
     milter.applicable_conditions = []

: milter.add_applicable_condition(name)

   Adds an applicable condition to the child milter. See
   milter.applicable_conditions about 'applicable condition'.

   Example:
     milter.add_applicable_condition("S25R")

: milter.command

   Specifies command that run the child milter.
   The child milter is ran by the command automatically when
   connecting milter.connection_spec is failed. It is
   enabled only if security.privilege_mode is true and
   milter-manager is ran.

   Command may be run script that is placed in
   /etc/init.d/ or /usr/local/etc/rc.d/.

   Command is specified like
   "/etc/init.d/milter-greylist". Command should be surround
   with '"' (double quote). If you don't want to run
   automatically, use nil.

   Example:
     milter.command = "/etc/init.d/milter-greylist"

   Default:
     milter.command = nil

: milter.command_options

   Specifies options to be passed to milter.command.

   Options are specified like "start". Options should be
   surround with '"' (double quote).  If some options are
   specified, use "--option1 --option2" or ["--option1",
   "--option2"].

   Example:
     milter.command_options = "start"
     milter.command_options = ["--option1", "--option2"]

   Default:
     milter.command_options = nil

: milter.user_name

   Specifies user name to run milter.command.

   User name is specified like "nobody". User name should be
   surround with '"' (double quote). If you want to run
   milter.command as root, use nil.

   Example:
     milter.user_name = "nobody"

   Default:
     milter.user_name = nil

: milter.connection_timeout

   Specifies timeout in seconds for trying to connect to
   child milter.

   Example:
     milter.connection_timeout = 60

   Default:
     milter.connection_timeout = 300.0

: milter.writing_timeout

   Specifies timeout in seconds for trying to write to
   child milter.

   Example:
     milter.writing_timeout = 15

   Default:
     milter.writing_timeout = 10.0

: milter.reading_timeout

   Specifies timeout in seconds for trying to read from
   child milter.

   Example:
     milter.reading_timeout = 15

   Default:
     milter.reading_timeout = 10.0

: milter.end_of_message_timeout

   Specifies timeout in seconds for trying to wait response
   of xxfi_eom() from child milter.

   Example:
     milter.end_of_message_timeout = 300

   Default:
     milter.end_of_message_timeout = 200.0

=== Operate child milter

There is convenient features to operate defined child
milters. But the features require knowledge about Ruby a
bit.

defined_milters returns a list of define child milter names.

  define_milter("milter1") do |milter|
    ...
  end

  define_milter("milter2") do |milter|
    ...
  end

  defined_milters # => ["milter1", "milter2"]

It's easy that changing configuration of all child milter by
this feature.

Here is an example that disabling all child milters:

  define_milters.each do |name|
    define_milter(name) do |milter|
      milter.enabled = false
    end
  end

Here is an example that adding "S25R" applicable condition
to all child milters.

  define_milters.each do |name|
    define_milter(name) do |milter|
      milter.add_applicable_condition("S25R")
    end
  end


: defined_milters
   Returns a list of defined child milter names. Returned
   value is an array of string.

   Example:
     defined_milters # => ["milter1", "milter2"]

