require "milter/manager/detector"
require "milter/manager/debian-init-detector"
require "milter/manager/debian-systemd-detector"

module Milter::Manager
  class DebianDetector
    include Detector

    def detect
      @systemd_detector.detect
      @init_detector.detect
    end

    def apply(loader, &block)
      if @systemd_detector.enabled?
        @systemd_detector.apply(loader, &block)
      else
        @init_detector.apply(loader, &block)
      end
    end

    def run_command
      if @systemd_detector.enabled?
        @systemd_detector.command
      else
        @init_detector.command
      end
    end

    def command_options
      if @systemd_detector.enabled?
        @systemd_detector.command_options
      else
        @init_detector.command_options
      end
    end

    def enabled?
      @systemd_detector.enabled? or
        @init_detector.enabled?
    end

    private

    def init_variables
      super
      @init_detector = DebianInitDetector.new(@configuration, @script_name,
                                              &@connection_spec_detector)
      @systemd_detector = DebianSystemdDetector.new(@configuration, @script_name,
                                                    &@connection_spec_detector)
    end
  end
end
