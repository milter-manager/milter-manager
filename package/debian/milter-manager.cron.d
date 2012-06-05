PATH=/bin:/usr/local/bin:/usr/bin
*/5 * * * * root cat /var/log/mail.log | su milter-manager -s /bin/sh -c "nice milter-manager-log-analyzer --output-directory ~milter-manager/public_html/log"
