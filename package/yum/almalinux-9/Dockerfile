ARG FROM=almalinux:9
FROM ${FROM}

ARG DEBUG

ENV SCL=rh-ruby30

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release && \
  dnf groupinstall --enablerepo=powertools -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
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
  dnf clean ${quiet} all
