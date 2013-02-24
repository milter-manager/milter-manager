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

require 'pathname'
require 'shellwords'
require 'erb'
require 'yaml'
require "English"

require "rexml/document"
require "rexml/streamlistener"

require 'milter'
require 'milter_manager.so'

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

module Milter
  module Manager
    class Configuration
      include Milter::PathResolver

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

      def event_loop_created(loop)
        @event_loop = loop
        event_loop_created_hooks.each do |hook|
          hook.call(self, loop)
        end
      end

      def clear
        @maintained_hooks = nil
        @event_loop_created_hooks = nil
        @netstat_connection_checker = nil
        @database ||= Client::Configuration::DatabaseConfiguration.new(self)
        @database.clear
      end

      def maintained_hooks
        @maintained_hooks ||= []
      end

      def event_loop_created_hooks
        @event_loop_created_hooks ||= []
      end

      def netstat_connection_checker
        @netstat_connection_checker ||= NetstatConnectionChecker.new
      end

      def expand_path(path)
        return path if Pathname(path).absolute?
        _prefix = (parsed_package_options || {})["prefix"] || prefix
        File.join(_prefix, path)
      end

      def update_location(key, reset, deep_level)
        if reset
          reset_location(key)
        else
          file, line, _ = caller[deep_level].split(/:/, 3)
          set_location(key, file, line.to_i)
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
        dump_log_items
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

      def dump_log_items
        c = @configuration
        level = Milter::Logger.default.target_level
        level_names = []
        Milter::LogLevelFlags.values.each do |value|
          unless (level.to_i & value.to_i).zero?
            level_names << value.nick
          end
        end
        level_names << "default" if level_names.empty?
        dump_item("log.level", level_names.join("|").inspect)
        dump_item("log.use_syslog", c.use_syslog?)
        dump_item("log.syslog_facility", c.syslog_facility.inspect)
        @result << "\n"
      end

      def dump_manager_items
        c = @configuration
        dump_item("manager.connection_spec", c.manager_connection_spec.inspect)
        dump_item("manager.unix_socket_mode", "%#o" % c.manager_unix_socket_mode)
        dump_item("manager.unix_socket_group",
                  c.manager_unix_socket_group.inspect)
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
        dump_item("manager.chunk_size", c.chunk_size)
        dump_item("manager.max_pending_finished_sessions",
                  c.max_pending_finished_sessions)
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
        @result << "define_applicable_condition(#{name.dump}) do |condition|\n"
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
      InvalidValue = Client::ConfigurationLoader::InvalidValue
      NonexistentPath = Client::ConfigurationLoader::NonexistentPath
      MissingValue = Client::ConfigurationLoader::MissingValue

      class << self
        @@current_configuration = nil
        def load(configuration, file)
          @@current_configuration = configuration
          Milter::Callback.guard do
            new(configuration).load_configuration(file)
          end
          GC.start
        end

        def load_custom(configuration, file)
          @@current_configuration = configuration
          Milter::Callback.guard do
            new(configuration).load_custom_configuration(file)
          end
          GC.start
        end
      end

      attr_reader :package, :security, :controller, :manager, :database, :log
      attr_reader :configuration
      def initialize(configuration)
        @load_level = 0
        @configuration = configuration
        @package = PackageConfigurationLoader.new(configuration)
        @security = SecurityConfigurationLoader.new(configuration)
        @controller = ControllerConfigurationLoader.new(configuration)
        @manager = ManagerConfigurationLoader.new(configuration)
        client_config = Client::Configuration
        client_config_loader = Client::ConfigurationLoader
        database_config = configuration.database
        @database =
          client_config_loader::DatabaseConfigurationLoader.new(database_config)
        log_config = client_config::LogConfiguration.new(configuration)
        log_config.use_syslog = configuration.use_syslog?
        @log = client_config_loader::LogConfigurationLoader.new(log_config)
        @policy_manager = Manager::PolicyManager.new(self)
      end

      def load_configuration(file)
        begin
          @load_level += 1
          content = File.read(file)
          Logger.debug("[configuration][load][start] <#{file}>")
          instance_eval(content, file)
          apply_configurations if @load_level == 1
        rescue InvalidValue
          backtrace = $!.backtrace.collect do |info|
            if /\A#{Regexp.escape(file)}:/ =~ info
              info
            else
              nil
            end
          end.compact
          Logger.error("#{backtrace[0]}: #{$!.message}")
          Logger.error(backtrace[1..-1].join("\n"))
        ensure
          @load_level -= 1
          Logger.debug("[configuration][load][end] <#{file}>")
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
          load_configuration(full_path)
        end
      end

      def load_if_exist(path)
        load(path)
      rescue NonexistentPath => e
        raise unless e.path == path
        # ignore. log this?
      end

      def load_default
        platform = @configuration.package_platform
        load_if_exist("defaults/#{platform}.conf") if platform
      end

      def define_milter(name, &block)
        @configuration.update_location("milter[#{name}]", false, 1)
        loader = EggConfigurationLoader.new(name, self)
        yield(loader)
        loader.apply
      end

      # more better name is needed?
      def remove_milter(name)
        @configuration.remove_egg(name)
      end

      def define_applicable_condition(name)
        @configuration.update_location("applicable_condition[#{name}]", false, 1)
        loader = ApplicableConditionConfigurationLoader.new(name, self)
        yield(loader)
        loader.apply
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
        resolved_paths = @configuration.resolve_path(path)
        raise NonexistentPath.new(path) if resolved_paths.empty?
        resolved_paths
      end

      def apply_configurations
        apply_log
        apply_policies
      end

      def apply_log
        @configuration.use_syslog = @log.use_syslog?
        @configuration.syslog_facility = @log.syslog_facility
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
            Logger.debug("[configuration][xml][load][start] <#{file}>")
            parser = REXML::Parsers::StreamParser.new(input, listener)
            begin
              parser.parse
            rescue InvalidValue
              source = parser.source
              info = "#{file}:#{source.line}:#{source.position}"
              info << ":#{source.current_line}: #{$!.message}"
              Logger.error(info)
            ensure
              Logger.debug("[configuration][xml][load][end] <#{file}>")
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
                milter.applicable_conditions =
                  @egg_config["applicable_conditions"]
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

      class PackageConfigurationLoader
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

      class SecurityConfigurationLoader
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
          @configuration.update_location(full_key, reset, deep_level)
        end
      end

      class ControllerConfigurationLoader
        def initialize(configuration)
          @configuration = configuration
        end

        def connection_spec=(spec)
          Connection.parse_spec(spec) unless spec.nil?
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
          @configuration.update_location(full_key, reset, deep_level)
        end
      end

      class ManagerConfigurationLoader < Client::ConfigurationLoader::MilterConfigurationLoader
        class ConfigurationWrapper
          def initialize(configuration)
            @configuration = configuration
          end

          def connection_spec
            @configuration.manager_connection_spec
          end

          def connection_spec=(spec)
            @configuration.manager_connection_spec = spec
          end

          def unix_socket_mode
            @configuration.manager_unix_socket_mode
          end

          def unix_socket_mode=(mode)
            @configuration.manager_unix_socket_mode = mode
          end

          def unix_socket_group
            @configuration.manager_unix_socket_group
          end

          def unix_socket_group=(group)
            @configuration.manager_unix_socket_group = group
          end

          def remove_unix_socket_on_create?
            @configuration.remove_manager_unix_socket_on_create?
          end

          def remove_unix_socket_on_create=(remove)
            @configuration.remove_manager_unix_socket_on_create = remove
          end

          def remove_unix_socket_on_close?
            @configuration.remove_manager_unix_socket_on_close?
          end

          def remove_unix_socket_on_close=(remove)
            @configuration.remove_manager_unix_socket_on_close = remove
          end

          def daemon?
            @configuration.daemon?
          end

          def daemon=(boolean)
            @configuration.daemon = boolean
          end

          def pid_file
            @configuration.pid_file
          end

          def pid_file=(pid_file)
            @configuration.pid_file = pid_file
          end

          def maintenance_interval
            @configuration.maintenance_interval
          end

          def maintenance_interval=(n_sessions)
            @configuration.maintenance_interval = n_sessions
          end

          def suspend_time_on_unacceptable
            @configuration.suspend_time_on_unacceptable
          end

          def suspend_time_on_unacceptable=(seconds)
            @configuration.suspend_time_on_unacceptable = seconds
          end

          def max_connections
            @configuration.max_connections
          end

          def max_connections=(n_connections)
            @configuration.max_connections = n_connections
          end

          def max_file_descriptors
            @configuration.max_file_descriptors
          end

          def max_file_descriptors=(n_descriptors)
            @configuration.max_file_descriptors = n_descriptors
          end

          def fallback_status
            @raw_configuration.fallback_status
          end

          def fallback_status=(status)
            @configuration.fallback_status = status
          end

          def event_loop_backend
            @configuration.event_loop_backend
          end

          def event_loop_backend=(backend)
            @configuration.event_loop_backend = backend
          end

          def n_workers
            @configuration.n_workers
          end

          def n_workers=(n_workers)
            @configuration.n_workers = n_workers
          end

          def packet_buffer_size
            @configuration.default_packet_buffer_size
          end

          def packet_buffer_size=(size)
            @configuration.default_packet_buffer_size = size
          end

          def max_pending_finished_sessions
            @configuration.max_pending_finished_sessions
          end

          def max_pending_finished_sessions=(n_sessions)
            @configuration.max_pending_finished_sessions = n_sessions
          end

          def maintained_hooks
            @configuration.maintained_hooks
          end

          def event_loop_created_hooks
            @configuration.event_loop_created_hooks
          end

          def update_location(key, reset, deep_level=2)
            @configuration.update_location(key, reset, deep_level)
          end
        end

        def initialize(configuration)
          @raw_configuration = configuration
          super(ConfigurationWrapper.new(@raw_configuration))
          @connection_checker_ids = {}
        end

        def custom_configuration_directory
          @raw_configuration.custom_configuration_directory
        end

        def custom_configuration_directory=(directory)
          update_location("custom_configuration_directory", directory.nil?)
          @raw_configuration.custom_configuration_directory = directory
        end

        def fallback_status_at_disconnect
          @raw_configuration.fallback_status_at_disconnect
        end

        def fallback_status_at_disconnect=(status)
          update_location("fallback_status_at_disconnect", status.nil?)
          @raw_configuration.fallback_status_at_disconnect = status
        end

        def chunk_size
          @raw_configuration.chunk_size
        end

        def chunk_size=(size)
          update_location("chunk_size", size.nil?)
          size ||= Milter::CHUNK_SIZE
          @raw_configuration.chunk_size = size
        end

        def connection_check_interval
          @raw_configuration.connection_check_interval
        end

        def connection_check_interval=(interval)
          update_location("connection_check_interval", interval.nil?)
          interval ||= 0
          @raw_configuration.connection_check_interval = interval
        end

        def netstat_connection_checker
          @raw_configuration.netstat_connection_checker
        end

        def define_connection_checker(name, &block)
          old_id = @connection_checker_ids[name]
          @raw_configuration.signal_handler_disconnect(old_id) if old_id
          id = @raw_configuration.signal_connect('connected') do |config, leader|
            leader.signal_connect('connection-check') do |_leader|
              Milter::Callback.guard(true) do
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

        def report_memory_statistics
          memory_reporter = MemoryReporter.new
          maintained do
            memory_reporter.report
          end
        end

        def report_memory_profile
          maintained do
            MemoryProfile.report
          end
        end

        private
        def update_location(key, reset, deep_level=2)
          super(key, reset, deep_level + 2)
        end

        def full_key(key)
          "manager.#{key}"
        end

        class MemoryReporter
          def report
            n_objects = count_alive_objects
            message = "[maintain][memory] (#{memory_usage_in_kb}KB) "
            message << "total:#{n_objects[:total]} "
            message << "Proc:#{n_objects[:proc]} "
            message << "GLib::Object:#{n_objects[:glib_object]}"
            data = MemoryProfile.data
            if data
              n_allocates, n_zero_initializes, n_frees = data
              n_used_in_kb = (n_allocates - n_frees) / 1024.0
              used_percent = n_used_in_kb / memory_usage_in_kb * 100
              message << ("  (%dKB)(%.2f%%)" % [n_used_in_kb.to_i, used_percent])
              message << " allocated:#{n_allocates}"
              free_percent = n_frees.to_f / n_allocates * 100
              message << (" freed:%d(%.2f%%)" % [n_frees, free_percent])
            end
            Logger.statistics(message)
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

      class EggConfigurationLoader
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
            if name.is_a?(ApplicableCondition)
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
            "accept" => Status::ACCEPT,
            "reject" => Status::REJECT,
            "temporary-failure" => Status::TEMPORARY_FAILURE,
            "discard" => Status::DISCARD,
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
          @loader.configuration.update_location(full_key, reset, deep_level)
        end
      end

      class ApplicableConditionConfigurationLoader
        def initialize(name, loader)
          @condition = ApplicableCondition.new(name)
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
                  Milter::Callback.guard(false) do
                    stopper.call(child_context, host, address)
                  end
                end
              end
            end

            unless @helo_stoppers.empty?
              child.signal_connect("stop-on-helo") do |_child, fqdn|
                child_context = ChildContext.new(child, children, context)
                @helo_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context, fqdn)
                  end
                end
              end
            end

            unless @envelope_from_stoppers.empty?
              child.signal_connect("stop-on-envelope-from") do |_child, from|
                child_context = ChildContext.new(child, children, context)
                @envelope_from_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context, from)
                  end
                end
              end
            end

            unless @envelope_recipient_stoppers.empty?
              child.signal_connect("stop-on-envelope-recipient") do |_child, recipient|
                child_context = ChildContext.new(child, children, context)
                @envelope_recipient_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context, recipient)
                  end
                end
              end
            end

            unless @data_stoppers.empty?
              child.signal_connect("stop-on-data") do |_child|
                child_context = ChildContext.new(child, children, context)
                @data_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context)
                  end
                end
              end
            end

            unless @header_stoppers.empty?
              child.signal_connect("stop-on-header") do |_child, name, value|
                child_context = ChildContext.new(child, children, context)
                @header_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context, name, value)
                  end
                end
              end
            end

            unless @end_of_header_stoppers.empty?
              child.signal_connect("stop-on-end-of-header") do |_child|
                child_context = ChildContext.new(child, children, context)
                @end_of_header_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context)
                  end
                end
              end
            end

            unless @body_stoppers.empty?
              child.signal_connect("stop-on-body") do |_child, chunk|
                child_context = ChildContext.new(child, children, context)
                @body_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context, name, chunk)
                  end
                end
              end
            end

            unless @end_of_message_stoppers.empty?
              child.signal_connect("stop-on-end-of-message") do |_child, chunk|
                child_context = ChildContext.new(child, children, context)
                @end_of_message_stoppers.any? do |stopper|
                  Milter::Callback.guard(false) do
                    stopper.call(child_context)
                  end
                end
              end
            end
          end
        end

        def update_location(name, reset, deep_level=2)
          full_key = "applicable_condition[#{@condition.name}].#{name}"
          @loader.configuration.update_location(full_key, reset, deep_level)
        end
      end
    end
  end
end
