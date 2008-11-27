require 'pathname'
require 'shellwords'

require 'milter'
require 'milter_manager.so'

module Milter::Manager
  class ConfigurationLoader
    class << self
      def load(configuration, file)
        new(configuration).load_configuration(file)
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

    def define_milter(name, &block)
      egg_configuration = EggConfiguration.new(name)
      yield(egg_configuration)
      egg_configuration.apply(@configuration)
    end

    class SecurityConfiguration
      def initialize(configuration)
        @configuration = configuration
      end

      def privilege_mode?
        @configuration.privilege_mode?
      end

      def privilege_mode=(mode)
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
              !@target_hosts.all? {|_host| _host === host} and
                !@target_addresses.all? {|_address| _address === address}
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
