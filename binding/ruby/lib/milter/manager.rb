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
      @eggs = []
    end

    def load_configuration(file)
      instance_eval(File.read(file), file)
      @eggs.each do |egg|
        @configuration.add_egg(egg)
      end
    end

    def define_milter(name, &block)
      egg = Egg.new(name)
      @eggs << egg
      yield(egg)
    end
  end
end
