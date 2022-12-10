---
title: milter-manager-log-analyzer / milter manager / milter manager's manual
---

# milter-manager-log-analyzer / milter manager / milter manager's manual

## NAME

milter-manager-log-analyzer - log analyzer for milter-manager

## SYNOPSIS

<code>milter-manager-log-analyzer</code> [<em>option ...</em>]

## DESCRIPTION

milter-manager-log-analyzer analyzes milter-manager log andgenerates graphs for milters' result. It's useful forconfirming system status transition because graphs showsdata in time-line. Graphs can also be used for comparingchanges between before introducing new milter and afterintroducing new milter.

## Options

<dl>
<dt>--help</dt>
<dd>Shows available options and exits.</dd>
<dt>--log=lOG_FILE</dt>
<dd>Reads log from LOG_FILE

The default is standard input.</dd>
<dt>--output-directory=DIRECTORY</dt>
<dd>Outputs graphs, HTML and data to DIRECTORY.

The default is the current directory. (".")</dd>
<dt>--no-update-db</dt>
<dd>Doesn't update database. It's useful for just generatesgraphs.

If this option is not specified, database will be updated.</dd></dl>

## EXIT STATUS

Always 0.

## EXAMPLE

milter-manager-log-analyzer will be used in crontab. Here isa sample crontab:

<pre>PATH=/bin:/usr/local/bin:/usr/bin
*/5 * * * * root cat /var/log/mail.info | su milter-manager -s /bin/sh -c "milter-manager-log-analyzer --output-directory ~milter-manager/public_html/log"</pre>

In the above sample, mail log are read by root andmilter-manager-log-analyzer run as milter-manager user isreceived it. milter-manager-log-analyzer outputs analyzedresult into ~milter-manager/public_html/log/. Analyzedresult can be seen at http://localhost/~milter-manager/log/.

## SEE ALSO

milter-manager.rd(1)


