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

echo "::group::Prepare"
echo "debconf debconf/frontend select Noninteractive" | \
  sudo debconf-set-selections
sudo apt update
sudo apt install -V -y curl lsb-release
echo "::endgroup::"

echo "::group::Install"
distribution=$(lsb_release --short --id | tr 'A-Z' 'a-z')
code_name=$(lsb_release --codename --short)
architecture=$(dpkg --print-architecture)

repositories_dir=/vagrant/package/apt/repositories
sudo apt install -V -y \
  ${repositories_dir}/${distribution}/pool/${code_name}/*/*/*/*_{${architecture},all}.deb
echo "::endgroup::"

echo "::group::Test milter-manager"
systemctl status milter-manager
# TODO: More test
echo "::endgroup::"

echo "::group::Test Python bindings"
/usr/share/doc/python3-milter-client/examples/milter-external.py --help
/usr/share/doc/python3-milter-client/examples/milter-replace.py \
  --pid-file=/tmp/milter-repace.pid &
milter-test-server --connection-spec inet:20025@127.0.0.1
kill $(cat /tmp/milter-replace.pid)

python3 -c "import milter.server"
# TODO: More test
echo "::endgroup::"

echo "::group::Test Ruby bindings"
# TODO
echo "::endgroup::"

echo "::group::Purge"
sudo apt purge -y libmilter-core2
! systemctl status milter-manager
echo "::endgroup::"

echo "::group::Upgrade"
if [ "${distribution}" = "ubuntu" ]; then
  sudo apt -y install software-properties-common
  sudo add-apt-repository -y ppa:milter-manager/ppa
else
  curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.deb.sh | \
    sudo bash
fi
sudo apt install -V -y milter-manager
sudo apt install -V -y \
  ${repositories_dir}/${distribution}/pool/${code_name}/*/*/*/*_{${architecture},all}.deb
systemctl status milter-manager
echo "::endgroup::"
