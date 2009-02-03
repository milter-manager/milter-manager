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
      @variables["ENABLED"] != "0"
    end

    private
    def parse_custom_conf
      parse_default_conf(default_conf)
    end

    def parse_init_script
      content = File.open(init_script).read

      content = content.gsub(/\A#!\s*\/bin\/sh\s*/m, '')
      before, init_info_content, after = extract_meta_data_blocks(content)
      parse_init_info(init_info_content) if init_info_content

      extract_variables(content)

      @name = @variables["NAME"] || @info["Provides"] || @script_name
    end

    def parse_default_conf(file)
      return unless File.exist?(file)
      File.open(file) do |input|
        extract_variables(input)
      end
    end

    def guess_spec
      spec = nil
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self))
      end
      spec ||= normalize_spec(@variables["SOCKET"])
      spec ||= normalize_spec(@variables["SOCKFILE"])
      spec ||= normalize_spec(@variables["CONNECTION_SPEC"])
      spec ||= extract_spec_parameter_from_flags(@variables["OPTARGS"])
      spec ||= extract_spec_parameter_from_flags(@variables["DAEMON_ARGS"])
      spec ||= extract_spec_parameter_from_flags(@variables["PARAMS"])
      spec
    end

    def default_conf
      File.join(default_dir, @script_name)
    end

    def default_dir
      "/etc/default"
    end
  end
end
