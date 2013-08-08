# Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
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

class TestConfiguration < Test::Unit::TestCase
  include MilterEventLoopTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loop = create_event_loop
  end

  def test_controller_connection_spec
    @configuration.controller_connection_spec = "inet:2929@localhost"
    assert_equal("inet:2929@localhost",
                 @configuration.controller_connection_spec)
    @configuration.controller_connection_spec = nil
    assert_nil(@configuration.controller_connection_spec)
  end

  def test_manager_connection_spec
    @configuration.manager_connection_spec = "inet:10025@localhost"
    assert_equal("inet:10025@localhost", @configuration.manager_connection_spec)
    @configuration.manager_connection_spec = nil
    assert_nil(@configuration.manager_connection_spec)
  end

  def test_maintenance_interval
    assert_equal(10, @configuration.maintenance_interval)
    @configuration.maintenance_interval = 0
    assert_equal(0, @configuration.maintenance_interval)
  end

  def test_suspend_time_on_unacceptable
    assert_equal(5, @configuration.suspend_time_on_unacceptable)
    @configuration.suspend_time_on_unacceptable = 10
    assert_equal(10, @configuration.suspend_time_on_unacceptable)
  end

  def test_max_connections
    assert_equal(0, @configuration.max_connections)
    @configuration.max_connections = 10
    assert_equal(10, @configuration.max_connections)
  end

  def test_max_file_descriptors
    assert_equal(0, @configuration.max_file_descriptors)
    @configuration.max_file_descriptors = 10
    assert_equal(10, @configuration.max_file_descriptors)
  end

  def test_max_connection_check_interval
    assert_equal(0, @configuration.connection_check_interval)
    @configuration.connection_check_interval = 10
    assert_equal(10, @configuration.connection_check_interval)
  end

  def test_event_loop_backend
    assert_equal("glib", @configuration.event_loop_backend.nick)
    @configuration.event_loop_backend = "libev"
    assert_equal("libev", @configuration.event_loop_backend.nick)
    @configuration.event_loop_backend = "glib"
    assert_equal("glib", @configuration.event_loop_backend.nick)
  end

  def test_n_workers
    assert_equal(0, @configuration.n_workers)
    @configuration.n_workers = 10
    assert_equal(10, @configuration.n_workers)
  end

  def test_default_packet_buffer_size
    assert_equal(0, @configuration.default_packet_buffer_size)
    @configuration.default_packet_buffer_size = 4096
    assert_equal(4096, @configuration.default_packet_buffer_size)
  end

  def test_max_pending_finished_sessions
    assert_equal(0, @configuration.max_pending_finished_sessions)
    @configuration.max_pending_finished_sessions = 29
    assert_equal(29, @configuration.max_pending_finished_sessions)
  end

  def test_package
    @configuration.package_platform = "pkgsrc"
    assert_equal("pkgsrc", @configuration.package_platform)
    @configuration.package_options = "prefix=/etc"
    assert_equal("prefix=/etc", @configuration.package_options)
    assert_equal({"prefix" => "/etc"}, @configuration.parsed_package_options)
  end

  def test_package_options_with_spaces
    @configuration.package_options = "prefix = /etc , DESTDIR = /tmp/local"
    assert_equal("prefix = /etc , DESTDIR = /tmp/local",
                 @configuration.package_options)
    assert_equal({"prefix" => "/etc", "DESTDIR" => "/tmp/local"},
                 @configuration.parsed_package_options)
  end

  def test_add_egg
    assert_equal(0, create_children.length)

    name = "child-milter"
    egg = Milter::Manager::Egg.new(name)
    egg.connection_spec = "unix:/tmp/socket"
    @configuration.add_egg(egg)
    assert_equal(1, create_children.length)

    @configuration.clear_eggs
    assert_equal([], @configuration.eggs)
    assert_equal(0, create_children.length)
  end

  def test_find_egg
    egg1 = Milter::Manager::Egg.new("milter1")
    egg2 = Milter::Manager::Egg.new("milter2")
    @configuration.add_egg(egg1)
    @configuration.add_egg(egg2)

    assert_equal(egg1, @configuration.find_egg("milter1"))
    assert_nil(@configuration.find_egg("nonexistent"))
  end

  def test_remove_egg
    egg1 = Milter::Manager::Egg.new("milter1")
    egg2 = Milter::Manager::Egg.new("milter2")
    @configuration.add_egg(egg1)
    @configuration.add_egg(egg2)

    assert_equal([egg1, egg2], @configuration.eggs)
    @configuration.remove_egg("milter1")
    assert_equal([egg2], @configuration.eggs)
    @configuration.remove_egg(egg2)
    assert_equal([], @configuration.eggs)
  end

  def test_applicable_condition
    assert_equal([], @configuration.applicable_conditions)

    name = "S25R"
    condition = Milter::Manager::ApplicableCondition.new(name)
    @configuration.add_applicable_condition(condition)
    assert_equal([name],
                 @configuration.applicable_conditions.collect {|cond| cond.name})

    @configuration.clear_applicable_conditions
    assert_equal([], @configuration.applicable_conditions)
  end

  def test_find_applicable_condition
    s25r = Milter::Manager::ApplicableCondition.new("S25R")
    remote_network = Milter::Manager::ApplicableCondition.new("Remote Network")
    @configuration.add_applicable_condition(s25r)
    @configuration.add_applicable_condition(remote_network)

    assert_equal(s25r, @configuration.find_applicable_condition("S25R"))
    assert_nil(@configuration.find_applicable_condition("nonexistent"))
  end

  def test_remove_applicable_condition
    s25r = Milter::Manager::ApplicableCondition.new("S25R")
    remote_network = Milter::Manager::ApplicableCondition.new("Remote Network")
    @configuration.add_applicable_condition(s25r)
    @configuration.add_applicable_condition(remote_network)

    assert_equal([s25r, remote_network], @configuration.applicable_conditions)
    @configuration.remove_applicable_condition("S25R")
    assert_equal([remote_network], @configuration.applicable_conditions)
    @configuration.remove_applicable_condition(remote_network)
    assert_equal([], @configuration.applicable_conditions)
  end

  def test_to_xml
    assert_equal(["<configuration>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)

    name = "child-milter"
    spec = "unix:/tmp/socket"
    egg = Milter::Manager::Egg.new(name)
    egg.connection_spec = spec
    @configuration.add_egg(egg)
    assert_equal(["<configuration>",
                  "  <milters>",
                  "    <milter>",
                  "      <name>#{name}</name>",
                  "      <enabled>true</enabled>",
                  "      <fallback-status>accept</fallback-status>",
                  "      <evaluation-mode>false</evaluation-mode>",
                  "      <connection-spec>#{spec}</connection-spec>",
                  "    </milter>",
                  "  </milters>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)

    @configuration.signal_connect("to-xml") do |_, xml, indent|
      xml << " " * indent
      xml << "<additional-info>INFO</additional-info>\n"
    end
    assert_equal(["<configuration>",
                  "  <milters>",
                  "    <milter>",
                  "      <name>#{name}</name>",
                  "      <enabled>true</enabled>",
                  "      <fallback-status>accept</fallback-status>",
                  "      <evaluation-mode>false</evaluation-mode>",
                  "      <connection-spec>#{spec}</connection-spec>",
                  "    </milter>",
                  "  </milters>",
                  "  <additional-info>INFO</additional-info>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)
  end

  def test_load_paths
    @configuration.clear_load_paths
    assert_equal([], @configuration.load_paths)

    @configuration.append_load_path("XXX")
    assert_equal(["XXX"], @configuration.load_paths)

    @configuration.prepend_load_path("prepend")
    assert_equal(["prepend", "XXX"], @configuration.load_paths)

    @configuration.append_load_path("append")
    assert_equal(["prepend", "XXX", "append"], @configuration.load_paths)

    @configuration.clear_load_paths
    assert_equal([], @configuration.load_paths)
  end

  def test_dump
    assert_equal(<<-EOD.strip,
# default
package.platform = #{@configuration.package_platform.inspect}
# default
package.options = #{@configuration.package_options.inspect}

# default
security.privilege_mode = false
# default
security.effective_user = nil
# default
security.effective_group = nil

# default
log.level = "none"
# default
log.use_syslog = true
# default
log.syslog_facility = nil

# default
manager.connection_spec = #{@configuration.manager_connection_spec.inspect}
# default
manager.unix_socket_mode = 0660
# default
manager.unix_socket_group = nil
# default
manager.remove_unix_socket_on_create = true
# default
manager.remove_unix_socket_on_close = true
# default
manager.daemon = false
# default
manager.pid_file = nil
# default
manager.maintenance_interval = 10
# default
manager.suspend_time_on_unacceptable = 5
# default
manager.max_connections = 0
# default
manager.max_file_descriptors = 0
# default
manager.custom_configuration_directory = nil
# default
manager.fallback_status = "accept"
# default
manager.fallback_status_at_disconnect = "temporary-failure"
# default
manager.event_loop_backend = "glib"
# default
manager.n_workers = 0
# default
manager.packet_buffer_size = 0
# default
manager.connection_check_interval = 0
# default
manager.chunk_size = 65535
# default
manager.max_pending_finished_sessions = 0

# default
controller.connection_spec = nil
# default
controller.unix_socket_mode = 0660
# default
controller.unix_socket_group = nil
# default
controller.remove_unix_socket_on_create = true
# default
controller.remove_unix_socket_on_close = true

# default
database.type = nil
# default
database.name = nil
# default
database.host = nil
# default
database.port = nil
# default
database.path = nil
# default
database.user = nil
# default
database.password = nil
EOD
                 @configuration.dump)
  end

  def test_dump_full
    loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    loader.security.effective_user = "nobody"; effective_user_line = __LINE__
    loader.security.effective_group = "nogroup"; effective_group_line = __LINE__

    manager_unix_socket_group = __LINE__ + 1
    loader.manager.unix_socket_group = "nogroup"
    manager_pid_file = __LINE__ + 1
    loader.manager.pid_file = "/tmp/milter-manager.pid"
    manager_custom_configuration_directory = __LINE__ + 1
    loader.manager.custom_configuration_directory = "/tmp/milter-manager/"

    controller_connection_spec = __LINE__ + 1
    loader.controller.connection_spec = "inet:10025"
    controller_unix_socket_group = __LINE__ + 1
    loader.controller.unix_socket_group = "nogroup"

    database_type = __LINE__ + 1
    loader.database.type = "sqlite3"
    database_name = __LINE__ + 1
    loader.database.name = ":memory:"

    s25r_lines = {}
    s25r_lines[:define] = __LINE__ + 1
    loader.define_applicable_condition("S25R") do |condition|
      s25r_lines[:description] = __LINE__ + 1
      condition.description = "Selective SMTP Rejection"
    end

    milter1_lines = {}
    milter1_lines[:define] = __LINE__ + 1
    loader.define_milter("milter1") do |milter|
      milter1_lines[:description] = __LINE__ + 1
      milter.description = "The first milter"
      milter1_lines[:connection_spec] = __LINE__ + 1
      milter.connection_spec = "unix:/tmp/xxx"
      milter1_lines[:applicable_condition_s25r] = __LINE__ + 1
      milter.add_applicable_condition("S25R")
    end

    milter2_lines = {}
    milter2_lines[:define] = __LINE__ + 1
    loader.define_milter("milter2") do |milter|
      milter2_lines[:description] = __LINE__ + 1
      milter.description = "The second milter"
      milter2_lines[:connection_spec] = __LINE__ + 1
      milter.connection_spec = "inet:2929"
      milter2_lines[:enabled] = __LINE__ + 1
      milter.enabled = false
      milter2_lines[:evaluation_mode] = __LINE__ + 1
      milter.evaluation_mode = true
    end

    assert_equal(<<-EOD.strip,
# default
package.platform = #{@configuration.package_platform.inspect}
# default
package.options = #{@configuration.package_options.inspect}

# default
security.privilege_mode = false
# #{__FILE__}:#{effective_user_line}
security.effective_user = "nobody"
# #{__FILE__}:#{effective_group_line}
security.effective_group = "nogroup"

# default
log.level = "none"
# default
log.use_syslog = true
# default
log.syslog_facility = nil

# default
manager.connection_spec = #{@configuration.manager_connection_spec.inspect}
# default
manager.unix_socket_mode = 0660
# #{__FILE__}:#{manager_unix_socket_group}
manager.unix_socket_group = "nogroup"
# default
manager.remove_unix_socket_on_create = true
# default
manager.remove_unix_socket_on_close = true
# default
manager.daemon = false
# #{__FILE__}:#{manager_pid_file}
manager.pid_file = "/tmp/milter-manager.pid"
# default
manager.maintenance_interval = 10
# default
manager.suspend_time_on_unacceptable = 5
# default
manager.max_connections = 0
# default
manager.max_file_descriptors = 0
# #{__FILE__}:#{manager_custom_configuration_directory}
manager.custom_configuration_directory = "/tmp/milter-manager/"
# default
manager.fallback_status = "accept"
# default
manager.fallback_status_at_disconnect = "temporary-failure"
# default
manager.event_loop_backend = "glib"
# default
manager.n_workers = 0
# default
manager.packet_buffer_size = 0
# default
manager.connection_check_interval = 0
# default
manager.chunk_size = 65535
# default
manager.max_pending_finished_sessions = 0

# #{__FILE__}:#{controller_connection_spec}
controller.connection_spec = "inet:10025"
# default
controller.unix_socket_mode = 0660
# #{__FILE__}:#{controller_unix_socket_group}
controller.unix_socket_group = "nogroup"
# default
controller.remove_unix_socket_on_create = true
# default
controller.remove_unix_socket_on_close = true

# #{__FILE__}:#{database_type}
database.type = "sqlite3"
# #{__FILE__}:#{database_name}
database.name = ":memory:"
# default
database.host = nil
# default
database.port = nil
# default
database.path = nil
# default
database.user = nil
# default
database.password = nil

# #{__FILE__}:#{s25r_lines[:define]}
define_applicable_condition("S25R") do |condition|
  # #{__FILE__}:#{s25r_lines[:description]}
  condition.description = "Selective SMTP Rejection"
end

# #{__FILE__}:#{milter1_lines[:define]}
define_milter("milter1") do |milter|
  # #{__FILE__}:#{milter1_lines[:connection_spec]}
  milter.connection_spec = "unix:/tmp/xxx"
  # #{__FILE__}:#{milter1_lines[:description]}
  milter.description = "The first milter"
  # default
  milter.enabled = true
  # default
  milter.fallback_status = "accept"
  # default
  milter.evaluation_mode = false
  milter.applicable_conditions = [
    # #{__FILE__}:#{milter1_lines[:applicable_condition_s25r]}
    "S25R",
  ]
  # default
  milter.command = nil
  # default
  milter.command_options = nil
  # default
  milter.user_name = nil
  # default
  milter.connection_timeout = 297.0
  # default
  milter.writing_timeout = 7.0
  # default
  milter.reading_timeout = 7.0
  # default
  milter.end_of_message_timeout = 297.0
end

# #{__FILE__}:#{milter2_lines[:define]}
define_milter("milter2") do |milter|
  # #{__FILE__}:#{milter2_lines[:connection_spec]}
  milter.connection_spec = "inet:2929"
  # #{__FILE__}:#{milter2_lines[:description]}
  milter.description = "The second milter"
  # #{__FILE__}:#{milter2_lines[:enabled]}
  milter.enabled = false
  # default
  milter.fallback_status = "accept"
  # #{__FILE__}:#{milter2_lines[:evaluation_mode]}
  milter.evaluation_mode = true
  # default
  milter.applicable_conditions = []
  # default
  milter.command = nil
  # default
  milter.command_options = nil
  # default
  milter.user_name = nil
  # default
  milter.connection_timeout = 297.0
  # default
  milter.writing_timeout = 7.0
  # default
  milter.reading_timeout = 7.0
  # default
  milter.end_of_message_timeout = 297.0
end
EOD
                 @configuration.dump)
  end

  def test_locations
    assert_equal({}, @configuration.locations)
    @configuration.set_location("security.privilege_mode",
                                "milter-manager.conf", 29)
    assert_equal({
                   "file" => "milter-manager.conf",
                   "line" => 29
                 },
                 @configuration.get_location("security.privilege_mode"))
    assert_equal({
                   "security.privilege_mode" => {
                     "file" => "milter-manager.conf",
                     "line" => 29
                   }
                 },
                 @configuration.locations)

    @configuration.reset_location("security.privilege_mode")
    assert_equal({}, @configuration.locations)
  end

  private
  def create_children
    children = Milter::Manager::Children.new(@configuration, @loop)
    client_context = Milter::ClientContext.new
    @configuration.setup_children(children, client_context)
    children
  end
end
