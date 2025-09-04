FROM almalinux:10

RUN \
  dnf install -q -y \
    epel-release && \
  dnf install --enablerepo=crb --allowerasing -q -y \
    ccache \
    curl \
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
    ruby-devel \
    rubygem-rexml \
    rpm-build \
    rrdtool \
    sudo \
    tar \
    tzdata && \
  curl \
    --silent \
    --location \
    https://raw.githubusercontent.com/clear-code/cutter/HEAD/data/travis/setup.sh | \
      CUTTER_MAIN=yes bash && \
  gem install \
    gio2 \
    test-unit && \
  dnf clean all

ENV PATH=/usr/lib64/ccache:$PATH

RUN useradd -m --user-group --shell /bin/bash milter-manager
RUN mkdir /build && \
    chown -R milter-manager: /build

USER milter-manager
