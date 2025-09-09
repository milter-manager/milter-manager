ARG FROM=ubuntu:noble
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    autotools-dev \
    build-essential \
    ccache \
    debhelper \
    devscripts \
    intltool \
    libev-dev \
    libgirepository1.0-dev \
    libglib2.0-dev \
    lsb-release \
    meson \
    ninja-build \
    python3 \
    rrdtool \
    ruby \
    ruby-dev \
    ruby-gnome-dev \
    ruby-test-unit \
    tzdata && \
  apt clean
