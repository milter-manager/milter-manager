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
      instance_eval(File.read(file), file)
    end
  end
end
