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
require 'milter/manager/enma-socket-detector'

module Milter::Manager
  class RedHatInitDetector
    include InitDetector

    def description
      @info["description"]
    end

    def enabled?
      !guess_spec.nil? and !rc_files.empty? and
        !(clamav_milter_0_95_or_later? and clamav_milter_example?)
    end

    def detect_enma_connection_spec
      conf_file = @variables["CONF_FILE"] || etc_file("enma.conf")
      Milter::Manager::EnmaSocketDetector.new(conf_file).detect
    end

    def enma?
      @script_name == "enma"
    end

    def detect_clamav_milter_connection_spec
      clamav_milter_config_parser.milter_socket
    end

    def clamav_milter?
      @script_name == "clamav-milter"
    end

    def clamav_milter_0_95_or_later?
      clamav_milter? and !@variables["SOCKET_ADDRESS"]
    end

    def detect_milter_greylist_connection_spec
      milter_greylist_config_parser.socket
    end

    def milter_greylist?
      @script_name == "milter-greylist"
    end

    def opendkim?
      @script_name == "opendkim"
    end

    private
    def parse_custom_conf
      parse_sysconfig(sysconfig)
    end

    def parse_init_script
      content = File.open(init_script).read

      before, init_info_content, after = extract_meta_data_blocks(content)
      parse_tag_block(before) if before
      parse_init_info(init_info_content) if init_info_content
      parse_tag_block(after) if after

      extract_variables(@variables, content)

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
      return unless File.readable?(file)
      File.open(file) do |input|
        extract_variables(@variables, input)
      end
    end

    def extract_variables(output, content)
      super(output, content, :accept_lower_case => true)
    end

    def clamav_milter_conf
      flags = @variables["CLAMAV_FLAGS"]
      extract_parameter_from_flags(flags, "--config-file") ||
        extract_parameter_from_flags(flags, "-c") ||
        etc_file("clamav-milter.conf")
    end

    def milter_greylist_conf
      extract_parameter_from_flags(@variables["OPTIONS"], "-f") ||
        @info["config"] ||
        etc_file("mail", "greylist.conf")
    end

    def opendkim_conf
      @variables["CONFIG"] || etc_file("opendkim.conf")
    end

    def guess_spec
      spec = nil
      spec ||= guess_application_specific_spec_before
      spec ||= normalize_spec(@variables["SOCKET"])
      spec ||= normalize_spec(@variables["SOCKET_ADDRESS"])
      spec ||= normalize_spec(@variables["MILTER_SOCKET"])
      spec ||= normalize_spec(@variables["CONNECTION_SPEC"])
      spec ||= extract_spec_parameter_from_flags(@variables["OPTIONS"])
      spec ||= guess_application_specific_spec_after
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self, spec)) || spec
      end
      spec
    end

    def guess_application_specific_spec_before
      spec = nil
      spec ||= detect_enma_connection_spec if enma?
      if clamav_milter_0_95_or_later?
        spec ||= detect_clamav_milter_connection_spec
      end
      spec ||= detect_opendkim_connection_spec if opendkim?
      spec
    end

    def guess_application_specific_spec_after
      spec = nil
      spec ||= detect_milter_greylist_connection_spec if milter_greylist?
      spec
    end

    def sysconfig
      File.join(sysconfig_dir, @script_name)
    end

    def sysconfig_dir
      File.join(etc_dir, "sysconfig")
    end

    def rc_files
      Dir.glob(File.join(init_base_dir,
                         "rc[0-6].d",
                         "[SK][0-9][0-9]#{@script_name}"))
    end
  end
end
