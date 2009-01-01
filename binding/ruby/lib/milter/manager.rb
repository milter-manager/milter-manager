require 'pathname'
require 'shellwords'
require 'erb'
require 'yaml'
require "English"

require "rexml/document"
require "rexml/streamlistener"

require 'milter'
require 'milter_manager.so'

module Milter::Manager
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
      def load(configuration, file)
        guard do
          new(configuration).load_configuration(file)
        end
      end

      def load_custom(configuration, file)
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
      def initialize(configuration)
        @configuration = configuration
      end

      def load(file)
        listener = Listener.new(@configuration)
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

        def initialize(configuration)
          @configuration = configuration
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
            @configuration.define_milter(@egg_config["name"]) do |milter|
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
            @configuration.send(@state_stack.last).send("#{local}=", text)
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
            if @configuration.send(current_state).respond_to?("#{local}=")
              local
            else
              raise "unexpected element: #{current_path}"
            end
          when :milter
            available_locals = ["name", "description",
                                "enabled", "connection_spec"]
            case local
            when "applicable_conditions"
              @egg_config["applicable_conditions"] = []
              :milter_applicable_conditions
            when *available_locals
              "milter_#{local}"
            else
              raise "unexpected element: #{current_path}"
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

      def remove_unix_socket_on_close?
        @configuration.remove_manager_unix_socket_on_close?
      end

      def remove_unix_socket_on_close=(remove)
        @configuration.remove_manager_unix_socket_on_close = remove
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
        @connect_checkers = []
        @envelope_from_checkers = []
        @envelope_recipient_checkers = []
        @header_checkers = []

        exist_condition = @loader.configuration.find_applicable_condition(name)
        @condition.merge(exist_condition) if exist_condition
      end

      def define_connect_checker(&block)
        @connect_checkers << block
      end

      def define_envelope_from_checker(&block)
        @envelope_from_checkers << block
      end

      def define_envelope_recipient_checker(&block)
        @envelope_recipient_checkers << block
      end

      def define_header_checker(&block)
        @header_checkers << block
      end

      def have_checker?
        [@connect_checkers,
         @envelope_from_checkers,
         @envelope_recipient_checkers,
         @header_checkers].any? do |checkers|
          not checkers.empty?
        end
      end

      def method_missing(name, *args, &block)
        @condition.send(name, *args, &block)
      end

      def apply
        @loader.configuration.remove_applicable_condition(@condition.name)
        setup_checker
        @loader.configuration.add_applicable_condition(@condition)
      end

      private
      def setup_checker
        return unless have_checker?

        @condition.signal_connect("attach-to") do |_, child|
          unless @connect_checkers.empty?
            child.signal_connect("check-connect") do |_child, host, address|
              @connect_checkers.any? do |checker|
                checker.call(_child, host, address)
              end
            end
          end

          unless @envelope_from_checkers.empty?
            child.signal_connect("check-envelope-from") do |_child, from|
              @envelope_from_checkers.any? do |checker|
                checker.call(_child, from)
              end
            end
          end

          unless @envelope_recipient_checkers.empty?
            child.signal_connect("check-envelope-recipient") do |_child, recipient|
              @envelope_recipient_checkers.any? do |checker|
                checker.call(_child, recipient)
              end
            end
          end

          unless @header_checkers.empty?
            child.signal_connect("check-header") do |_child, name, value|
              @header_checkers.any? do |checker|
                checker.call(_child, name, value)
              end
            end
          end
        end
      end
    end
  end
end
