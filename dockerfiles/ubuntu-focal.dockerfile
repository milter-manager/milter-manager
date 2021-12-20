FROM ubuntu:focal

RUN apt-get update && \
    apt-get install -qq -y \
      debhelper dh-systemd autotools-dev gtk-doc-tools \
      libglib2.0-dev libev-dev ruby ruby-dev ruby-gnome2-dev ruby-test-unit-rr \
      intltool it libtool lsb-release sudo rrdtool && \
    curl -L https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh && \
    gem install --no-rdoc --no-ri pkg-config && \
    useradd -m --user-group --shell /bin/bash milter-manager

USER milter-manager
