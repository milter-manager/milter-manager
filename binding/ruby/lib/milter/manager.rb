# Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

require 'pathname'
require 'shellwords'
require 'erb'
require 'yaml'
require "English"

require "rexml/document"
require "rexml/streamlistener"

require 'milter'
require 'milter_manager.so'

require 'milter/compatible'

require 'milter/manager/exception'

require 'milter/manager/child-context'
require 'milter/manager/connection-check-context'
require 'milter/manager/netstat-connection-checker'

require 'milter/manager/policy-manager'
require 'milter/manager/address-matcher'
require 'milter/manager/breaker'

require 'milter/manager/debian-init-detector'
require 'milter/manager/redhat-detector'
require 'milter/manager/freebsd-rc-detector'
require 'milter/manager/pkgsrc-rc-detector'

require 'milter/manager/postfix-cidr-table'
require 'milter/manager/postfix-regexp-table'

Milter::Logger.domain = "milter-manager"

module Milter::Manager
  class Configuration
    attr_reader :database

    def dump
      ConfigurationDumper.new(self).dump
    rescue Exception
      [$!.message, $@].join("\n")
    end

    def parsed_package_options
      _package_options = package_options
      return nil if _package_options.nil?

      options = {}
      _package_options.split(/\s*,\s*/).each do |option|
        name, value = option.split(/\s*=\s*/, 2)
        options[name] = value
      end
      options
    end

    def maintained
      maintained_hooks.each do |hook|
        hook.call(self)
      end
    end

    def maintained_hooks
      @maintained_hooks ||= []
    end

    def clear
      @maintained_hooks = nil
      @netstat_connection_checker = nil
      @database = Database.new
    end

    def netstat_connection_checker
      @netstat_connection_checker ||= NetstatConnectionChecker.new
    end

    class Database
      attr_accessor :type, :name, :host, :port, :path
      attr_accessor :user, :password
      def initialize
        @type = nil
        @name = nil
        @host = nil
        @port = nil
        @path = nil
        @user = nil
        @password = nil
      end

      def to_hash
        {
          :type => @type,
          :name => @name,
          :host => @host,
          :port => @port,
          :path => @path,
          :user => @user,
          :password => @password,
        }
      end
    end
  end

  class ConfigurationDumper
    def initialize(configuration)
      @configuration = configuration
      @locations = configuration.locations
    end

    def dump
      @result = ''

      dump_package_items
      dump_security_items
      dump_manager_items
      dump_controller_items
      dump_database_items
      dump_applicable_condition_items
      dump_egg_items

      @result.strip
    end

    private
    def dump_package_items
      c = @configuration
      dump_item("package.platform", c.package_platform.inspect)
      dump_item("package.options", c.package_options.inspect)
      @result << "\n"
    end

    def dump_security_items
      c = @configuration
      dump_item("security.privilege_mode", c.privilege_mode?)
      dump_item("security.effective_user", c.effective_user.inspect)
      dump_item("security.effective_group", c.effective_group.inspect)
      @result << "\n"
    end

    def dump_manager_items
      c = @configuration
      dump_item("manager.connection_spec", c.manager_connection_spec.inspect)
      dump_item("manager.unix_socket_mode", "%#o" % c.manager_unix_socket_mode)
      dump_item("manager.unix_socket_group", c.manager_unix_socket_group.inspect)
      dump_item("manager.remove_unix_socket_on_create",
                c.remove_manager_unix_socket_on_create?)
      dump_item("manager.remove_unix_socket_on_close",
                c.remove_manager_unix_socket_on_close?)
      dump_item("manager.daemon", c.daemon?)
      dump_item("manager.pid_file", c.pid_file.inspect)
      _maintenance_interval = c.maintenance_interval
      _maintenance_interval = nil if _maintenance_interval.zero?
      dump_item("manager.maintenance_interval", _maintenance_interval.inspect)
      dump_item("manager.suspend_time_on_unacceptable",
                c.suspend_time_on_unacceptable.inspect)
      dump_item("manager.max_connections", c.max_connections.inspect)
      dump_item("manager.max_file_descriptors", c.max_file_descriptors.inspect)
      dump_item("manager.custom_configuration_directory",
                c.custom_configuration_directory.inspect)
      dump_item("manager.fallback_status", c.fallback_status.nick.dump)
      dump_item("manager.fallback_status_at_disconnect",
                c.fallback_status_at_disconnect.nick.dump)
      dump_item("manager.event_loop_backend",
                c.event_loop_backend.nick.dump)
      dump_item("manager.n_workers", c.n_workers)
      dump_item("manager.packet_buffer_size", c.default_packet_buffer_size)
      dump_item("manager.connection_check_interval",
                c.connection_check_interval.inspect)
      @result << "\n"
    end

    def dump_controller_items
      c = @configuration
      dump_item("controller.connection_spec",
                c.controller_connection_spec.inspect)
      dump_item("controller.unix_socket_mode",
                "0%o" % c.controller_unix_socket_mode)
      dump_item("controller.unix_socket_group",
                c.controller_unix_socket_group.inspect)
      dump_item("controller.remove_unix_socket_on_create",
                c.remove_controller_unix_socket_on_create?)
      dump_item("controller.remove_unix_socket_on_close",
                c.remove_controller_unix_socket_on_close?)
      @result << "\n"
    end

    def dump_database_items
      c = @configuration
      dump_item("database.type", c.database.type.inspect)
      dump_item("database.name", c.database.name.inspect)
      dump_item("database.host", c.database.host.inspect)
      dump_item("database.port", c.database.port.inspect)
      dump_item("database.path", c.database.path.inspect)
      dump_item("database.user", c.database.user.inspect)
      dump_item("database.password", c.database.password.inspect)
      @result << "\n"
    end

    def dump_applicable_condition_items
      @configuration.applicable_conditions.each do |condition|
        dump_applicable_condition(condition)
        @result << "\n"
      end
    end

    def dump_egg_items
      @configuration.eggs.each do |egg|
        dump_egg(egg)
        @result << "\n"
      end
    end

    def dump_location(key, indent="")
      if key.nil?
        location = nil
      else
        location = @locations[key]
      end
      if location
        @result << "#{indent}# #{location['file']}:#{location['line']}\n"
      else
        @result << "#{indent}# default\n"
      end
    end

    def dump_item(key, value, indent="", location_key=nil)
      dump_location(location_key || key, indent)
      @result << "#{indent}#{key} = #{value}\n"
    end

    def dump_applicable_condition_item(name, key, value)
      dump_item("condition.#{key}",
                value,
                "  ",
                "applicable_condition[#{name}].#{key}")
    end

    def dump_applicable_condition(condition)
      name = condition.name
      dump_location("applicable_condition[#{name}]")
      @result << "define_applicable_condition(#{name.inspect}) do |condition|\n"
      dump_applicable_condition_item(name, "description",
                                     condition.description.inspect)
      @result << "end\n"
    end

    def dump_egg_item(name, key, value)
      dump_item("milter.#{key}",
                value,
                "  ",
                "milter[#{name}].#{key}")
    end

    def dump_egg_applicable_conditions(egg)
      conditions = egg.applicable_conditions
      if conditions.empty?
        dump_location(nil, "  ")
        @result << "  milter.applicable_conditions = []\n"
      else
        egg_name = egg.name
        @result << "  milter.applicable_conditions = [\n"
        conditions.each do |condition|
          name = condition.name
          dump_location("milter[#{egg.name}].applicable_condition[#{name}]",
                        "    ")
          @result << "    #{name.inspect},\n"
        end
        @result << "  ]\n"
      end
    end

    def dump_egg(egg)
      name = egg.name
      dump_location("milter[#{name}]")
      @result << "define_milter(#{name.inspect}) do |milter|\n"
      dump_egg_item(name, "connection_spec", egg.connection_spec.inspect)
      dump_egg_item(name, "description", egg.description.inspect)
      dump_egg_item(name, "enabled", egg.enabled?)
      dump_egg_item(name, "fallback_status", egg.fallback_status.nick.inspect)
      dump_egg_item(name, "evaluation_mode", egg.evaluation_mode?)
      dump_egg_applicable_conditions(egg)
      dump_egg_item(name, "command", egg.command.inspect)
      dump_egg_item(name, "command_options", egg.command_options.inspect)
      dump_egg_item(name, "user_name", egg.user_name.inspect)
      dump_egg_item(name, "connection_timeout", egg.connection_timeout)
      dump_egg_item(name, "writing_timeout", egg.writing_timeout)
      dump_egg_item(name, "reading_timeout", egg.reading_timeout)
      dump_egg_item(name, "end_of_message_timeout", egg.end_of_message_timeout)
      @result << "end\n"
    end
  end

  class ConfigurationLoader
    class Error < Milter::Manager::Error
    end

    class InvalidValue < Error
      def initialize(target, available_values, actual_value)
        @target = target
        @available_values = available_values
        @actual_value = actual_value
        super("#{@target} should be one of #{@available_values.inspect} " +
              "but was #{@actual_value.inspect}")
      end
    end

    class NonexistentPath < Error
      def initialize(path)
        @path = path
        super("#{@path} doesn't exist.")
      end
    end

    class MissingValue < Error
      def initialize(target)
        @target = target
        super("#{@target} should be set")
      end
    end

    class << self
      @@current_configuration = nil
      def load(configuration, file)
        @@current_configuration = configuration
        guard do
          new(configuration).load_configuration(file)
        end
        GC.start
      end

      def load_custom(configuration, file)
        @@current_configuration = configuration
        guard do
          new(configuration).load_custom_configuration(file)
        end
        GC.start
      end

      def guard(fallback_value=nil)
        yield
      rescue Exception => error
        Milter::Logger.error(error)
        fallback_value
      end

      def update_location(configuration, key, reset, deep_level)
        if reset
          configuration.reset_location(key)
        else
          file, line, _ = caller[deep_level].split(/:/, 3)
          configuration.set_location(key, file, line.to_i)
        end
      end
    end

    attr_reader :package, :security, :controller, :manager, :database
    attr_reader :configuration
    def initialize(configuration)
      @load_level = 0
      @configuration = configuration
      @package = PackageConfiguration.new(configuration)
      @security = SecurityConfiguration.new(configuration)
      @controller = ControllerConfiguration.new(configuration)
      @manager = ManagerConfiguration.new(configuration)
      @database = DatabaseConfiguration.new(configuration)
      @policy_manager = Milter::Manager::PolicyManager.new(self)
    end

    def load_configuration(file)
      begin
        @load_level += 1
        content = File.read(file)
        Milter::Logger.debug("loading configuration file: '#{file}'")
        instance_eval(content, file)
        apply_policies if @load_level == 1
      rescue InvalidValue
        backtrace = $!.backtrace.collect do |info|
          if /\A#{Regexp.escape(file)}:/ =~ info
            info
          else
            nil
          end
        end.compact
        Milter::Logger.error("#{backtrace[0]}: #{$!.message}")
        Milter::Logger.error(backtrace[1..-1].join("\n"))
      ensure
        @load_level -= 1
      end
    end

    def load_custom_configuration(file)
      first_line = File.open(file) {|config| config.gets}
      case first_line
      when /\A\s*</m
        XMLConfigurationLoader.new(self).load(file)
      # when /\A#\s*-\*-\s*yaml\s*-\*-/i
      #   YAMLConfigurationLoader.new(self).load(file)
      # else
      #   load_configuration(file)
      end
    end

    def load(path)
      resolve_path(path).each do |full_path|
        if File.directory?(full_path)
          Dir.open(full_path) do |dir|
            dir.each do |sub_path|
              next if sub_path == "." or sub_path == ".."
              full_sub_path = File.join(full_path, sub_path)
              next if File.directory?(full_sub_path)
              load_configuration(full_sub_path)
            end
          end
        elsif File.exist?(full_path)
          load_configuration(full_path)
        else
          Dir.glob(full_path).each do |_path|
            next if File.directory?(_path)
            load_configuration(_path)
          end
        end
      end
    end

    def load_if_exist(path)
      load(path)
    rescue NonexistentPath
      # ignore. log this?
    end

    def load_default
      platform = @configuration.package_platform
      load_if_exist("defaults/#{platform}.conf") if platform
    end

    def define_milter(name, &block)
      ConfigurationLoader.update_location(@configuration,
                                          "milter[#{name}]",
                                          false, 1)
      egg_configuration = EggConfiguration.new(name, self)
      yield(egg_configuration)
      egg_configuration.apply
    end

    # more better name is needed?
    def remove_milter(name)
      @configuration.remove_egg(name)
    end

    def define_applicable_condition(name)
      ConfigurationLoader.update_location(@configuration,
                                          "applicable_condition[#{name}]",
                                          false, 1)
      condition_configuration = ApplicableConditionConfiguration.new(name, self)
      yield(condition_configuration)
      condition_configuration.apply
    end

    def defined_milters
      @configuration.eggs.collect do |egg|
        egg.name
      end
    end

    def define_policy(name)
      policy = Policy.new(name)
      yield(policy)
      @policy_manager << policy
    end

    private
    def resolve_path(path)
      return [path] if Pathname(path).absolute?
      resolved_paths = @configuration.load_paths.collect do |load_path|
        full_path = File.join(load_path, path)
        if File.directory?(full_path)
          return [full_path]
        elsif File.exist?(full_path)
          return [full_path]
        else
          Dir.glob(full_path)
        end
      end.flatten.compact
      raise NonexistentPath.new(path) if resolved_paths.empty?
      resolved_paths
    end

    def apply_policies
      @policy_manager.apply
    end

    class XMLConfigurationLoader
      def initialize(loader)
        @loader = loader
      end

      def load(file)
        listener = Listener.new(@loader)
        File.open(file) do |input|
          Milter::Logger.debug("loading XML configuration file: '#{file}'")
          parser = REXML::Parsers::StreamParser.new(input, listener)
          begin
            parser.parse
          rescue InvalidValue
            source = parser.source
            info = "#{file}:#{source.line}:#{source.position}"
            info << ":#{source.current_line}: #{$!.message}"
            Milter::Logger.error(info)
          end
        end
      end

      class Listener
        include REXML::StreamListener

        def initialize(loader)
          @loader = loader
          @ns_stack = [{"xml" => :xml}]
          @tag_stack = [["", :root]]
          @text_stack = ['']
          @state_stack = [:root]
          @egg = nil
        end

        def tag_start(name, attributes)
          @text_stack.push('')

          ns = @ns_stack.last.dup
          attrs = {}
          attributes.each do |n, v|
            if /\Axmlns(?:\z|:)/ =~ n
              ns[$POSTMATCH] = v
            else
              attrs[n] = v
            end
          end
          @ns_stack.push(ns)

          prefix, local = split_name(name)
          uri = _ns(ns, prefix)
          @tag_stack.push([uri, local])

          @state_stack.push(next_state(@state_stack.last, uri, local))
        end

        def tag_end(name)
          state = @state_stack.pop
          text = @text_stack.pop
          uri, local = @tag_stack.pop
          no_action_states = [:root, :configuration, :security,
                              :controller, :manager, :milters,
                              :milter_applicable_conditions]
          case state
          when *no_action_states
            # do nothing
          when :milter
            @loader.define_milter(@egg_config["name"]) do |milter|
              spec = @egg_config["connection_spec"] || ""
              milter.connection_spec = spec unless spec.empty?
              milter.applicable_conditions = @egg_config["applicable_conditions"]
              if @egg_config.has_key?("enabled")
                milter.enabled = @egg_config["enabled"]
              end
              if @egg_config.has_key?("description")
                milter.description = @egg_config["description"]
              end
              if @egg_config.has_key?("command")
                milter.command = @egg_config["command"]
                milter.command_options = @egg_config["command_options"]
              end
              if @egg_config.has_key?("fallback_status")
                milter.fallback_status = @egg_config["fallback_status"]
              end
              if @egg_config.has_key?("evaluation_mode")
                milter.evaluation_mode = @egg_config["evaluation_mode"]
              end
            end
            @egg_config = nil
          when "milter_applicable_condition"
            @egg_config["applicable_conditions"] << text
          when "milter_enabled"
            @egg_config["enabled"] = text == "true"
          when "milter_evaluation_mode"
            @egg_config["evaluation_mode"] = text == "true"
          when /\Amilter_/
            @egg_config[$POSTMATCH] = text
          else
            local = normalize_local(local)
            @loader.send(@state_stack.last).send("#{local}=", text)
          end
          @ns_stack.pop
        end

        def text(data)
          @text_stack.last << data
        end

        private
        def _ns(ns, prefix)
          ns.fetch(prefix, "")
        end

        NAME_SPLIT = /^(?:([\w:][-\w\d.]*):)?([\w:][-\w\d.]*)/
        def split_name(name)
          name =~ NAME_SPLIT
          [$1 || '', $2]
        end

        def next_state(current_state, uri, local)
          local = normalize_local(local)
          case current_state
          when :root
            if local != "configuration"
              raise "root element must be <configuration>"
            end
            :configuration
          when :configuration
            case local
            when "security"
              :security
            when "controller"
              :controller
            when "manager"
              :manager
            when "milters"
              :milters
            else
              raise "unexpected element: #{current_path}"
            end
          when :security, :controller, :manager
            if @loader.send(current_state).respond_to?("#{local}=")
              local
            else
              raise "unexpected element: #{current_path}"
            end
          when :milter
            available_locals = ["name", "description",
                                "enabled", "connection_spec",
                                "command", "command_options",
                                "fallback_status", "evaluation_mode"]
            case local
            when "applicable_conditions"
              @egg_config["applicable_conditions"] = []
              :milter_applicable_conditions
            when *available_locals
              "milter_#{local}"
            else
              message = "unexpected element: #{current_path}: " +
                "available elements: #{available_locals.inspect}"
              raise message
            end
          when :milter_applicable_conditions
            if local == "applicable_condition"
              "milter_#{local}"
            else
              raise "unexpected element: #{current_path}"
            end
          when :milters
            if local == "milter"
              @egg_config = {}
              :milter
            else
              raise "unexpected element: #{current_path}"
            end
          else
            raise "unexpected element: #{current_path}"
          end
        end

        def current_path
          locals = @tag_stack.collect do |uri, local|
            local
          end
          ["", *locals].join("/")
        end

        def normalize_local(local)
          local.gsub(/-/, "_")
        end
      end
    end

    class PackageConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def platform
        @configuration.package_platform
      end

      def platform=(platform)
        @configuration.package_platform = platform
      end

      def options
        @options ||= @configuration.parsed_package_options
      end

      def options=(options)
        @configuration.package_options = encode_options(options)
        @options = nil
      end

      private
      def encode_options(options)
        return nil if options.nil?
        return options if options.is_a?(String)
        options.collect do |name, value|
          "#{name}=#{value}"
        end.join(",")
      end
    end

    class SecurityConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def privilege_mode?
        @configuration.privilege_mode?
      end

      def privilege_mode=(mode)
        mode = false if mode == "false"
        mode = true if mode == "true"
        available_values = [true, false]
        unless available_values.include?(mode)
          raise InvalidValue.new("security.privilege_mode",
                                 available_values,
                                 mode)
        end
        update_location("privilege_mode", false)
        @configuration.privilege_mode = mode
      end

      def effective_user
        @configuration.effective_user
      end

      def effective_user=(user_name)
        update_location("effective_user", user_name.nil?)
        @configuration.effective_user = user_name
      end

      def effective_group
        @configuration.effective_group
      end

      def effective_group=(group_name)
        update_location("effective_group", group_name.nil?)
        @configuration.effective_group = group_name
      end

      private
      def update_location(key, reset, deep_level=2)
        full_key = "security.#{key}"
        ConfigurationLoader.update_location(@configuration, full_key, reset,
                                            deep_level)
      end
    end

    class ControllerConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def connection_spec=(spec)
        Milter::Connection.parse_spec(spec) unless spec.nil?
        update_location("connection_spec", spec.nil?)
        @configuration.controller_connection_spec = spec
      end

      def unix_socket_mode
        @configuration.controller_unix_socket_mode
      end

      def unix_socket_mode=(mode)
        update_location("unix_socket_mode", false)
        @configuration.controller_unix_socket_mode = mode
      end

      def unix_socket_group
        @configuration.controller_unix_socket_group
      end

      def unix_socket_group=(group)
        update_location("unix_socket_group", group.nil?)
        @configuration.controller_unix_socket_group = group
      end

      def remove_unix_socket_on_create?
        @configuration.remove_controller_unix_socket_on_create?
      end

      def remove_unix_socket_on_create=(remove)
        update_location("remove_unix_socket_on_create", false)
        @configuration.remove_controller_unix_socket_on_create = remove
      end

      def remove_unix_socket_on_close?
        @configuration.remove_controller_unix_socket_on_close?
      end

      def remove_unix_socket_on_close=(remove)
        update_location("remove_unix_socket_on_close", false)
        @configuration.remove_controller_unix_socket_on_close = remove
      end

      private
      def update_location(key, reset, deep_level=2)
        full_key = "controller.#{key}"
        ConfigurationLoader.update_location(@configuration, full_key, reset,
                                            deep_level)
      end
    end

    class ManagerConfiguration
      def initialize(configuration)
        @configuration = configuration
        @connection_checker_ids = {}
      end

      def connection_spec=(spec)
        Milter::Connection.parse_spec(spec) unless spec.nil?
        update_location("connection_spec", spec.nil?)
        @configuration.manager_connection_spec = spec
      end

      def unix_socket_mode
        @configuration.manager_unix_socket_mode
      end

      def unix_socket_mode=(mode)
        update_location("unix_socket_mode", false)
        @configuration.manager_unix_socket_mode = mode
      end

      def unix_socket_group
        @configuration.manager_unix_socket_group
      end

      def unix_socket_group=(group)
        update_location("unix_socket_group", group.nil?)
        @configuration.manager_unix_socket_group = group
      end

      def remove_unix_socket_on_create?
        @configuration.remove_manager_unix_socket_on_create?
      end

      def remove_unix_socket_on_create=(remove)
        update_location("remove_unix_socket_on_create", false)
        @configuration.remove_manager_unix_socket_on_create = remove
      end

      def remove_unix_socket_on_close?
        @configuration.remove_manager_unix_socket_on_close?
      end

      def remove_unix_socket_on_close=(remove)
        update_location("remove_unix_socket_on_close", false)
        @configuration.remove_manager_unix_socket_on_close = remove
      end

      def daemon=(boolean)
        update_location("daemon", false)
        @configuration.daemon = boolean
      end

      def daemon?
        @configuration.daemon?
      end

      def pid_file=(pid_file)
        update_location("pid_file", pid_file.nil?)
        @configuration.pid_file = pid_file
      end

      def pid_file
        @configuration.pid_file
      end

      def maintenance_interval=(n_sessions)
        update_location("maintenance_interval", n_sessions.nil?)
        n_sessions ||= 0
        @configuration.maintenance_interval = n_sessions
      end

      def maintenance_interval
        @configuration.maintenance_interval
      end

      def suspend_time_on_unacceptable=(seconds)
        update_location("suspend_time_on_unacceptable", seconds.nil?)
        seconds ||= Milter::Client::DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE
        @configuration.suspend_time_on_unacceptable = seconds
      end

      def suspend_time_on_unacceptable
        @configuration.suspend_time_on_unacceptable
      end

      def max_connections=(n_connections)
        update_location("max_connections", n_connections.nil?)
        n_connections ||= Milter::Client::DEFAULT_MAX_CONNECTIONS
        @configuration.max_connections = n_connections
      end

      def max_connections
        @configuration.max_connections
      end

      def max_file_descriptors=(n_descriptors)
        update_location("max_file_descriptors", n_descriptors.nil?)
        n_descriptors ||= 0
        @configuration.max_file_descriptors = n_descriptors
      end

      def max_descriptors
        @configuration.max_descriptors
      end

      def custom_configuration_directory=(directory)
        update_location("custom_configuration_directory", directory.nil?)
        @configuration.custom_configuration_directory = directory
      end

      def custom_configuration_directory
        @configuration.custom_configuration_directory
      end

      def connection_check_interval=(interval)
        update_location("connection_check_interval", interval.nil?)
        interval ||= 0
        @configuration.connection_check_interval = interval
      end

      def event_loop_backend
        @configuration.event_loop_backend
      end

      def event_loop_backend=(backend)
        update_location("event_loop_backend", backend.nil?)
        @configuration.event_loop_backend = backend
      end

      def n_workers
        @configuration.n_workers
      end

      def n_workers=(n_workers)
        update_location("n_workers", n_workers.nil?)
        n_workers ||= 0
        @configuration.n_workers = n_workers
      end

      def packet_buffer_size
        @configuration.default_packet_buffer_size
      end

      def packet_buffer_size=(size)
        update_location("size", size.nil?)
        size ||= 0
        @configuration.default_packet_buffer_size = size
      end

      def fallback_status
        @configuration.fallback_status
      end

      def fallback_status=(status)
        update_location("fallback_status", status.nil?)
        @configuration.fallback_status = status
      end

      def fallback_status_at_disconnect
        @configuration.fallback_status_at_disconnect
      end

      def fallback_status_at_disconnect=(status)
        update_location("fallback_status_at_disconnect", status.nil?)
        @configuration.fallback_status_at_disconnect = status
      end

      def netstat_connection_checker
        @configuration.netstat_connection_checker
      end

      def define_connection_checker(name, &block)
        old_id = @connection_checker_ids[name]
        @configuration.signal_handler_disconnect(old_id) if old_id
        id = @configuration.signal_connect('connected') do |_config, leader|
          leader.signal_connect('connection-check') do |_leader|
            ConfigurationLoader.guard(true) do
              block.call(ConnectionCheckContext.new(leader))
            end
          end
        end
        @connection_checker_ids[name] = id
      end

      def use_netstat_connection_checker(interval=5)
        self.connection_check_interval = interval
        checker = netstat_connection_checker
        checker.database_lifetime = interval
        define_connection_checker("netstat") do |context|
          checker.connected?(context)
        end
      end

      def maintained(hook=Proc.new)
        guarded_hook = Proc.new do |configuration|
          ConfigurationLoader.guard do
            hook.call
          end
        end
        @configuration.maintained_hooks << guarded_hook
      end

      def report_memory_statistics
        memory_reporter = MemoryReporter.new
        maintained do
          memory_reporter.report
        end
      end

      def report_memory_profile
        maintained do
          Milter::MemoryProfile.report
        end
      end

      private
      def update_location(key, reset, deep_level=2)
        full_key = "manager.#{key}"
        ConfigurationLoader.update_location(@configuration, full_key, reset,
                                            deep_level)
      end

      class MemoryReporter
        def report
          n_objects = count_alive_objects
          message = "[maintain][memory] (#{memory_usage_in_kb}KB) "
          message << "total:#{n_objects[:total]} "
          message << "Proc:#{n_objects[:proc]} "
          message << "GLib::Object:#{n_objects[:glib_object]}"
          data = Milter::MemoryProfile.data
          if data
            n_allocates, n_zero_initializes, n_frees = data
            n_used_in_kb = (n_allocates - n_frees) / 1024.0
            used_percent = n_used_in_kb / memory_usage_in_kb * 100
            message << ("  (%dKB)(%.2f%%)" % [n_used_in_kb.to_i, used_percent])
            message << " allocated:#{n_allocates}"
            free_percent = n_frees.to_f / n_allocates * 100
            message << (" freed:%d(%.2f%%)" % [n_frees, free_percent])
          end
          Milter::Logger.statistics(message)
        end

        private
        def count_alive_objects
          n_procs = 0
          n_glib_objects = 0
          n_objects = ObjectSpace.each_object do |object|
            case object
            when Proc
              n_procs += 1
            when GLib::Object
              n_glib_objects += 1
            end
          end
          {
            :total => n_objects,
            :proc => n_procs,
            :glib_object => n_glib_objects,
          }
        end

        def memory_usage_in_kb
          `ps -o rss -p #{Process.pid}`.split(/\n/).last.to_i
        end
      end
    end

    class DatabaseConfiguration
      def initialize(configuration)
        @configuration = configuration
        @database = @configuration.database
      end

      def type
        @database.type
      end

      def type=(type)
        update_location("type", nil)
        @database.type = type
      end

      def name
        @database.name
      end

      def name=(name)
        update_location("name", nil)
        @database.name = name
      end

      def host
        @database.host
      end

      def host=(host)
        update_location("host", nil)
        @database.host = host
      end

      def port
        @database.port
      end

      def port=(port)
        update_location("port", nil)
        @database.port = port
      end

      def path
        @database.path
      end

      def path=(path)
        update_location("path", nil)
        @database.path = path
      end

      def user
        @database.user
      end

      def user=(user)
        update_location("user", nil)
        @database.user = user
      end

      def password
        @database.password
      end

      def password=(password)
        update_location("password", nil)
        @database.password = password
      end

      def setup
        case @database.type
        when nil
          raise MissingValue.new("database.type")
        when "mysql"
          options = mysql_options
        else
          options = default_options
        end
        Milter::Logger.info("[configuration][database][setup] " +
                            "<#{options.inspect}>")
        require 'active_record'
        ActiveRecord::Base.logger = Milter::Logger
        ActiveRecord::Base.establish_connection(options)
      end

      private
      def update_location(key, reset, deep_level=2)
        full_key = "database.#{key}"
        ConfigurationLoader.update_location(@configuration, full_key, reset,
                                            deep_level)
      end

      def mysql_options
        options = {}
        options[:adapter] = @database.type
        raise MissingValue.new("database.name") if @database.name.nil?
        options[:database] = @database.name
        options[:host] = @database.host || "localhost"
        options[:port] = @database.port || 3306
        default_path = "/var/run/mysqld/mysqld.sock"
        default_path = nil unless File.exist?(default_path)
        options[:path] = @database.path || default_path
        options[:user] = @database.user || "root"
        options[:password] = @database.password
        options
      end

      def default_options
        options = @database.to_hash
        options[:adatper] = options.delete(:type)
        options[:database] = options.delete(:name)
        options[:username] = options.delete(:user)
        options
      end
    end

    class EggConfiguration
      def initialize(name, loader)
        @egg = Egg.new(name)
        @loader = loader

        exist_egg = @loader.configuration.find_egg(name)
        @egg.merge(exist_egg) if exist_egg
      end

      def add_applicable_condition(name)
        configuration = @loader.configuration
        condition = configuration.find_applicable_condition(name)
        if condition.nil?
          conditions = configuration.applicable_conditions
          raise InvalidValue.new("applicable condition",
                                 conditions.collect {|cond| cond.name},
                                 name)
        end
        update_location("applicable_condition[#{name}]", false)
        @egg.add_applicable_condition(condition)
      end

      def applicable_conditions=(condition_names)
        configuration = @loader.configuration
        conditions = (condition_names || []).collect do |name|
          if name.is_a?(Milter::Manager::ApplicableCondition)
            condition = name
          else
            condition = configuration.find_applicable_condition(name)
          end
          if condition.nil?
            available_conditions = configuration.applicable_conditions
            available_condition_names =
              available_conditions.collect {|cond| cond.name}
            raise InvalidValue.new("applicable condition",
                                   available_condition_names,
                                   condition_names)
          end
          condition
        end
        @egg.applicable_conditions.each do |condition|
          update_location("applicable_condition[#{condition.name}]", true)
        end
        @egg.clear_applicable_conditions
        conditions.each do |condition|
          @egg.add_applicable_condition(condition)
        end
      end

      def command_options=(options)
        if options.is_a?(Array)
          options = options.collect do |option|
            Shellwords.escape(option)
          end.join(' ')
        end
        update_location("command_options", options.empty?)
        @egg.command_options = options
      end

      def fallback_status
        status = @egg.fallback_status
        status.nick
      end

      def fallback_status=(status)
        available_values = {
          "accept" => Milter::Status::ACCEPT,
          "reject" => Milter::Status::REJECT,
          "temporary-failure" => Milter::Status::TEMPORARY_FAILURE,
          "discard" => Milter::Status::DISCARD,
        }
        if status.respond_to?(:nick)
          normalized_status = status.nick
        else
          normalized_status = status.to_s.downcase.gsub(/_/, '-')
        end
        value = available_values[normalized_status]
        if value.nil?
          raise InvalidValue.new("milter.fallback_status",
                                 available_values.keys,
                                 status)
        end
        update_location("fallback_status", false)
        @egg.fallback_status = value
      end

      def method_missing(name, *args, &block)
        result = @egg.send(name, *args, &block)
        if /=\z/ =~ name.to_s
          update_location($PREMATCH, args[0].nil?)
        end
        result
      end

      def apply
        @loader.configuration.remove_egg(@egg.name)
        @loader.configuration.add_egg(@egg)
      end

      private
      def update_location(name, reset, deep_level=2)
        full_key = "milter[#{@egg.name}].#{name}"
        ConfigurationLoader.update_location(@loader.configuration,
                                            full_key, reset, deep_level)
      end
    end

    class ApplicableConditionConfiguration
      def initialize(name, loader)
        @condition = Milter::Manager::ApplicableCondition.new(name)
        @loader = loader
        @connect_stoppers = []
        @helo_stoppers = []
        @envelope_from_stoppers = []
        @envelope_recipient_stoppers = []
        @data_stoppers = []
        @header_stoppers = []
        @end_of_header_stoppers = []
        @body_stoppers = []
        @end_of_message_stoppers = []

        exist_condition = @loader.configuration.find_applicable_condition(name)
        @condition.merge(exist_condition) if exist_condition
      end

      def define_connect_stopper(&block)
        @connect_stoppers << block
      end

      def define_helo_stopper(&block)
        @helo_stoppers << block
      end

      def define_envelope_from_stopper(&block)
        @envelope_from_stoppers << block
      end

      def define_envelope_recipient_stopper(&block)
        @envelope_recipient_stoppers << block
      end

      def define_data_stopper(&block)
        @data_stoppers << block
      end

      def define_header_stopper(&block)
        @header_stoppers << block
      end

      def define_end_of_header_stopper(&block)
        @end_of_header_stoppers << block
      end

      def define_body_stopper(&block)
        @body_stoppers << block
      end

      def define_end_of_message_stopper(&block)
        @end_of_message_stoppers << block
      end

      def have_stopper?
        [@connect_stoppers,
         @helo_stoppers,
         @envelope_from_stoppers,
         @envelope_recipient_stoppers,
         @data_stoppers,
         @header_stoppers,
         @end_of_header_stoppers,
         @body_stoppers,
         @end_of_message_stoppers,
        ].any? do |stoppers|
          not stoppers.empty?
        end
      end

      def method_missing(name, *args, &block)
        result = @condition.send(name, *args, &block)
        if /=\z/ =~ name.to_s
          update_location($PREMATCH, args[0].nil?, 2)
        end
        result
      end

      def apply
        @loader.configuration.remove_applicable_condition(@condition.name)
        setup_stoppers
        @loader.configuration.add_applicable_condition(@condition)
      end

      private
      def setup_stoppers
        return unless have_stopper?

        @condition.signal_connect("attach-to") do |_, child, children, context|
          unless @connect_stoppers.empty?
            child.signal_connect("stop-on-connect") do |_child, host, address|
              child_context = ChildContext.new(child, children, context)
              @connect_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context, host, address)
                end
              end
            end
          end

          unless @helo_stoppers.empty?
            child.signal_connect("stop-on-helo") do |_child, fqdn|
              child_context = ChildContext.new(child, children, context)
              @helo_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context, fqdn)
                end
              end
            end
          end

          unless @envelope_from_stoppers.empty?
            child.signal_connect("stop-on-envelope-from") do |_child, from|
              child_context = ChildContext.new(child, children, context)
              @envelope_from_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context, from)
                end
              end
            end
          end

          unless @envelope_recipient_stoppers.empty?
            child.signal_connect("stop-on-envelope-recipient") do |_child, recipient|
              child_context = ChildContext.new(child, children, context)
              @envelope_recipient_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context, recipient)
                end
              end
            end
          end

          unless @data_stoppers.empty?
            child.signal_connect("stop-on-data") do |_child|
              child_context = ChildContext.new(child, children, context)
              @data_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context)
                end
              end
            end
          end

          unless @header_stoppers.empty?
            child.signal_connect("stop-on-header") do |_child, name, value|
              child_context = ChildContext.new(child, children, context)
              @header_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context, name, value)
                end
              end
            end
          end

          unless @end_of_header_stoppers.empty?
            child.signal_connect("stop-on-end-of-header") do |_child|
              child_context = ChildContext.new(child, children, context)
              @end_of_header_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context)
                end
              end
            end
          end

          unless @body_stoppers.empty?
            child.signal_connect("stop-on-body") do |_child, chunk|
              child_context = ChildContext.new(child, children, context)
              @body_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context, name, chunk)
                end
              end
            end
          end

          unless @end_of_message_stoppers.empty?
            child.signal_connect("stop-on-end-of-message") do |_child, chunk|
              child_context = ChildContext.new(child, children, context)
              @end_of_message_stoppers.any? do |stopper|
                ConfigurationLoader.guard(false) do
                  stopper.call(child_context)
                end
              end
            end
          end
        end
      end

      def update_location(name, reset, deep_level=2)
        full_key = "applicable_condition[#{@condition.name}].#{name}"
        ConfigurationLoader.update_location(@loader.configuration,
                                            full_key, reset, deep_level)
      end
    end
  end
end
