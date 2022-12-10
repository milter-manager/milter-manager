---
title: インストール
---

# インストール --- milter managerのインストール方法

## このドキュメントについて

milter managerのインストール方法について説明します。

## 依存ソフトウェア

milter managerが依存しているソフトウェアについて説明します。

### 必須

milter managerは以下のソフトウェアに依存しているため、miltermanager のビルドには以下のソフトウェアが事前にインストールされている必要があります。

* GLib &gt;= 2.12.3
* Ruby &gt;= 1.8.5 (milter manager 1.8.4 から Ruby1.9 にも対応しました)
* Ruby/GLib2 (Ruby-GNOME2) &gt;= 0.16.0

### 任意: テスト実行

milter managerの単体テストを実行するためには以下のソフトウェアが必要ですが、milter managerの実行には必須ではありません。

* Cutter &gt;= 1.0.6
* LCOV

### 任意: グラフ生成

milter managerはログからmilterの適用状況などをグラフ化する機能も提供しています。グラフを生成する場合は以下のソフトウェアが必要ですが、milter managerの実行には必須ではありません。

* RRDtool
* RRDtoolのRubyバインディング

[Munin](http://munin-monitoring.org/)と連携する場合は以下のソフトウェアも必要です。

* munin-node

## milter-manager

milter-managerはmilter managerパッケージの中核となるプログラムです。milter-managerがmilterとして動作し、MTA・子milterと接続します。

milter-managerのインストール方法はプラットフォーム毎に解説しています。

* Debian
* Ubuntu
* CentOS
* FreeBSD
* その他

## milter-manager-log-analyzer

milter-manager-log-analyzerが生成するグラフ

milter-manager-log-analyzerはmilter-managerのログからグラフを出力するプログラムです。milter-manager-log-analyzerの設定は必須ではありません。

milter-manager-log-analyzerを用いると、milterの状況を時系列で確認することができます。新しく追加したmilterの効果や、milterの適用結果の傾向などを視覚的に確認したい場合に有用です。

milter-manager-log-analyzerはsyslogに出力されたmilter-managerのログを解析し、[RRDtool](http://oss.oetiker.ch/rrdtool/)でグラフ化します。cronを設定し、定期的にログを確認します。

milter-manager-log-analyzerのインストール方法はプラットフォーム毎に解説しています。

* Debian
* Ubuntu
* CentOS
* FreeBSD


