FROM centos:7

RUN yum update -q -y && \
    yum install -q -y \
      http://sourceforge.net/projects/cutter/files/centos/cutter-release-1.3.0-1.noarch.rpm && \
    yum install -q -y \
      cutter \
      gcc \
      gettext \
      git \
      glib2-devel \
      gobject-introspection-devel \
      gtk-doc \
      intltool \
      libtool \
      make \
      rpm-build \
      rrdtool \
      ruby \
      ruby-devel \
      rubygems \
      sudo \
      tar

RUN useradd -m --user-group --shell /bin/bash milter-manager
RUN mkdir /build && \
    chown -R milter-manager: /build

USER milter-manager
