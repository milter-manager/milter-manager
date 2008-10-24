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
      @children = []
    end

    def load_configuration(file)
      instance_eval(File.read(file), file)
      @children.each do |child_config|
        @configuration.add_child(child_config.create_child)
      end
    end

    def define_milter(name, &block)
      milter = ChildConfiguration.new(name)
      @children << milter
      milter.configure(&block)
    end
  end

  class ChildConfiguration
    attr_accessor :spec
    def initialize(name)
      @name = name
      @spec = nil
    end

    def configure
      yield(self)
    end

    def create_child
      child = Child.new(@name)
      child.connection_spec = @spec
      child
    end
  end
end
