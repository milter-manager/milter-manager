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
project_include_sub_dir = meson.project_name()
milter_include_sub_dir = project_include_sub_dir / 'milter'
milter_include_dir = get_option('includedir') / milter_include_sub_dir
data_dir = prefix / get_option('datadir')
gir_dir = data_dir / 'gir-1.0'
project_lib_dir = get_option('libdir') / meson.project_name()
module_dir = project_lib_dir / 'module'
module_configuration_dir = module_dir / 'configuration'

fs = import('fs')
gnome = import('gnome')
pkgconfig = import('pkgconfig')
pkgconfig_variables = ['girdir=@0@'.format(gir_dir)]


# CentOS 7: GLib 2.50
# AlmaLinux 8: GLib 2.56
# AlmaLinux 9: GLib 2.68
# Ubuntu 18.04: GLib 2.56
# Ubuntu 20.04: GLib 2.64
# Ubuntu 22.04: GLib 2.72
glib_version_min_required = 'GLIB_VERSION_2_50'


package_platform = get_option('package-platform')
if package_platform == ''
  uname = run_command(find_program('uname'), capture: true, check: true).stdout()
  if uname == 'Linux'
    if fs.is_file('/etc/debian_version') or fs.exists('/usr/bin/dpkg')
      package_platform = 'debian'
    elif fs.is_file('/etc/redhat-release')
      package_platform = 'redhat'
    endif
  elif uname == 'FreeBSD'
    package_platform = 'freebsd'
  elif uname == 'NetBSD' or uname == 'DragonFly'
    package_platform = 'pkgsrc'
  endif
endif
if package_platform == 'redhat'
  distribution_version = run_command(find_program('sh'),
                                     '-c',
                                     '. /etc/os-release && ' +
                                     'echo ${VESRION_ID} | ' +
                                     'grep -oE "^[0-9]+"',
                                     capture: true,
                                     check: true).stdout()
  package_platform += distribution_version
endif
if package_platform == ''
  package_platform = 'unknown'
endif


default_connection_spec = get_option('default-connection-spec')
if default_connection_spec == ''
  if package_platform == 'debian'
    default_connection_spec = \
      'unix:/run/@0@/@0@.sock'.format(meson.project_name(),
                                      meson.project_name())
  elif (package_platform == 'redhat' or
        package_platform == 'freebsd' or
        package_platform == 'pkgsrc')
    default_connection_spec = \
      'unix:/var/run/@0@/@0@.sock'.format(meson.project_name(),
                                          meson.project_name())
  else
    default_connection_spec = 'inet:10025@[127.0.0.1]'
  endif
endif


default_effective_group = get_option('default-effective-group')
default_effective_user = get_option('default-effective-user')
default_pid_file = get_option('default-pid-file')
default_socket_group = get_option('default-socket-group')
rcddir = get_option('rcddir')
package_options = get_option('package-options')
if rcddir != ''
  if package_options == ''
    package_options = 'rcddir=@0@'.format(rcddir)
  else
    package_options = 'rcddir=@0@,@0@'.format(rcddir, package_options)
  endif
endif


config_h_conf = configuration_data()
config_h_conf.set_quoted('CONFIG_DIR',
                         prefix /
                         get_option('sysconfdir') /
                         meson.project_name())
config_h_conf.set_quoted('CONFIG_FILE_NAME',
                         '@0@.conf'.format(meson.project_name()))
config_h_conf.set_quoted('CUSTOM_CONFIG_FILE_NAME',
                         '@0@.custom.conf'.format(meson.project_name()))
config_h_conf.set_quoted('CONFIGURATION_MODULE_DIR',
                         prefix /
                         get_option('libdir') /
                         meson.project_name() /
                         'module' /
                         'configuration')
config_h_conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h_conf.set('GLIB_VERSION_MIN_REQUIRED', glib_version_min_required)
config_h_conf.set_quoted('LOCALEDIR',
                         prefix / get_option('localedir'))
config_h_conf.set_quoted('MILTER_MANAGER_DEFAULT_CONNECTION_SPEC',
                         default_connection_spec)
if default_effective_group == ''
  config_h_conf.set('MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP', 'NULL')
else
  config_h_conf.set_quoted('MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP',
                           default_effective_group)
endif
if default_effective_user == ''
  config_h_conf.set('MILTER_MANAGER_DEFAULT_EFFECTIVE_USER', 'NULL')
else
  config_h_conf.set_quoted('MILTER_MANAGER_DEFAULT_EFFECTIVE_USER',
                           default_effective_user)
endif
if default_pid_file == ''
  config_h_conf.set('MILTER_MANAGER_DEFAULT_PID_FILE', 'NULL')
else
  config_h_conf.set_quoted('MILTER_MANAGER_DEFAULT_PID_FILE', default_pid_file)
endif
if default_socket_group == ''
  config_h_conf.set('MILTER_MANAGER_DEFAULT_SOCKET_GROUP', 'NULL')
else
  config_h_conf.set_quoted('MILTER_MANAGER_DEFAULT_SOCKET_GROUP',
                           default_socket_group)
endif
if package_options == ''
  config_h_conf.set('MILTER_MANAGER_PACKAGE_OPTIONS', 'NULL')
else
  config_h_conf.set_quoted('MILTER_MANAGER_PACKAGE_OPTIONS', package_options)
endif
config_h_conf.set_quoted('MILTER_MANAGER_PACKAGE_PLATFORM', package_platform)
config_h_conf.set_quoted('PACKAGE', meson.project_name())
config_h_conf.set_quoted('PREFIX', prefix)
config_h_conf.set_quoted('VERSION', meson.project_version())
config_h = configure_file(input: 'meson-config.h.in',
                          output: 'config.h',
                          configuration: config_h_conf)
config = declare_dependency(compile_args: '-DHAVE_CONFIG_H',
                            include_directories: include_directories('.'),
                            sources: config_h)

subdir('milter/core')
subdir('milter/client')
subdir('milter/server')
subdir('milter/manager')
subdir('libmilter')

if meson.version().version_compare('>=0.60.0')
  libruby = dependency('ruby-3.2', 'ruby-3.1', 'ruby-3.0', 'ruby-2.7', 'ruby',
                       required: true)
else
  # depencency():
  # Before 0.60.0 only a single dependency name was allowed.
  # https://mesonbuild.com/Reference-manual_functions.html#dependency
  foreach ruby: ['ruby-3.2', 'ruby-3.1', 'ruby-3.0', 'ruby-2.7', 'ruby']
    libruby = dependency(ruby, required: false)
    if libruby.found()
      break
    endif
  endforeach
  if not libruby.found()
    error('Ruby not found.')
  endif
endif

ruby_install_dir = get_option('ruby-install-dir')

if meson.version().version_compare('>=0.58.0')
  if ruby_install_dir == 'site'
    ruby_install_dir = libruby.get_variable('sitelibdir')
  elif ruby_install_dir == 'vendor'
    ruby_install_dir = libruby.get_variable('vendorlibdir')
  endif
  ruby = find_program(libruby.get_variable('ruby'), required: true)
else
  # dep.get_variable():
  # Before 0.58.0 `varname` was not implemented.
  # https://mesonbuild.com/Reference-manual_returned_dep.html#depget_variable
  if ruby_install_dir == 'site'
    ruby_install_dir = libruby.get_variable(pkgconfig: 'sitelibdir')
  elif ruby_install_dir == 'vendor'
    ruby_install_dir = libruby.get_variable(pkgconfig: 'vendorlibdir')
  endif
  ruby = find_program(libruby.get_variable(pkgconfig: 'ruby'), required: true)
endif

rb_glib2_gem_path = \
  run_command(ruby,
              '-e',
              'print(Gem::Specification.find_by_name("glib2").full_gem_path)',
              capture: true,
              check: true).stdout()
rb_glib2_compile_args = [
  '-I@0@/lib'.format(rb_glib2_gem_path),
]
rb_glib2 = declare_dependency(compile_args: rb_glib2_compile_args,
                              dependencies: [libruby])
subdir('binding/ruby')
subdir('module/configuration/ruby')
subdir('src')
subdir('tool')

python = import('python').find_installation(get_option('python-path'),
                                            required: get_option('python'))
if python.found()
  subdir('binding/python')
endif
