#!/bin/bash
#
# Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

set -exu

os=$(. /etc/os-release && echo $ID)
version=$(. /etc/os-release && echo $VERSION_ID | grep -oE '^[0-9]+')

case ${os} in
  centos)
    DNF=yum
    yum install -y \
      centos-release-scl-rh \
      epel-release
    ;;
  almalinux)
    case ${version} in
      8)
        DNF="dnf --enablerepo=powertools"
        dnf --enablerepo=powertools install -y epel-release
        dnf module -y enable ruby:3.0
        ;;
      9)
        DNF="dnf --enablerepo=crb"
        dnf --enablerepo=crb install -y epel-release
        ;;
    esac
    ;;
esac

# TODO: Need this for testing package upgrade
# curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | \
#   sudo bash

repositories_dir=/vagrant/package/yum/repositories
${DNF} install -y \
  ${repositories_dir}/${os}/${version}/x86_64/Packages/*.rpm

systemctl status milter-manager

# TODO: Run tests or something

# TODO: Test package remove

# TODO: Test package upgrade
