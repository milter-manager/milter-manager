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

api_version = '1.0'
so_version = 1
library_version = '@0@.0.1'.format(so_version)

prefix = get_option('prefix')
include_dir = join_paths(prefix, get_option('includedir'))
data_dir = join_paths(prefix, get_option('datadir'))
gir_dir = join_paths(data_dir, 'gir-1.0')
vapi_dir = join_paths(data_dir, 'vala', 'vapi')

gnome = import('gnome')
pkgconfig = import('pkgconfig')
pkgconfig_variables = ['girdir=@0@'.format(gir_dir)]

base_include_directories = [
  include_directories('.')
]

gi = dependency('gobject-introspection-1.0')

subdir('milter/core')
