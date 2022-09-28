FROM centos:7

ENV SCL=rh-ruby30

RUN yum update -q -y && \
    yum install -q -y \
      centos-release-scl-rh \
      epel-release \
      http://sourceforge.net/projects/cutter/files/centos/cutter-release-1.3.0-1.noarch.rpm && \
    yum install -q -y \
      ccache \
      cutter \
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
      ${SCL}-ruby \
      ${SCL}-ruby-devel \
      ${SCL}-rubygem-rexml \
      rpm-build \
      rrdtool \
      sudo \
      tar \
      tzdata && \
   (source /opt/rh/${SCL}/enable && \
    gem install \
      gio2 \
      test-unit)

ENV PATH=/usr/lib64/ccache:$PATH

RUN useradd -m --user-group --shell /bin/bash milter-manager
RUN mkdir /build && \
    chown -R milter-manager: /build

USER milter-manager
