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
  class RedHatInitDetector
    include InitDetector

    def initialize(script_name, &connection_spec_detector)
      @script_name = script_name
      @connection_spec_detector = connection_spec_detector
      init_variables
    end

    def description
      @info["description"]
    end

    def enabled?
      !guess_spec.nil? and !rc_files.empty?
    end

    private
    def parse_custom_conf
      parse_sysconfig(sysconfig)
    end

    def parse_init_script
      content = File.open(init_script).read

      before, init_info_content, after = extract_meta_data_blocks(content)
      parse_tag_block(before)
      parse_init_info(init_info_content) if init_info_content
      parse_tag_block(after) if after

      extract_variables(content)

      @name = @script_name
    end

    def parse_tag_block(block)
      description_continue = false
      block.gsub(/^# /, '').each_line do |line|
        if description_continue
          value = line.strip
          description_continue = /\\\z/.match(value)
          @info["description"] << normalize_description(value)
          next
        end

        case line
        when /\A(chkconfig|processname|config|pidfile|probe):\s*/
          @info[$1] = $POSTMATCH.strip
        when /\Adescription:\s*/
          value = $POSTMATCH.strip
          description_continue = /\\\z/.match(value)
          @info["description"] = normalize_description(value)
        end
      end
    end

    def normalize_description(description)
      description = description.strip
      if /\\\z/ =~ description
        $PREMATCH
      else
        description
      end
    end

    def parse_sysconfig(file)
      return unless File.exist?(file)
      File.open(file) do |input|
        extract_variables(input)
      end
    end

    def extract_variables(content)
      super(content, :accept_lower_case => true)
    end

    def guess_spec
      spec = nil
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self))
      end
      spec ||= normalize_spec(@variables["SOCKET"])
      spec ||= normalize_spec(@variables["SOCKET_ADDRESS"])
      spec ||= normalize_spec(@variables["MILTER_SOCKET"])
      spec ||= normalize_spec(@variables["CONNECTION_SPEC"])
      spec ||= extract_spec_parameter_from_flags(@variables["OPTIONS"])
      spec
    end

    def sysconfig
      File.join(sysconfig_dir, @script_name)
    end

    def sysconfig_dir
      "/etc/sysconfig"
    end

    def rc_files
      Dir.glob(File.join(init_base_dir,
                         "rc[0-6].d",
                         "[SK][0-9][0-9]#{@script_name}"))
    end
  end
end
