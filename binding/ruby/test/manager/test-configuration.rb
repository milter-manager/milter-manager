# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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
  def setup
    @configuration = Milter::Manager::Configuration.new
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
    assert_equal(100, @configuration.maintenance_interval)
    @configuration.maintenance_interval = 0
    assert_equal(0, @configuration.maintenance_interval)
  end

  def test_package
    @configuration.package_platform = "pkgsrc"
    assert_equal("pkgsrc", @configuration.package_platform)
    @configuration.package_options = "prefix=/etc"
    assert_equal("prefix=/etc", @configuration.package_options)
  end

  def test_add_egg
    assert_equal(0, @configuration.create_children.length)

    name = "child-milter"
    egg = Milter::Manager::Egg.new(name)
    egg.connection_spec = "unix:/tmp/socket"
    @configuration.add_egg(egg)
    assert_equal(1, @configuration.create_children.length)

    @configuration.clear_eggs
    assert_equal([], @configuration.eggs)
    assert_equal(0, @configuration.create_children.length)
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
package.platform = #{@configuration.package_platform.inspect}
package.options = #{@configuration.package_options.inspect}

security.privilege_mode = false
security.effective_user = nil
security.effective_group = nil

manager.connection_spec = "inet:10025@[127.0.0.1]"
manager.unix_socket_mode = 0660
manager.unix_socket_group = nil
manager.remove_unix_socket_on_create = true
manager.remove_unix_socket_on_close = true
manager.daemon = false
manager.pid_file = nil
manager.maintenance_interval = 100

controller.connection_spec = nil
controller.unix_socket_mode = 0660
controller.unix_socket_group = nil
controller.remove_unix_socket_on_create = true
controller.remove_unix_socket_on_close = true
EOD
                 @configuration.dump)
  end

  def test_dump_full
    loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    loader.security.effective_user = "nobody"
    loader.security.effective_group = "nogroup"

    loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection"
    end

    loader.define_milter("milter1") do |milter|
      milter.description = "The first milter"
      milter.connection_spec = "unix:/tmp/xxx"
      milter.add_applicable_condition("S25R")
    end

    loader.define_milter("milter2") do |milter|
      milter.description = "The second milter"
      milter.connection_spec = "inet:2929"
      milter.enabled = false
    end

    assert_equal(<<-EOD.strip,
package.platform = #{@configuration.package_platform.inspect}
package.options = #{@configuration.package_options.inspect}

security.privilege_mode = false
security.effective_user = "nobody"
security.effective_group = "nogroup"

manager.connection_spec = "inet:10025@[127.0.0.1]"
manager.unix_socket_mode = 0660
manager.unix_socket_group = nil
manager.remove_unix_socket_on_create = true
manager.remove_unix_socket_on_close = true
manager.daemon = false
manager.pid_file = nil
manager.maintenance_interval = 100

controller.connection_spec = nil
controller.unix_socket_mode = 0660
controller.unix_socket_group = nil
controller.remove_unix_socket_on_create = true
controller.remove_unix_socket_on_close = true

define_applicable_condition("S25R") do |condition|
  condition.description = "Selective SMTP Rejection"
end

define_milter("milter1") do |milter|
  milter.connection_spec = "unix:/tmp/xxx"
  milter.description = "The first milter"
  milter.enabled = true
  milter.fallback_status = "accept"
  milter.applicable_conditions = ["S25R"]
  milter.command = nil
  milter.command_options = nil
  milter.user_name = nil
  milter.connection_timeout = 300.0
  milter.writing_timeout = 10.0
  milter.reading_timeout = 10.0
  milter.end_of_message_timeout = 300.0
end

define_milter("milter2") do |milter|
  milter.connection_spec = "inet:2929"
  milter.description = "The second milter"
  milter.enabled = false
  milter.fallback_status = "accept"
  milter.applicable_conditions = []
  milter.command = nil
  milter.command_options = nil
  milter.user_name = nil
  milter.connection_timeout = 300.0
  milter.writing_timeout = 10.0
  milter.reading_timeout = 10.0
  milter.end_of_message_timeout = 300.0
end
EOD
                 @configuration.dump)
  end
end
