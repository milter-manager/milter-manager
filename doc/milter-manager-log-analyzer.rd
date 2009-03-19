= milter-manager-log-analyzer / milter manager / milter manager's manual

== NAME

milter-manager-log-analyzer - log analyzer for milter-manager

== SYNOPSIS

(({milter-manager-log-analyzer})) [((*option ...*))]

== DESCRIPTION

milter-manager-log-analyzer analyzes milter-manager log and
generates graphs for milters' result. It's useful for
confirming system status transition because graphs shows
data in time-line. Graphs can also be used for comparing
changes between before introducing new milter and after
introducing new milter.

== Options

: --help

   Shows available options and exits.

: --log=lOG_FILE

   Reads log from LOG_FILE

   The default is standard input.

: --output-directory=DIRECTORY

   Outputs graphs, HTML and data to DIRECTORY.

   The default is the current directory. (".")

: --no-update-db

   Doesn't update database. It's useful for just generates
   graphs.

   If this option is not specified, database will be updated.

== EXIT STATUS

Always 0.

== EXAMPLE

milter-manager-log-analyzer will be used in crontab. Here is
a sample crontab:

  PATH=/bin:/usr/local/bin:/usr/bin
  */5 * * * * root cat /var/log/mail.info | su milter-manager -s /bin/sh -c "milter-manager-log-analyzer --output-directory ~milter-manager/public_html/log"

In the above sample, mail log are read by root and
milter-manager-log-analyzer run as milter-manager user is
received it. milter-manager-log-analyzer outputs analyzed
result into ~milter-manager/public_html/log/. Analyzed
result can be seen at http://localhost/~milter-manager/log/.

== SEE ALSO

((<milter-manager.rd>))(1)
