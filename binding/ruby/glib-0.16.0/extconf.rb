=begin
extconf.rb for Ruby/GLib extention library
=end

TOPDIR = File.expand_path(File.dirname(__FILE__) + '/..')
MKMF_GNOME2_DIR = TOPDIR + '/glib-0.16.0/src/lib'
SRCDIR = TOPDIR + '/glib-0.16.0/src'

$LOAD_PATH.unshift MKMF_GNOME2_DIR

PACKAGE_NAME = "glib2"

require 'mkmf-gnome2'

PKGConfig.have_package('gobject-2.0') or exit 1
PKGConfig.have_package('gthread-2.0')

setup_win32(PACKAGE_NAME)

have_func("g_spawn_close_id")
have_func("g_thread_init")
have_func("g_main_depth")
have_func("g_listenv")

have_func("rb_check_array_type")
have_func("rb_exec_recursive")
have_header('yarv.h')

def parse_config_override_option(name, config_name)
  value = with_config(name)
  CONFIG[config_name] = value if value
end
parse_config_override_option("prefix", "prefix")
parse_config_override_option("exec-prefix", "exec_prefix")
parse_config_override_option("libdir", "libdir")
parse_config_override_option("pkglibdir", "pkglibdir")
parse_config_override_option("bindingdir", "bindingdir")
parse_config_override_option("sitelibdir", "sitelibdir")
parse_config_override_option("sitearchdir", "sitearchdir")

create_makefile_at_srcdir(PACKAGE_NAME, SRCDIR, "-DRUBY_GLIB2_COMPILATION") do
  enum_type_prefix = "glib-enum-types"
  include_paths = PKGConfig.cflags_only_I("glib-2.0")
  headers = include_paths.split.inject([]) do |result, path|
    result + Dir.glob(File.join(path.sub(/^-I/, ""), "glib", "*.h"))
  end.reject do |file|
    /g(iochannel|scanner)\.h/ =~ file
  end
  glib_mkenums(enum_type_prefix, headers, "G_TYPE_", ["glib.h"])
end

create_top_makefile
File.open("Makefile", "a") do |makefile|
  makefile.puts
  makefile.puts("distdir:")
  makefile.puts("\t:")
end
