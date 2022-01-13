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

os=$(cut -d: -f4 /etc/system-release-cpe)
case ${os} in
  centos)
    version=$(cut -d: -f5 /etc/system-release-cpe)
    ;;
  *) # For AlmaLinux
    version=$(cut -d: -f5 /etc/system-release-cpe | sed -e 's/\.[0-9]$//')
    ;;
esac

case ${version} in
  7)
    DNF=yum
    ;;
  *)
    DNF="dnf --enablerepo=powertools"
    ;;
esac

# TODO: Need this for testing package upgrade
# curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | \
#   sudo bash

${DNF} install -y epel-release

repositories_dir=/host/packages/yum/repositories
${DNF} install -y \
  ${repositories_dir}/${os}/${version}/x86_64/Packages/*.rpm

milter-manager --version

# TODO: Run tests or something

# TODO: Test package remove

# TODO: Test package upgrade
