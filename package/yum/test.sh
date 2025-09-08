#!/bin/bash
#
# Copyright (C) 2022-2025  Sutou Kouhei <kou@clear-code.com>
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

case ${version} in
  8)
    DNF="dnf --enablerepo=powertools"
    sudo ${DNF} install -y epel-release
    sudo ${DNF} module -y enable ruby:3.0
    ;;
  9)
    DNF="dnf --enablerepo=crb"
    sudo ${DNF} install -y epel-release
    DNF="${DNF} --enablerepo=epel-testing"
    ;;
  *)
    DNF="dnf --enablerepo=crb"
    sudo ${DNF} install -y epel-release
    ;;
esac

# TODO: Need this for testing package upgrade
# curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | \
#   sudo bash

repositories_dir=/host/package/yum/repositories
sudo ${DNF} install -y \
  ${repositories_dir}/${os}/${version}/x86_64/Packages/*.rpm

! sudo systemctl status milter-manager
sudo systemctl enable --now milter-manager
sudo systemctl status milter-manager

# TODO: Run tests or something

# TODO: Test package remove

# TODO: Test package upgrade
