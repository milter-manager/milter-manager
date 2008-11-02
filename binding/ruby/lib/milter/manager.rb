require 'pathname'
require 'shellwords'

require 'glib2'
require 'milter'
require 'milter_manager.so'

module Milter::Manager
  class ConfigurationLoader
    class << self
      @@load_paths = []
      def add_load_path(path)
        @@load_paths << path
      end

      def load(configuration, file)
        full_path = nil
        (@@load_paths + ["."]).each do |load_path|
          path = Pathname.new(load_path) + file
          if path.exist?
            full_path = path.expand_path.to_s
            break
          end
        end
        if full_path.nil?
          raise Errno::ENOENT, "#{file} in load path (#{@@load_paths.inspect})"
        end
        new(configuration).load_configuration(full_path)
      end
    end

    attr_reader :security
    def initialize(configuration)
      @configuration = configuration
      @security = SecurityConfiguration.new(configuration)
    end

    def load_configuration(file)
      instance_eval(File.read(file), file)
    end

    def define_milter(name, &block)
      configuration = EggConfiguration.new(@configuration, name)
      yield(configuration)
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

    class EggConfiguration
      def initialize(configuration, name)
        @configuration = configuration
        @egg = Egg.new(name)
        @configuration.add_egg(@egg)
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
    end
  end
end
