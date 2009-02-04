# Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>
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

require 'milter/manager/child-context'

require 'milter/manager/debian-init-detector'
require 'milter/manager/redhat-init-detector'
require 'milter/manager/freebsd-rc-detector'
require 'milter/manager/pkgsrc-rc-detector'

require 'milter/manager/enma-socket-detector'

module Milter::Manager
  class Configuration
    def dump
      result = ''

      result << "security.privilege_mode = #{privilege_mode?}\n"
      result << "security.effective_user = #{effective_user.inspect}\n"
      result << "security.effective_group = #{effective_group.inspect}\n"
      result << "\n"

      result << "manager.connection_spec = #{manager_connection_spec.inspect}\n"
      result << "manager.unix_socket_mode = 0%o\n" % manager_unix_socket_mode
      result << "manager.remove_unix_socket_on_create = #{remove_manager_unix_socket_on_create?}\n"
      result << "manager.remove_unix_socket_on_close = #{remove_manager_unix_socket_on_close?}\n"
      result << "manager.daemon = #{daemon?}\n"
      result << "manager.pid_file = #{pid_file.inspect}\n"
      result << "\n"

      result << "controller.connection_spec = #{controller_connection_spec.inspect}\n"
      result << "controller.unix_socket_mode = 0%o\n" % controller_unix_socket_mode
      result << "controller.remove_unix_socket_on_create = #{remove_controller_unix_socket_on_create?}\n"
      result << "controller.remove_unix_socket_on_close = #{remove_controller_unix_socket_on_close?}\n"
      result << "\n"

      applicable_conditions.each do |condition|
        result << "define_applicable_condition(#{condition.name.inspect}) do |condition|\n"
        result << "  condition.description = #{condition.description.inspect}\n"
        result << "end\n"
        result << "\n"
      end

      eggs.each do |egg|
        result << "define_milter(#{egg.name.inspect}) do |milter|\n"
        result << "  milter.connection_spec = #{egg.connection_spec.inspect}\n"
        result << "  milter.description = #{egg.description.inspect}\n"
        result << "  milter.enabled = #{egg.enabled?}\n"
        condition_names = egg.applicable_conditions.collect do |condition|
          condition.name
        end
        result << "  milter.applicable_conditions = #{condition_names.inspect}\n"
        result << "  milter.command = #{egg.command.inspect}\n"
        result << "  milter.command_options = #{egg.command_options.inspect}\n"
        result << "  milter.user_name = #{egg.user_name.inspect}\n"
        result << "  milter.connection_timeout = #{egg.connection_timeout}\n"
        result << "  milter.writing_timeout = #{egg.writing_timeout}\n"
        result << "  milter.reading_timeout = #{egg.reading_timeout}\n"
        result << "  milter.end_of_message_timeout = #{egg.end_of_message_timeout}\n"
        result << "end\n"
        result << "\n"
      end

      result.strip
    rescue Exception
      [$!.message, $@].join("\n")
    end
  end

  class ConfigurationLoader
    class Error < StandardError
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

    class << self
      @@current_configuration = nil
      def load(configuration, file)
        @@current_configuration = configuration
        guard do
          new(configuration).load_configuration(file)
        end
      end

      def load_custom(configuration, file)
        @@current_configuration = configuration
        guard do
          new(configuration).load_custom_configuration(file)
        end
      end

      def guard
        yield
      rescue Exception => error
        location = error.backtrace[0].split(/(:\d+):?/, 2)[0, 2].join
        puts "#{location}: #{error.message} (#{error.class})"
        # FIXME: log full error
      end
    end

    attr_reader :security, :controller, :manager, :configuration
    def initialize(configuration)
      @configuration = configuration
      @security = SecurityConfiguration.new(configuration)
      @controller = ControllerConfiguration.new(configuration)
      @manager = ManagerConfiguration.new(configuration)
    end

    def load_configuration(file)
      begin
        instance_eval(File.read(file), file)
      rescue InvalidValue
        backtrace = $!.backtrace.collect do |info|
          if /\A#{Regexp.escape(file)}:/ =~ info
            info
          else
            nil
          end
        end.compact
        puts "#{backtrace[0]}: #{$!.message}"
        puts backtrace[1..-1]
      end
    end

    def load_custom_configuration(file)
      first_line = File.open(file) {|config| config.gets}
      case first_line
      when /\A\s*</m
        XMLConfigurationLoader.new(self).load(file)
#       when /\A#\s*-\*-\s*yaml\s*-\*-/i
#         YAMLConfigurationLoader.new(self).load(file)
      else
        load_configuration(file)
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
      egg_configuration = EggConfiguration.new(name, self)
      yield(egg_configuration)
      egg_configuration.apply
    end

    def define_applicable_condition(name)
      condition_configuration = ApplicableConditionConfiguration.new(name, self)
      yield(condition_configuration)
      condition_configuration.apply
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

    class XMLConfigurationLoader
      def initialize(loader)
        @loader = loader
      end

      def load(file)
        listener = Listener.new(@loader)
        File.open(file) do |input|
          parser = REXML::Parsers::StreamParser.new(input, listener)
          begin
            parser.parse
          rescue InvalidValue
            source = parser.source
            info = "#{file}:#{source.line}:#{source.position}"
            info << ":#{source.current_line}: #{$!.message}"
            puts info
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
              (@egg_config["applicable_conditions"] || []).each do |condition|
                milter.add_applicable_condition(condition)
              end
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
            end
            @egg_config = nil
          when "milter_applicable_condition"
            @egg_config["applicable_conditions"] << text
          when "milter_enabled"
            @egg_config["enabled"] = text == "true"
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
                                "command", "command_options"]
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
        @configuration.privilege_mode = mode
      end

      def effective_user
        @configuration.effective_user
      end

      def effective_user=(user_name)
        @configuration.effective_user = user_name
      end

      def effective_group
        @configuration.effective_group
      end

      def effective_group=(group_name)
        @configuration.effective_group = group_name
      end
    end

    class ControllerConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def connection_spec=(spec)
        Milter::Connection.parse_spec(spec) unless spec.nil?
        @configuration.controller_connection_spec = spec
      end

      def unix_socket_mode
        @configuration.controller_unix_socket_mode
      end

      def unix_socket_mode=(mode)
        @configuration.controller_unix_socket_mode = mode
      end

      def remove_unix_socket_on_create?
        @configuration.remove_controller_unix_socket_on_create?
      end

      def remove_unix_socket_on_create=(remove)
        @configuration.remove_controller_unix_socket_on_create = remove
      end

      def remove_unix_socket_on_close?
        @configuration.remove_controller_unix_socket_on_close?
      end

      def remove_unix_socket_on_close=(remove)
        @configuration.remove_controller_unix_socket_on_close = remove
      end
    end

    class ManagerConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def connection_spec=(spec)
        Milter::Connection.parse_spec(spec) unless spec.nil?
        @configuration.manager_connection_spec = spec
      end

      def unix_socket_mode
        @configuration.manager_unix_socket_mode
      end

      def unix_socket_mode=(mode)
        @configuration.manager_unix_socket_mode = mode
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

      def daemon=(boolean)
        @configuration.daemon = boolean
      end

      def daemon?
        @configuration.daemon?
      end

      def pid_file=(pid_file)
        @configuration.pid_file = pid_file
      end

      def pid_file
        @configuration.pid_file
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
        @egg.add_applicable_condition(condition)
      end

      def applicable_conditions=(condition_names)
        configuration = @loader.configuration
        conditions = condition_names.collect do |name|
          condition = configuration.find_applicable_condition(name)
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
        @egg.command_options = options
      end

      def method_missing(name, *args, &block)
        @egg.send(name, *args, &block)
      end

      def apply
        @loader.configuration.remove_egg(@egg.name)
        @loader.configuration.add_egg(@egg)
      end
    end

    class ApplicableConditionConfiguration
      def initialize(name, loader)
        @condition = Milter::Manager::ApplicableCondition.new(name)
        @loader = loader
        @connect_stoppers = []
        @envelope_from_stoppers = []
        @envelope_recipient_stoppers = []
        @header_stoppers = []

        exist_condition = @loader.configuration.find_applicable_condition(name)
        @condition.merge(exist_condition) if exist_condition
      end

      def define_connect_stopper(&block)
        @connect_stoppers << block
      end

      def define_envelope_from_stopper(&block)
        @envelope_from_stoppers << block
      end

      def define_envelope_recipient_stopper(&block)
        @envelope_recipient_stoppers << block
      end

      def define_header_stopper(&block)
        @header_stoppers << block
      end

      def have_stopper?
        [@connect_stoppers,
         @envelope_from_stoppers,
         @envelope_recipient_stoppers,
         @header_stoppers].any? do |stoppers|
          not stoppers.empty?
        end
      end

      def method_missing(name, *args, &block)
        @condition.send(name, *args, &block)
      end

      def apply
        @loader.configuration.remove_applicable_condition(@condition.name)
        setup_stoppers
        @loader.configuration.add_applicable_condition(@condition)
      end

      private
      def setup_stoppers
        return unless have_stopper?

        @condition.signal_connect("attach-to") do |_, child, children|
          unless @connect_stoppers.empty?
            child.signal_connect("stop-on-connect") do |_child, host, address|
              context = ChildContext.new(child, children)
              @connect_stoppers.any? do |stopper|
                stopper.call(context, host, address)
              end
            end
          end

          unless @envelope_from_stoppers.empty?
            child.signal_connect("stop-on-envelope-from") do |_child, from|
              context = ChildContext.new(child, children)
              @envelope_from_stoppers.any? do |stopper|
                stopper.call(context, from)
              end
            end
          end

          unless @envelope_recipient_stoppers.empty?
            child.signal_connect("stop-on-envelope-recipient") do |_child, recipient|
              context = ChildContext.new(child, children)
              @envelope_recipient_stoppers.any? do |stopper|
                stopper.call(context, recipient)
              end
            end
          end

          unless @header_stoppers.empty?
            child.signal_connect("stop-on-header") do |_child, name, value|
              context = ChildContext.new(child, children)
              @header_stoppers.any? do |stopper|
                stopper.call(context, name, value)
              end
            end
          end
        end
      end
    end
  end
end
