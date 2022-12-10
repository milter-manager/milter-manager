---
title: Ubuntuへインストール（任意）
---

# Ubuntuへインストール（任意） --- Ubuntu Linuxへのmilter manager関連ソフトウェアのインストール方法

## このドキュメントについて

Ubuntu Linuxに特化したmilter manager関連ソフトウェアのインストール方法について説明します。milter manager本体のインストール情報はUbuntuへインストール、Ubuntu Linuxに依存しない一般的なインストール情報はインストールを見てください。

## milter-manager-log-analyzerのインストール {#milter-manager-log-analyzer}

milter-manager-log-analyzerはmilter-managerパッケージに含まれているので、すでにインストールされています。ここでは、milter-manager-log-analyzerが出力するグラフを閲覧するための設定を行います。

milter-manager-log-analyzerが出力するグラフは同じホストにWebサーバを設置して閲覧する方法と[Munin](http://munin-monitoring.org/)を利用して別ホストで閲覧する方法があります。すでにMuninを導入していたり、メールサーバとは別にシステム監視用のサーバがある場合はMuninを利用するとよいでしょう。そうでない場合は同じホストにWebサーバを設置するのがよいでしょうFIXME。

まずは、同じホストにWebサーバを設置する方法を説明し、次に別ホストにあるMuninから閲覧する方法を説明します。

### 同じホストにWebサーバを設置する場合

#### パッケージのインストール

WebサーバとしてApacheを使います。

<pre>% sudo apt -V -y install apache2</pre>

#### milter-manager-log-analyzerの設定

グラフはインストール時に作成したmilter-managerユーザのホームディレクトリ（/var/lib/milter-manager/）以下に出力されています。このグラフをhttp://localhost/milter-manager-log/で閲覧できるようにします。

以下の内容の/etc/apache2/conf.d/milter-manager-logを作成します。

/etc/apache2/conf.d/milter-manager-log:

<pre>Alias /milter-manager-log/ /var/lib/milter-manager/public_html/log/</pre>

作成したら、再読み込みします。

<pre>% sudo /etc/init.d/apache2 force-reload</pre>

これでhttp://localhost/milter-manager-log/でグラフを閲覧できるようになります。

### 別ホストにあるMuninを利用する方法 {#munin}

次は、別ホストにあるMunin上でグラフを閲覧する方法です。

#### パッケージのインストール

milter-manager-log-analyzerが収集した統計情報をMuninに提供するmilter-manager-munin-pluginsパッケージをインストールします。

<pre>% sudo apt -V -y install milter-manager-munin-plugins</pre>

<em>注: Muninに統計情報を提供する場合はmilter manager 1.5.0以降のmilter-manager-log-analyzerが生成したデータベースを使う必要があります。1.5.0より前のバージョンからアップデートしている場合は~milter-manager/public_html/log/以下を削除してください。削除すると5分後に新しく統計情報データベースが作成されます。</em>

#### munin-nodeの設定

Muninサーバからの接続を許可します。Muninサーバが192.168.1.254の場合は以下の行を/etc/munin/munin-node.confに追加します。

/etc/munin/munin-node.conf:

<pre>allow ^192\.168\.1\.254$</pre>

munin-nodeを再起動し設定を反映させます。

<pre>% sudo /usr/sbin/service munin-node restart</pre>

#### Muninサーバの設定

ここからは監視用サーバでの設定です。監視用サーバもUbuntuで動いているとします。

まず、muninとApacheをインストールします。

<pre>monitoring-server% sudo apt -V -y install munin httpd</pre>

muninの監視対象にmunin-nodeが動いているメールサーバを追加します。メールサーバが以下の場合の/etc/munin/munin.confへ追加する設定項目を示します。

<dl>
<dt>ホスト名</dt>
<dd>mail.example.com</dd>
<dt>IPアドレス</dt>
<dd>192.168.1.2</dd></dl>

このメールサーバを登録するには、以下の内容を/etc/munin/munin.confに追記します。

/etc/munin/munin.conf:

<pre>[mail.example.com]
    address 192.168.1.2
    use_node_name yes</pre>

5分後にはhttp://monitoring-server/munin/でグラフを閲覧できるようになります。

## まとめ

milter-manager-log-analyzerを利用することによりmilterを導入した効果を視覚的に確認することができます。MTAとしてPostfixを使用しているのであれば、[Mailgraph](http://mailgraph.schweikert.ch/)のグラフと見くらべてmilter導入の効果を確認することができます。milterを試験的に導入している場合などに有効に活用できます。


