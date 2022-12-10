---
title: FreeBSDで更新
---

# FreeBSDで更新 --- FreeBSDでのmilter managerの更新方法

## このドキュメントについて

FreeBSDに特化したmilter managerの更新方法について説明します。新規にインストールする方法はFreeBSDへインストールを見てください。

## ビルドと更新

上書きインストールで更新できます。ただし、/usr/local/etc/milter-manager/milter-manager.confが上書きされてしまうので、milter-manager.confを変更している場合は事前にバックアップをとってください。

milter-manager.confではなくmilter-manager.local.confを利用している場合はバックアップをとる必要はありません。

<pre>% sudo pkg update
% sudo pkg upgrade --yes milter-manager</pre>

## まとめ

milter managerは簡単に更新することができる、メンテナンスコストが低いソフトウェアです。

新しいバージョンで行われている様々な改善を利用したい場合は更新を検討してください。

追加パッケージもインストールしている場合はFreeBSDで更新（任意）も参照してください。


