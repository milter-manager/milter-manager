require 'pathname'
require 'shellwords'
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

    class << self
      def load(configuration, file)
        new(configuration).load_configuration(file)
      end

      def load_custom(configuration, file)
        new(configuration).load_custom_configuration(file)
      end
    end

    attr_reader :security, :control, :manager
    def initialize(configuration)
      @configuration = configuration
      @security = SecurityConfiguration.new(configuration)
      @control = ControlConfiguration.new(configuration)
      @manager = ManagerConfiguration.new(configuration)
    end

    def load_configuration(file)
      begin
        instance_eval(File.read(file), file)
      rescue Exception => error
        location = error.backtrace[0].split(/(:\d+):?/, 2)[0, 2].join
        puts "#{location}: #{error.message}(#{error.class})"
        # FIXME: log full error
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

    def define_milter(name, &block)
      egg_configuration = EggConfiguration.new(name)
      yield(egg_configuration)
      egg_configuration.apply(@configuration)
    end

    class XMLConfigurationLoader
      def initialize(configuration)
        @configuration = configuration
      end

      def load(file)
        listener = Listener.new(@configuration)
        File.open(file) do |input|
          REXML::Document.parse_stream(input, listener)
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
                              :control, :manager, :milters]
          case state
          when *no_action_states
            # do nothing
          when :milter
            @configuration.define_milter(@egg_config["name"]) do |milter|
              milter.connection_spec = @egg_config["connection_spec"]
              milter.target_hosts.concat(@egg_config["target_host"] || [])
              milter.target_addresses.concat(@egg_config["target_address"] || [])
              milter.target_senders.concat(@egg_config["target_sender"] || [])
              milter.target_recipients.concat(@egg_config["target_recipient"] || [])
              (@egg_config["target_header"] || []).each do |name, value|
                milter.add_target_header(name, value)
              end
            end
            @egg_config = nil
          when "milter_target_header_name"
            @egg_target_header_name = text
          when "milter_target_header_value"
            @egg_target_header_value = text
          when :milter_target_header
            if @egg_target_header_name.nil?
              raise "#{current_path}/name is missing"
            end
            if @egg_target_header_value.nil?
              raise "#{current_path}/value is missing"
            end
            @egg_config["target_header"] ||= {}
            @egg_config["target_header"][@egg_target_header_name] = @egg_target_header_value
            @egg_target_header_name = nil
            @egg_target_header_value = nil
          when /\Amilter_target_/
            key = "target_#{$POSTMATCH}"
            @egg_config[key] ||= []
            @egg_config[key] << text
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
            when "control"
              :control
            when "manager"
              :manager
            when "milters"
              :milters
            else
              raise "unexpected element: #{current_path}"
            end
          when :security, :control, :manager
            if @configuration.send(current_state).respond_to?("#{local}=")
              local
            else
              raise "unexpected element: #{current_path}"
            end
          when :milter
            available_locals = ["name", "connection_spec",
                                "target_host", "target_address",
                                "target_sender", "target_recipient"]
            case local
            when "target_header"
              @egg_target_header_name = nil
              @egg_target_header_value = nil
              :milter_target_header
            when *available_locals
              "milter_#{local}"
            else
              raise "unexpected element: #{current_path}"
            end
          when :milter_target_header
            if ["name", "value"].include?(local)
              "milter_target_header_#{name}"
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
    end

    class ControlConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def connection_spec=(spec)
        Milter::Connection.parse_spec(spec)
        @configuration.control_connection_spec = spec
      end
    end

    class ManagerConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def connection_spec=(spec)
        Milter::Connection.parse_spec(spec)
        @configuration.manager_connection_spec = spec
      end
    end

    class EggConfiguration
      attr_reader :target_hosts, :target_addresses
      attr_reader :target_senders, :target_recipients
      def initialize(name)
        @egg = Egg.new(name)
        @target_hosts = []
        @target_addresses = []
        @target_senders = []
        @target_recipients = []
        @target_headers = []
      end

      def add_target_header(name, value)
        @target_headers << [name, value]
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

      def apply(configuration)
        setup_check_callback
        configuration.add_egg(@egg)
      end

      private
      def need_check?
        not (@target_hosts.empty? and
             @target_addresses.empty? and
             @target_senders.empty? and
             @target_recipients.empty? and
             @target_headers.empty?)
      end

      def setup_check_callback
        return unless need_check?

        @egg.signal_connect("hatched") do |_, child|
          if !@target_hosts.empty? or !@target_addresses.empty?
            child.signal_connect("check-connect") do |_, host, address|
              !(@target_hosts.all? {|_host| _host === host} and
                @target_addresses.all? {|_address| _address === address})
            end
          end

          if !@target_senders.empty?
            child.signal_connect("check-envelope-from") do |_, from|
              !@target_senders.all? {|_sender| _sender === from}
            end
          end

          if !@target_recipients.empty?
            child.signal_connect("check-envelope-recipient") do |_, recipient|
              !@target_recipients.all? {|_recipient| _recipient === recipient}
            end
          end

          if !@target_headers.empty?
            child.signal_connect("check-header") do |_, name, value|
              !@target_headers.all? do |_name, _value|
                _name === name and _value === value
              end
            end
          end
        end
      end
    end
  end
end
