# Copyright (C) 2011-2013  Kouhei Sutou <kou@clear-code.com>
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

module Milter
  class Client
    class Configuration
      include Milter::PathResolver

      attr_accessor :environment
      attr_reader :milter, :security, :log, :database, :load_paths, :prefix
      def initialize
        clear
      end

      def clear
        @environment = ENV["MILTER_ENV"] || "development"
        @milter = MilterConfiguration.new(self)
        @security = SecurityConfiguration.new(self)
        @log = LogConfiguration.new(self)
        @database = DatabaseConfiguration.new(self)
        @load_paths = []
        @locations = {}
        @prefix = nil
      end

      def setup(client)
        @log.setup(client, @milter)
        @database.setup
        @security.setup(client)
        @milter.setup(client)
      end

      def expand_path(path)
        return path if Pathname(path).absolute?
        if @prefix
          File.join(@prefix, path)
        else
          path
        end
      end

      def update_location(key, reset, deep_level)
        if reset
          @locations.delete(key)
        else
          file, line, _ = caller[deep_level].split(/:/, 3)
          @locations[key] = [file, line.to_i]
        end
      end

      class MilterConfiguration
        attr_accessor :name, :connection_spec
        attr_accessor :unix_socket_mode, :unix_socket_group
        attr_accessor :remove_unix_socket_on_create
        attr_accessor :remove_unix_socket_on_close
        attr_accessor :pid_file, :maintenance_interval
        attr_accessor :suspend_time_on_unacceptable, :max_connections
        attr_accessor :max_file_descriptors, :event_loop_backend
        attr_accessor :n_workers, :packet_buffer_size
        attr_accessor :max_pending_finished_sessions
        attr_accessor :fallback_status
        attr_writer :daemon, :handle_signal, :run_gc_on_maintain
        attr_reader :maintained_hooks, :event_loop_created_hooks
        def initialize(base_configuration)
          @base_configuration = base_configuration
          clear
        end

        def daemon?
          @daemon
        end

        def handle_signal?
          @handle_signal
        end

        def run_gc_on_maintain?
          @run_gc_on_maintain
        end

        def clear
          @name = File.basename($PROGRAM_NAME, ".*"),
          @connection_spec = "inet:20025"
          @unix_socket_mode = 0770
          @unix_socket_group = nil
          @remove_unix_socket_on_create = true
          @remove_unix_socket_on_close = true
          @daemon = false
          @pid_file = nil
          @maintenance_interval = 100
          @suspend_time_on_unacceptable =
            Milter::Client::DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE
          @max_connections = Milter::Client::DEFAULT_MAX_CONNECTIONS
          @max_file_descriptors = 0
          @fallback_status = "accept"
          @event_loop_backend = Milter::Client::EVENT_LOOP_BACKEND_GLIB.nick
          @n_workers = 0
          @packet_buffer_size = 0
          @max_pending_finished_sessions = 0
          @run_gc_on_maintain = true
          @handle_signal = true
          @maintained_hooks = []
          @event_loop_created_hooks = []
        end

        def setup(client)
          client.connection_spec = @connection_spec
          client.unix_socket_group = @unix_socket_group
          client.unix_socket_mode = @unix_socket_mode if @unix_socket_mode
          client.fallback_status = @fallback_status
          client.event_loop_backend = @event_loop_backend
          client.default_packet_buffer_size = @packet_buffer_size
          client.pid_file = @pid_file
          client.maintenance_interval = @maintenance_interval
          if @run_gc_on_maintain
            client.on_maintain do
              GC.start
            end
          end
          client.n_workers = @n_workers
          client.max_pending_finished_sessions = @max_pending_finished_sessions
          unless @maintained_hooks.empty?
            client.on_maintain do
              maintained
            end
          end
          unless @event_loop_created_hooks.empty?
            client.on_event_loop_created do |_client, loop|
              event_loop_created(loop)
            end
          end
          if @max_file_descriptors.nonzero?
            begin
              Process.setrlimit(Process::RLIMIT_NOFILE, @max_file_descriptors)
            rescue => error
              Milter::Logger.error("[configuration][setrlimit][nofile] #{error.message}")
              exit(false)
            end
          end
        end

        def maintained
          @maintained_hooks.each do |hook|
            hook.call(self)
          end
        end

        def event_loop_created(loop)
          @event_loop = loop
          @event_loop_created_hooks.each do |hook|
            hook.call(self, loop)
          end
        end

        def update_location(key, reset, deep_level)
          @base_configuration.update_location(key, reset, deep_level + 1)
        end
      end

      class SecurityConfiguration
        attr_accessor :effective_user, :effective_group
        def initialize(base_configuration)
          @base_configuration = base_configuration
          clear
        end

        def clear
          @effective_user = nil
          @effective_group = nil
        end

        def setup(client)
          client.effective_user = @effective_user if @effective_user
          client.effective_group = @effective_group if @effective_group
        end

        def update_location(key, reset, deep_level)
          @base_configuration.update_location(key, reset, deep_level + 1)
        end
      end

      class LogConfiguration
        attr_accessor :syslog_facility
        attr_writer :use_syslog
        def initialize(base_configuration)
          @base_configuration = base_configuration
          clear
        end

        def use_syslog?
          @use_syslog
        end

        def level
          Milter::Logger.default.target_level
        end

        def level=(level)
          level = nil if default_log_level?(level)
          Milter::Logger.default.target_level = level
        end

        def default_log_level?(level)
          return true if level.respond_to?(:empty?) and level.empty?
          return true if level == "default"
          return true if level == :default
          false
        end

        def clear
          @use_syslog = false
          @syslog_facility = "mail"
        end

        def setup(client, milter)
          client.start_syslog(milter.name, @syslog_facility) if use_syslog?
        end

        def update_location(key, reset, deep_level)
          @base_configuration.update_location(key, reset, deep_level + 1)
        end
      end

      class DatabaseConfiguration
        attr_accessor :type, :name, :host, :port, :path
        attr_accessor :user, :password, :extra_options
        def initialize(base_configuration)
          @base_configuration = base_configuration
          @loaded_model_files = []
          clear
        end

        def clear
          @setup_done = false
          @type = nil
          @name = nil
          @host = nil
          @port = nil
          @path = nil
          @user = nil
          @password = nil
          @extra_options = {}
          unless @loaded_model_files.empty?
            $LOADED_FEATURES.reject! do |required_path|
              @loaded_model_files.include?(required_path)
            end
          end
          @loaded_model_files.clear
        end

        def missing_values
          return [:type] if @type.nil?
          options = active_record_options
          options[:missing_values] || []
        end

        def setup
          return if @setup_done
          return if @type.nil?
          _missing_values = missing_values
          options = active_record_options
          unless _missing_values.empty?
            Milter::Logger.warning("[configuration][database][setup][ignore]" +
                                   "[missing] " +
                                   "<#{_missing_values.inspect}>:" +
                                   "<#{options.inspect}>")
            return
          end
          Milter::Logger.info("[configuration][database][setup] " +
                              "<#{options.inspect}>")
          require 'rubygems' unless defined?(Gem)
          require 'active_record'
          logger = Milter::ActiveRecordLogger.new(Milter::Logger.default)
          ActiveRecord::Base.logger = logger
          ActiveRecord::Base.establish_connection(options)
          @setup_done = true
        end

        def to_hash
          {
            :type => type,
            :name => name,
            :host => host,
            :port => port,
            :path => path,
            :user => user,
            :password => password,
          }.merge(extra_options)
        end

        def update_location(key, reset, deep_level)
          @base_configuration.update_location(key, reset, deep_level + 1)
        end

        def resolve_path(path)
          @base_configuration.resolve_path(path)
        end

        def add_loaded_model_file(model_file)
          @loaded_model_files << model_file
        end

        private
        def active_record_options
          case @type
          when "mysql", "mysql2"
            options = mysql_options
          when "sqlite3"
            options = sqlite3_options
          else
            options = default_options
          end
          options.merge(extra_options)
        end

        def mysql_options
          options = {}
          options[:adapter] = @type
          if @name.nil?
            options[:missing_values] = [:name]
            return options
          end
          options[:database] = @name
          options[:host] = @host || "localhost"
          options[:port] = @port || 3306
          default_path = "/var/run/mysqld/mysqld.sock"
          default_path = nil unless File.exist?(default_path)
          options[:path] = @path || default_path
          options[:username] = @user || "root"
          options[:password] = @password
          options
        end

        def sqlite3_options
          options = {}
          options[:adapter] = @type
          options[:database] = @name || @path
          unless options[:database] == ":memory:"
            options[:database] = expand_path(options[:database])
          end
          options
        end

        def default_options
          {
            :adapter => type,
            :database => name,
            :host => host,
            :port => port,
            :path => path,
            :username => user,
            :password => password,
          }
        end

        def expand_path(path)
          @base_configuration.expand_path(path)
        end
      end
    end

    class ConfigurationLoader
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
        attr_reader :path

        def initialize(path)
          @path = path
          super("#{@path} doesn't exist.")
        end
      end

      class MissingValue < Error
        attr_reader :target

        def initialize(target)
          @target = target
          super("#{@target} should be set")
        end
      end

      attr_reader :milter, :security, :log, :database
      def initialize(configuration)
        @configuration = configuration
        @milter = MilterConfigurationLoader.new(@configuration.milter)
        @security = SecurityConfigurationLoader.new(@configuration.security)
        @log = LogConfigurationLoader.new(@configuration.log)
        @database = DatabaseConfigurationLoader.new(@configuration.database)
        @depth = 0
      end

      def load(path)
        resolved_paths = @configuration.resolve_path(path)
        raise NonexistentPath.new(path) if resolved_paths.empty?
        resolved_paths.each do |resolved_path|
          load_path(resolved_path)
        end
      end

      def load_if_exist(path)
        load(path)
      rescue NonexistentPath => e
        raise unless e.path == path
        Milter::Logger.debug("[configuration][load][nonexistent][ignore] " +
                             "<#{path}>")
      end

      def environment
        @configuration.environment
      end

      private
      def load_path(path)
        begin
          content = File.read(path)
          Milter::Logger.debug("[configuration][load][start] <#{path}>")
          instance_eval(content, path)
        rescue InvalidValue, MissingValue
          backtrace = $!.backtrace.collect do |info|
            if /\A#{Regexp.escape(path)}:/ =~ info
              info
            else
              nil
            end
          end.compact
          Milter::Logger.error("#{backtrace[0]}: #{$!.message}")
          Milter::Logger.error(backtrace[1..-1].join("\n"))
        ensure
          Milter::Logger.debug("[configuration][load][end] <#{path}>")
        end
      end

      class MilterConfigurationLoader
        attr_reader :available_fallback_statuses
        def initialize(configuration)
          @configuration = configuration
          @available_fallback_statuses = ["accept", "reject",
                                          "temporary-failure", "discard"]
        end

        def name
          @configuration.name
        end

        def connection_spec
          @configuration.connection_spec
        end

        def connection_spec=(spec)
          Milter::Connection.parse_spec(spec) unless spec.nil?
          update_location("connection_spec", spec.nil?)
          @configuration.connection_spec = spec
        end

        def unix_socket_mode
          @configuration.unix_socket_mode
        end

        def unix_socket_mode=(mode)
          update_location("unix_socket_mode", false)
          @configuration.unix_socket_mode = mode
        end

        def unix_socket_group
          @configuration.unix_socket_group
        end

        def unix_socket_group=(group)
          update_location("unix_socket_group", group.nil?)
          @configuration.unix_socket_group = group
        end

        def remove_unix_socket_on_create?
          @configuration.remove_unix_socket_on_create?
        end

        def remove_unix_socket_on_create=(remove)
          update_location("remove_unix_socket_on_create", false)
          @configuration.remove_unix_socket_on_create = remove
        end

        def remove_unix_socket_on_close?
          @configuration.remove_unix_socket_on_close?
        end

        def remove_unix_socket_on_close=(remove)
          update_location("remove_unix_socket_on_close", false)
          @configuration.remove_unix_socket_on_close = remove
        end

        def daemon?
          @configuration.daemon?
        end

        def daemon=(boolean)
          update_location("daemon", false)
          @configuration.daemon = boolean
        end

        def pid_file
          @configuration.pid_file
        end

        def pid_file=(pid_file)
          update_location("pid_file", pid_file.nil?)
          @configuration.pid_file = pid_file
        end

        def maintenance_interval
          @configuration.maintenance_interval
        end

        def maintenance_interval=(n_sessions)
          update_location("maintenance_interval", n_sessions.nil?)
          n_sessions ||= 0
          @configuration.maintenance_interval = n_sessions
        end

        def suspend_time_on_unacceptable
          @configuration.suspend_time_on_unacceptable
        end

        def suspend_time_on_unacceptable=(seconds)
          update_location("suspend_time_on_unacceptable", seconds.nil?)
          seconds ||= Milter::Client::DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE
          @configuration.suspend_time_on_unacceptable = seconds
        end

        def max_connections
          @configuration.max_connections
        end

        def max_connections=(n_connections)
          update_location("max_connections", n_connections.nil?)
          n_connections ||= Milter::Client::DEFAULT_MAX_CONNECTIONS
          @configuration.max_connections = n_connections
        end

        def max_file_descriptors
          @configuration.max_file_descriptors
        end

        def max_file_descriptors=(n_descriptors)
          update_location("max_file_descriptors", n_descriptors.nil?)
          n_descriptors ||= 0
          @configuration.max_file_descriptors = n_descriptors
        end

        def fallback_status
          @configuration.fallback_status
        end

        def fallback_status=(status)
          available_values = @available_fallback_statuses
          normalized_status = status
          unless status.nil?
            normalized_status = status.to_s.downcase.gsub(/_/, '-')
            unless available_values.include?(normalized_status)
              raise InvalidValue.new(full_key("fallback_status"),
                                     available_values, status)
            end
          end
          update_location("fallback_status", status.nil?)
          @configuration.fallback_status = normalized_status
        end

        def event_loop_backend
          @configuration.event_loop_backend
        end

        def event_loop_backend=(backend)
          update_location("event_loop_backend", backend.nil?)
          @configuration.event_loop_backend = resolve_event_loop_backend(backend)
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
          @configuration.packet_buffer_size
        end

        def packet_buffer_size=(size)
          update_location("size", size.nil?)
          size ||= 0
          @configuration.packet_buffer_size = size
        end

        def max_pending_finished_sessions
          @configuration.max_pending_finished_sessions
        end

        def max_pending_finished_sessions=(n_sessions)
          update_location("max_pending_finished_sessions", n_sessions.nil?)
          n_sessions ||= 0
          @configuration.max_pending_finished_sessions = n_sessions
        end

        def maintained(hook=Proc.new)
          guarded_hook = Proc.new do |configuration|
            Milter::Callback.guard do
              hook.call
            end
          end
          @configuration.maintained_hooks << guarded_hook
        end

        def event_loop_created(hook=Proc.new)
          guarded_hook = Proc.new do |configuration, loop|
            Milter::Callback.guard do
              hook.call(loop)
            end
          end
          @configuration.event_loop_created_hooks << guarded_hook
        end

        private
        def update_location(key, reset, deep_level=2)
          @configuration.update_location(full_key(key), reset, deep_level)
        end

        def full_key(key)
          "milter.#{key}"
        end

        # For Ruby 1.8.5 + old Ruby/GLib2. It should be
        # removed when CentOS 5 support is dropped.
        def resolve_event_loop_backend(backend)
          case backend
          when nil
            nil
          when String
            Milter::ClientEventLoopBackend.values.find do |value|
              value.nick == backend
            end
          else
            backend
          end
        end
      end

      class SecurityConfigurationLoader
        def initialize(configuration)
          @configuration = configuration
        end

        def effective_user
          @configuration.effective_user
        end

        def effective_user=(user)
          update_location("effective_user", user.nil?)
          @configuration.effective_user = user
        end

        def effective_group
          @configuration.effective_group
        end

        def effective_group=(group)
          update_location("effective_group", group.nil?)
          @configuration.effective_group = group
        end

        private
        def update_location(key, reset, deep_level=2)
          full_key = "security.#{key}"
          @configuration.update_location(full_key, reset, deep_level)
        end
      end

      class LogConfigurationLoader
        def initialize(configuration)
          @configuration = configuration
        end

        def use_syslog?
          @configuration.use_syslog?
        end

        def use_syslog=(boolean)
          update_location("use_syslog", !boolean)
          @configuration.use_syslog = boolean
        end

        def syslog_facility
          @configuration.syslog_facility
        end

        def syslog_facility=(facility)
          update_location("syslog_facility", facility == "mail")
          @configuration.syslog_facility = facility
        end

        def level
          @configuration.level
        end

        def level=(level)
          update_location("level", @configuration.default_log_level?(level))
          @configuration.level = level
        end

        private
        def update_location(key, reset, deep_level=2)
          full_key = "log.#{key}"
          @configuration.update_location(full_key, reset, deep_level)
        end
      end

      class DatabaseConfigurationLoader
        def initialize(configuration)
          @configuration = configuration
        end

        def type
          @configuration.type
        end

        def type=(type)
          update_location("type", type.nil?)
          @configuration.type = type
        end

        def name
          @configuration.name
        end

        def name=(name)
          update_location("name", name.nil?)
          @configuration.name = name
        end

        def host
          @configuration.host
        end

        def host=(host)
          update_location("host", host.nil?)
          @configuration.host = host
        end

        def port
          @configuration.port
        end

        def port=(port)
          update_location("port", port.nil?)
          @configuration.port = port
        end

        def path
          @configuration.path
        end

        def path=(path)
          update_location("path", path.nil?)
          @configuration.path = path
        end

        def user
          @configuration.user
        end

        def user=(user)
          update_location("user", user.nil?)
          @configuration.user = user
        end

        def password
          @configuration.password
        end

        def password=(password)
          update_location("password", password.nil?)
          @configuration.password = password
        end

        def extra_options
          @configuration.extra_options
        end

        def extra_options=(options)
          options ||= {}
          update_location("extra_options", options.empty?)
          @configuration.extra_options = options
        end

        def setup
          missing_values = @configuration.missing_values
          unless missing_values.empty?
            raise MissingValue.new("database.#{missing_values.first}")
          end
          @configuration.setup
        end

        def load_models(path)
          resolved_paths = @configuration.resolve_path(path)
          resolved_paths.each do |resolved_path|
            begin
              require(resolved_path)
              @configuration.add_loaded_model_file(resolved_path)
            rescue Exception => error
              Milter::Logger.error(error)
            end
          end
        end

        private
        def update_location(key, reset, deep_level=2)
          full_key = "database.#{key}"
          @configuration.update_location(full_key, reset, deep_level)
        end
      end
    end
  end
end
