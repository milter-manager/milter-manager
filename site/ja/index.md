---
title: none
---

<div class="jumbotron">
  <h1>
    <img alt="{{ site.title }}"
         title="{{ site.title }}"
         src="{% link /images/milter-manager-logo.png %}">
  </h1>
  <p>{{ site.description.ja }}</p>
  <p>最新版（<a href="{% link ja/news/index.md %}#version-{{ site.milter_manager_version | replace:".", "-" }}">{{ site.milter_manager_version }}</a>）は{{ site.milter_manager_release_date }}にリリースされました。
  </p>
  <p>
    <a href="{% link ja/install/index.md %}"
       class="btn btn-primary btn-lg"
       role="button">Install</a>
  </p>
</div>

## milter managerとは {#about}

milter managerはmilterを使って効果的に迷惑メール対策（スパムメール対策・ウィルスメール対策）を行うフリーソフトウェアです。

milter managerはmilterをサポートしているSendmailやPostfixといったMTAと一緒に使うことができます。Debian GNU/Linux、Ubuntu、CentOS、AlmaLinux、FreeBSDなどのプラットフォーム上で動作します。

* [milter managerの紹介]({% link ja/milter-manager/introduction.md %})

## サポート {#support}

[GitHub Discussions](https://github.com/milter-manager/milter-manager/discussions)で質問・バグレポートを受け付けています。新バージョンのアナウンスもGitHub Discussionsに告知されるので、milter managerを利用している場合はGitHub Discussionsをウォッチしてください。

企業導入など有償でのサポートや導入支援が必要な場合は[クリアコードが提供するmilter manager関連サービス](https://www.clear-code.com/services/milter-manager.html)を検討してください。

## ライセンス {#license}

milter managerは次のライセンスでリリースされています。

* コマンド：GPL-3.0+
* ドキュメント：GFDL-1.3+
* 画像：CC0-1.0
* ライブラリー：LGPL-3.0+
