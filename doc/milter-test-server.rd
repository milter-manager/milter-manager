= milter-test-server / milter manager / milter manager manual

== NAME

milter-test-server - MTA side milter protocol implemented program

== SYNOPSIS

(({milter-test-server})) [((*option ...*))]

== DESCRIPTION

milter-test-server talks MTA side milter protocol. It can
connect to a milter without MTA. For now, there is no
similar tool. It is useful to test milter not MTA +
milter. For example, it can be used for the following
situation:

  * milter's performance check
  * milter's operation check

milter-test-server can be used for simple performance check
because it shows elapsed time. You can confirm elapsed time
without MTA's processing time. You will find a problem of a
milter more easily because it doesn't depend on MTA.

If a milter changes headers and/or body, milter-test-server
can show changed message. It can be used for testing a
milter that may change headers and/or body. If it is used
with ((<unit testing frame like
Cutter|URL:http://cutter.sourceforge.net/>)), you can write
automated unit tests.

== Options

: --help

   Shows available options and exits.

: --connection-spec=SPEC

   Specifies a socket to connect to milter. SPEC should be
   formatted as one of the followings:

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

: --negotiate-version=VERSION

    Use VERSION as milter protocol version sent to milter.

    Default value is 8. The value is the same as Sendmail
    8.14's default value.

: --connect-host=HOST

    Uses HOST as connected host.

    The host name is passed to milter's xxfi_connect() callback.

: --connect-address=SPEC

    Uses SPEC as connected address. SPEC format is same as
    --connection-spec option's SPEC.

    The address is passed to milter's xxfi_connect() callback.

: --helo-fqdn=FQDN

    Uses FQDN for 'HELO/EHLO' SMTP command.

    The FQDN is passed to milter's xxfi_helo() callback.

: --from=FROM

    Uses FROM for 'MAIL FROM' SMTP command.

    The address is passed to milter's xxfi_envfrom() callback.

: --recipient=RECIPIENT

    Uses RECIPIENT for 'RCPT TO' SMTP command. If you want
    to use multiple recipients, specify --recipient option
    n-times.

    The address is passed to milter's xxfi_envrcpt()
    callback. xxfi_envrcpt() is called for each recipient.

: --header=NAME:VALUE

    Adds a header that names NAME and its value is VALUE. If
    you want to multiple headers, specify --header option
    n-times.

    The header is passed to milter's xxfi_header() callback.
    xxfi_header() is called for each header.

: --body=CHUNK

    Adds CHUNK as body chunk. If you want to multiple
    chunks, specify --body option n-times.

    The chunk is passed to milter's xxfi_body()
    callback. xxfi_body() is called for each chunk.

: --unknown=COMMAND

    Uses COMMAND as unknown SMTP command.

    The command is passed to milter's xxfi_unknown()
    callback. xxfi_unknown() is called between
    xxfi_envrcpt() and xxfi_data().

: --mail-file=PATH

    Uses file exists at PATH as mail content. If the file
    has 'From:' and/or 'To:', they are used for from and/or
    recipient addresses.

: --output-message

    Shows a message applied a milter. If you want to
    check milter's operation that may change header and/or
    body, specify this option.

: --verbose

   Logs verbosely.

   'MILTER_LOG_LEVEL=all' environment variable configuration
   has the same effect.

: --version

   Shows version and exits.

== EXIT STATUS

Normally, the exit status is 0 if milter session is started
and non 0 otherwise. milter session can't be started when
connection spec is invalid format or milter-test-server
can't connect to a milter.

== EXAMPLE

The following example talks with a milter that works on host
192.168.1.29 and is listened at 10025 port.

  % milter-test-server -s inet:10025@192.168.1.29

== SEE ALSO

((<milter-test-client.rd>))(1),
((<milter-performance-check.rd>))(1)
