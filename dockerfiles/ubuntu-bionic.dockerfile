FROM ubuntu:bionic

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

RUN apt-get update && \
    apt-get install -qq -y \
      curl \
      gtk-doc-tools \
      intltool \
      libev-dev \
      libgirepository1.0-dev \
      libglib2.0-dev \
      libtool \
      lsb-release \
      rrdtool \
      ruby \
      ruby-dev \
      ruby-gnome2-dev \
      sudo && \
    curl -L https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh

RUN useradd -m --user-group --shell /bin/bash milter-manager
RUN mkdir /build && \
    chown -R milter-manager: /build

USER milter-manager
