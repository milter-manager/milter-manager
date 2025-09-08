ARG FROM=almalinux:10
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} epel-release && \
  dnf install --enablerepo=crb -y ${quiet} \
    ccache \
    gcc \
    gettext \
    glib2-devel \
    gtk-doc \
    intltool \
    libev-devel \
    make \
    pkgconfig\(gobject-introspection-1.0\) \
    ruby \
    ruby-devel \
    rubygem-rexml \
    rpmdevtools \
    tzdata && \
  dnf clean ${quiet} all
