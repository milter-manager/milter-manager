require 'glib2'
require 'milter'
require 'milter_manager.so'

module Milter::Manager
  class ConfigurationLoader
    class << self
      def load(configuration, file)
        new(configuration).load_configuration(file)
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
