---
title: FreeBSDへインストール（任意）
---

# FreeBSDへインストール（任意） --- FreeBSDへのmilter manager関連ソフトウェアのインストール方法

## このドキュメントについて

FreeBSDに特化したmilter manager関連ソフトウェアのインストール方法について説明します。milter manager本体のインストール情報はFreeBSDへインストール、FreeBSDに依存しない一般的なインストール情報はインストールを見てください。

## milter-manager-log-analyzerのインストール {#milter-manager-log-analyzer}

milter-manager-log-analyzerはmilter managerのログを解析して統計情報をグラフ化します。

milter-manager-log-analyzerが出力するグラフは同じホストにWebサーバを設置して閲覧する方法と[Munin](http://munin-monitoring.org/)を利用して別ホストで閲覧する方法があります。すでにMuninを導入していたり、メールサーバとは別にシステム監視用のサーバがある場合はMuninを利用するとよいでしょう。そうでない場合は同じホストにWebサーバを設置するのがよいでしょうFIXME。

まずは、milter-manager-log-analyzerのインストール方法を説明します。次に、グラフを閲覧する環境を説明します。

グラフを閲覧する環境の設定方法は、まず、同じホストにWebサーバを設置する方法を説明し、次に別ホストにあるMuninから閲覧する方法を説明します。

それでは、まず、milter-manager-log-analyzerのインストール方法です。

### パッケージのインストール

グラフを生成するためにRRDtoolを使います。RRDtoolのRubyインターフェイスもインストールします。

<em>注: RRDtool 1.3.8のRubyバインディングにはバグがあるので、1.3.8を使うときは"-M'WITH_RUBY_MODULE=yes'"オプションは指定しないでください。</em>

<pre>% sudo /usr/local/sbin/portupgrade -NRr -m 'WITH_RUBY_MODULE=true' databases/rrdtool</pre>

### milter-manager-log-analyzerの設定

milter-manager-log-analyzerはcronで定期的にログを解析し、グラフを生成します。そのためのcronの設定をするファイルが/usr/local/etc/milter-manager/cron.d/freebsd/milter-manager-logにインストールされています。このファイルを使うと、5分毎にログを解析し、~milter-manager/public_html/log/以下にグラフを生成します。

<pre>% sudo -u milter-manager -H crontab /usr/local/etc/milter-manager/cron.d/freebsd/milter-manager-log</pre>

milter-manager-log-analyzerは5分おきに実行されているかどうかは、/var/log/cronを見ると確認できます。

これで、milter-manager-log-analyzerのインストールが完了したので、milter-manager-log-analyzerが生成するグラフを閲覧するための環境を設定します。まずは、同じホストにWebサーバを設置する方法です。

### 同じホストにWebサーバを設置する場合

#### パッケージのインストール

WebサーバとしてApacheを使います。ここでは、Apache 2.2系列（www/apache22）をインストールします。

<pre>% sudo /usr/local/sbin/portupgrade -NRr www/apache22</pre>

#### Apacheの設定

グラフは~milter-manager/public_html/log/以下に生成しているので、http://localhost/~milter-manager/log/で閲覧できるようにします。

Apacheで各ユーザ毎にファイルを公開できるようにします。/usr/local/etc/apache22/httpd.confを以下のように編集します。

編集前:

<pre># User home directories
#Include etc/apache22/extra/httpd-userdir.conf</pre>

編集後:

<pre># User home directories
Include etc/apache22/extra/httpd-userdir.conf</pre>

設定を再読み込みします。

<pre>% sudo /usr/local/etc/rc.d/apache22 reload</pre>

これでhttp://localhost/~milter-manager/log/でグラフを閲覧できるようになります。

### 別ホストにあるMuninを利用する方法 {#munin}

次は、別ホストにあるMunin上でグラフを閲覧する方法です。

#### パッケージのインストール

Muninサーバに統計情報を送るmunin-nodeをインストールします。

<pre>% sudo /usr/local/sbin/portupgrade -NRr munin-node</pre>

#### munin-nodeの設定

milter-manager-log-analyzerが収集した統計情報をMuninに提供するMuninプラグインをインストールします。プラグインは/usr/local/share/munin/plugins/以下にインストールされているので、まずは、それらをmunin-nodeのプラグインディレクトリにインストールします。

<pre>% sudo ln -s /usr/local/share/milter-manager/munin/plugins/* /usr/local/share/munin/plugins</pre>

これらのプラグインの設定を書いた/usr/local/etc/munin/plugin-conf.d/milter-manager.confを作成します。

/usr/local/etc/munin/plugin-conf.d/milter-manager.conf:

<pre>[milter_manager_*]
  user milter-manager
  env.PATH /bin:/usr/local/bin:/usr/bin
  env.logdir /home/milter-manager/public_html/log
  env.pidfile /var/run/milter-manager/milter-manager.pid</pre>

最後にインストールしたプラグインのうち必要なものだけ有効にします。

<pre>% sudo /usr/local/sbin/munin-node-configure --shell | grep -e '\(milter_manager_\|postfix_processes\|sendmail_processes\)' | sudo sh</pre>

プラグインのインストールはこれで完了です。

<em>注: Muninに統計情報を提供する場合はmilter manager 1.5.0以降のmilter-manager-log-analyzerが生成したデータベースを使う必要があります。1.5.0より前のバージョンからアップデートしている場合は~milter-manager/public_html/log/以下を削除してください。削除すると5分後に新しく統計情報データベースが作成されます。</em>

次に、Muninサーバからの接続を許可します。Muninサーバが192.168.1.254の場合は以下の行を/usr/local/etc/munin/munin-node.confに追加します。

/usr/local/etc/munin/munin-node.conf:

<pre>allow ^192\.168\.1\.254$</pre>

munin-nodeを再起動し設定を反映させます。

<pre>% sudo /usr/local/etc/rc.d/munin-node.sh restart</pre>

#### Muninサーバの設定

ここからは監視用サーバでの設定です。監視用サーバもFreeBSDで動いているとします。

まず、muninとApacheをインストールします。

<pre>monitoring-server% sudo /usr/local/sbin/portupgrade -NRr munin-main www/apache22</pre>

muninの監視対象にmunin-nodeが動いているメールサーバを追加します。メールサーバが以下の場合の/usr/local/etc/munin/munin.confへ追加する設定項目を示します。

<dl>
<dt>ホスト名</dt>
<dd>mail.example.com</dd>
<dt>IPアドレス</dt>
<dd>192.168.1.2</dd></dl>

このメールサーバを登録するには、以下の内容を/usr/local/etc/munin/munin.confに追記します。

/usr/local/etc/munin/munin.conf:

<pre>[mail.example.com]
    address 192.168.1.2
    use_node_name yes</pre>

Muninは/usr/local/www/munin/以下にグラフを生成するので、それをhttp://monitoring-server/munin/で閲覧できるようにします。

<pre>% sudo ln -s /usr/local/www/munin/ /usr/local/www/apache22/data/</pre>

5分後にはhttp://monitoring-server/munin/でグラフを閲覧できるようになります。

## まとめ

milter-manager-log-analyzerを利用することによりmilterを導入した効果を視覚的に確認することができます。MTAとしてPostfixを使用しているのであれば、[Mailgraph](http://mailgraph.schweikert.ch/)のグラフと見くらべてmilter導入の効果を確認することができます。milterを試験的に導入している場合などに有効に活用できます。


