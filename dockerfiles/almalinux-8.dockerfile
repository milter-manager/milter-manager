FROM almalinux:8

RUN dnf update -q -y && \
    dnf install -q -y epel-release && \
    dnf module -y enable ruby:3.3 && \
    dnf install --enablerepo=powertools -q -y \
      ccache \
      gcc \
      gettext \
      git \
      glib2-devel \
      gobject-introspection-devel \
      gtk-doc \
      intltool \
      libtool \
      libev-devel \
      make \
      ruby \
      ruby-devel \
      rubygem-rexml \
      rpm-build \
      rrdtool \
      sudo \
      tar \
      tzdata && \
    gem install \
      gio2 \
      test-unit

ENV PATH=/usr/lib64/ccache:$PATH

RUN useradd -m --user-group --shell /bin/bash milter-manager
RUN mkdir /build && \
    chown -R milter-manager: /build

USER milter-manager
