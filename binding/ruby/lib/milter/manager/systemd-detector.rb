require 'milter/manager/detector'

module Milter::Manager
  module SystemdDetector
    include Detector

    def detect
      return unless unit_file_readable?

      parse_unit_file

      @enabled = File.exist?(installed_unit_file)
    end

    def unit_file_readable?
      File.readable?(unit_file)
    end

    def description
      @description
    end

    def enabled?
      !guess_spec.nil? and @enabled and
        !(clamav_milter? and clamav_milter_example?)
    end

    def detect_milter_greylist_connection_spec
      milter_greylist_config_parser.socket
    end

    def clamav_milter?
      @script_name == "clamav-milter"
    end

    def milter_greylist?
      @script_name == "milter-greylist"
    end

    def opendkim?
      @script_name == "opendkim"
    end

    def rmilter?
      @script_name == "rmilter"
    end

    def rspamd_proxy?
      @script_name == "rspamd"
    end

    private
    def init_variables
      super
      @description = nil
      @enabled = false
      @exec = nil
    end

    def parse_unit_file
      content = FileReader.read(unit_file)

      section = nil
      content.each_line do |line|
        case line.strip
        when /\A\[(.+?)\]\z/
          section = $1
        when /\A(.+?)\s*=\s*(.+?)\z/
          name = $1
          value = $2
          case section
          when "Unit"
            case name
            when "Description"
              @description = value
            end
          when "Service"
            case name
            when "ExecStart"
              @exec = value
            when "EnvironmentFile"
              file = value.gsub(/\A-/, "")
              if File.exist?(file)
                extract_variables(@variables, FileReader.read(file))
              end
            end
          end
        end
      end

      @name = @script_name
    end

    def guess_spec
      spec = nil
      spec ||= guess_application_specific_spec
      spec ||= normalize_spec(@variables["SOCKET"])
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self, spec)) || spec
      end
      spec
    end

    def guess_application_specific_spec
      spec = nil
      spec ||= detect_clamav_milter_connection_spec if clamav_milter?
      spec ||= detect_milter_greylist_connection_spec if milter_greylist?
      spec ||= detect_opendkim_connection_spec if opendkim?
      spec ||= detect_rmilter_connection_spec if rmilter?
      spec ||= detect_rspamd_proxy_connection_spec if rspamd_proxy?
      spec
    end

    def unit_file_base_name
      "#{@script_name}.service"
    end

    def installed_unit_file
      etc_file("systemd", "system", "multi-user.target.wants",
               unit_file_base_name)
    end

    def unit_file
      File.join(system_dir, unit_file_base_name)
    end

    def system_dir
      File.join("", "lib", "systemd", "system")
    end

    def etc_dir
      File.join("", "etc")
    end

    def etc_file(*paths)
      File.join(etc_dir, *paths)
    end
  end
end
