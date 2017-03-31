# Copyright (C) 2016  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

require "milter/manager/file-reader"
require "milter/manager/detector"

module Milter::Manager
  class RedHatSystemdDetector
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

    def clamav_milter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("mail", "clamav-milter.conf")
    end

    def milter_greylist_conf
      extract_parameter_from_flags(@exec, "-f") ||
        etc_file("mail", "greylist.conf")
    end

    def opendkim_conf
      extract_parameter_from_flags(@variables["OPTIONS"] || "", "-x") ||
        etc_file("opendkim.conf")
    end

    def rmilter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("rmilter", "rmilter.conf")
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
      File.join("", "usr", "lib", "systemd", "system")
    end

    def etc_dir
      File.join("", "etc")
    end

    def etc_file(*paths)
      File.join(etc_dir, *paths)
    end
  end
end
