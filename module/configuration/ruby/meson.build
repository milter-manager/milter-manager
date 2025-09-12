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

shared_module(
    'milter-manager-ruby-configuration',
    'milter-manager-ruby-configuration.c',
    c_args: '-DMILTER_LOG_DOMAIN="milter-manager-ruby-configuration"',
    dependencies: [milter_manager, ruby_glib2],
    install: true,
    install_dir: module_configuration_dir,
    # Recently, hardened build is enabled by default on
    # Fedora and RHEL related systems, but there is a case
    # that it causes error (-Wl,-z,now) for some plugins
    # systems. To fix such a issue, lazy binding
    # (-Wl,-z,lazy) should be enabled explicitly.
    # https://fedoraproject.org/wiki/Changes/Harden_All_Packages#Troubleshooting_steps_for_package_maintainers
    link_args: ['-Wl,-z,lazy'],
    name_prefix: '',
)
