FROM debian:sid

ENV CODE_NAME=unstable

RUN apt-get update && \
    apt-get install -qq -y \
      debhelper dh-systemd autotools-dev gtk-doc-tools \
      libglib2.0-dev libev-dev ruby ruby-dev ruby-gnome2-dev ruby-test-unit-rr \
      intltool lcov git libtool sudo lsb-release apt-transport-https \
      rrdtool rsyslog && \
    curl -L https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | env CUTTER_MASTER=yes sh && \
    gem install --no-rdoc --no-ri coveralls-lcov && \
    gem install --no-rdoc --no-ri pkg-config && \
    useradd -m --user-group --shell /bin/bash milter-manager

WORKDIR /home/milter-manager/milter-manager
COPY . .
RUN chown -R milter-manager:milter-manager .
USER milter-manager
