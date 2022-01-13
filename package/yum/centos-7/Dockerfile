ARG FROM=centos:7
FROM ${FROM}

ARG DEBUG

ENV SCL=rh-ruby30

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  yum update -y ${quiet} && \
  yum install -y ${quiet} centos-release-scl && \
  yum install -y ${quiet} \
    ccache \
    gcc \
    gettext \
    glib2-devel \
    gtk-doc \
    intltool \
    make \
    pkgconfig\(gobject-introspection-1.0\) \
    ${SCL}-ruby \
    ${SCL}-ruby-devel \
    ${SCL}-rubygem-rexml \
    rpmdevtools \
    scl-utils \
    tzdata && \
  yum clean ${quiet} all
