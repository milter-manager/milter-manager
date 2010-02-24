# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

require 'milter/manager/detector'

module Milter::Manager
  class RedHatUpstartDetector
    include Detector

    def detect
      return unless upstart_script_readable?

      parse_upstart_script
    end

    def run_command
      "/sbin/start"
    end

    def command_options
      if have_service_command?
        super
      else
        [@script_name]
      end
    end

    def upstart_script_readable?
      File.readable?(upstart_script)
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

    private
    def init_variables
      super
      @description = nil
      @enabled = false
      @exec = nil
    end

    def parse_upstart_script
      content = File.open(upstart_script).read

      description = nil
      first_comment_block = true
      first_comment_content = []
      content.each_line do |line|
        case line
        when /^# /
          comment = $POSTMATCH.strip
          if first_comment_block and /^\s*$/ !~ comment
            first_comment_content << comment
            next
          end
        when /^start on /
          @enabled = true
        when /^exec /
          @exec = $POSTMATCH
        end
        if first_comment_content
          unless first_comment_content.empty?
            @description = first_comment_content.join(' ')
          end
          first_comment_block = false
        end
      end

      @name = @script_name
    end

    def clamav_milter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("clamav-milter.conf")
    end

    def milter_greylist_conf
      extract_parameter_from_flags(@exec, "-f") ||
        etc_file("mail", "greylist.conf")
    end

    def guess_spec
      spec = nil
      spec ||= guess_application_specific_spec
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self, spec)) || spec
      end
      spec
    end

    def guess_application_specific_spec
      spec = nil
      spec ||= detect_clamav_milter_connection_spec if clamav_milter?
      spec ||= detect_milter_greylist_connection_spec if milter_greylist?
      spec
    end

    def upstart_script
      File.join(event_d, @script_name)
    end

    def event_d
      File.join(etc_dir, "event.d")
    end

    def etc_dir
      File.join("", "etc")
    end

    def etc_file(*paths)
      File.join(etc_dir, *paths)
    end
  end
end
