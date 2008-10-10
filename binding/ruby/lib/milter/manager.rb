require 'pathname'

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

    def initialize(configuration)
      @configuration = configuration
      @child_milters = []
    end

    def load_configuration(file)
      instance_eval(File.read(file), file)
      @child_milters.each do |child_milter_config|
        @configuration.add_child_milter(child_milter_config.create)
      end
    end

    def define_milter(name, &block)
      milter = ChildMilterConfiguration.new(name)
      @child_milters << milter
      milter.configure(&block)
    end
  end

  class ChildMilterConfiguration
    def initialize(name)
      @name = name
    end

    def configure
      yield(self)
    end

    def create
      milter = ChildMilter.new(@name)
      milter
    end
  end
end
