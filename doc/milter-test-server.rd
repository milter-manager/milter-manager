= milter-test-server / milter manager / milter manager's manual

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
with ((<unit testing framework like
Cutter|URL:http://cutter.sourceforge.net/>)), you can write
automated unit tests.

== Options

: --help

   Shows available options and exits.

: --name=NAME

   Uses NAME as milter-test-server's name. The name is used
   as a value of "{daemon_name}" macro for example.

   The default value is "milter-test-server" that is the
   command file name.

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

    Uses VERSION as milter protocol version sent to milter.

    The default value is 8. The value is the same as Sendmail
    8.14's default value.

: --connect-host=HOST

    Uses HOST as connected host.

    The host name is passed to milter's xxfi_connect() callback.

: --connect-address=SPEC

    Uses SPEC as connected address. SPEC format is same as
    --connection-spec option's SPEC.

    The address is passed to milter's xxfi_connect() callback.

: --connect-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_connect() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "client_connections" name and "1" value is sent on
   xxfi_connect() callback:

     --connect-macro client_connections:1

: --helo-fqdn=FQDN

    Uses FQDN for 'HELO/EHLO' SMTP command.

    The FQDN is passed to milter's xxfi_helo() callback.

: --helo-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_helo() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "client_ptr" name and "unknown" value is sent on
   xxfi_helo() callback:

     --helo-macro client_ptr:unknown

: --envelope-from=FROM, -fFROM

    Uses FROM for 'MAIL FROM' SMTP command.

    The address is passed to milter's xxfi_envfrom() callback.

: --envelope-from-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_envfrom() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "client_addr" name and "192.168.0.1" value is sent on
   xxfi_envfrom() callback:

     --envelope-from-macro client_addr:192.168.0.1

: --envelope-recipient=RECIPIENT, -rRECIPIENT

    Uses RECIPIENT for 'RCPT TO' SMTP command. If you want
    to use multiple recipients, specify --envelope-recipient
    option n-times.

    The address is passed to milter's xxfi_envrcpt()
    callback. xxfi_envrcpt() is called for each recipient.

: --envelope-recipient-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_envrcpt() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "client_ptr" name and "2929" value is sent on
   xxfi_envrcpt() callback:

     --envelope-recipient-macro client_ptr:2929

: --data-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_data() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "client_name" name and "unknown" value is sent on
   xxfi_data() callback:

     --data-macro client_name:unknown

: --header=NAME:VALUE

    Adds a header that names NAME and its value is VALUE. If
    you want to multiple headers, specify --header option
    n-times.

    The header is passed to milter's xxfi_header() callback.
    xxfi_header() is called for each header.

: --end-of-header-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_eoh() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "n_headers" name and "100" value is sent on
   xxfi_eoh() callback:

     --end-of-header-macro n_headers:100

: --body=CHUNK

    Adds CHUNK as body chunk. If you want to multiple
    chunks, specify --body option n-times.

    The chunk is passed to milter's xxfi_body()
    callback. xxfi_body() is called for each chunk.

: --end-of-message-macro=NAME:VALUE

   Adds a macro that is sent on xxfi_eom() callback. The
   macro has NAME name and VALUE value. This option can be
   specified N-times for N additional macros.

   Here is an example that a macro that has
   "elapsed" name and "0.29" value is sent on
   xxfi_eom() callback:

     --end-of-message-macro elapsed:0.29

: --unknown=COMMAND

    Uses COMMAND as unknown SMTP command.

    The command is passed to milter's xxfi_unknown()
    callback. xxfi_unknown() is called between
    xxfi_envrcpt() and xxfi_data().

: --authenticated-name=NAME

    Uses ((|NAME|)) as an authorized user name on SMTP
    Auth. It corresponds to SASL login name. ((|NAME|)) is
    passed as a value of (({{auth_authen}})) on MAIL FROM.

: --authenticated-type=TYPE

    Uses ((|TYPE|)) as an authorized type on SMTP Auth. It
    corresponds to SASL login method. ((|TYPE|)) is passed
    as a value of (({{auth_type}})) on MAIL FROM.

: --authenticated-author=AUTHOR

    Uses ((|AUTHOR|)) as an authorized sender on SMTP
    Auth. It corresponds to SASL sender. ((|AUTHOR|)) is
    passed as a value of (({{auth_author}})) on MAIL FROM.

: --mail-file=PATH

    Uses file exists at ((|PATH|)) as mail content. If the file
    has 'From:' and/or 'To:', they are used for from and/or
    recipient addresses.

: --output-message

    Shows a message applied a milter. If you want to
    check milter's operation that may change header and/or
    body, specify this option.

: --color=[yes|true|no|false|auto]

    Shows a messaged applied a milter with colorization if
    --color, --color=yes or --color=true is specified. If
    --color=auto is specified, colorization is enabled on
    terminal environment.

    The default is off.

: --connection-timeout=SECONDS

    Specifies timeout on connecting to a milter.
    An error is occurred when a connection can't be
    established in ((|SECONDS|)) seconds.

    The default is 300 seconds. (5 minutes)

: --reading-timeout=SECONDS

    Specifies timeout on reading a response from a milter.
    An error is occurred when the milter doesn't respond to a
    request in ((|SECONDS|)) seconds.

    The default is 10 seconds.

: --writing-timeout=SECONDS

    Specifies timeout on writing a request to a milter.
    An error is occurred when request to the milter isn't
    completed in ((|SECONDS|)) seconds.

    The default is 10 seconds.

: --end-of-message-timeout=SECONDS

    Specifies timeout on reading a response of
    end-of-message command from a milter.
    An error is occurred when the milter doesn't complete its
    response to the end-of-message command in ((|SECONDS|))
    seconds.

    The default is 300 seconds. (5 minutes)

: --threads=N

   Use N threads to request a milter.

   The default is 0. (main thread only)

: --verbose

   Logs verbosely.

   "MILTER_LOG_LEVEL=all" environment variable configuration
   has the same effect.

: --version

   Shows version and exits.

== EXIT STATUS

The exit status is 0 if milter session is started and non 0
otherwise. milter session can't be started when connection
spec is invalid format or milter-test-server can't connect
to a milter.

== EXAMPLE

The following example talks with a milter that works on host
192.168.1.29 and is listened at 10025 port.

  % milter-test-server -s inet:10025@192.168.1.29

== SEE ALSO

((<milter-test-client.rd>))(1),
((<milter-performance-check.rd>))(1)
