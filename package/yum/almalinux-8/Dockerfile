ARG FROM=almalinux:8
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} epel-release && \
  dnf install --enablerepo=powertools -y ${quiet} \
    ccache \
    gcc \
    gettext \
    glib2-devel \
    gtk-doc \
    intltool \
    make \
    pkgconfig\(gobject-introspection-1.0\) \
    ruby \
    ruby-devel \
    rubygem-rexml \
    rpmdevtools \
    scl-utils \
    tzdata && \
  dnf clean ${quiet} all
