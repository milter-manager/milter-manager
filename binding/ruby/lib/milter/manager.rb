require 'glib2'
require 'milter'
require 'milter_manager.so'

module Milter::Manager
  class ConfigurationLoader
    class << self
      def load(configuration, file)
        new(configuration).load(file)
      end
    end

    def initialize(configuration)
      @configuration = configuration
    end

    def load(file)
      p file
      instance_eval(File.read(file), file)
    end
  end
end
