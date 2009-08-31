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

require 'milter/manager/rcng-detector'
require 'milter/manager/enma-socket-detector'
require 'milter/manager/clamav-milter-config-parser'

module Milter::Manager
  class FreeBSDRCDetector
    include RCNGDetector

    def rcvar
      @rcvar || "#{@name}_enable"
    end

    def rcvar_value
      @rcvar_value || @variables[rcvar_variable_name] || "NO"
    end

    def detect_enma_connection_spec
      conf_file = @variables["cfgfile"]
      conf_file ||= extract_parameter_from_flags(command_args, "-c")
      conf_file ||= "/usr/local/etc/enma.conf"
      Milter::Manager::EnmaSocketDetector.new(conf_file).detect
    end

    def enma?
      @script_name == "milter-enma" or @name == "milterenma"
    end

    def detect_clamav_milter_connection_spec
      clamav_milter_config_parser.milter_socket
    end

    def clamav_milter?
      @script_name == "clamav-milter" or @name == "clamav_milter"
    end

    private
    def parse_rc_conf_unknown_line(line)
      case line
      when /\Arcvar=`set_rcvar`/
        @rcvar = "#{@name}_enable"
      when /\A#{Regexp.escape(rcvar)}=(.+)/
        @rcvar_value = normalize_variable_value($1)
      end
    end

    def rcvar_variable_name
      rcvar.gsub(/\A#{Regexp.escape(@name)}_/, '')
    end

    def rc_d
      "/usr/local/etc/rc.d"
    end

    def guess_application_specific_spec(guessed_spec)
      spec = nil
      spec ||= detect_enma_connection_spec if enma?
      spec ||= detect_clamav_milter_connection_spec if clamav_milter?
      spec
    end

    def init_variables
      super
      @clamav_milter_config_parser = nil
    end

    def clamav_milter_config_parser
      if @clamav_milter_config_parser.nil?
        conf_file = @other_variables["conf_file"]
        conf_file ||= extract_parameter_from_flags(command_args, "-c")
        conf_file ||= "/usr/local/etc/clamav-milter.conf"
        @clamav_milter_config_parser =
          Milter::Manager::ClamAVMilterConfigParser.new
        @clamav_milter_config_parser.parse(conf_file)
      end
      @clamav_milter_config_parser
    end
  end
end
