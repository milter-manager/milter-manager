---
title: milter-statistics-report / milter manager / milter managerのマニュアル
---

# milter-statistics-report / milter manager / milter managerのマニュアル

## 名前

milter-statistics-report - milter（などの）プロセスの統計情報を表示するプログラム

## 書式

<code>milter-statistics-report</code> [<em>オプション ...</em>] <em>監視対象のプロセス名 ...</em>

## 説明

milter-statistics-reportはmilterプロセスを監視し、milterプロセスの統計情報を定期的に出力します。表示する統計情報は以下の通りです。

* プロセスID
* 使用仮想メモリ量
* 使用実メモリ量
* CPU使用率
* CPU使用時間
* 開いているファイルディスクリプタ数

なお、milterプロセス以外にも任意のプロセスを監視することが可能です。

## オプション

<dl>
<dt>--help</dt>
<dd>利用できるオプションを表示して終了します。</dd>
<dt>--filter=REGEXP</dt>
<dd>監視対象のプロセスを正規表現<var>REGEXP</var>で絞り込みます。<em>監視対象のプロセス名</em>で指定したプロセスのうち、<var>REGEXP</var>にマッチしたプロセスの統計情報だけが出力します。

既定値はありません。 （絞り込みません。）</dd>
<dt>--interval=INTERVAL</dt>
<dd>INTERVAL秒毎に統計情報を出力します。

既定値は1秒です。</dd></dl>

## 終了ステータス

常に0。

## 例

以下の例では、milter-test-clientとsmtpdプロセスを監視します。

<pre>% milter-report-statistics milter-test-client smtpd
    Time    PID       VSS       RSS  %CPU CPU time   #FD command
16:42:02  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:02  23231  56656 kB   2904 kB   4.0  0:00.06     0 smtpd -n smtp -t inet -u
16:42:03  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:03  23234  56656 kB   2900 kB   2.0  0:00.02     0 smtpd -n smtp -t inet -u
16:42:04  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:05  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:05  23237  56656 kB   2904 kB   4.0  0:00.04     0 smtpd -n smtp -t inet -u
16:42:06  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:06  23239  57436 kB   2900 kB   4.0  0:00.02     0 smtpd -n smtp -t inet -u
16:42:07  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:07  23239  57436 kB   2900 kB                    0 smtpd -n smtp -t inet -u
16:42:08  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
16:42:08  23242  56656 kB   2904 kB   4.0  0:00.03     0 smtpd -n smtp -t inet -u</pre>

以下の例ではRubyで実装されたmilterを5秒毎に監視します。

<pre>% milter-report-statistics --filter milter --interval 5 ruby
    Time    PID       VSS       RSS  %CPU CPU time   #FD command
16:44:45  23257 184224 kB   9700 kB   0.0  0:05.79    10 ruby milter-test-client.rb
16:44:50  23257 184224 kB   9700 kB  34.0  0:07.02    14 ruby milter-test-client.rb
16:44:55  23257 184224 kB   9700 kB  36.0  0:08.79    13 ruby milter-test-client.rb
16:45:00  23257 184224 kB   9728 kB  36.0  0:10.57    14 ruby milter-test-client.rb
16:45:05  23257 184224 kB   9728 kB  36.0  0:11.42    14 ruby milter-test-client.rb</pre>

## 関連項目

milter-performance-check.rd.ja(1)


