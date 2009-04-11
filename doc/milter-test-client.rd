= milter-test-client / milter manager / milter manager's manual

== NAME

milter-test-client - milter side milter protocol implemented program

== SYNOPSIS

(({milter-test-client})) [((*option ...*))]

== DESCRIPTION

milter-test-client is a milter that just shows received data
from MTA. It also shows macros received from MTA, it can be
used for confirming MTA's milter configuration.

Postfix's source archive includes similar tool.
It's src/milter/test-milter.c. It seems that it's used for
testing Postfix's milter implementation. But test-milter
doesn't show macros. If you have a milter that doesn't work
as you expect and uses macro, milter-test-client is useful
tool for looking into the problems.

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
     * unix:/tmp/milter-test-client.sock
     * inet:10025
     * inet:10025@localhost
     * inet:10025@[127.0.0.1]
     * inet6:10025
     * inet6:10025@localhost
     * inet6:10025@[::1]

: --verbose

   Logs verbosely.

   "MILTER_LOG_LEVEL=all" environment variable configuration
   has the same effect.

: --version

   Shows version and exits.

== EXIT STATUS

The exit status is 0 if milter starts to listen and non 0
otherwise. milter-test-client can't start to listen when
connection spec is invalid format or other connection
specific problems. e.g. the port number is already used,
permission isn't granted for create UNIX domain socket and
so on.

== EXAMPLE

The following example runs a milter which listens at 10025
port and waits a connection from MTA.

  % milter-test-client -s inet:10025

== SEE ALSO

((<milter-test-server.rd>))(1),
((<milter-performance-check.rd>))(1)
