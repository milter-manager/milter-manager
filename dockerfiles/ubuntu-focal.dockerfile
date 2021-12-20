FROM ubuntu:focal

RUN apt-get update && \
    apt-get install -qq -y \
      libev-dev \
      libglib2.0-dev \
      libtool \
      lsb-release \
      rrdtool \
      ruby \
      ruby-dev \
      ruby-gnome2-dev \
      ruby-test-unit-rr \
      sudo && \
    curl -L https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh

RUN gem install --no-rdoc --no-ri pkg-config

RUN useradd -m --user-group --shell /bin/bash milter-manager
RUN mkdir /build && \
    chown -R milter-manager: /build

USER milter-manager
