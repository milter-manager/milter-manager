---
title: Debianで更新
---

# Debianで更新 --- Debian GNU/Linuxでのmilter managerの更新方法

## このドキュメントについて

Debian GNU/Linuxに特化したmilter managerの更新方法について説明します。新規にインストールする方法はDebianへインストールを見てください。

## 更新

パッケージを更新するだけで、追加の作業は必要ありません。

<pre>% sudo apt -y upgrade</pre>

### 2.1.0 以前からの更新

2.1.0から[packagecloud](https://packagecloud.io/milter-manager/repos)でパッケージを配布しているので、リポジトリの情報を更新する必要があります。

sid向けのパッケージは、packagecloudで配布できないので配布方法を検討中です。

## まとめ

milter managerは簡単に更新することができる、メンテナンスコストが低いソフトウェアです。

新しいバージョンで行われている様々な改善を利用したい場合は更新を検討してください。

追加パッケージもインストールしている場合はDebianで更新（任意）も参照してください。


