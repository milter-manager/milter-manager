= milter-manager / milter manager / milter manager's manual

== NAME

milter-manager - an effective anti-spam and anti-virus solution with milters

== SYNOPSIS

(({milter-manager})) [((*option ...*))]

== DESCRIPTION

milter-manager is a milter that provides an effective
anti-spam and anti-virus solution with milters.

milter-manager provides a platform to use milters
effectively and flexibly. milter-manager has embedded Ruby
interpreter that is used for dynamic milter applicable
condition. milter-manager can provide the platform by
embedded Ruby interpreter.

milter-manager reads its configuration file. The current
configuration can be confirmed by --show-config option:

  % milter-manager --show-config

milter-manager also provides other options that overrides
configurations specified in configuration file.

== Options

: --help

   Shows available options and exits.

: --connection-spec=SPEC

   Specifies a socket that accepts connections from
   MTA. SPEC should be formatted as one of the followings:

     * unix:PATH
     * inet:PORT
     * inet:PORT@HOST
     * inet:PORT@[ADDRESS]
     * inet6:POST
     * inet6:PORT@HOST
     * inet6:PORT@[ADDRESS]

   Examples:
     * unix:/var/run/milter/milter-manager.sock
     * inet:10025
     * inet:10025@localhost
     * inet:10025@[127.0.0.1]
     * inet6:10025
     * inet6:10025@localhost
     * inet6:10025@[::1]

   This option overrides "manager.connection_spec" in
   configuration file.

: --config-dir=DIRECTORY

    Specifies a directory that includes milter-manager's
    configuration file. milter-manager tries to load
    DIRECTORY/milter-manager.conf. If it isn't find,
    milter-manager loads milter-manager.conf in default
    directory.

: --pid-file=FILE

   Saves process ID of milter-manager to FILE.

   This option overrides "manager.pid_file" in configuration
   file.

: --user-name=NAME

   Runs milter-manager as NAME user.
   milter-manager should be started by root.

   This option overrides "security.effective_user" in
   configuration file.

: --group-name=NAME

   Runs milter-manager as NAME group.
   milter-manager should be started by root.

   This option overrides "security.effective_group" in
   configuration file.

: --socket-group-name=NAME

   Changes group of UNIX domain socket for accepting
   connection by milter-manager to NAME group. Specified
   group should be one of the effective user's supplementary
   groups.

   This option overrides "manager.unix_socket_group" in
   configuration file.

: --daemon

   Runs milter-manager as daemon process.

   This option overrides "manager.daemon" in configuration
   file.

: --no-daemon

   This option cancels the prior --daemon option.

: --show-config

   Shows the current configuration and exits. The output
   format can be used in configuration file. This option is
   useful for confirming registered milters and reporting your
   milter-manager's configuration when you report
   milter-manager's problems.

: --verbose

   Logs verbosely. Logs by syslog with "mail". If
   milter-manager isn't daemon process, standard output is
   also used.

   "MILTER_LOG_LEVEL=all" environment variable configuration
   has the same effect.

: --version

   Shows version and exits.

== EXIT STATUS

The exit status is 0 if milter starts to listen and non 0
otherwise. milter-manager can't start to listen when
connection spec is invalid format or other connection
specific problems. e.g. the port number is already used,
permission isn't granted for create UNIX domain socket and
so on.

== FILES

: /usr/local/etc/milter-manager/milter-manager.conf

   The default configuration file.

== EXAMPLE

The following example is good for debugging milter-manager
behavior. In the case, milter-manager works in the
foreground and logs are outputted to the standard output.

  % milter-manager --no-daemon --verbose

== SEE ALSO

((<milter-test-server.rd>))(1),
((<milter-test-client.rd>))(1),
((<milter-performance-check.rd>))(1),
((<milter-manager-log-analyzer.rd>))(1)
