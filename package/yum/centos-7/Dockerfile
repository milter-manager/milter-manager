ARG FROM=centos:7
FROM ${FROM}

ARG DEBUG

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
    rh-ruby30 \
    rh-ruby30-ruby-devel \
    rpmdevtools \
    scl-utils && \
  yum clean ${quiet} all
