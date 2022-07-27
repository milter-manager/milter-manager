
project('milter-manager', 'c',
        license: 'LGPL-3.0-or-later',
        version: '2.1.6')

api_version = '1.0'
so_version = 1
library_version = '1.0.1'

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
