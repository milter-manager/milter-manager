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
  load_if_exist("milter-manager.local.conf")

Normally, the part should not be changed. If you need to
change your configuration, you should write your
configurations to "milter-manager.local.conf" file in the
same directory of "milter-manager.conf".

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
  manager.maintenance_interval = 100
  manager.suspend_time_on_unacceptable = 5
  manager.max_connections = 0
  manager.custom_configuration_directory = nil

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

: manager.maintenance_interval

   Specifies maintenance process is ran after each N sessions.

   For now, the maintenance process is memory release
   process.

   It's a good choice that maintenance process is ran
   after each few sessions on few concurrently access
   environment. The configuration will keep memory usage
   low.

   It's a good choice that maintenance process is ran
   after each many sessions on many concurrently access
   environment. The configuration will keep process
   efficiency.

   0 or nil means maintenance process is never ran.

   Example:
     manager.maintenance_interval = nil

   Default:
     manager.maintenance_interval = 100

: manager.suspend_time_on_unacceptable

   Specifies how many seconds are suspended on
   milter-manager can't accept a connection from MTA. In the
   case, milter-manager has many connections. Please
   consider that you increment number of openable file
   descriptors by ulimit or limit in the case.

   Example:
     manager.suspend_time_on_unacceptable = 10
   Default:
     manager.suspend_time_on_unacceptable = 5

: manager.max_connections

   Since 1.3.1.

   Specifies max concurrent connections. 0 means no
   limit. It's the default value.

   A newly connection is waited until an existing connection
   is closed when there are max concurrent
   connections. Number of the current existing connections
   are confirmed each
   ((<manager.suspend_time_on_unacceptable|.#manager.suspend_time_on_unacceptable>))
   seconds.

   Example:
     manager.max_connections = 10 # accepts only 10 connections concurrency

   Default:
     manager.max_connections = 5

: manager.max_file_descriptors

   Since 1.3.1.

   Specifies max number of file descriptors that can be
   opened by a milter-manager process. 0 means that the
   system default is used. The system is used by default
   because the default value is 0.

   milter-manager opens "number of child milters + 1 (for
   MTA)" file descriptors for each
   connection. milter-manager also opens a few file
   descriptors for its internal usage. milter-manager should
   be able to open number of file descriptors computed by
   the following expression:
     (number of child milters + 1) * max concurrent connections +
       10（milter-manager internal usage + alpha）

   This value is used as soft limit by setrlimit(2).

   Example:
     manager.max_file_descriptors = 65535

   Default:
     manager.max_file_descriptors = 0

: manager.custom_configuration_directory

   Specifies a directory to save custom configuration via Web
   interface.

   Directory name is specified like
   "/tmp/milter-manager". Directory name should be
   surround with '"' (double quote).

   If you specify 'nil', milter-manager creates
   ".milter-manager" directory under effective user's home
   directory and use it for custom configuration directory.

   Example:
     manager.custom_configuration_directory = "/tmp/milter-manager/"

   Default:
     manager.custom_configuration_directory = nil

== Controller

: controller.connection_spec

   Specifies a socket that milter-manager accepts connection
   for controlling milter-manager.

   Format is same as manager.connection_spec.

   Example:
     controller.connection_spec = "inet:10026@localhost"

   Default:
     controller.connection_spec = nil

: controller.unix_socket_mode

   Specifies permission of UNIX domain socket for
   controlling milter-manager. This is used only when UNIX
   domain socket is specified for controller.connection_spec.

   Don't forget to prepend '0' to specify permission as
   octal notation.

   Example:
     controller.unix_socket_mode = 0600

   Default:
     controller.unix_socket_mode = 0660

: controller.remove_unix_socket_on_create

   Specifies whether remove existing UNIX domain socket
   for controlling milter-manager before creating new UNIX
   domain socket or not. This is used only when UNIX domain
   socket is specified for controller.connection_spec.

   True if remove, false otherwise.

   Example:
     controller.remove_unix_socket_on_create = false

   Default:
     controller.remove_unix_socket_on_create = true

: controller.remove_unix_socket_on_close

   Specifies whether remove used UNIX domain socket for
   controlling milter-manager after shutting down. This is
   used only when UNIX domain socket is specified for
   controller.connection_spec.

   True if remove, false otherwise.

   Example:
     controller.remove_unix_socket_on_close = false

   Default:
     controller.remove_unix_socket_on_close = true

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

: milter.fallback_status

   Specifies a status that is used the child milter causes
   an error.

   Here is available values:

     * "accept": Accepts a mail. It's the default.
     * "temporary-failure": Rejects a mail temporary.
     * "reject": Rejects a mail.
     * "discard": Discards a mail.

   Example:
     milter.fallback_status = "temporary-failure"

   Default:
     milter.fallback_status = "accept"

: milter.evaluation_mode

   Since 1.3.1.

   Whether turn on evaluation mode or not. The child milter
   doesn't return its result on evaluation mode. It means
   the child milter doesn't affect the existing mail system.

   Graphs are still generated on evaluation mode because
   statistics are logged.

   True if evaluation mode is turned on, false otherwise.

   On false (default) case, message handling in milter
   session is aborted when a child milter returns "reject"
   because milter manager returns "reject" to MTA. On true
   case, message handling in milter session is continued
   when a child milter returns "reject" because milter
   manager doesn't return "reject" to MTA. We can use a
   condition that "a child milter returns 'reject'" in
   applicable conditions.

   Example:
     milter.evaluation_mode = true

   Default:
     milter.evaluation_mode = false

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
   applicable condition. See
   ((<Applicable condition|#applicable-condition>))
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

  defined_milters.each do |name|
    define_milter(name) do |milter|
      milter.enabled = false
    end
  end

Here is an example that removing all child milters:

  defined_milters.each do |name|
    remove_milter(name)
  end

Removing differs from disabling. You need to redefine a
milter again when you want to use a removed milter again.

Here is an example that adding "S25R" applicable condition
to all child milters.

  defined_milters.each do |name|
    define_milter(name) do |milter|
      milter.add_applicable_condition("S25R")
    end
  end


: defined_milters
   Returns a list of defined child milter names. Returned
   value is an array of string.

   Example:
     defined_milters # => ["milter1", "milter2"]

: remove_milter(name)
   Removes a child milter that is named as 'name'. You
   should use ((<milter.enabled|.#milter.enabled>)) when you
   may reuse the child milter.

   Example:
     # Removes a milter that is defined as "milter1".
     remove_milter("milter1")

== [applicable-condition] Applicable condition

We need knowledge about Ruby from this section. Some useful
applicable conditions are provided by default. We can define
new applicable conditions if we need more conditions. We can
decide which child milter is applied or not dynamically by
defining our applicable conditions.

We define an applicable condition with the following
syntax. We need knowledge about Ruby for defining applicable
condition.

  define_applicable_condition("NAME") do |condition|
    condition.description = ...
    condition.define_connect_stopper do |...|
      ...
    end
    ...
  end

For example, here is an applicable condition to implement
S25R:

  define_applicable_condition("S25R") do |condition|
    condition.description = "Selective SMTP Rejection"

    condition.define_connect_stopper do |context, host, socket_address|
      case host
      when "unknown",
        /\A\[.+\]\z/,
        /\A[^.]*\d[^\d.]+\d.*\./,
        /\A[^.]*\d{5}/,
        /\A(?:[^.]+\.)?\d[^.]*\.[^.]+\..+\.[a-z]/i,
        /\A[^.]*\d\.[^.]*\d-\d/,
        /\A[^.]*\d\.[^.]*\d\.[^.]+\..+\./,
        /\A(?:dhcp|dialup|ppp|[achrsvx]?dsl)[^.]*\d/i
        false
      else
        true
      end
    end
  end

'host' is "[IP ADDRESS]" not "unknown" when name resolution
is failed. So, "unknown" is needless. It's sufficient that
we just use /\A\[.+\]\z/. But it is included just in case. :-)

Here are configurable items in 'define_applicable_condition
do ... end'.

There is no required item.

: condition.description

   Specifies description of the applicable condition.

   Description is specified like "test condition". Description is
   surrounded with '"' (double quote).

   Example:
     condition.description = "test condition"

   Default:
     condition.description = nil

: condition.define_connect_stopper {|context, host, socket_address| ...}

   Decides whether the child milter is applied or not with
   host name and IP address of connected SMTP client. The
   available information is same as milter's xxfi_connect.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : host
      The host name (string) of connected SMTP client. It is
      got by resolving connected IP address. It may be "[IP
      ADDRESS]" string when name resolution is failed. For
      example, "[1.2.3.4]".

   : socket_address
      The object that describes connected IP
      address. Details are said later.

   Here is an example that we stop the child milter when
   SMTP client is connected from resolvable host:

     condition.define_connect_stopper do |context, host, socket_address|
       if /\A\[.+\]\z/ =~ host
         false
       else
         true
       end
     end

: condition.define_helo_stopper {|context, fqdn| ...}

   Decides whether the child milter is applied or not with
   FQDN on HELO/EHLO command. The available information is
   same as milter's xxfi_helo.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : fqdn
      The FQDN (string) sent by SMTP client on HELO/EHLO.

   Here is an example that we stop the child milter when
   FQDN is "localhost.localdomain".

     condition.define_helo_stopper do |context, helo|
       helo == "localhost.localdomain"
     end

: define_envelope_from_stopper {|context, from| ...}

   Decides whether the child milter is applied or not with
   envelope from address passed on MAIL FROM command of
   SMTP. The available information is same as milter's
   xxfi_envfrom.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : from
      The envelope from address passed on MAIL FROM command.
      For example, "<sender@example.com>".

   Here is an example that we stop the child milter when
   mails are sent from example.com.

     condition.define_envelope_from_stopper do |context, from|
       if /@example.com>\z/ =~ from
         true
       else
         false
       end
     end

: define_envelope_recipient_stopper {|context, recipient| ...}

   Decides whether the child milter is applied or not with
   envelope recipient address passed on RCPT TO command of
   SMTP. The available information is same as milter's
   xxfi_envrcpt. This callback is called one or more times
   when there are multiple recipients.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : recipient
      The envelope recipient address passed on RCPT TO command.
      For example, "<receiver@example.com>".

   Here is an example that we stop the child milter when
   mails are sent to ml.example.com.

     condition.define_envelope_recipient_stopper do |context, recipient|
       if /@ml.example.com>\z/ =~ recipient
         true
       else
         false
       end
     end

: condition.define_data_stopper {|context| ...}

   Decides whether the child milter is applied or not on
   DATA. The available information is same as milter's
   xxfi_data.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that we stop the child milter on DATA.
   milter can only add/delete/modify header and/or body
   after whole message is processed. If we stop the child
   milter on DATA, we ensure that milter don't
   add/delete/modify header and/or body. We can confirm
   the child milter's work if the child milter logs its
   processed result.

     condition.define_data_stopper do |context|
       true
     end

: define_header_stopper {|context, name, value| ...}

   Decides whether the child milter is applied or not with
   header information. The available information is same as
   milter's xxfi_header. This callback is called for each
   header.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : name
      The header name. For example, "From".

   : value
      The header value. For example, "sender@example.com".

   Here is an example that we stop the child milter when
   mails have a header that name is "X-Spam-Flag" and value
   is "YES".

     condition.define_header_stopper do |context, name, value|
       if ["X-Spam-Flag", "YES"] == [name, value]
         true
       else
         false
       end
     end

: condition.define_end_of_header_stopper {|context| ...}

   Decides whether the child milter is applied or not after
   all headers are processed. The available information is
   same as milter's xxfi_eoh.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that we stop the child milter after all
   headers are processed.

     condition.define_end_of_header_stopper do |context|
       true
     end

: condition.define_body_stopper {|context, chunk| ...}

   Decides whether the child milter is applied or not with
   a body chunk. The available information is same as
   milter's xxfi_body. This callback may be called multiple
   times for a large body mail.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that we stop the child milter after all
   headers are processed.

   : chunk
      A chunk of body. A large body is not processed at a
      time. It is processed as small chunks. The maximum
      chunk size is 65535 byte.

   Here is an example that we stop the child milter when
   chunk contains PGP signature.

     condition.define_body_stopper do |context, chunk|
       if /^-----BEGIN PGP SIGNATURE-----$/ =~ chunk
         true
       else
         false
       end
     end

: condition.define_end_of_message_stopper {|context| ...}

   Decides whether the child milter is applied or not after
   a mail is processed. The available information is
   same as  milter's xxfi_eom.

   It returns true if we stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that we stop the child milter after
   a mail is processed.

     condition.define_end_of_message_stopper do |context|
       true
     end

=== context

The object that has several information when we decide
whether a child milter is applied or not. (The class of
context is Milter::Manager::ChildContext.)

It has the following information.

: context.name

   Returns the name of child milter. It's name used when
   define_milter.

   Example:
     context.name # -> "clamav-milter"

: context[name]

   Returns value of available macro in the child milter In
   libmilter API, we need to surround macro name that has
   two or more length with "{}" but it isn't
   required. context[name] works well with/without "{}".

   Example:
     context["j"] # -> "mail.example.com"
     context["rcpt_address"] # -> "receiver@example.com"
     context["{rcpt_address}"] # -> "receiver@example.com"

: context.reject?

   Returns true if the child milter returns 'reject'.
   The child milter must be enabled
   ((<milter.evaluation_mode|.#milter.evaluation_mode>)).

   Passed context should be always processing. So,
   context.reject? never return true. It's usuful when we
   use the other child milter's result. The other child can
   be retrieved by context.children[].

   Example:
     context.reject? # -> false
     context.children["milter-greylist"].reject? # -> true or false

: context.temporary_failure?

   Returns true if the child milter returns 'temporary failure'.
   The child milter must be enabled
   ((<milter.evaluation_mode|.#milter.evaluation_mode>)).

   Passed context should be always processing. So,
   context.temporay_failure? never return true. It's usuful
   when we use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.temporary_failure? # -> false
     context.children["milter-greylist"].temporary_failure? # -> true or false

: context.accept?

   Returns true if the child milter returns 'accept'.

   Passed context should be always processing. So,
   context.accept? never return true. It's usuful
   when we use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.accept? # -> false
     context.children["milter-greylist"].accept? # -> true or false

: context.discard?

   Returns true if the child milter returns 'discard'.
   The child milter must be enabled
   ((<milter.evaluation_mode|.#milter.evaluation_mode>)).

   Passed context should be always processing. So,
   context.discard? never return true. It's usuful
   when we use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.discard? # -> false
     context.children["milter-greylist"].discard? # -> true or false

: context.quitted?

   Returns true if the child milter was quitted.

   Passed context should be always processing. So,
   context.quitted? never return true. It's usuful
   when we use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.quitted? # -> false
     context.children["milter-greylist"].quitted? # -> true or false

: context.children[name]

   Returnes the other child milter's context.

   The name to refere the other child milter is a name that
   is used for define_milter. (i.e. the name returned by
   context.name)

   It returns nil if we refer with nonexistent name.

   Example:
     context.children["milter-greylist"] # -> milter-greylist's context
     context.children["nonexistent"]     # -> nil
     context.children[context.name]      # -> my context

: context.postfix?

   Returns true when MTA is Postfix. It is decided by "v"
   macro's value. If the value includes "Postfix", MTA will
   be Postfix.

   Returns true when MTA is Postfix, false otherwise.

   Example:
     context["v"]     # -> "Postfix 2.5.5"
     context.postfix? # -> true

     context["v"]     # -> "2.5.5"
     context.postfix? # -> false

     context["v"]     # -> nil
     context.postfix? # -> false

: context.authenticated?

   Returns true when sender is authenticated. It is decided by
   "auto_type" macro or "auth_authen" macro is
   available. They are available since MAIL FROM. So, it
   always returns false before MAIL FROM. We don't forget to
   add the following configuration to main.cf if we are
   using Postfix.

     milter_mail_macros = {auth_author} {auth_type} {auth_authen}

   It returns true when sender is authenticated, false otherwise.

   Example:
     context["auth_type"]   # -> nil
     context["auth_authen"] # -> nil
     context.authenticated? # -> false

     context["auth_type"]   # -> "CRAM-MD5"
     context["auth_authen"] # -> nil
     context.authenticated? # -> true

     context["auth_type"]   # -> nil
     context["auth_authen"] # -> "sender"
     context.authenticated? # -> true

=== socket_address

The object that describes socket address. Socket is one of
IPv4 socket, IPv6 socket and UNIX domain socket. Socket
address is described as corresponding object.

==== Milter::SocketAddress::IPv4

It describes IPv4 socket address. It has the following methods.

: address
   Returns dot-notation IPv4 address.

   Example:
     socket_address.address # -> "192.168.1.1"

: port
   Returns port number.

   Example:
     socket_address.port # -> 12345

: to_s
   Returns IPv4 address formated as connection_spec format.

   Example:
     socket_address.to_s # -> "inet:12345@[192.168.1.1]"

: local?
   Returns true if the address is private network address,
   false otherwise.

   Example:
     socket_address.to_s   # -> "inet:12345@[127.0.0.1]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet:12345@[192.168.1.1]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet:12345@[160.XXX.XXX.XXX]"
     socket_address.local? # -> false

==== Milter::SocketAddress::IPv6

It describes IPv6 socket address. It has the following methods.

: address
   Returns colon-notation IPv6 address.

   Example:
     socket_address.address # -> "::1"

: port
   Returns port number.

   Example:
     socket_address.port # -> 12345

: to_s
   Returns IPv6 address formated as connection_spec format.

   Example:
     socket_address.to_s # -> "inet6:12345@[::1]"

: local?
   Returns true if the address is private network address,
   false otherwise.

   Example:
     socket_address.to_s   # -> "inet6:12345@[::1]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet6:12345@[fe80::XXXX]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet6:12345@[2001::XXXX]"
     socket_address.local? # -> false

==== Milter::SocketAddress::Unix

It describes UNIX domain ssocket address. It has the following methods.

: path
   Returns path of the socket.

   Example:
     socket_address.path # -> "/tmp/local.sock"

: to_s
   Returns UNIX domain socket address formated as
   connection_spec format.

   Exampel:
     socket_address.to_s # -> "unix:/tmp/local.sock"

: local?
   Always returns true.

   Example:
     socket_address.local? # -> true
