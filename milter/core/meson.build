
sources = files(
  # 'milter-enum-types.c',
  # 'milter-marshalers.c',
  # 'milter-marshalers.h',
  'milter-core.c',
  'milter-protocol.c',
  'milter-decoder.c',
  'milter-command-decoder.c',
  'milter-reply-decoder.c',
  'milter-encoder.c',
  'milter-command-encoder.c',
  'milter-reply-encoder.c',
  'milter-error-emittable.c',
  'milter-finished-emittable.c',
  'milter-agent.c',
  'milter-protocol-agent.c',
  'milter-macros-requests.c',
  'milter-option.c',
  'milter-reader.c',
  'milter-writer.c',
  'milter-headers.c',
  'milter-logger.c',
  'milter-syslog-logger.c',
  'milter-reply-signals.c',
  'milter-utils.c',
  'milter-connection.c',
  'milter-esmtp.c',
  'milter-message-result.c',
  'milter-session-result.c',
  'milter-event-loop.c',
  'milter-libev-event-loop.c',
  'milter-glib-event-loop.c',
  'milter-core-internal.h',
)

headers = files(
  'milter-protocol.h',
  'milter-decoder.h',
  'milter-command-decoder.h',
  'milter-reply-decoder.h',
  'milter-encoder.h',
  'milter-command-encoder.h',
  'milter-reply-encoder.h',
  'milter-error-emittable.h',
  'milter-finished-emittable.h',
  'milter-agent.h',
  'milter-protocol-agent.h',
  'milter-macros-requests.h',
  'milter-option.h',
  'milter-reader.h',
  'milter-writer.h',
  'milter-headers.h',
  'milter-logger.h',
  'milter-syslog-logger.h',
  'milter-reply-signals.h',
  'milter-utils.h',
  'milter-connection.h',
  'milter-esmtp.h',
  'milter-message-result.h',
  'milter-session-result.h',
  'milter-event-loop.h',
  'milter-libev-event-loop.h',
  'milter-glib-event-loop.h',
)

version_h_conf = configuration_data()
version_components = meson.version().split('.')
version_h_conf.set('MILTER_MANAGER_VERSION', meson.version())
version_h_conf.set('MILTER_MANAGER_VERSION_MAJOR', version_components[0])
version_h_conf.set('MILTER_MANAGER_VERSION_MINOR', version_components[1])
version_h_conf.set('MILTER_MANAGER_VERSION_MICRO', version_components[2])
version_h = configure_file(input: 'milter-version.h.in',
                           output: 'milter-version.h',
                           configuration: version_h_conf)
headers += version_h

# enums = gnome.mkenums('milter-enum-types',
#                       sources: headers,
#                       identifier_prefix: 'Milter',
#                       symbol_prefix: 'milter',
#                       c_template: 'enums.c.template',
#                       h_template: 'enums.h.template',
#                       install_dir: join_paths(include_dir, 'milter', 'core'),
#                       install_header: true)
enums = gnome.mkenums_simple('milter-enum-types', sources: headers)
enums_source = enums[0]
enums_header = enums[1]

install_headers(headers, subdir: join_paths('milter', 'core'))

gobject = dependency('gobject-2.0')
gthread = dependency('gthread-2.0')
dependencies = [
  gobject,
  gthread,
]
milter_core = library('milter-core',
                      sources: sources + enums,
                      install: true,
                      dependencies: dependencies,
                      include_directories: base_include_directories,
                      soversion: so_version,
                      version: library_version)

pkgconfig.generate(milter_core,
                   description: 'common milter features',
                   filebase: 'milter-core',
                   name: 'milter core library',
                   requires: ['gobject-2.0', 'gthread-2.0'],
                   variables: pkgconfig_variables,
                   version: meson.version())

milter_core_gir = gnome.generate_gir(milter_core,
                                     export_packages: 'milter-core',
                                     extra_args: [
                                       '--warn-all',
                                     ],
                                     fatal_warnings: true,
                                     header: 'milter/core.h',
                                     identifier_prefix: 'Milter',
                                     includes: [
                                       'GObject-2.0',
                                     ],
                                     install: true,
                                     namespace: 'Milter',
                                     nsversion: api_version,
                                     sources: sources + headers + enums,
                                     symbol_prefix: 'milter')
