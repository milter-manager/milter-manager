---
title: none
---

<div class="jumbotron">
  <h1>
    <img alt="{{ site.title }}"
         title="{{ site.title }}"
         src="{% link /images/milter-manager-logo.png %}">
  </h1>
  <p>{{ site.description.en }}</p>
  <p>The latest version
     (<a href="{% link news/index.md %}#version-{{ site.milter_manager_version | replace:".", "-" }}">{{ site.milter_manager_version }}</a>)
     has been released at {{ site.milter_manager_release_date }}.
  </p>
  <p>
    <a href="{% link install/index.md %}"
       class="btn btn-primary btn-lg"
       role="button">Install</a>
  </p>
</div>

## About milter manager {#about}

milter manager is a free software to protect you from spam mails and virus mails effectively with milter.

milter manager can be used with MTAs that supports milter like Sendmail and Postfix. It works on Debian GNU/Linux, Ubuntu, CentOS, AlmaLinux, FreeBSD and so on.

* [Introduction of milter manager]({% link milter-manager/introduction.md %})

## Support {#support}

Questions and bug reports are accepted on [GitHub Discussions](https://github.com/milter-manager/milter-manager/discussions). New release announce is also done on the GitHub Discussions. If you are using milter manager, it's a good idea that you watch the GitHub Discussions.

## License {#license}

milter manager is released under the following licenses:

* Commands: GPL-3.0+
* Documents: GFDL-1.3+
* Images: CC0-1.0
* Libraries: LGPL-3.0+
