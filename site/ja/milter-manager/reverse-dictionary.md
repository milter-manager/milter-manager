---
title: 逆引きリファレンス
---

# 逆引きリファレンス --- やりたいことからやり方へ

## このドキュメントについて

「やりたいこと」をキーにして、そのやり方を見つけるためのドキュメントです。

## インストール

### Debian GNU/Linuxにインストールする

Debian GNU/Linuxへインストールで説明しています。aptitudeでインストールできるので、メンテナンスが楽です。

### Ubuntuにインストールする

Ubuntuへインストールで説明しています。aptitudeでインストールできるので、メンテナンスが楽です。

### CentOSにインストールする

CentOSへインストールで説明しています。RPMパッケージでインストールできるので、メンテナンスが楽です。

### FreeBSDにインストールする

FreeBSDへインストールで説明しています。

### 統計情報をグラフ化する

プラットフォーム毎にドキュメントが用意されています。

* Debian用
* Ubuntu用
* CentOS用
* FreeBSD用

## 設定: 基本編

### 設定ファイルを見つける {#configuration-basic-find-configuration-file}

インストールマニュアル通りにインストールしている場合は、カスタマイズ用の設定ファイルは以下のようになります。

* Ubuntu: /etc/milter-manager/milter-manager.local.conf
* CentOS: /etc/milter-manager/milter-manager.local.conf
* Ubuntu: /usr/local/etc/milter-manager/milter-manager.local.conf

milter-manager.local.confは新規に作成するファイルです。既定値では、milter-managerはmilter-manager.confと同じディレクトリにmilter-manager.local.confがあると自動的に読み込みます。

### TCP/IPでmilter-managerに接続する

manager.connection_specで"inet:"を指定します。

<pre># 10025番ポートで待ち受ける。自ホストからのみ接続可。
manager.connection_spec = "inet:10025@localhost"</pre>

### UNIXドメインソケットでmilter-managerに接続する

manager.connection_specで"unix:"を指定します。

<pre># /var/run/milter/milter-manager.sockで待ち受ける
manager.connection_spec = "unix://var/run/milter/milter-manager.sock"</pre>

ソケットファイルのパーミッションはmanager.unix_socket_modeで指定できます。

<pre># 同じグループのユーザは接続可能。
manager.unix_socket_mode = 0660</pre>

ソケットファイルのグループはmanager.unix_socket_groupで指定できます。

<pre># ソケットファイルは"milter"グループが所有する
manager.unix_socket_group = "milter"</pre>

### UNIXドメインソケットの後始末する

UNIXドメインソケット作成時に同名のファイルがある場合、ソケット作成に失敗します。そのような状況を避けるために、以下のタイミングでソケットファイルを削除する設定があります。

1. UNIXドメインソケット作成前
2. UNIXドメインソケット使用後

既定値では両方とも有効になっているので、通常はソケットファイルがあるという理由でソケット作成に失敗することはありません。

もし、無効にする場合は、それぞれ、以下の設定を変更します。

1. manager.remove_unix_socket_on_create
2. manager.remove_unix_socket_on_close

以下は、両方を無効にする設定例です。

<pre># ソケット作成前に既存のソケットファイルを削除しない
manager.remove_unix_socket_on_create = false
# ソケット使用後にソケットファイルを削除しない
manager.remove_unix_socket_on_close = false</pre>

## 設定: 応用編

### 特定のアカウントのみmilterを適用する

サンプルとして特定のアカウントのみ登録されているmilterを適用する設定が組み込まれています。このサンプルを使用して、以下のアカウントのみmilterを適用する例を示します。

1. test-user@example.com
2. test.example.comドメインのすべてのアカウント

設定はmilter-manager.local.confに追記します。

<pre>restrict_accounts_by_list("test-user@example.com",
                          /@test\.example\.com\z/)</pre>

この機能は現在はまだサンプル扱いのため、今後変更される可能性がありますが、その場合も同様の機能は提供され続けます。変更される場合は、データベースやLDAPからのアカウント取得機能など、より豊富な機能が備わっているはずです。


