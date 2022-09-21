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

sudo apt update
sudo apt install -V -y curl lsb-release

distribution=$(lsb_release --short --id | tr 'A-Z' 'a-z')
code_name=$(lsb_release --codename --short)
architecture=$(dpkg --print-architecture)

# TODO: Need this for testing package upgrade
# curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.deb.sh | \
#   sudo bash

repositories_dir=/vagrant/package/apt/repositories
sudo apt install -V -y \
  ${repositories_dir}/${distribution}/pool/${code_name}/*/*/*/*_{${architecture},all}.deb

systemctl status milter-manager

# TODO: Run tests or something

# TODO: Test package purge

# TODO: Test package upgrade
