# Copyright (C) 2022  Daijiro Fukuda <fukuda@clear-code.com>
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

project('milter-manager',
        'c',
        license: 'LGPL-3.0-or-later',
        version: '2.1.6')

api_version = '2.0'
so_version = 2
library_version = '@0@.0.0'.format(so_version)

prefix = get_option('prefix')
include_dir = join_paths(prefix, get_option('includedir'))
data_dir = join_paths(prefix, get_option('datadir'))
gir_dir = join_paths(data_dir, 'gir-1.0')

gnome = import('gnome')
pkgconfig = import('pkgconfig')
pkgconfig_variables = ['girdir=@0@'.format(gir_dir)]

config_h_conf = configuration_data()
config_h_conf.set('GETTEXT_PACKAGE', 'milter-manager')
# CentOS 7: GLib 2.50
# AlmaLinux 8: GLib 2.56
# AlmaLinux 9: GLib 2.68
# Ubuntu 18.04: GLib 2.56
# Ubuntu 20.04: GLib 2.64
# Ubuntu 22.04: GLib 2.72
config_h_conf.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_50')
config_h = configure_file(input: 'meson-config.h.in',
                          output: 'config.h',
                          configuration: config_h_conf)
config = declare_dependency(compile_args: '-DHAVE_CONFIG_H',
                            include_directories: include_directories('.'),
                            sources: config_h)

subdir('milter/core')
subdir('milter/client')
