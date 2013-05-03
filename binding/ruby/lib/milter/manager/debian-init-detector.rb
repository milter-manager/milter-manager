# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

require 'milter/manager/init-detector'

module Milter::Manager
  class DebianInitDetector
    include InitDetector

    def description
      @info["Description"] || @variables["DESC"]
    end

    def enabled?
      @variables["ENABLED"] != "0" and
        !(clamav_milter_0_95_or_later? and clamav_milter_example?)
    end

    def clamav_milter?
      @script_name == "clamav-milter"
    end

    def clamav_milter_0_95_or_later?
      clamav_milter? and
        (/clamav-milter/ =~ @variables["CLAMAVCONF"].to_s or
         @variables["CLAMAVMILTERCONF"])
    end

    def milter_greylist?
      @script_name == "milter-greylist"
    end

    def opendkim?
      @script_name == "opendkim"
    end

    private
    def parse_custom_conf
      parse_default_conf(default_conf)
    end

    def parse_init_script
      content = File.read(init_script)
      content.force_encoding("UTF-8") if content.respond_to?(:force_encoding)

      content = content.gsub(/\A#!\s*\/bin\/sh\s*/m, '')
      before, init_info_content, after = extract_meta_data_blocks(content)
      parse_init_info(init_info_content) if init_info_content

      extract_variables(@variables, content)

      @name = @variables["NAME"] || @info["Provides"] || @script_name
    end

    def parse_default_conf(file)
      return unless File.readable?(file)
      File.open(file) do |input|
        extract_variables(@variables, input)
      end
    end

    def clamav_milter_conf
      @variables["CLAMAVCONF"] ||
        @variables["CLAMAVMILTERCONF"] ||
        "/etc/clamav/clamav-milter.conf"
    end

    def opendkim_conf
      "/etc/opendkim.conf"
    end

    def guess_spec
      spec = nil
      spec ||= guess_application_specific_spec
      spec ||= normalize_spec(@variables["SOCKET"])
      spec ||= normalize_spec(@variables["SOCKFILE"])
      spec ||= normalize_spec(@variables["CONNECTION_SPEC"])
      spec ||= extract_spec_parameter_from_flags(@variables["OPTARGS"])
      spec ||= extract_spec_parameter_from_flags(@variables["DAEMON_ARGS"])
      spec ||= extract_spec_parameter_from_flags(@variables["PARAMS"])
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self, spec)) || spec
      end
      spec
    end

    def guess_application_specific_spec
      spec = nil
      if clamav_milter_0_95_or_later?
        spec ||= detect_clamav_milter_connection_spec
      end
      if opendkim?
        spec ||= detect_opendkim_connection_spec
      end
      spec
    end

    def default_conf
      File.join(default_dir, @script_name)
    end

    def default_dir
      "/etc/default"
    end

    def milter_greylist_conf
      extract_parameter_from_flags(@variables["OPTIONS"], "-f") ||
        "/etc/milter-greylist/greylist.conf"
    end
  end
end
