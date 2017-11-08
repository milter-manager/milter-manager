FROM centos:7

RUN yum update -q -y && \
    yum install -q -y \
      libtool intltool gettext gcc make glib2-devel git tar rpm-build gtk-doc \
      ruby ruby-devel rubygems lcov rsyslog rrdtool sudo git epel-release && \
    yum install -q -y  http://sourceforge.net/projects/cutter/files/centos/cutter-release-1.3.0-1.noarch.rpm && \
    yum makecache && \
    yum install -q -y lcov && \
    yum install -q -y cutter && \
    gem install --no-rdoc --no-ri coveralls-lcov pkg-config test-unit-rr && \
    useradd -m --user-group --shell /bin/bash milter-manager

WORKDIR /home/milter-manager/milter-manager
COPY . .
RUN chown -R milter-manager:milter-manager .
USER milter-manager
