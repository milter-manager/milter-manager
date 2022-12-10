---
title: CentOSで更新
---

# CentOSで更新 --- CentOSでのmilter managerの更新方法

## このドキュメントについて

CentOSに特化したmilter managerの更新方法について説明します。新規にインストールする方法はCentOSへインストールを見てください。

## 更新

パッケージを更新するだけで、追加の作業は必要ありません。

<pre>% sudo yum update -y milter-manager</pre>

### 2.1.0 未満からの更新

2.1.0から[packagecloud](https://packagecloud.io/milter-manager/repos)でパッケージを配布しているので、リポジトリの情報を更新する必要があります。milter-manager-releaseパッケージは削除してください。

<pre>% sudo yum remove -y milter-manager-release</pre>

## まとめ

milter managerは簡単に更新することができる、メンテナンスコストが低いソフトウェアです。

新しいバージョンで行われている様々な改善を利用したい場合は更新を検討してください。

追加パッケージもインストールしている場合はCentOSで更新（任意）も参照してください。


