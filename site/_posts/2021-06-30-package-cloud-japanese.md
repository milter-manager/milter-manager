---
tags: []
title: milter-managerはpackagecloud.ioの支援を受けています
---
2016年の10月頃から milter-manager はpackagecloud.ioの支援を受けています。
これまでpackagecloud.ioがOSS向けのプランを提供してくれているおかげでAPT/YUMリポジトリを提供できました。
<!--more-->


インタビュー形式でmilter-managerのユースーケースを紹介します。

### packagecloudがもたらした定量的なインパクトと、その他のメリットはなんでしたか?

milter-managerをリリースするたびに、APT/YUMのリポジトリを自分たちで更新しなくてもよくなりました。
milter-managerはそれなりに歴史があるプロジェクトなので、パッケージ数も多いですし、ディスク容量も必要です。
packagecloud.ioは十分なディスク容量を提供してくれています。

そして、自分たちで署名用の鍵を維持しなくてもよくなりました！

### packagecloudの他に検討したものはありますか?

移行に取り掛かった当時、[OSDN](https://ja.osdn.net/)を検討していました。
ただそのときはOSDNはAPT/YUMリポジトリのホスティングに対応していませんでした。
そのため、packagecloud.ioへ移行することにしたのです。

### packagecloudを使う前はどんなでしたか?

packagecloud.io前に私達はSourceforge.netを使っていました。
当時APT/YUMリポジトリもそこでメンテナンスしていたのですが、あまり安定して使えませんでした。
ダウンロードURLが事前のアナウンスなしに変わるということがしばしばあったからです。随時対応しないといけませんでした。

### packagecloudが解決した問題はなんですか?

すでに述べたように、APT/YUMリポジトリのメンテンナンス作業から開放されました。
そのため、milter-managerの開発に注力できるようになりました。

packagecloud.ioがスポンサーしてくれていることは大変ありがたいです!
